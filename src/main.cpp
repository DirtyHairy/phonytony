#include <Arduino.h>
#include <MCP23S17.h>
#include <MFRC522.h>
#include <SD.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <mad.h>

#include <Lock.hxx>
#include <iomanip>
#include <sstream>

#include "DirectoryPlayer.hxx"

#define PIN_SD_CS GPIO_NUM_5

#define PIN_I2S_BCK GPIO_NUM_26
#define PIN_I2S_WC GPIO_NUM_22
#define PIN_I2S_DATA GPIO_NUM_25

#define PIN_RFID_IRQ GPIO_NUM_34
#define PIN_RFID_CS GPIO_NUM_15

#define MCP23S17_CS GPIO_NUM_27
#define MCP23S17_IRQ GPIO_NUM_32

#define SPI_FREQ_SD 40000000
#define HSPI_FREQ 20000000

#define PLAYBACK_CHUNK_SIZE 1024
#define PLAYBACK_QUEUE_SIZE 8

static SPIClass spiVSPI(VSPI);
static SPIClass spiHSPI(HSPI);
static SemaphoreHandle_t hspiMutex;

DRAM_ATTR static QueueHandle_t rfidInterruptQueue;
DRAM_ATTR static QueueHandle_t gpioInterruptQueue;

bool probeSd() {
    if (!SD.begin(PIN_SD_CS, spiVSPI, SPI_FREQ_SD)) {
        Serial.println("Failed to initialize SD");
        return false;
    }

    Serial.println("SD card initialized OK");

    return true;
}

void i2sStreamTask(void* payload) {
    void* buffer = malloc(PLAYBACK_CHUNK_SIZE);

    i2s_start(I2S_NUM_0);

    QueueHandle_t* queue = (QueueHandle_t*)payload;
    size_t bytes_written;

    while (true) {
        xQueueReceive(*queue, buffer, portMAX_DELAY);

        i2s_write(I2S_NUM_0, buffer, PLAYBACK_CHUNK_SIZE, &bytes_written, portMAX_DELAY);
    }
}

void audioTask_() {
    DirectoryPlayer player;

    if (!player.open("/album")) {
        Serial.println("unable to open /album");
        return;
    }

    int16_t* buffer = (int16_t*)malloc(PLAYBACK_CHUNK_SIZE);
    QueueHandle_t audioQueue = xQueueCreate(PLAYBACK_QUEUE_SIZE, PLAYBACK_CHUNK_SIZE);

    TaskHandle_t task;
    xTaskCreatePinnedToCore(i2sStreamTask, "i2sstream", 1024, (void*)&audioQueue, 1, &task, 1);

    size_t samplesDecoded = 0;

    while (true) {
        samplesDecoded = 0;

        while (samplesDecoded < PLAYBACK_CHUNK_SIZE / 4) {
            samplesDecoded += player.decode(buffer, (PLAYBACK_CHUNK_SIZE / 4 - samplesDecoded));

            if (player.isFinished()) player.rewind();
        }

        for (int i = 0; i < PLAYBACK_CHUNK_SIZE / 2; i++) {
            buffer[i] /= 5;
        }

        xQueueSend(audioQueue, (void*)buffer, portMAX_DELAY);
    }
}

void audioTask(void* payload) {
    audioTask_();
    vTaskDelete(NULL);
}

IRAM_ATTR void rfidIsrHandler() {
    uint32_t val = 1;

    xQueueSendFromISR(rfidInterruptQueue, &val, NULL);
}

void _rfidTask() {
    MFRC522 mfrc522(spiHSPI);

    {
        Lock lock(hspiMutex);

        mfrc522.PCD_Init(PIN_RFID_CS, MFRC522::UNUSED_PIN);
    }

    delay(10);

    bool selfTestSucceeded;
    {
        Lock lock(hspiMutex);

        selfTestSucceeded = mfrc522.PCD_PerformSelfTest();
    }

    if (!selfTestSucceeded) {
        Serial.println("RC522 self test failed");
        return;
    }

    Serial.println("RC522 self test succeeded");

    rfidInterruptQueue = xQueueCreate(1, 4);
    pinMode(PIN_RFID_IRQ, INPUT);
    attachInterrupt(PIN_RFID_IRQ, rfidIsrHandler, FALLING);

    {
        Lock lock(hspiMutex);

        mfrc522.PCD_Init();
        mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, 0xa0);
    }

    while (true) {
        {
            Lock lock(hspiMutex);

            mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg, mfrc522.PICC_CMD_REQA);
            mfrc522.PCD_WriteRegister(mfrc522.CommandReg, mfrc522.PCD_Transceive);
            mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x87);
        }

        uint32_t value;
        if (xQueueReceive(rfidInterruptQueue, &value, 100) != pdTRUE) continue;

        bool readSerialSucceeded;

        {
            Lock lock(hspiMutex);

            mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7f);
            readSerialSucceeded = mfrc522.PICC_ReadCardSerial();
            mfrc522.PICC_HaltA();
        }

        if (readSerialSucceeded) {
            std::stringstream sstream;

            for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
                sstream << std::setw(2) << std::setfill('0') << std::hex << (int)mfrc522.uid.uidByte[i] << " ";
            }

            Serial.printf("RFID: %s\r\n", sstream.str().c_str());
        }
    }
}

void rfidTask(void*) {
    _rfidTask();
    vTaskDelete(NULL);
}

IRAM_ATTR void gpioIsr() {
    uint32_t val = 1;

    xQueueSendFromISR(gpioInterruptQueue, &val, NULL);
}

void _gpioTask() {
    MCP23S17 mcp23s17(spiHSPI, MCP23S17_CS, 0);

    gpioInterruptQueue = xQueueCreate(1, 4);
    pinMode(MCP23S17_IRQ, INPUT_PULLUP);
    attachInterrupt(MCP23S17_IRQ, gpioIsr, FALLING);

    {
        Lock lock(hspiMutex);

        mcp23s17.begin();
        mcp23s17.setMirror(false);
        mcp23s17.setInterruptOD(false);
        mcp23s17.setInterruptLevel(LOW);

        for (uint8_t i = 8; i < 16; i++) {
            mcp23s17.pinMode(i, INPUT_PULLUP);
            mcp23s17.enableInterrupt(i, CHANGE);
        }
    }

    while (true) {
        uint32_t value;
        if (xQueueReceive(gpioInterruptQueue, &value, 100) != pdTRUE) continue;

        uint16_t signalingPins, pins;

        {
            Lock lock(hspiMutex);

            signalingPins = mcp23s17.getInterruptPins();
            pins = mcp23s17.getInterruptValue();
        }

        uint8_t pushedButtons = (signalingPins >> 8) & (~pins >> 8);

        if (pushedButtons) Serial.printf("GPIO interrupt: 0x%02x\r\n", pushedButtons);
    }
}

void gpioTask(void*) {
    _gpioTask();
    vTaskDelete(NULL);
}

void setup() {
    hspiMutex = xSemaphoreCreateMutex();

    Serial.begin(115200);

    spiVSPI.begin();
    spiHSPI.begin();
    spiHSPI.setFrequency(HSPI_FREQ);

    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
    };

    const i2s_pin_config_t i2s_pins = {
        .bck_io_num = PIN_I2S_BCK, .ws_io_num = PIN_I2S_WC, .data_out_num = PIN_I2S_DATA, .data_in_num = -1};

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_pins);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    i2s_stop(I2S_NUM_0);

    if (!probeSd()) {
        return;
    }

    TaskHandle_t audioTaskHandle;
    xTaskCreatePinnedToCore(audioTask, "playback", 0x8000, NULL, 10, &audioTaskHandle, 1);

    TaskHandle_t rfidTaskHandle;
    xTaskCreatePinnedToCore(rfidTask, "rfid", 0x0800, NULL, 10, &rfidTaskHandle, 0);

    TaskHandle_t gpioTaskHandle;
    xTaskCreatePinnedToCore(gpioTask, "gpio", 0x0800, NULL, 10, &gpioTaskHandle, 0);
}

void loop() { delay(10000); }

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>
#include <sd_diskio.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <cmath>
#include <Adafruit_SSD1351.h>
#include <Adafruit_GFX.h>
#include <MFRC522.h>

#define PIN_POWER       GPIO_NUM_27
#define PIN_CS_SD       GPIO_NUM_5
#define PIN_CS_OLED     GPIO_NUM_15
#define PIN_DC_OLED     GPIO_NUM_21
#define PIN_RESET       GPIO_NUM_4
#define PIN_I2S_BCK     GPIO_NUM_26
#define PIN_I2S_WC      GPIO_NUM_22
#define PIN_I2S_DATA    GPIO_NUM_25
#define PIN_RFID_IRQ    GPIO_NUM_33
#define PIN_CS_RFID     GPIO_NUM_2

#define SPI_FREQ_SD     40000000
#define SPI_FREQ_OLED   15000000

#define PLAYBACK_CHUNK_SIZE 1024
#define PLAYBACK_QUEUE_SIZE 8

static SPIClass spiHSPI(HSPI);
static SPIClass spiVSPI(VSPI);

bool probeSd() {
    if (!SD.begin(PIN_CS_SD, spiVSPI, SPI_FREQ_SD)) {
        Serial.println("Failed to initialize SD");
        return false;
    }

    Serial.println("SD card initialized OK");

    File root = SD.open("/");
    if (!root) {
        Serial.println("unable to open SD root");
        return false;
    }

    if (!root.isDirectory()){
        Serial.println("not a directory");
        root.close();
        return false;
    }

    File file;
    while (file = root.openNextFile()) {
        Serial.printf("file: %s\n", file.name());
        file.close();
    }

    root.close();

    return true;
 }

void fsStreamThread(void* payload) {
    void* buffer = malloc(PLAYBACK_CHUNK_SIZE);

    i2s_start(I2S_NUM_0);

    QueueHandle_t *queue = (QueueHandle_t*)payload;
    size_t bytes_written;

    while (true) {
        xQueueReceive(*queue, buffer, portMAX_DELAY);

        i2s_write(I2S_NUM_0, buffer, PLAYBACK_CHUNK_SIZE, &bytes_written, portMAX_DELAY);
    }
}

void audioThread(void* payload) {
    File file = SD.open("/test.wav");

    if (!file) {
        Serial.println("unable to open test.wav from SD");
        return;
    }

    file.seek(44);

    int16_t* buffer = (int16_t*)malloc(PLAYBACK_CHUNK_SIZE);

    QueueHandle_t audioQueue = xQueueCreate(PLAYBACK_QUEUE_SIZE, PLAYBACK_CHUNK_SIZE);

    TaskHandle_t task;
    xTaskCreatePinnedToCore(fsStreamThread, "fsstream", 1024, (void*)&audioQueue, 1, &task, 1);

    size_t bytes_read = 0;
    size_t bytes_written;

    while (true) {
        size_t bytes_read = file.read((uint8_t*)buffer, PLAYBACK_CHUNK_SIZE);

        if (bytes_read < PLAYBACK_CHUNK_SIZE) {
            file.seek(44);
            file.read((uint8_t*)buffer + bytes_read, PLAYBACK_CHUNK_SIZE - bytes_read);
        }

        for (int i = 0; i < PLAYBACK_CHUNK_SIZE / 2; i++) {
            buffer[i] /= 3;
        }

        xQueueSend(audioQueue, (void*)buffer, portMAX_DELAY);
    }
}

void runDisplay() {
    Adafruit_SSD1351 tft = Adafruit_SSD1351(128, 128, &spiHSPI, PIN_CS_OLED, PIN_DC_OLED);
    tft.begin(SPI_FREQ_OLED);

    tft.fillScreen(0);

    while (true) {
        int r = rand();
        int x0 = r & 0x7f, y0 = (r >> 7) & 0x7f, x1 = (r >> 14) & 0x7f, y1 = (r >> 21) & 0x7f;
        r = rand();

        tft.drawLine(x0, y0, x1, y1, tft.color565((r & 0xff) % 99, ((r >> 8) & 0xff) % 99, ((r >> 16) & 0xff) % 99));

        taskYIELD();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("power on");
    pinMode(PIN_POWER, OUTPUT);
    digitalWrite(PIN_POWER, 0);

    delay(250);

    pinMode(PIN_CS_RFID, OUTPUT);
    pinMode(PIN_RESET, OUTPUT);
    pinMode(PIN_CS_SD, OUTPUT);

    digitalWrite(PIN_RESET, 0);
    digitalWrite(PIN_CS_RFID, 1);
    digitalWrite(PIN_CS_SD, 1);
    digitalWrite(PIN_CS_OLED, 1);

    delay(1);

    digitalWrite(PIN_RESET, 1);
    spiHSPI.begin();
    spiVSPI.begin();

    delay(1);

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
        .bck_io_num = PIN_I2S_BCK,
        .ws_io_num = PIN_I2S_WC,
        .data_out_num = PIN_I2S_DATA,
        .data_in_num = -1
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_pins);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    i2s_stop(I2S_NUM_0);

    // Serial.begin(115200);
    Serial.println("Hello world!");

    if (!probeSd()) {
        return;
    }

    TaskHandle_t audioTask;
    xTaskCreatePinnedToCore(audioThread, "playback", 4098, NULL, 10, &audioTask, 1);

   runDisplay();

/*
  MFRC522 mfrc522 = MFRC522(PIN_CS_RFID, MFRC522::UNUSED_PIN, spiHSPI);
  mfrc522.PCD_Init();

  delay(500);

  mfrc522.PCD_DumpVersionToSerial();

  if (mfrc522.PCD_PerformSelfTest()) {
      Serial.println("MFRC522 self test succeeded.");
  } else {
      Serial.println("MFRC522 self test failed.");
  }*/
}

void loop() {}

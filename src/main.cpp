#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <driver/i2s.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <mad.h>

#include "DirectoryPlayer.hxx"

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

static SPIClass spiVSPI(VSPI);

bool probeSd() {
    if (!SD.begin(PIN_CS_SD, spiVSPI, SPI_FREQ_SD)) {
        Serial.println("Failed to initialize SD");
        return false;
    }

    Serial.println("SD card initialized OK");

    return true;
}


void i2sStreamTask(void* payload) {
    void* buffer = malloc(PLAYBACK_CHUNK_SIZE);

    i2s_start(I2S_NUM_0);

    QueueHandle_t *queue = (QueueHandle_t*)payload;
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

        while (samplesDecoded < PLAYBACK_CHUNK_SIZE/4) {
            samplesDecoded += player.decode(buffer, (PLAYBACK_CHUNK_SIZE/4 - samplesDecoded));

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


void setup() {
    Serial.begin(115200);

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

    if (!probeSd()) {
        return;
    }

    TaskHandle_t audioTaskHandle;
    xTaskCreatePinnedToCore(audioTask, "playback", 0xA000, NULL, 10, &audioTaskHandle, 1);
}

void loop() {
}

#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <driver/i2s.h>
#include <sd_diskio.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <cmath>

bool probeSd() {
    if (!SD.begin(SS, SPI, 15000000)) {
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

void audioThread(void* payload) {
    void* buffer = malloc(1024);

    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3,
        .dma_buf_count = 10,
        .dma_buf_len = 60,
        .use_apll = false
    };

    const i2s_pin_config_t i2s_pins = {
        .bck_io_num = GPIO_NUM_26,
        .ws_io_num = GPIO_NUM_22,
        .data_out_num = GPIO_NUM_25,
        .data_in_num = -1
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_pins);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);

    QueueHandle_t *queue = (QueueHandle_t*)payload;
    size_t bytes_written;

    while (true) {
        xQueueReceive(*queue, buffer, portMAX_DELAY);

        i2s_write(I2S_NUM_0, buffer, 1024, &bytes_written, portMAX_DELAY);
    }
}

void playbackAudio() {
    File file = SD.open("/test.wav");

    if (!file) {
        Serial.println("unable to open test.wav from SD");
        return;
    }

    file.seek(44);

    int16_t* buffer = (int16_t*)malloc(1024);

    QueueHandle_t audioQueue = xQueueCreate(4, 1024);

    TaskHandle_t task;
    xTaskCreatePinnedToCore(audioThread, "athread", 1024, (void*)&audioQueue, 10, &task, 1);

    size_t bytes_read = 0;
    size_t bytes_written;

    while (true) {
        size_t bytes_read = file.read((uint8_t*)buffer, 1024);

        if (bytes_read < 1024) {
            file.seek(44);
            file.read((uint8_t*)buffer + bytes_read, 1024 - bytes_read);
        }

        for (int i = 0; i < 512; i++) {
            buffer[i] /= 8;
        }

        xQueueSend(audioQueue, (void*)buffer, portMAX_DELAY);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Hello world!");

    uint32_t* psram_ptr = (uint32_t*) ps_malloc(2* 1024 * 1024);

    Serial.printf("allocated 2MB of PSRAM at %p\n", psram_ptr);

    Serial.println("writing to PSRAM...");

    int i;

    for (i = 0; i < 512*1024; i++) psram_ptr[i] = i;

    Serial.println("verifying PSRAM...");

    for (i = 0; i < 512*1024; i++) {
        if (psram_ptr[i] != i) {
            Serial.printf("RAM differs at %i.\n", i);
            break;
        }
    }

    if (i == 512*1024) {
        Serial.println("RAM verified OK");
    }

    free(psram_ptr);

    if (!probeSd()) {
        return;
    }

    playbackAudio();
}

void loop() {}

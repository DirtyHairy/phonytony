#include "AudioTask.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/i2s.h>

#include "DirectoryPlayer.hxx"

#define COMMAND_QUEUE_SIZE 3
#define VOLUME_STEP 10
#define VOLUME_LIMIT 100
#define VOLUME_FULL 100
#define VOLLUME_DEFAULT 20

namespace {

enum class Command : uint8_t { togglePause, volumeDown, volumeUp, previous, next };

QueueHandle_t commandQueue;

bool paused = false;
int32_t volume = VOLLUME_DEFAULT;

void i2sStreamTask(void* payload) {
    void* buffer = malloc(PLAYBACK_CHUNK_SIZE);

    QueueHandle_t* queue = (QueueHandle_t*)payload;
    size_t bytes_written;

    while (true) {
        xQueueReceive(*queue, buffer, portMAX_DELAY);

        i2s_write(I2S_NUM_0, buffer, PLAYBACK_CHUNK_SIZE, &bytes_written, portMAX_DELAY);
    }
}

void executeCommandPause() {
    paused = !paused;

    if (paused) {
        i2s_stop(I2S_NUM_0);
    } else {
        i2s_start(I2S_NUM_0);
    }
}

void executeCommandVolumeUp() { volume = std::min((int32_t)VOLUME_LIMIT, volume + VOLUME_STEP); }

void executeCommandVolumeDown() { volume = std::max((int32_t)VOLUME_STEP, volume - VOLUME_STEP); }

void receiveAndHandleCommand() {
    Command command;

    if (xQueueReceive(commandQueue, (void*)&command, 0) == pdTRUE) {
        switch (command) {
            case Command::togglePause:
                executeCommandPause();
                break;

            case Command::volumeUp:
                executeCommandVolumeUp();
                break;

            case Command::volumeDown:
                executeCommandVolumeDown();
                break;

            default:
                Serial.printf("unhandled audio command: %i\r\n", (int)command);
        }
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
    xTaskCreatePinnedToCore(i2sStreamTask, "i2s", STACK_SIZE_I2S, (void*)&audioQueue, TASK_PRIORITY_I2S, &task,
                            AUDIO_CORE);

    paused = false;
    i2s_start(I2S_NUM_0);

    size_t samplesDecoded = 0;

    while (true) {
        receiveAndHandleCommand();

        if (paused) {
            delay(100);
        } else {
            samplesDecoded = 0;

            while (samplesDecoded < PLAYBACK_CHUNK_SIZE / 4) {
                samplesDecoded += player.decode(buffer, (PLAYBACK_CHUNK_SIZE / 4 - samplesDecoded));

                if (player.isFinished()) player.rewind();
            }

            for (int i = 0; i < PLAYBACK_CHUNK_SIZE / 2; i++) {
                buffer[i] = (static_cast<int32_t>(buffer[i]) * volume) / VOLUME_FULL;
            }

            xQueueSend(audioQueue, (void*)buffer, portMAX_DELAY);
        }
    }
}

void audioTask(void* payload) {
    audioTask_();
    vTaskDelete(NULL);
}

void setupI2s() {
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
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
    i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    i2s_stop(I2S_NUM_0);
}

void dispatchCommand(Command command) { xQueueSend(commandQueue, (void*)&command, portMAX_DELAY); }

}  // namespace

void AudioTask::initialize() { commandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(Command)); }

void AudioTask::start() {
    setupI2s();

    TaskHandle_t audioTaskHandle;
    xTaskCreatePinnedToCore(audioTask, "audio", STACK_SIZE_AUDIO, NULL, TASK_PRIORITY_AUDIO, &audioTaskHandle,
                            AUDIO_CORE);
}

void AudioTask::togglePause() { dispatchCommand(Command::togglePause); }

void AudioTask::volumeUp() { dispatchCommand(Command::volumeUp); }

void AudioTask::volumeDown() { dispatchCommand(Command::volumeDown); }

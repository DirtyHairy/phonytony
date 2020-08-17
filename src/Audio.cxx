#include "Audio.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/i2s.h>
#include <atomic>

#include "DirectoryPlayer.hxx"
#include "Gpio.hxx"

#define COMMAND_QUEUE_SIZE 3
#define I2S_NUM I2S_NUM_0

namespace {

enum class Command : uint8_t { togglePause, volumeDown, volumeUp, previous, next, rewind };

struct Chunk {
    bool paused;
    bool clearDmaBufferOnResume;

    int16_t samples[PLAYBACK_CHUNK_SIZE];
};

QueueHandle_t commandQueue;
QueueHandle_t audioQueue;

bool paused = false;
std::atomic<bool> shutdown;
bool clearDmaBufferOnResume = false;
int32_t volume = VOLLUME_DEFAULT;

DirectoryPlayer player;

void i2sStreamTask(void* payload) {
    Chunk* chunk = new Chunk();

    QueueHandle_t* queue = (QueueHandle_t*)payload;
    size_t bytes_written;
    bool wasPaused = true;

    while (true) {
        xQueueReceive(*queue, chunk, portMAX_DELAY);

        if (chunk->paused && !wasPaused) i2s_stop(I2S_NUM);

        if (!chunk->paused && wasPaused) {
            if (chunk->clearDmaBufferOnResume) i2s_zero_dma_buffer(I2S_NUM);
            i2s_start(I2S_NUM);
        }

        wasPaused = chunk->paused;

        if (!paused) i2s_write(I2S_NUM, chunk->samples, PLAYBACK_CHUNK_SIZE, &bytes_written, portMAX_DELAY);
    }
}

void resetAudio() {
    if (paused) {
        clearDmaBufferOnResume = true;
        xQueueReset(audioQueue);
    }
}

void receiveAndHandleCommand(bool block) {
    Command command;

    if (xQueueReceive(commandQueue, (void*)&command, block ? portMAX_DELAY : 0) == pdTRUE) {
        switch (command) {
            case Command::togglePause:
                paused = !paused;
                break;

            case Command::volumeUp:
                volume = std::min((int32_t)VOLUME_LIMIT, volume + VOLUME_STEP);
                break;

            case Command::volumeDown:
                volume = std::max((int32_t)VOLUME_STEP, volume - VOLUME_STEP);
                break;

            case Command::previous:
                resetAudio();

                if (player.trackPosition() / (SAMPLE_RATE / 1000) < REWIND_TIMEOUT)
                    player.previousTrack();
                else
                    player.rewindTrack();

                break;

            case Command::next:
                resetAudio();
                player.nextTrack();

                break;

            case Command::rewind:
                resetAudio();
                player.rewind();

                break;

            default:
                Serial.printf("unhandled audio command: %i\r\n", (int)command);
        }
    }
}

void audioTask_() {
    if (!player.open("/album")) {
        Serial.println("unable to open /album");
        return;
    }

    Chunk* chunk = new Chunk();

    Gpio::enableAmp();

    TaskHandle_t task;
    xTaskCreatePinnedToCore(i2sStreamTask, "i2s", STACK_SIZE_I2S, (void*)&audioQueue, TASK_PRIORITY_I2S, &task,
                            AUDIO_CORE);

    paused = false;
    clearDmaBufferOnResume = false;

    while (true) {
        receiveAndHandleCommand(paused || shutdown);

        chunk->paused = paused || shutdown;
        chunk->clearDmaBufferOnResume = clearDmaBufferOnResume;

        if (!paused) {
            clearDmaBufferOnResume = false;
            size_t samplesDecoded = 0;

            while (samplesDecoded < PLAYBACK_CHUNK_SIZE / 4) {
                samplesDecoded += player.decode(chunk->samples, (PLAYBACK_CHUNK_SIZE / 4 - samplesDecoded));

                if (player.isFinished()) player.rewind();
            }

            for (int i = 0; i < PLAYBACK_CHUNK_SIZE / 2; i++) {
                chunk->samples[i] = (static_cast<int32_t>(chunk->samples[i]) * volume) / VOLUME_FULL;
            }
        }

        xQueueSend(audioQueue, (void*)chunk, portMAX_DELAY);
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

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &i2s_pins);
    i2s_set_clk(I2S_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    i2s_stop(I2S_NUM);
}

void dispatchCommand(Command command) { xQueueSend(commandQueue, (void*)&command, portMAX_DELAY); }

}  // namespace

void Audio::initialize() {
    commandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(Command));
    audioQueue = xQueueCreate(PLAYBACK_QUEUE_SIZE, sizeof(Chunk));
    shutdown = false;
}

void Audio::start() {
    setupI2s();

    TaskHandle_t audioTaskHandle;
    xTaskCreatePinnedToCore(audioTask, "audio", STACK_SIZE_AUDIO, NULL, TASK_PRIORITY_AUDIO, &audioTaskHandle,
                            AUDIO_CORE);
}

void Audio::togglePause() { dispatchCommand(Command::togglePause); }

void Audio::volumeUp() { dispatchCommand(Command::volumeUp); }

void Audio::volumeDown() { dispatchCommand(Command::volumeDown); }

void Audio::previous() { dispatchCommand(Command::previous); }

void Audio::next() { dispatchCommand(Command::next); }

void Audio::rewind() { dispatchCommand(Command::rewind); }

void Audio::prepareSleep() { shutdown = true; }

#include "Audio.hxx"

// clang-format off
#include <freertos/FreeRTOS.h>
// clang-format on

#include <driver/i2s.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <atomic>

#include "DirectoryPlayer.hxx"
#include "Gpio.hxx"
#include "Lock.hxx"
#include "Log.hxx"
#include "Power.hxx"
#include "Signal.hxx"
#include "Watchdog.hxx"

#define TAG "audio"

#define COMMAND_QUEUE_SIZE 3
#define I2S_NUM I2S_NUM_0

namespace {

struct Chunk {
    bool paused;
    bool clearDmaBufferOnResume;

    int16_t samples[PLAYBACK_CHUNK_SIZE / 2];
};

struct State {
    int32_t volume;

    char album[256];
    uint32_t track;
    size_t position;

    void setAlbum(const char* album) { strncpy(this->album, album, 255); }

    void clearAlbum() { album[0] = 0; }

    bool hasAlbum() { return album[0] != 0; }

    State& operator=(const State& state) {
        if (this != &state) {
            this->volume = state.volume;
            this->track = state.track;
            this->position = state.position;

            setAlbum(state.album);
        }

        return *this;
    }
};

struct Command {
    enum Type : uint8_t {
        cmdTogglePause,
        cmdVolumeDown,
        cmdVolumeUp,
        cmdPrevious,
        cmdNext,
        cmdRewind,
        cmdPlay,
        cmdSignalError
    };

    Type type;
    char album[256];

    Command(Type type) : type(type) {}
    Command() {}

    void setAlbum(const char* album) { strncpy(this->album, album, 255); }
};

QueueHandle_t commandQueue;
QueueHandle_t audioQueue;

std::atomic<bool> paused;
std::atomic<bool> shutdown;
bool clearDmaBufferOnResume = false;
int32_t volume = VOLLUME_DEFAULT;

State state;
RTC_SLOW_ATTR State persistentState;
SemaphoreHandle_t stateMutex;

Signal signal;
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

        if (!chunk->paused) i2s_write(I2S_NUM, chunk->samples, PLAYBACK_CHUNK_SIZE, &bytes_written, portMAX_DELAY);
    }
}

void resetAudio() {
    if (paused) {
        clearDmaBufferOnResume = true;
        xQueueReset(audioQueue);
    }
}

void setVolume(int32_t newVolume) {
    Lock lock(stateMutex);

    state.volume = volume = newVolume;
}

void updatePlaybackState() {
    Lock lock(stateMutex);

    state.track = player.getTrack();
    state.position = player.getSeekPosition();
}

std::string directoryForAlbum(const char* album) { return std::string("/music/") + std::string(album); }

void play(const char* album) {
    Lock lock(stateMutex);

    if (strcmp(state.album, album) == 0 && player.isValid()) {
        player.rewind();
        paused = false;
    } else {
        paused = !player.open(directoryForAlbum(album).c_str());
    }

    if (paused) {
        state.clearAlbum();
        LOG_WARN(TAG, "failed to open album %s", album);
    } else {
        state.setAlbum(album);
        LOG_INFO(TAG, "playback switched to %s", album);
    }
}

void receiveAndHandleCommand(bool block) {
    Command command;

    if (xQueueReceive(commandQueue, (void*)&command, block ? portMAX_DELAY : 0) == pdTRUE) {
        switch (command.type) {
            case Command::cmdTogglePause:
                paused = !paused;
                break;

            case Command::cmdVolumeUp:
                setVolume(std::min((int32_t)VOLUME_LIMIT, volume + VOLUME_STEP));

                break;

            case Command::cmdVolumeDown:
                setVolume(state.volume = volume = std::max((int32_t)VOLUME_STEP, volume - VOLUME_STEP));

                break;

            case Command::cmdPrevious:
                resetAudio();

                if (player.getTrackPosition() / (SAMPLE_RATE / 1000) < REWIND_TIMEOUT)
                    player.previousTrack();
                else
                    player.rewindTrack();

                updatePlaybackState();

                break;

            case Command::cmdNext:
                resetAudio();
                player.nextTrack();

                updatePlaybackState();

                break;

            case Command::cmdRewind:
                resetAudio();
                player.rewind();

                updatePlaybackState();

                break;

            case Command::cmdPlay:
                LOG_INFO(TAG, "switching playback to %s", command.album);

                resetAudio();

                play(command.album);

                if (!paused) {
                    signal.start(Signal::commandReceived);
                    updatePlaybackState();
                }

                break;

            case Command::cmdSignalError:
                signal.start(Signal::error);
                break;

            default:
                LOG_ERROR(TAG, "unhandled audio command: %i", (int)command.type);
        }
    }
}

bool tryToRestore() {
    if (!Power::isResumeFromSleep()) return false;

    Lock lock(stateMutex);

    volume = state.volume;

    if (!(state.hasAlbum() && player.open(directoryForAlbum(state.album).c_str(), state.track))) return false;
    if (player.getTrack() == state.track) player.seekTo(state.position);

    return true;
}

bool pauseI2s() { return ((!player.isValid() || paused) && !signal.isActive()) || shutdown; }

void audioTask_() {
    paused = !tryToRestore();

    Chunk* chunk = new Chunk();

    Gpio::enableAmp();

    TaskHandle_t task;
    xTaskCreatePinnedToCore(i2sStreamTask, "i2s", STACK_SIZE_I2S, (void*)&audioQueue, TASK_PRIORITY_I2S, &task,
                            AUDIO_CORE);

    clearDmaBufferOnResume = false;

    while (true) {
        Watchdog::notify();

        receiveAndHandleCommand(pauseI2s());

        chunk->paused = pauseI2s();
        chunk->clearDmaBufferOnResume = clearDmaBufferOnResume;

        if (!chunk->paused) {
            clearDmaBufferOnResume = false;
            size_t samplesDecoded = 0;

            while (samplesDecoded < PLAYBACK_CHUNK_SIZE / 4) {
                if (signal.isActive()) {
                    samplesDecoded += signal.play(chunk->samples, (PLAYBACK_CHUNK_SIZE / 4 - samplesDecoded));
                } else if (!paused && player.isValid()) {
                    samplesDecoded += player.decode(chunk->samples, (PLAYBACK_CHUNK_SIZE / 4 - samplesDecoded));

                    if (player.isFinished()) {
                        player.rewind();
                        paused = true;
                    }
                } else {
                    memset(chunk->samples + 2 * samplesDecoded, 0, PLAYBACK_CHUNK_SIZE - samplesDecoded * 4);

                    break;
                }
            }

            for (int i = 0; i < PLAYBACK_CHUNK_SIZE / 2; i++) {
                chunk->samples[i] = (static_cast<int32_t>(chunk->samples[i]) * volume) / VOLUME_FULL;
            }

            updatePlaybackState();
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

void dispatchCommand(const Command& command) { xQueueSend(commandQueue, (void*)&command, portMAX_DELAY); }

void dispatchCommand(Command::Type type) { dispatchCommand(Command(type)); }

}  // namespace

void Audio::initialize() {
    commandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(Command));
    audioQueue = xQueueCreate(PLAYBACK_QUEUE_SIZE, sizeof(Chunk));

    stateMutex = xSemaphoreCreateMutex();

    if (Power::isResumeFromSleep()) {
        Lock lock(stateMutex);

        state = persistentState;
    } else {
        Lock lock(stateMutex);

        state.volume = volume;
        state.clearAlbum();
    }

    shutdown = false;
}

void Audio::start() {
    setupI2s();

    TaskHandle_t audioTaskHandle;
    xTaskCreatePinnedToCore(audioTask, "audio", STACK_SIZE_AUDIO, NULL, TASK_PRIORITY_AUDIO, &audioTaskHandle,
                            AUDIO_CORE);
}

void Audio::togglePause() { dispatchCommand(Command::cmdTogglePause); }

void Audio::volumeUp() { dispatchCommand(Command::cmdVolumeUp); }

void Audio::volumeDown() { dispatchCommand(Command::cmdVolumeDown); }

void Audio::previous() { dispatchCommand(Command::cmdPrevious); }

void Audio::next() { dispatchCommand(Command::cmdNext); }

void Audio::rewind() { dispatchCommand(Command::cmdRewind); }

void Audio::play(const char* album) {
    Command command(Command::cmdPlay);
    command.setAlbum(album);

    dispatchCommand(command);
}

void Audio::prepareSleep() {
    shutdown = true;

    Lock lock(stateMutex);
    persistentState = state;
}

bool Audio::isPlaying() { return player.isValid() && !paused; }

void Audio::signalError() { dispatchCommand(Command::cmdSignalError); }

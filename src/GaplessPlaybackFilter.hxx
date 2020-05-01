#ifndef GAPLESS_PLAYBACK_FILTER_HXX
#define GAPLESS_PLAYBACK_FILTER_HXX

#include <cstdint>

class GaplessPlaybackFilter {
   public:
    GaplessPlaybackFilter(uint32_t maxSilenceStart = 3000, uint32_t maxSilenceEnd = 3000);

    void reset();

    void end();

    bool needsInput() const;

    void push(int16_t sampleL, int16_t sampleR);

    bool nextSample(int16_t& sampleL, int16_t& sampleR);

   private:
    enum class State { recordingSilence, playback };
    enum class StreamPosition { start, within, end };

    const uint32_t maxSilenceStart;
    const uint32_t maxSilenceEnd;

    State state{State::recordingSilence};
    StreamPosition streamPosition{StreamPosition::start};

    int16_t pendingSampleL{0};
    int16_t pendingSampleR{0};
    bool samplePending{false};

    uint32_t zeroes{0};
};

#endif  // GAPLESS_PLAYBACK_FILTER

#include "./GaplessPlaybackFilter.hxx"

#include <iostream>

GaplessPlaybackFilter::GaplessPlaybackFilter(uint32_t maxSilenceStart, uint32_t maxSilenceEnd)
    : maxSilenceStart(maxSilenceStart), maxSilenceEnd(maxSilenceEnd) {}

void GaplessPlaybackFilter::reset() {
    state = State::recordingSilence;
    streamPosition = StreamPosition::start;

    pendingSampleL = 0;
    pendingSampleR = 0;
    samplePending = false;

    zeroes = 0;
}

void GaplessPlaybackFilter::end() {
    if (state == State::recordingSilence &&
        zeroes <= (streamPosition == StreamPosition::start ? maxSilenceStart + maxSilenceEnd : maxSilenceEnd))
        zeroes = 0;

    streamPosition = StreamPosition::end;
    state = State::playback;
}

bool GaplessPlaybackFilter::needsInput() const {
    return (state == State::recordingSilence) ||
           (zeroes == 0 && !samplePending && streamPosition != StreamPosition::end);
}

void GaplessPlaybackFilter::push(int16_t sampleL, int16_t sampleR) {
    if (streamPosition == StreamPosition::end) return;

    if (sampleL == 0 && sampleR == 0) {
        zeroes++;
        state = State::recordingSilence;
    } else {
        this->pendingSampleL = sampleL;
        this->pendingSampleR = sampleR;
        samplePending = true;

        if (state == State::recordingSilence && streamPosition == StreamPosition::start && zeroes <= maxSilenceStart)
            zeroes = 0;

        state = State::playback;
        streamPosition = StreamPosition::within;
    }
}

bool GaplessPlaybackFilter::nextSample(int16_t& sampleL, int16_t& sampleR) {
    if (state == State::recordingSilence) return false;

    if (zeroes > 0) {
        sampleL = sampleR = 0;
        zeroes--;

        return true;
    }

    if (samplePending) {
        sampleL = pendingSampleR;
        sampleR = pendingSampleR;
        samplePending = false;

        return true;
    }

    return false;
}

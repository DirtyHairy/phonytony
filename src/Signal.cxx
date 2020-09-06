#include "Signal.hxx"

#include <Arduino.h>

#include <cmath>

#define SAMPLE_RATE 44100

namespace {

const Signal::Step signalCommandReceived[] = {{.note = 0, .duration = 50, .amplitude = 0},
                                              {.note = 36, .duration = 100, .amplitude = 0x0fff},
                                              {.note = 0, .duration = 50, .amplitude = 0},
                                              {.note = 0, .duration = 0, .amplitude = 0}};

const Signal::Step* signals[] = {signalCommandReceived};

const float frequencies[49] = {0.,
                               130.8127826502992,
                               138.59131548843592,
                               146.83238395870364,
                               155.56349186104035,
                               164.81377845643485,
                               174.6141157165018,
                               184.9972113558171,
                               195.99771799087452,
                               207.65234878997245,
                               219.9999999999999,
                               233.08188075904485,
                               246.94165062806195,
                               261.6255653005985,
                               277.18263097687196,
                               293.66476791740746,
                               311.12698372208087,
                               329.62755691286986,
                               349.22823143300377,
                               369.9944227116344,
                               391.99543598174927,
                               415.30469757994507,
                               440,
                               466.1637615180899,
                               493.8833012561241,
                               523.2511306011974,
                               554.3652619537443,
                               587.3295358348153,
                               622.253967444162,
                               659.2551138257401,
                               698.456462866008,
                               739.9888454232691,
                               783.9908719634989,
                               830.6093951598906,
                               880.0000000000005,
                               932.3275230361803,
                               987.7666025122487,
                               1046.5022612023952,
                               1108.730523907489,
                               1174.659071669631,
                               1244.5079348883246,
                               1318.5102276514806,
                               1396.912925732017,
                               1479.977690846539,
                               1567.9817439269987,
                               1661.218790319782,
                               1760.0000000000018,
                               1864.6550460723615,
                               1975.5332050244986};

}  // namespace

void Signal::start(Type type) {
    active = true;
    step = signals[type];
    currentSampleIndex = 0;
}

uint32_t Signal::play(int16_t* buffer, uint32_t count) {
    if (!active) return 0;

    uint32_t samplesGenerated = 0;

    while (samplesGenerated < count) {
        if ((1000 * currentSampleIndex) / SAMPLE_RATE > step->duration) {
            step++;

            if (step->duration == 0) {
                active = false;
                break;
            }

            currentSampleIndex = 0;
        }

        buffer[0] = buffer[1] = floorf(step->amplitude * sinf(currentSampleIndex / static_cast<float>(SAMPLE_RATE) *
                                                              frequencies[step->note] * 2. * PI));

        buffer += 2;
        currentSampleIndex++;
        samplesGenerated++;
    }

    return samplesGenerated;
}

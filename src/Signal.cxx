#include "Signal.hxx"

#include <cmath>

#include "Lock.hxx"

#define SAMPLE_RATE 44100
#define _PI 3.1415926535897932384626433832795f
#define AMPLITUDE 0x0fff

namespace {

const Signal::Step signalCommandReceived[] = {{.note = 0, .duration = 50, .amplitude = 0},
                                              {.note = 36, .duration = 100, .amplitude = AMPLITUDE},
                                              {.note = 0, .duration = 50, .amplitude = 0},
                                              {.note = 0, .duration = 0, .amplitude = 0}};

const Signal::Step signalError[] = {{.note = 0, .duration = 100, .amplitude = 0},
                                    {.note = 34, .duration = 200, .amplitude = AMPLITUDE},
                                    {.note = 22, .duration = 200, .amplitude = AMPLITUDE},
                                    {.note = 0, .duration = 100, .amplitude = 0},
                                    {.note = 0, .duration = 0, .amplitude = 0}};

const Signal::Step* signals[] = {signalCommandReceived, signalError};

const float frequencyFactors[49] = {0.,
                                    130.8127826502992f * 2.f * _PI / SAMPLE_RATE,
                                    138.59131548843592f * 2.f * _PI / SAMPLE_RATE,
                                    146.83238395870364f * 2.f * _PI / SAMPLE_RATE,
                                    155.56349186104035f * 2.f * _PI / SAMPLE_RATE,
                                    164.81377845643485f * 2.f * _PI / SAMPLE_RATE,
                                    174.6141157165018f * 2.f * _PI / SAMPLE_RATE,
                                    184.9972113558171f * 2.f * _PI / SAMPLE_RATE,
                                    195.99771799087452f * 2.f * _PI / SAMPLE_RATE,
                                    207.65234878997245f * 2.f * _PI / SAMPLE_RATE,
                                    219.9999999999999f * 2.f * _PI / SAMPLE_RATE,
                                    233.08188075904485f * 2.f * _PI / SAMPLE_RATE,
                                    246.94165062806195f * 2.f * _PI / SAMPLE_RATE,
                                    261.6255653005985f * 2.f * _PI / SAMPLE_RATE,
                                    277.18263097687196f * 2.f * _PI / SAMPLE_RATE,
                                    293.66476791740746f * 2.f * _PI / SAMPLE_RATE,
                                    311.12698372208087f * 2.f * _PI / SAMPLE_RATE,
                                    329.62755691286986f * 2.f * _PI / SAMPLE_RATE,
                                    349.22823143300377f * 2.f * _PI / SAMPLE_RATE,
                                    369.9944227116344f * 2.f * _PI / SAMPLE_RATE,
                                    391.99543598174927f * 2.f * _PI / SAMPLE_RATE,
                                    415.30469757994507f * 2.f * _PI / SAMPLE_RATE,
                                    440.f * 2.f * _PI / SAMPLE_RATE,
                                    466.1637615180899f * 2.f * _PI / SAMPLE_RATE,
                                    493.8833012561241f * 2.f * _PI / SAMPLE_RATE,
                                    523.2511306011974f * 2.f * _PI / SAMPLE_RATE,
                                    554.3652619537443f * 2.f * _PI / SAMPLE_RATE,
                                    587.3295358348153f * 2.f * _PI / SAMPLE_RATE,
                                    622.253967444162f * 2.f * _PI / SAMPLE_RATE,
                                    659.2551138257401f * 2.f * _PI / SAMPLE_RATE,
                                    698.456462866008f * 2.f * _PI / SAMPLE_RATE,
                                    739.9888454232691f * 2.f * _PI / SAMPLE_RATE,
                                    783.9908719634989f * 2.f * _PI / SAMPLE_RATE,
                                    830.6093951598906f * 2.f * _PI / SAMPLE_RATE,
                                    880.0000000000005f * 2.f * _PI / SAMPLE_RATE,
                                    932.3275230361803f * 2.f * _PI / SAMPLE_RATE,
                                    987.7666025122487f * 2.f * _PI / SAMPLE_RATE,
                                    1046.5022612023952f * 2.f * _PI / SAMPLE_RATE,
                                    1108.730523907489f * 2.f * _PI / SAMPLE_RATE,
                                    1174.659071669631f * 2.f * _PI / SAMPLE_RATE,
                                    1244.5079348883246f * 2.f * _PI / SAMPLE_RATE,
                                    1318.5102276514806f * 2.f * _PI / SAMPLE_RATE,
                                    1396.912925732017f * 2.f * _PI / SAMPLE_RATE,
                                    1479.977690846539f * 2.f * _PI / SAMPLE_RATE,
                                    1567.9817439269987f * 2.f * _PI / SAMPLE_RATE,
                                    1661.218790319782f * 2.f * _PI / SAMPLE_RATE,
                                    1760.0000000000018f * 2.f * _PI / SAMPLE_RATE,
                                    1864.6550460723615f * 2.f * _PI / SAMPLE_RATE,
                                    1975.5332050244986 * 2.f * _PI / SAMPLE_RATE};
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

        buffer[0] = buffer[1] = floorf(step->amplitude * sinf(currentSampleIndex * frequencyFactors[step->note]));

        buffer += 2;
        currentSampleIndex++;
        samplesGenerated++;
    }

    return samplesGenerated;
}

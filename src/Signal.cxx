#include "Signal.hxx"

#include <cmath>

#define SAMPLE_RATE 44100
#define _PI 3.1415926535897932384626433832795f

namespace {

const Signal::Step signalCommandReceived[] = {{.note = 0, .duration = 50, .amplitude = 0},
                                              {.note = 36, .duration = 100, .amplitude = 0x0fff},
                                              {.note = 0, .duration = 50, .amplitude = 0},
                                              {.note = 0, .duration = 0, .amplitude = 0}};

const Signal::Step* signals[] = {signalCommandReceived};

const float frequencies[49] = {0.,
                               130.8127826502992f,
                               138.59131548843592f,
                               146.83238395870364f,
                               155.56349186104035f,
                               164.81377845643485f,
                               174.6141157165018f,
                               184.9972113558171f,
                               195.99771799087452f,
                               207.65234878997245f,
                               219.9999999999999f,
                               233.08188075904485f,
                               246.94165062806195f,
                               261.6255653005985f,
                               277.18263097687196f,
                               293.66476791740746f,
                               311.12698372208087f,
                               329.62755691286986f,
                               349.22823143300377f,
                               369.9944227116344f,
                               391.99543598174927f,
                               415.30469757994507f,
                               440.f,
                               466.1637615180899f,
                               493.8833012561241f,
                               523.2511306011974f,
                               554.3652619537443f,
                               587.3295358348153f,
                               622.253967444162f,
                               659.2551138257401f,
                               698.456462866008f,
                               739.9888454232691f,
                               783.9908719634989f,
                               830.6093951598906f,
                               880.0000000000005f,
                               932.3275230361803f,
                               987.7666025122487f,
                               1046.5022612023952f,
                               1108.730523907489f,
                               1174.659071669631f,
                               1244.5079348883246f,
                               1318.5102276514806f,
                               1396.912925732017f,
                               1479.977690846539f,
                               1567.9817439269987f,
                               1661.218790319782f,
                               1760.0000000000018f,
                               1864.6550460723615f,
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
                                                              frequencies[step->note] * 2.f * _PI));

        buffer += 2;
        currentSampleIndex++;
        samplesGenerated++;
    }

    return samplesGenerated;
}

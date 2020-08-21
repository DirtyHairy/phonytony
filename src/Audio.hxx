#ifndef AUDIO_HXX
#define AUDIO_HXX

#include "config.h"

namespace Audio {

void initialize();

void start();

void togglePause();
void volumeUp();
void volumeDown();
void previous();
void next();
void rewind();

void prepareSleep();

bool isPaused();

}  // namespace Audio

#endif  // AUDIO_HXX

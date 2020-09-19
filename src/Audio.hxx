#ifndef AUDIO_HXX
#define AUDIO_HXX

#include "config.h"

namespace Audio {

void initialize();

void start(bool silent);

void togglePause();
void volumeUp();
void volumeDown();
void previous();
void next();
void rewind();
void play(const char* album);

void stop();

bool isPlaying();

void signalError();
void signalCommandReceived();

}  // namespace Audio

#endif  // AUDIO_HXX

#ifndef DIRECTORY_PLAYER_HXX
#define DIRECTORY_PLAYER_HXX

#include <stdint.h>

#include "DirectoryReader.hxx"
#include "MadDecoder.hxx"

class DirectoryPlayer {
   public:
    DirectoryPlayer();

    bool open(const char* directory);

    uint32_t decode(int16_t* buffer, uint32_t count);

    bool isFinished() { return trackIndex >= directoryReader.getLength(); }

    void rewind();

    void rewindTrack();

    void previousTrack();

    void nextTrack();

    bool goToTrack(uint32_t index);
    uint32_t getTrack() { return trackIndex; }

    void seekTo(size_t seekPosition);
    size_t getSeekPosition();

    uint32_t getTrackPosition() const;

    void close();

   private:
    MadDecoder decoder;

    DirectoryReader directoryReader;

    uint32_t trackIndex{0};
};

#endif  // DIRECTORY_PLAYER_HXX

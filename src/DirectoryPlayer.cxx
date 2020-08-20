#include "DirectoryPlayer.hxx"

DirectoryPlayer::DirectoryPlayer() {}

bool DirectoryPlayer::open(const char* dirname, uint32_t track) {
    if (!directoryReader.open(dirname)) return false;

    trackIndex = track < directoryReader.getLength() ? track : 0;
    decoder.open(directoryReader.getTrack(trackIndex));

    return true;
}

void DirectoryPlayer::rewind() {
    trackIndex = 0;

    decoder.open(directoryReader.getTrack(trackIndex));
}

void DirectoryPlayer::previousTrack() {
    if (trackIndex == 0) {
        rewindTrack();
        return;
    }

    decoder.open(directoryReader.getTrack(--trackIndex));
}

void DirectoryPlayer::nextTrack() {
    trackIndex = (trackIndex + 1) % directoryReader.getLength();

    decoder.open(directoryReader.getTrack(trackIndex));
}

bool DirectoryPlayer::goToTrack(uint32_t index) {
    if (index >= directoryReader.getLength()) return false;

    trackIndex = index;

    decoder.open(directoryReader.getTrack(index));

    return true;
}

uint32_t DirectoryPlayer::decode(int16_t* buffer, uint32_t count) {
    uint32_t decodedSamples = 0;

    while (decodedSamples < count && trackIndex < directoryReader.getLength()) {
        if (!decoder.isFinished()) {
            uint32_t decoded = decoder.decode(buffer, count - decodedSamples);

            buffer += 2 * decoded;
            decodedSamples += decoded;
        }

        if (decoder.isFinished()) {
            if (trackIndex < directoryReader.getLength())
                nextTrack();
            else
                rewind();
        }
    }

    return decodedSamples;
}

void DirectoryPlayer::close() {
    directoryReader.close();
    decoder.close();
}

void DirectoryPlayer::rewindTrack() { return decoder.rewind(); }

uint32_t DirectoryPlayer::getTrackPosition() const { return decoder.getPosition(); }

void DirectoryPlayer::seekTo(size_t frame) { decoder.seekTo(frame); }

size_t DirectoryPlayer::getSeekPosition() { return decoder.getSeekPosition(); }

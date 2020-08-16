#include "DirectoryPlayer.hxx"

DirectoryPlayer::DirectoryPlayer() {}

bool DirectoryPlayer::open(const char* dirname) {
    if (!directoryReader.open(dirname)) return false;

    rewind();

    return true;
}

void DirectoryPlayer::rewind() {
    // for (trackIndex = 0; trackIndex < directoryReader.getLength(); trackIndex++)
    //     if (decoder.open(directoryReader.getTrack(trackIndex))) break;
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

uint32_t DirectoryPlayer::trackPosition() const { return decoder.position(); }

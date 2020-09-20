#include "DirectoryPlayer.hxx"

DirectoryPlayer::DirectoryPlayer() {}

bool DirectoryPlayer::open(const char* dirname, uint32_t track) {
    valid = false;
    this->dirname = dirname;

    if (directoryReader.open(dirname) && directoryReader.getLength() > 0) {
        trackIndex = track < directoryReader.getLength() ? track : 0;
        openTrack(trackIndex);

        valid = true;
    }

    return valid;
}

bool DirectoryPlayer::isValid() const { return valid; }

void DirectoryPlayer::rewind() {
    trackIndex = 0;

    openTrack(trackIndex);
}

void DirectoryPlayer::previousTrack() {
    if (directoryReader.getLength() == 0) return;

    if (trackIndex == 0) {
        rewindTrack();
        return;
    }

    openTrack(--trackIndex);
}

void DirectoryPlayer::nextTrack() {
    if (directoryReader.getLength() == 0) return;

    trackIndex = (trackIndex + 1) % directoryReader.getLength();

    openTrack(trackIndex);
}

bool DirectoryPlayer::goToTrack(uint32_t index) {
    if (index >= directoryReader.getLength()) return false;

    trackIndex = index;

    openTrack(index);

    return true;
}

uint32_t DirectoryPlayer::decode(int16_t* buffer, uint32_t count) {
    uint32_t decodedSamples = 0;

    while (decodedSamples < count && !isFinished()) {
        if (!decoder.isFinished()) {
            uint32_t decoded = decoder.decode(buffer, count - decodedSamples);

            buffer += 2 * decoded;
            decodedSamples += decoded;
        }

        if (decoder.isFinished()) {
            if (++trackIndex < directoryReader.getLength()) {
                openTrack(trackIndex);
            } else {
                decoder.close();
            }
        }
    }

    return decodedSamples;
}

void DirectoryPlayer::close() {
    directoryReader.close();
    decoder.close();
}

void DirectoryPlayer::openTrack(uint32_t index) {
    std::string path = dirname + "/" + directoryReader.getTrack(trackIndex);
    decoder.open(path.c_str());
}

void DirectoryPlayer::rewindTrack() { return decoder.rewind(); }

uint32_t DirectoryPlayer::getTrackPosition() const { return decoder.getPosition(); }

void DirectoryPlayer::seekTo(size_t frame) { decoder.seekTo(frame); }

size_t DirectoryPlayer::getSeekPosition() { return decoder.getSeekPosition(); }

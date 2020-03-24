#include "DirectoryPlayer.hxx"

DirectoryPlayer::DirectoryPlayer() :
    trackIndex(0)
{}

bool DirectoryPlayer::open(const char* dirname) {
    if (!directoryReader.open(dirname)) return false;

    rewind();

    return true;
}

void DirectoryPlayer::rewind() {
    for (trackIndex = 0; trackIndex < directoryReader.getLength(); trackIndex++)
        if (decoder.open(directoryReader.getTrack(trackIndex))) break;
}

uint32_t DirectoryPlayer::decode(int16_t* buffer, uint32_t count) {
    uint32_t decodedSamples = 0;

    while (decodedSamples < count && trackIndex < directoryReader.getLength()) {
        if (!decoder.isFinished()) {
            uint32_t decoded = decoder.decode(buffer, count - decodedSamples);

            buffer += 2*decoded;
            decodedSamples += decoded;
        }

        if (decoder.isFinished()) {
            decoder.open(directoryReader.getTrack(++trackIndex));
        }
    }

    return decodedSamples;
}

void DirectoryPlayer::close() {
    directoryReader.close();
    decoder.close();
}

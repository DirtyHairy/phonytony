#include "MadDecoder.hxx"

#include <Arduino.h>

#include <iostream>

#include "Log.hxx"

#define TAG "mp3"

MadDecoder::MadDecoder() {}

MadDecoder::~MadDecoder() { close(); }

bool MadDecoder::open(const char* path) {
    if (initialized) close();

    file = fopen(path, "r");

    if (!file) return false;

    LOG_INFO(TAG, "now playing %s", path);

    if (!reset()) {
        close();

        return false;
    }

    LOG_DEBUG(TAG, "decoder initialized for file %s", path);

    return true;
}

bool MadDecoder::reset(size_t seekPosition) {
    if (!file) return false;

    deinit();

    mad_stream_init(&stream);
    mad_frame_init(&frame);
    mad_synth_init(&synth);
    mad_stream_options(&stream, 0);

    sampleNo = 0;
    sampleCount = 0;
    ns = 0;
    nsMax = 0;
    iBufferGuard = 0;
    leadInSamples = 0;
    totalSamples = 0;

    initialized = true;
    finished = false;
    leadIn = true;
    eof = false;

    fseek(file, seekPosition, SEEK_SET);

    return bufferChunk();
}

bool MadDecoder::bufferChunk() {
    size_t bytesToRead = CHUNK_SIZE;
    uint8_t* target = buffer;
    size_t unused = 0;

    if (stream.next_frame && stream.next_frame != (buffer + CHUNK_SIZE)) {
        unused = CHUNK_SIZE - (stream.next_frame - buffer);

        memmove(buffer, stream.next_frame, unused);

        bytesToRead -= unused;
        target = buffer + unused;
    }

    size_t bytesRead = 0;
    while (!eof && bytesRead < bytesToRead) {
        size_t r = fread(target + bytesRead, 1, bytesToRead - bytesRead, file);

        if (r == 0)
            eof = true;
        else
            bytesRead += r;
    }

    if (eof) {
        while (bytesRead < bytesToRead && iBufferGuard++ < MAD_BUFFER_GUARD) target[bytesRead++] = 0;
    }

    if (bytesRead == 0) return false;

    mad_stream_buffer(&stream, buffer, bytesRead + unused);

    LOG_VERBOSE(TAG, "buffered %du bytes", bytesRead);

    return true;
}

uint32_t MadDecoder::decode(int16_t* buffer, uint32_t count) {
    if (!initialized || finished) return 0;

    uint32_t decodedSamples = 0;

    while (decodedSamples < count) {
        if (!decodeOne(buffer[2 * decodedSamples], buffer[2 * decodedSamples + 1])) break;

        leadIn = leadIn && buffer[2 * decodedSamples] == 0 && buffer[2 * decodedSamples + 1] == 0 &&
                 leadInSamples++ < MAX_LEAD_IN_SAMPLES;

        if (!leadIn) decodedSamples++;
    }

    if (decodedSamples < count) {
        finished = true;

        LOG_DEBUG(TAG, "decoding finished after %i samples", decodedSamples);
    }

    totalSamples += decodedSamples;

    return decodedSamples;
}

bool MadDecoder::decodeOne(int16_t& sampleL, int16_t& sampleR) {
    if (finished) return false;

    if (sampleNo >= sampleCount) {
        if (ns >= nsMax) {
            while (true) {
                if (mad_frame_decode(&frame, &stream) == 0) break;

                if (stream.error == MAD_ERROR_BUFLEN) {
                    if (bufferChunk())
                        continue;
                    else
                        return false;
                }

                if (!MAD_RECOVERABLE(stream.error)) {
                    LOG_DEBUG(TAG, "decoding failed with mad error");
                    LOG_DEBUG(TAG, "%s", mad_stream_errorstr(&stream));
                }
            }

            ns = 0;
            nsMax = MAD_NSBSAMPLES(&frame.header);
        }

        switch (mad_synth_frame_onens(&synth, &frame, ns++)) {
            case MAD_FLOW_STOP:
            case MAD_FLOW_BREAK:

                LOG_DEBUG(TAG, "mad_synth_frame_onens failed");

                return false;

            default:
                break;
        }

        sampleNo = 0;
        sampleCount = synth.pcm.length;
    }

    sampleL = synth.pcm.samples[0][sampleNo];
    sampleR = synth.pcm.samples[1][sampleNo];

    sampleNo++;

    return true;
}

void MadDecoder::deinit() {
    if (initialized) {
        mad_stream_finish(&stream);
        mad_frame_finish(&frame);
        mad_synth_finish(&synth);
    }

    finished = true;
    initialized = false;
}

void MadDecoder::close() {
    if (file) {
        LOG_DEBUG(TAG, "decoder closed after decoding %du bytes", file.position());

        fclose(file);
    }

    deinit();

    LOG_DEBUG(TAG, "decoder closed");
}

void MadDecoder::rewind() { reset(); }

uint32_t MadDecoder::getPosition() const { return totalSamples; }

size_t MadDecoder::getSeekPosition() { return file ? ftell(file) : 0; }

void MadDecoder::seekTo(uint32_t seekPosition) { reset(seekPosition > CHUNK_SIZE ? seekPosition - CHUNK_SIZE : 0); }

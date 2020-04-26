#include "MadDecoder.hxx"

#include <Arduino.h>

// #define DEBUG
#define LOG_MAD_ERROR Serial.println(mad_stream_errorstr(&stream));

MadDecoder::MadDecoder() : initialized(false), finished(true), leadIn(true) {}

MadDecoder::~MadDecoder() { close(); }

bool MadDecoder::open(const char* path) {
    if (initialized) close();

    file = SD.open(path);

    if (!file) return false;

    Serial.printf("now playing %s\r\n", path);

    sampleNo = 0;
    sampleCount = 0;
    ns = 0;
    nsMax = 0;
    iBufferGuard = 0;

    mad_stream_init(&stream);
    mad_frame_init(&frame);
    mad_synth_init(&synth);
    mad_stream_options(&stream, 0);

    initialized = true;
    finished = false;
    leadIn = false;
    eof = false;

    if (!bufferChunk()) {
        close();
        return false;
    }

#ifdef DEBUG
    Serial.printf("decoder initialized for file %s\r\n", path);
#endif

    return true;
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

    size_t bytesRead = eof ? 0 : file.read(target, bytesToRead);
    eof = eof || bytesRead < bytesToRead;

    if (eof) {
        while (bytesRead < bytesToRead && iBufferGuard++ < MAD_BUFFER_GUARD) target[bytesRead++] = 0;
    }

    if (bytesRead == 0) return false;

    mad_stream_buffer(&stream, buffer, bytesRead + unused);

#ifdef DEBUG
    Serial.printf("buffered %lu bytes\r\n", bytesRead);
#endif

    return true;
}

uint32_t MadDecoder::decode(int16_t* buffer, uint32_t count) {
    if (!initialized || finished) return 0;

    uint32_t decodedSamples = 0;

    while (decodedSamples < count) {
        if (!decodeOne(buffer[2 * decodedSamples], buffer[2 * decodedSamples + 1])) break;

        leadIn = leadIn && buffer[2 * decodedSamples] == 0 && buffer[2 * decodedSamples + 1] == 0;

        if (!leadIn) decodedSamples++;
    }

    if (decodedSamples < count) {
        finished = true;

#ifdef DEBUG
        Serial.printf("decoding finished after %i samples\r\n", decodedSamples);
#endif
    }

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
#ifdef DEBUG
                    Serial.print("decoding failed with mad error: ");
                    LOG_MAD_ERROR;
#endif
                    return false;
                }
            }

            ns = 0;
            nsMax = MAD_NSBSAMPLES(&frame.header);
        }

        switch (mad_synth_frame_onens(&synth, &frame, ns++)) {
            case MAD_FLOW_STOP:
            case MAD_FLOW_BREAK:
#ifdef DEBUG
                Serial.println("mad_synth_frame_onens failed");
#endif

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

void MadDecoder::close() {
    if (file) {
#ifdef DEBUG
        Serial.printf("decoder closed after decoding %lu bytes\r\n", file.position());
#endif

        file.close();
    }

    if (initialized) {
        mad_stream_finish(&stream);
        mad_frame_finish(&frame);
        mad_synth_init(&synth);
    }

    finished = true;
    initialized = false;

#ifdef DEBUG
    Serial.println("decoder closed");
#endif
}

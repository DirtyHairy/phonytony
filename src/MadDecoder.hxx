#ifndef MAD_DECODER_HXX
#define MAD_DECODER_HXX

#include <stdint.h>
#include <SD.h>
#include <mad.h>

class MadDecoder {
    public:

        static constexpr int CHUNK_SIZE = 0x600;

    public:

        MadDecoder();

        ~MadDecoder();

        bool open(const char* file);

        uint32_t decode(int16_t* buffer, uint32_t count);

        bool isFinished() const { return finished; };

        void close();

    private:

        bool bufferChunk();

        bool decodeOne(int16_t& sampleL, int16_t& sampleR);

    private:

        File file;
        uint8_t buffer[CHUNK_SIZE];

        mad_stream stream;
        mad_frame frame;
        mad_synth synth;

        uint32_t sampleNo;
        uint32_t sampleCount;

        uint32_t ns;
        uint32_t nsMax;
        uint32_t iBufferGuard {0};

        bool initialized;
        bool finished;
        bool leadIn;
        bool eof {0};

    private:

        MadDecoder(const MadDecoder&) = delete;

        MadDecoder(MadDecoder&&) = delete;

        MadDecoder& operator=(const MadDecoder&) = delete;

        MadDecoder& operator=(MadDecoder&&) = delete;
};

#endif

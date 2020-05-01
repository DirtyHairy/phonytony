#ifndef MAD_DECODER_HXX
#define MAD_DECODER_HXX

#include <SD.h>
#include <mad.h>
#include <stdint.h>

class MadDecoder {
   public:
    static constexpr int CHUNK_SIZE = 0x600;
    static constexpr int MAX_LEAD_IN_SAMPLES = 3000;

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

    uint32_t sampleNo{0};
    uint32_t sampleCount{0};

    uint32_t ns{0};
    uint32_t nsMax{0};
    uint32_t iBufferGuard{0};
    uint32_t leadInSamples{0};

    bool initialized{false};
    bool finished{true};
    bool leadIn{true};
    bool eof{0};

   private:
    MadDecoder(const MadDecoder&) = delete;

    MadDecoder(MadDecoder&&) = delete;

    MadDecoder& operator=(const MadDecoder&) = delete;

    MadDecoder& operator=(MadDecoder&&) = delete;
};

#endif

#ifndef DIRECTORY_READER_HXX
#define DIRECTORY_READER_HXX

#include <stdint.h>

class DirectoryReader {
   public:
    DirectoryReader();

    ~DirectoryReader();

    bool open(const char* directory);

    void close();

    const char* getTrack(uint8_t index) { return index < length ? playlist[index] : nullptr; }

    uint32_t getLength() const { return length; }

   private:
    char* buffer;

    char** playlist;

    uint32_t length;

   private:
    DirectoryReader(const DirectoryReader&) = delete;

    DirectoryReader(DirectoryReader&&) = delete;

    DirectoryReader& operator=(const DirectoryReader&) = delete;

    DirectoryReader& operator=(DirectoryReader&&) = delete;
};

#endif  // DIRECTORY_READER_HXX

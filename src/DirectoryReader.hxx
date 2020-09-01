#ifndef DIRECTORY_READER_HXX
#define DIRECTORY_READER_HXX

#include <FS.h>
#include <stdint.h>

class DirectoryReader {
   public:
    DirectoryReader();

    ~DirectoryReader();

    bool open(const char* directory);

    void close();

    const char* getTrack(uint32_t index) { return index < length ? playlist[index] : nullptr; }

    uint32_t getLength() const { return length; }

   private:
    char* buffer{nullptr};

    char** playlist{nullptr};

    uint32_t length{0};

    bool scanDirectory(File& root);

    bool readIndex(File& index);

    void writeIndex(const char* path) const;

   private:
    DirectoryReader(const DirectoryReader&) = delete;

    DirectoryReader(DirectoryReader&&) = delete;

    DirectoryReader& operator=(const DirectoryReader&) = delete;

    DirectoryReader& operator=(DirectoryReader&&) = delete;
};

#endif  // DIRECTORY_READER_HXX

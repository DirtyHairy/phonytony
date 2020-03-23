#ifndef MP3_DIRECTORY_READER_HXX
#define MP3_DIRECTORY_READER_HXX

#include <stdint.h>

class Mp3DirectoryReader {
    public:

        Mp3DirectoryReader();

        ~Mp3DirectoryReader();

        void open(const char* directory);

        void close();

        const char** getSortedMp3s() { return (const char**)sortedMp3s; }

        uint32_t getLength() const { return length; }

    private:

        char* buffer;

        char** sortedMp3s;

        uint32_t length;

    private:

        Mp3DirectoryReader(const Mp3DirectoryReader&) = delete;

        Mp3DirectoryReader(Mp3DirectoryReader&&) = delete;

        Mp3DirectoryReader& operator=(const Mp3DirectoryReader&) = delete;

        Mp3DirectoryReader& operator=(Mp3DirectoryReader&&) = delete;
};

#endif // MP3_DIRECTORY_READER_HXX

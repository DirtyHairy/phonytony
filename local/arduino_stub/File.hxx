#ifndef File_hxx
#define File_hxx

#include <cstdint>
#include <cstdlib>

enum SeekMode { SeekSet = 0, SeekCur = 1, SeekEnd = 2 };

class File {
   public:
    virtual size_t read(uint8_t* buf, size_t size);
    virtual size_t readBytes(char* buffer, size_t length) { return read((uint8_t*)buffer, length); }

    virtual bool seek(uint32_t pos, SeekMode mode);
    virtual bool seek(uint32_t pos) { return seek(pos, SeekSet); }

    virtual void close();

    virtual boolean isDirectory(void);
    virtual File openNextFile(const char* mode = FILE_READ);
    virtual void rewindDirectory(void);
};

#endif  // File_hxx

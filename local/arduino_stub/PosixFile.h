#ifndef PosixFile_h
#define PosixFile_h

#include <FS.h>
#include <FSImpl.h>

#include <string>

class PosixFile : public fs::FileImpl {
   public:
    PosixFile(FILE* file, const char* name);

    virtual ~PosixFile() override;

    virtual size_t read(uint8_t* buf, size_t size) override;

    virtual void flush() override;

    virtual size_t write(const uint8_t* buf, size_t size) override;

    virtual bool seek(uint32_t pos, SeekMode mode) override;

    virtual size_t position() const override;

    virtual size_t size() const override;

    virtual void close() override;

    virtual time_t getLastWrite() override;

    virtual const char* name() const override;

    virtual boolean isDirectory(void) override;

    virtual fs::FileImplPtr openNextFile(const char* mode) override;

    virtual void rewindDirectory(void) override;

    virtual operator bool() override;

   private:
    FILE* file{nullptr};
    std::string filename;

   private:
    PosixFile() = delete;
    PosixFile(const PosixFile&) = delete;
    PosixFile(PosixFile&&) = delete;

    PosixFile& operator=(const PosixFile&) = delete;
    PosixFile& operator=(PosixFile&&) = delete;
};

#endif  // PosixFile_h

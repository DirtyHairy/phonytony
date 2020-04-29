#ifndef PosixDirectory_h
#define PosixDirectory_h

#include <FS.h>
#include <FSImpl.h>
#include <dirent.h>

#include <string>

class PosixDirectory : public fs::FileImpl {
   public:
    PosixDirectory(DIR* file, std::string name);

    virtual ~PosixDirectory() override;

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
    std::string qualifyPath(std::string path) const;

   private:
    DIR* dir{nullptr};
    std::string directoryName;

   private:
    PosixDirectory() = delete;
    PosixDirectory(const PosixDirectory&) = delete;
    PosixDirectory(PosixDirectory&&) = delete;

    PosixDirectory& operator=(const PosixDirectory&) = delete;
    PosixDirectory& operator=(PosixDirectory&&) = delete;
};

#endif  // PosixFile_h

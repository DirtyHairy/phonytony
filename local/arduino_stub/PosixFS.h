#ifndef POSIX_FS_H
#define POSIX_FS_H

#include <FS.h>
#include <FSImpl.h>

class PosixFS : public fs::FSImpl {
   public:
    PosixFS();

    virtual ~PosixFS();

    virtual fs::FileImplPtr open(const char* path, const char* mode) override;

    virtual bool exists(const char* path) override;

    virtual bool rename(const char* pathFrom, const char* pathTo) override;

    virtual bool remove(const char* path) override;

    virtual bool mkdir(const char* path) override;

    virtual bool rmdir(const char* path) override;
};

#endif  // POSIX_FS_H

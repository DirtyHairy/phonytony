#include "PosixFS.h"

#include <dirent.h>
#include <sys/stat.h>

#include <memory>
#include <stdexcept>

#include "PosixDirectory.h"
#include "PosixFile.h"

using namespace fs;
using std::make_shared;

#define NOT_IMPLEMENTED(s) throw std::runtime_error(s);

PosixFS::PosixFS() {}

PosixFS::~PosixFS() {}

FileImplPtr PosixFS::open(const char* path, const char* mode) {
    DIR* dir = opendir(path);
    if (dir) return make_shared<PosixDirectory>(dir, path);

    FILE* file = fopen(path, mode);
    if (file) return make_shared<PosixFile>(file, path);

    return nullptr;
}

bool PosixFS::exists(const char* path) {
    struct stat fstat;
    return stat(path, &fstat) == 0;
}

bool PosixFS::rename(const char* pathFrom, const char* pathTo) { NOT_IMPLEMENTED("PosixFS::rename"); }

bool PosixFS::remove(const char* path) { NOT_IMPLEMENTED("PosixFS::remove"); }

bool PosixFS::mkdir(const char* path) { NOT_IMPLEMENTED("PosixFS::mkdir"); }

bool PosixFS::rmdir(const char* path) { NOT_IMPLEMENTED("PosixFS::rmdir"); }

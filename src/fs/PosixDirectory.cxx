#include "PosixDirectory.h"

#include <memory>
#include <stdexcept>

#include "PosixFile.h"

#define NOT_IMPLEMENTED(s) throw std::runtime_error(s);

using namespace fs;
using std::make_shared;
using std::string;

PosixDirectory::PosixDirectory(DIR* dir, string name) : dir(dir), directoryName(name) {}

PosixDirectory::~PosixDirectory() { close(); }

size_t PosixDirectory::read(uint8_t* buf, size_t size) { return 0; }

void PosixDirectory::flush() {}

size_t PosixDirectory::write(const uint8_t* buf, size_t size) { return 0; }

bool PosixDirectory::seek(uint32_t pos, SeekMode mode) {
    if (!dir) return false;

    seekdir(dir, pos);
    return true;
}

size_t PosixDirectory::position() const { return dir ? telldir(dir) : 0; }

size_t PosixDirectory::size() const { NOT_IMPLEMENTED("PosixDirectory::size not implemented"); }

void PosixDirectory::close() {
    if (dir) {
        closedir(dir);
        dir = nullptr;
    }
}

time_t PosixDirectory::getLastWrite() { NOT_IMPLEMENTED("PosixDirectory::getLastWrite "); }

const char* PosixDirectory::name() const { return directoryName.c_str(); }

boolean PosixDirectory::isDirectory(void) { return true; }

string PosixDirectory::qualifyPath(string path) const {
    string prefix = directoryName;

    while (prefix.length() > 0 && prefix[prefix.length() - 1] == '/') prefix.resize(prefix.length() - 1);

    return prefix + "/" + path;
}

FileImplPtr PosixDirectory::openNextFile(const char* mode) {
    if (!dir) return nullptr;

    struct dirent* dent = readdir(dir);

    if (!dent) return nullptr;

    string path = qualifyPath(dent->d_name);

    DIR* maybeDir = opendir(path.c_str());
    if (maybeDir) return make_shared<PosixDirectory>(maybeDir, path);

    FILE* maybeFile = fopen(path.c_str(), mode);
    if (maybeFile) return make_shared<PosixFile>(maybeFile, path);

    return nullptr;
}

void PosixDirectory::rewindDirectory(void) {
    if (dir) rewinddir(dir);
}

PosixDirectory::operator bool() { return !!dir; }

#include "PosixFile.h"

#include <memory>
#include <stdexcept>

using namespace fs;
using std::runtime_error;
using std::string;

#define NOT_IMPLEMENTED(s) throw runtime_error(s);

namespace {
int whenceFromSeekMode(SeekMode seekMode) {
    switch (seekMode) {
        case SeekSet:
            return SEEK_SET;

        case SeekCur:
            return SEEK_CUR;

        case SeekEnd:
            return SEEK_END;

        default:
            throw runtime_error("bad SeekMode");
    }
}
}  // namespace

PosixFile::PosixFile(FILE* file, string name) : file(file), filename(name) {}

PosixFile::~PosixFile() { close(); }

size_t PosixFile::read(uint8_t* buf, size_t size) { return file ? fread((void*)buf, 1, size, file) : 0; }

void PosixFile::flush() {
    if (file) fflush(file);
}

size_t PosixFile::write(const uint8_t* buf, size_t size) { return file ? fwrite((void*)buf, 1, size, file) : 0; }

bool PosixFile::seek(uint32_t pos, SeekMode mode) {
    return file ? fseek(file, pos, whenceFromSeekMode(mode)) == 0 : false;
}

size_t PosixFile::position() const { return file ? ftell(file) : 0; }

size_t PosixFile::size() const {
    if (!file) return 0;
    long pos = ftell(file);

    fseek(file, 0, SEEK_END);

    long size = ftell(file);

    fseek(file, pos, SEEK_SET);

    return size;
}

void PosixFile::close() {
    if (file) {
        fclose(file);
        file = nullptr;
    }
}

time_t PosixFile::getLastWrite() { NOT_IMPLEMENTED("PosixFile::getLastWrite not implemented"); }

const char* PosixFile::name() const { return filename.c_str(); };

boolean PosixFile::isDirectory(void) { return false; }

FileImplPtr PosixFile::openNextFile(const char* mode) { return nullptr; }

void PosixFile::rewindDirectory(void) {}

PosixFile::operator bool() { return !!file; }

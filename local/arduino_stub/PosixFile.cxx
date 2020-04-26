#include "PosixFile.h"

#include <stdexcept>

using namespace fs;
using std::runtime_error;

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

PosixFile::PosixFile(FILE* file, const char* name) : file(file), filename(name) {}

PosixFile::~PosixFile() { close(); }

size_t PosixFile::read(uint8_t* buf, size_t size) { return file ? fread((void*)buf, size, 1, file) : 0; }

void PosixFile::flush() {
    if (file) fflush(file);
}

size_t PosixFile::write(const uint8_t* buf, size_t size) { return file ? fwrite((void*)buf, size, 1, file) : 0; }

bool PosixFile::seek(uint32_t pos, SeekMode mode) {
    return file ? fseek(file, pos, whenceFromSeekMode(mode)) == 0 : false;
}

size_t PosixFile::position() const { return file ? ftell(file) : 0; }

size_t PosixFile::size() const { NOT_IMPLEMENTED("PosixFile::size not implemented"); }

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

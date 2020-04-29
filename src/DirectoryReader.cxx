#include "DirectoryReader.hxx"

#include <SD.h>

#include <algorithm>

namespace {
    bool isMp3(const char* name) {
        const char* dot = strrchr(name, '.');

        if (!dot || (strlen(name) - (dot - name)) != 4) return false;
        if (toupper(dot[1]) != 'M' || toupper(dot[2]) != 'P' || dot[3] != '3') return false;

        return true;
    }

    const char* filename(const char* name) {
        const char* lastSlash = strrchr(name, '/');

        return lastSlash ? lastSlash + 1 : name;
    }

    bool compareFilenames(const char* n1, const char* n2) {
        n1 = filename(n1);
        n2 = filename(n2);

        char* l1;
        char* l2;

        long i1 = strtol(n1, &l1, 10);
        long i2 = strtol(n2, &l2, 10);

        if (l1 == n1 && l2 == n2) return strcasecmp(n1, n2) < 0;
        if (l2 == n2) return true;
        if (l1 == n1) return false;
        if (i1 == i2) return strcasecmp(l1, l2) < 0;
        return i1 < i2;
    }
}  // namespace

DirectoryReader::DirectoryReader() : buffer(nullptr), playlist(nullptr), length(0) {}

DirectoryReader::~DirectoryReader() { close(); }

bool DirectoryReader::open(const char* dirname) {
    close();

    File root = SD.open(dirname);

    if (!root) return false;

    if (!root.isDirectory()) {
        root.close();
        return false;
    }

    size_t bufferSize = 0;
    File file;

    while ((file = root.openNextFile())) {
        if (file.isDirectory()) continue;
        if (!isMp3(file.name())) continue;

        bufferSize += (strlen(file.name()) + 1);
        length++;
    }

    if (length == 0) {
        file.close();
        return true;
    }

    root.rewindDirectory();

    buffer = new char[bufferSize];
    playlist = new char*[length];

    char* buf = buffer;
    uint32_t i = 0;

    while ((file = root.openNextFile())) {
        if (file.isDirectory()) continue;
        if (!isMp3(file.name())) continue;

        strcpy(buf, file.name());

        playlist[i++] = buf;
        buf += (strlen(file.name()) + 1);
    }

    std::sort(playlist, playlist + length, compareFilenames);

    file.close();

    return true;
}

void DirectoryReader::close() {
    if (buffer) {
        delete buffer;
        buffer = nullptr;
    }

    if (playlist) {
        delete playlist;
        playlist = nullptr;
    }

    length = 0;
}

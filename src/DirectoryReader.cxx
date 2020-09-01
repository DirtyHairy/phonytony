#include "DirectoryReader.hxx"

#include <SD.h>

#include <algorithm>
#include <string>

#include "Log.hxx"

#define TAG "reader"

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

template <typename T>
T filename(T path) {
    T slash = strrchr(path, '/');

    return *(slash + 1) ? (slash + 1) : path;
}

}  // namespace

DirectoryReader::DirectoryReader() {}

DirectoryReader::~DirectoryReader() { close(); }

bool DirectoryReader::open(const char* dirname) {
    close();

    std::string indexPath = std::string(dirname) + "/index";
    File index = SD.open(indexPath.c_str(), "r");

    if (index) {
        if (!readIndex(index)) return false;
    } else {
        File root = SD.open(dirname);

        if (!root) return false;

        if (!scanDirectory(root)) return false;

        writeIndex(indexPath.c_str());
    }

    if (length > 0) std::sort(playlist, playlist + length, compareFilenames);

    return true;
}

bool DirectoryReader::scanDirectory(File& root) {
    if (!root.isDirectory()) {
        root.close();
        return false;
    }

    size_t bufferSize = 0;
    File file;

    while ((file = root.openNextFile())) {
        if (file.isDirectory()) continue;

        const char* name = filename(file.name());

        if (!isMp3(file.name())) continue;

        bufferSize += (strlen(name) + 1);
        length++;
    }

    if (length == 0) return true;

    root.rewindDirectory();

    buffer = (char*)ps_malloc(bufferSize);
    playlist = (char**)malloc(length * sizeof(char*));

    char* buf = buffer;
    uint32_t i = 0;

    while ((file = root.openNextFile())) {
        if (file.isDirectory()) continue;

        const char* name = filename(file.name());

        if (!isMp3(name)) continue;

        strcpy(buf, name);

        playlist[i++] = buf;
        buf += (strlen(name) + 1);
    }

    return true;
}

bool DirectoryReader::readIndex(File& index) {
    size_t bufferSize = index.size() + 1;

    buffer = (char*)ps_malloc(bufferSize);
    index.readBytes(buffer, bufferSize - 1);
    index.close();

    char lastChar = 0;
    buffer[bufferSize - 1] = 0;

    for (size_t i = 0; i < bufferSize; i++) {
        if (buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = 0;

        if (buffer[i] != 0 && lastChar == 0) length++;

        lastChar = buffer[i];
    }

    if (length == 0) return true;

    playlist = (char**)malloc(length * sizeof(char*));
    uint32_t iTrack = 0;
    lastChar = 0;

    for (size_t i = 0; i < bufferSize; i++) {
        if (buffer[i] != 0 && lastChar == 0) playlist[iTrack++] = &buffer[i];

        lastChar = buffer[i];
    }

    return true;
}

void DirectoryReader::writeIndex(const char* path) const {
    File index = SD.open(path, "w");
    if (!index) return;

    for (uint32_t i = 0; i < length; i++) index.println(playlist[i]);

    index.close();
}

void DirectoryReader::close() {
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }

    if (playlist) {
        free(playlist);
        playlist = nullptr;
    }

    length = 0;
}

#include "DirectoryReader.hxx"

#include <Arduino.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "Guard.hxx"
#include "Log.hxx"

#define TAG "reader"

namespace {
bool isMp3(const char* name) {
    const char* dot = strrchr(name, '.');

    if (!dot || (strlen(name) - (dot - name)) != 4) return false;
    if (toupper(dot[1]) != 'M' || toupper(dot[2]) != 'P' || dot[3] != '3') return false;

    return true;
}

bool compareFilenames(const char* n1, const char* n2) {
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

bool isDir(const std::string& name) {
    DIR* dir = opendir(name.c_str());

    if (!dir) return false;

    closedir(dir);

    return true;
}

}  // namespace

DirectoryReader::DirectoryReader() {}

DirectoryReader::~DirectoryReader() { close(); }

bool DirectoryReader::open(const char* dirname) {
    close();

    std::string indexPath = std::string(dirname) + "/index";
    FILE* index = fopen(indexPath.c_str(), "r");

    if (index) {
        Guard guard([=]() { fclose(index); });

        if (!readIndex(index)) return false;
    } else {
        DIR* root = opendir(dirname);
        if (!root) return false;

        Guard guard([=]() { closedir(root); });

        if (!scanDirectory(root, dirname)) return false;

        writeIndex(indexPath.c_str());
    }

    if (length > 0) std::sort(playlist, playlist + length, compareFilenames);

    return true;
}

bool DirectoryReader::scanDirectory(DIR* root, const char* dirname) {
    if (!root) {
        return false;
    }

    size_t bufferSize = 0;
    struct dirent* entry;

    while ((entry = readdir(root))) {
        if (isDir(std::string(dirname) + "/" + std::string(entry->d_name))) continue;

        if (!isMp3(entry->d_name)) continue;

        bufferSize += (strlen(entry->d_name) + 1);
        length++;
    }

    if (length == 0) return true;

    rewinddir(root);

    buffer = (char*)ps_malloc(bufferSize);
    playlist = (char**)ps_malloc(length * sizeof(char*));

    char* buf = buffer;
    uint32_t i = 0;

    while ((entry = readdir(root))) {
        if (isDir(std::string(dirname) + "/" + std::string(entry->d_name))) continue;

        if (!isMp3(entry->d_name)) continue;

        strcpy(buf, entry->d_name);

        playlist[i++] = buf;
        buf += (strlen(entry->d_name) + 1);
    }

    return true;
}

bool DirectoryReader::readIndex(FILE* index) {
    fseek(index, 0, SEEK_END);
    size_t bufferSize = ftell(index) + 1;
    fseek(index, 0, SEEK_SET);

    buffer = (char*)ps_malloc(bufferSize);

    size_t bytesRead = 0;
    while (bytesRead < bufferSize - 1) {
        size_t r = fread(buffer + bytesRead, 1, bufferSize - 1 - bytesRead, index);
        if (r == 0) break;

        bytesRead += r;
    }

    char lastChar = 0;
    buffer[bufferSize - 1] = 0;

    for (size_t i = 0; i < bufferSize; i++) {
        if (buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = 0;

        if (buffer[i] != 0 && lastChar == 0) length++;

        lastChar = buffer[i];
    }

    if (length == 0) return true;

    playlist = (char**)ps_malloc(length * sizeof(char*));
    uint32_t iTrack = 0;
    lastChar = 0;

    for (size_t i = 0; i < bufferSize; i++) {
        if (buffer[i] != 0 && lastChar == 0) playlist[iTrack++] = &buffer[i];

        lastChar = buffer[i];
    }

    return true;
}

void DirectoryReader::writeIndex(const char* path) const {
    FILE* index = fopen(path, "w");
    if (!index) return;

    for (uint32_t i = 0; i < length; i++) {
        fputs(playlist[i], index);
        fputs("\r\n", index);
    }

    fclose(index);
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

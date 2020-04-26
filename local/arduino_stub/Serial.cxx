#include "Serial.h"

#include <cstdio>
#include <memory>

#include "FS.h"
#include "PosixFile.h"

namespace {
    File _stderr(std::make_shared<PosixFile>(stderr, "/dev/stderr"));
}

Stream& Serial(_stderr);

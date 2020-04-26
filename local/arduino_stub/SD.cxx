#include "SD.h"

#include <memory>

#include "PosixFS.h"

FS SD(std::make_shared<PosixFS>());

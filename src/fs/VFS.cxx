#include "VFS.h"

#include <memory>

#include "PosixFS.h"

FS VFS(std::make_shared<PosixFS>());

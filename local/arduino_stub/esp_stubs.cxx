#include "esp_stubs.h"

#include <cstdlib>

void* ps_malloc(size_t size) { return malloc(size); }

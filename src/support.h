#pragma once

#include "./defs.h"
#include <pico/malloc.h>

const char *region_str(enum Region r);

void temp_init();
float temp_read();
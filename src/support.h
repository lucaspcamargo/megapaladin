#pragma once

#include "./defs.h"
#include <pico/malloc.h>

const char *region_str(enum Region r);

inline uint32_t fifo_pack(FIFOCmd *c) { return *((uint32_t*)c); }
inline void fifo_unpack(FIFOCmd *c, uint32_t in_val) { *c = *((FIFOCmd*)&in_val); }

void temp_init();
float temp_read();

void reboot();
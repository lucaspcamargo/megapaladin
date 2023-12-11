#pragma once

#include "./defs.h"
#include <pico/malloc.h>

void fifo_init();
bool fifo_push(FIFOCmd *incmd);
bool fifo_pop(FIFOCmd *incmd);

const char *region_str(enum Region r);

void temp_init();
float temp_read();

void reboot();
#pragma once

#include "./defs.h"
#include <pico/malloc.h>

void fifo_init();
bool fifo_push(FIFOCmd *incmd);
bool fifo_pop(FIFOCmd *incmd);

// for core 1 debug log
bool fifo_str_push(const char msg[MSG_LEN_MAX]);    // for core 1
bool fifo_str_pop(char *msg);                       // for core 0

const char * region_str(enum Region r);
const char * device_type_str(uint8_t dev_type);

void temp_init();
float temp_read();

void reboot();
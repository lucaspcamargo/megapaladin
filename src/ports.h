#pragma once

#include "defs.h"

#include <stdint.h>


// Definitions
#define PIN_COUNT 7
#define PORT_COUNT 2

#define PORT1_0_PIN 17
#define PORT1_1_PIN 8
#define PORT1_2_PIN 26
#define PORT1_3_PIN 27
#define PORT1_4_PIN 16
#define PORT1_5_PIN 28
#define PORT1_6_PIN 7

#define PORT2_0_PIN 6
#define PORT2_1_PIN 5
#define PORT2_2_PIN 4
#define PORT2_3_PIN 3
#define PORT2_4_PIN 0
#define PORT2_5_PIN 2
#define PORT2_6_PIN 1




void port_init();   // setup all port gpios as hi-z and init vars

void port_step();   // in loop of core 1, steps the controller emulation step machines 

uint8_t port_type_curr(uint8_t port);   
void port_type_set(uint8_t port, uint8_t type);

void port_on_host_event(const FIFOCmd *cmd);
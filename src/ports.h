#pragma once

#include "defs.h"
#include <hardware/pio.h>

// PIO programs
#include "sniff.pio.h"

#include <stdint.h>


// Definitions
#define PIN_COUNT 7
#define PORT_COUNT 2

#define PORT1_0_PIN     0  // D0
#define PORT1_1_PIN     1  // D1
#define PORT1_2_PIN     2  // D2
#define PORT1_3_PIN     3  // D3
#define PORT1_4_PIN     5  // TL
#define PORT1_5_PIN     4  // TH
#define PORT1_6_PIN     6  // TR
#define PORT1_OE_PIN    7

#define PORT2_0_PIN     8  // D0                           
#define PORT2_1_PIN     9  // D1
#define PORT2_2_PIN     10 // D2
#define PORT2_3_PIN     11 // D3
#define PORT2_4_PIN     13 // TL
#define PORT2_5_PIN     12 // TH
#define PORT2_6_PIN     14 // TR
#define PORT2_OE_PIN    15




void port_preinit();   // setup all port gpios as hi-z, form any core
void port_init();   // init vars and setup irqs, from core 0

void port_step();   // in loop of core 1, steps the controller emulation step machines 

uint8_t port_type_curr(uint8_t port);   
void port_type_set(uint8_t port, uint8_t type);

void port_on_host_event(const FIFOCmd *cmd);
uint64_t port_get_evt_count();
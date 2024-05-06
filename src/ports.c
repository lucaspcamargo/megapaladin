#include "ports.h"

#include <stdint.h>
#include <hardware/gpio.h>
#include <string.h>


static const uint8_t port1_pins[7] = {
    PORT1_0_PIN,
    PORT1_1_PIN,
    PORT1_2_PIN,
    PORT1_3_PIN,
    PORT1_4_PIN,
    PORT1_5_PIN,
    PORT1_6_PIN
};

static const uint8_t port2_pins[7] = {
    PORT2_0_PIN,
    PORT2_1_PIN,
    PORT2_2_PIN,
    PORT2_3_PIN,
    PORT2_4_PIN,
    PORT2_5_PIN,
    PORT2_6_PIN
};

static const uint8_t const *port_pins[] = {port1_pins, port2_pins};

static uint8_t port_types[PORT_COUNT];


void _port_handler_none(uint8_t port);
void _port_handler_joy(uint8_t port);
void _port_handler_mouse(uint8_t port);


void port_init()
{
    memset(port_types, 0x00, sizeof(port_types));

    for(uint8_t p_idx = 0; p_idx < PORT_COUNT; p_idx++)
    {
        for(uint8_t pin_i = 0; pin_i < PIN_COUNT; pin_i++)
        {
            const uint8_t pin = port_pins[p_idx][pin_i];
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_IN);
        }
    }
}

void port_step()
{

}


uint8_t port_type_curr(uint8_t port)
{

}


void port_type_set(uint8_t port, uint8_t type)
{

}


void port_on_host_event(const FIFOCmd *cmd)
{
    int port = (cmd->data[0] >> 4);
    if(port < PORT_COUNT)
    {

    }
}
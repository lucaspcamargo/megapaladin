#include "ports.h"

#include <pico/stdlib.h>
#include <hardware/gpio.h>

#include <stdint.h>
#include <string.h>

#define PIN_OFFSET_D0 0
#define PIN_OFFSET_D1 1
#define PIN_OFFSET_D2 2
#define PIN_OFFSET_D3 3
#define PIN_OFFSET_TL 4
#define PIN_OFFSET_TH 5
#define PIN_OFFSET_TR 6
#define PIN_OFFSET_OE 7

#define GPIO_EVT_BOTH_EDGES (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)

enum JoyFSMState
{
    JOY_STATE_STEP_0_IDLE_DIRS,
    JOY_STATE_STEP_1_A,       
    JOY_STATE_STEP_2_DIRS,
    JOY_STATE_STEP_3_A,
    JOY_STATE_STEP_4_DIRS,
    JOY_STATE_STEP_5_ZEROES,
    JOY_STATE_STEP_6_XYZ,
    JOY_STATE_STEP_7_ONES,
    JOY_STATE_STEP_COUNT
};

typedef struct joy_data_st
{
    enum JoyFSMState state;
    uint16_t buttons;
} joy_data;

typedef struct mouse_data_st
{
    enum JoyFSMState state;
    int16_t x_accum;
    int16_t y_accum;
    uint16_t buttons;
} mouse_data;

typedef struct port_data_st
{           
    uint64_t idle_timer;
    union
    {
        joy_data joy;
        mouse_data mouse;
    };
} port_data_td;

typedef struct irq_data_st
{
    uint8_t stage;
    uint32_t captures[JOY_STATE_STEP_COUNT];
} irq_data_td;

typedef struct pio_cfg_data_st
{
    PIO pio;
    uint sm;
    uint offset;
} pio_cfg_data_td;

static const uint8_t const PORT_PIN_0[PORT_COUNT] = {PORT1_0_PIN, PORT2_0_PIN};
static const uint8_t PORT_BTN_GPIO_OFFSET[BIT_BTN_COUNT] = {
    0, // up
    1, // down
    2, // left
    3, // right
    4, // A
    4, // B
    6, // C
    6, // start
    2, // X
    1, // Y
    0, // Z
    3, // mode
};
static uint8_t port_types[PORT_COUNT];
static port_data_td port_data[PORT_COUNT];
static irq_data_td irq_data[PORT_COUNT];
static pio_cfg_data_td pio_cfg[PORT_COUNT];
static uint64_t evt_counter;


void _port_mode_setup(uint8_t port, uint8_t type);
void _port_mode_reset(uint8_t port, uint8_t type);

void _port_handler_snoop(uint8_t port);
void _port_handler_joy(uint8_t port);
void _port_handler_mouse(uint8_t port);

// use core1 logging directly
extern void _core1_log_msg(const char *fmt, ...);


void port_preinit()
{
    for(uint8_t p_idx = 0; p_idx < PORT_COUNT; p_idx++)
    {
        for(uint8_t pin_i = 0; pin_i <= PIN_COUNT; pin_i++) // <= because of OE
        {
            const uint8_t pin = PORT_PIN_0[p_idx] + pin_i;
            const bool is_oe = (pin_i == PIN_OFFSET_OE);
            gpio_init(pin);
            gpio_set_dir(pin, is_oe? GPIO_OUT : GPIO_IN); // OE pin is output
        }
    }
}

void port_init()
{
    // zero all internal data
    memset(port_types, 0x00, sizeof(port_types));
    memset(port_data, 0x00, sizeof(port_data));
    memset(irq_data, 0x00, sizeof(irq_data));
    memset(pio_cfg, 0x00, sizeof(pio_cfg));
    for(uint8_t p_idx = 0; p_idx < PORT_COUNT; p_idx++)
        memset(&(irq_data[p_idx].captures), 0xff, sizeof(uint32_t)*JOY_STATE_STEP_COUNT);
    evt_counter = 0;

    // start devices in "no device" mode (sniff)
    for(uint8_t p_idx = 0; p_idx < PORT_COUNT; p_idx++)
    {
        _port_mode_setup(p_idx, DEVICE_TYPE_NONE);
    }

    irq_set_enabled(IO_IRQ_BANK0, true);
}

void port_step()
{
    for(int i = 0; i < PORT_COUNT; i++)
    {
        switch(port_types[i])
        {
            case DEVICE_TYPE_NONE:
                _port_handler_snoop(i);
                break;
            case DEVICE_TYPE_JOY:
                _port_handler_joy(i);
                break;
            case DEVICE_TYPE_MOUSE:
                _port_handler_mouse(i);
                break;
        }
    }
}


uint8_t port_type_curr(uint8_t port)
{
    return port_types[port];
}


void port_type_set(uint8_t port, uint8_t type)
{
    // TODO change device type for a port 
}


void port_on_host_event(const FIFOCmd *cmd)
{
    int port = (cmd->data[0] >> 4);
    if(port < PORT_COUNT)
    {

    }
    else 
        _core1_log_msg("WARN: IGNORED HOST EVT FOR PORT %d", (int)port);
}


void _port_mode_setup(uint8_t port, uint8_t mode)
{
    switch(mode)
    {
        case DEVICE_TYPE_NONE:
        {
            port_data[port].idle_timer = time_us_64();
            const uint gpio_oe = PORT_PIN_0[port] + PIN_OFFSET_OE;
            gpio_put(gpio_oe, true); // set OE high
            
            PIO pio;
            uint sm;
            uint offset;

            // load sniffer program in PIO
            bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&sniff_program, &pio, &sm, &offset, PORT_PIN_0[port], 7, true); 
            hard_assert(success);

            // save port pio settings for later use
            pio_cfg[port].pio = pio;
            pio_cfg[port].sm = sm;
            pio_cfg[port].offset = offset;

            // setup gpios to use PIO
            for(uint8_t pin = PORT_PIN_0[port]; pin < PORT_PIN_0[port] + 7; pin++)
                pio_gpio_init(pio, pin);
            pio_sm_set_consecutive_pindirs(pio, sm, PORT_PIN_0[port], 7, false); // set all pins as output

            // start PIO state machine
            pio_sm_config cfg = sniff_program_get_default_config(offset);
            hard_assert(pio_sm_init(pio, sm, offset, &cfg) == PICO_OK);
            pio_sm_set_enabled(pio, sm, true);

        }
        break;
        case DEVICE_TYPE_JOY:
            break;
        case DEVICE_TYPE_MOUSE:
            break;
    }
}

void _port_mode_reset(uint8_t port, uint8_t mode)
{
    assert(port < PORT_COUNT);

    switch(mode)
    {
        case DEVICE_TYPE_NONE:
        {
            const uint gpio_oe = PORT_PIN_0[port] + PIN_OFFSET_OE;
            gpio_put(gpio_oe, false); // set OE low, puts port pins on hi-Z

            if(pio_cfg[port].pio != NULL)
            {
                // remove PIO program and unclaim state machine
                pio_remove_program_and_unclaim_sm(&sniff_program, pio_cfg[port].pio, pio_cfg[port].sm, pio_cfg[port].offset);
                pio_cfg[port].pio = NULL;
                pio_cfg[port].sm = 0;
                pio_cfg[port].offset = 0;
            }
        }
        break;
        case DEVICE_TYPE_JOY:
            break;
        case DEVICE_TYPE_MOUSE:
            break;
    }
}


void _port_handler_snoop(uint8_t port)
{
    uint64_t now = time_us_64();

    // attempt to detect controller activity here
    // try to determine if port is occupied by a controller
    // as well as detect some button combinations
    // Check if PIO is setup for this port
    PIO pio = pio_cfg[port].pio;
    uint sm = pio_cfg[port].sm;
    int event_count = 0;
    if (pio != NULL) {
        // Non-blocking: check if RX FIFO has data
        while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            uint32_t value = pio_sm_get(pio, sm);
            event_count++;
            // Process or store value as needed (example: log or buffer)
            // check enough time has passed since last samples. if so, start processing from first fsm state
            if (port_data[port].idle_timer + 5000U < now) {
                port_data[port].joy.state = JOY_STATE_STEP_0_IDLE_DIRS;
                port_data[port].idle_timer = now;
            }
        }
        if (event_count > 0) {
            _core1_log_msg("Port %d RX FIFO events: %d", port, event_count);
        }
    }

    // update total event counter
    evt_counter += event_count;
}

void _port_handler_joy(uint8_t port)
{

}

void _port_handler_mouse(uint8_t port)
{

}

uint64_t port_get_evt_count()
{ 
    return evt_counter;
}
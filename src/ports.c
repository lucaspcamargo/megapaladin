#include "ports.h"

#include <pico/stdlib.h>
#include <hardware/gpio.h>

#include <stdint.h>
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

#define PIN_IDX_D0 0
#define PIN_IDX_D1 1
#define PIN_IDX_D2 2
#define PIN_IDX_D3 3
#define PIN_IDX_TL 4
#define PIN_IDX_TH 5
#define PIN_IDX_TR 6

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

typedef struct pin_watch_data_st
{
    bool initialized;
    bool curr;
    bool last;
    uint64_t last_change;
    uint64_t last_change_delta;
} pin_watch_data;

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

static const uint8_t const *port_pins[] = {port1_pins, port2_pins};
static uint8_t port_types[PORT_COUNT];
static port_data_td port_data[PORT_COUNT];
static irq_data_td irq_data[PORT_COUNT];
static uint64_t evt_counter;


void _port_mode_setup(uint8_t port, uint8_t type);
void _port_mode_reset(uint8_t port, uint8_t type);

void _port_irq(uint gpio, uint32_t evt_mask);

void _port_handler_snoop(uint8_t port);
void _port_handler_joy(uint8_t port);
void _port_handler_mouse(uint8_t port);
bool _port_pin_watch_update(pin_watch_data *data, uint64_t now, uint8_t pin); // returns true is there was change

// use core1 logging directly
extern void _core1_log_msg(const char *fmt, ...);


void port_preinit()
{
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

void port_init()
{
    memset(port_types, 0x00, sizeof(port_types));
    memset(port_data, 0x00, sizeof(port_data));
    memset(irq_data, 0x00, sizeof(irq_data));
    for(uint8_t p_idx = 0; p_idx < PORT_COUNT; p_idx++)
        memset(&(irq_data[p_idx].captures), 0xff, sizeof(uint32_t)*JOY_STATE_STEP_COUNT);
    evt_counter = 0;

    gpio_set_irq_callback(&_port_irq);

    for(uint8_t p_idx = 0; p_idx < PORT_COUNT; p_idx++)
    {
        _port_mode_setup(p_idx, DEVICE_TYPE_NONE);
    }

    irq_set_enabled(IO_IRQ_BANK0, true);
}

void port_step()
{
    return;

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
            const uint gpio = port_pins[port][PIN_IDX_TH];
            gpio_set_irq_enabled(gpio, GPIO_EVT_BOTH_EDGES, true);
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
            const uint gpio = port_pins[port][PIN_IDX_TH];
            gpio_set_irq_enabled(gpio, GPIO_EVT_BOTH_EDGES, false);
        }
        break;
        case DEVICE_TYPE_JOY:
            break;
        case DEVICE_TYPE_MOUSE:
            break;
    }
}

void _port_irq(uint gpio, uint32_t evt_mask)
{
    uint8_t port = 0;
    if(gpio == PORT1_5_PIN)
        port = 0;
    else if(gpio == PORT2_5_PIN)
        port = 1;
    else return;
    
    switch(port_types[port])
    {
        case DEVICE_TYPE_NONE:
        {
            // controller snoop mode
            uint8_t curr_stage = irq_data[port].stage + 1;
            const uint32_t capture = gpio_get_all();
            if(curr_stage == JOY_STATE_STEP_COUNT)
                curr_stage = 0;
            irq_data[port].captures[curr_stage] = capture;
            irq_data[port].stage = curr_stage;
        }
        break;
    }

    evt_counter++;
}


void _port_handler_snoop(uint8_t port)
{
    // attempt to detect controller activity here
    // try to determine if port is occupied by a controller
    // as well as detect some button combinations
    
    const uint64_t now = time_us_64();
    port_data_td *data = port_data + port;
    const uint8_t th_pin = port_pins[port][PIN_IDX_TH];

    // handle idle timer for resetting controller stage
    if(irq_data[port].stage == 0)
        port_data[port].idle_timer = now;
    else if(now - port_data[port].idle_timer > PORT_CYCLE_US)
    {
        irq_data[port].stage = 0;
        port_data[port].idle_timer = now;
    }

}

void _port_handler_joy(uint8_t port)
{

}

void _port_handler_mouse(uint8_t port)
{

}


bool _port_pin_watch_update(pin_watch_data *data, const uint64_t now, const uint8_t pin)
{
    bool curr = gpio_get(pin);

    if(!data->initialized)
    {
        data->last = curr;
        data->curr = curr;
        data->last_change = now;
        data->last_change_delta = 0;
        data->initialized = true;
        return false;
    }
    else
    {
        data->last = data->curr;
        data->curr = curr;
        if(data->last != data->curr)
        {
            // changed
            data->last_change_delta = now - data->last_change;
            data->last_change = now;
            return true;
        }
        else
        {
            // not changed
            return false;
        }
    }

}


uint64_t port_get_evt_count()
{ 
    return evt_counter;
}
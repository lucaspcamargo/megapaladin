#include "support.h"

#include <hardware/adc.h>
#include <pico/util/queue.h>

static queue_t q_120, q_021;
static queue_t q_str;

void fifo_init()
{
    queue_init(&q_120, sizeof(FIFOCmd), CMD_BUF_SZ);
    queue_init(&q_021, sizeof(FIFOCmd), CMD_BUF_SZ);
    queue_init(&q_str, MSG_LEN_MAX, MSG_BUF_SZ);
}

bool fifo_push(FIFOCmd *incmd)
{
    return queue_try_add(get_core_num()?&q_120:&q_021, incmd);
}

bool fifo_pop(FIFOCmd *incmd)
{
    return queue_try_remove(get_core_num()?&q_021:&q_120, incmd);
}


bool fifo_str_push(const char msg[MSG_LEN_MAX])
{
    return queue_try_add(&q_str, msg);
}

bool fifo_str_pop(char *msg)
{
    return queue_try_remove(&q_str, msg);
}

const char *region_str(enum Region r)
{
    switch (r)
    {
    case REGION_US:
        return "US";
    case REGION_EU:
        return "EU";
    case REGION_JP:
        return "JP";
    default:
        return "??";
    }
}

const char * device_type_str(uint8_t dev_type)
{
    switch (dev_type)
    {
    case DEVICE_TYPE_NONE:
        return "NONE";
    case DEVICE_TYPE_JOY:
        return "JOY";
    case DEVICE_TYPE_MOUSE:
        return "MOUSE";
    default:
        return "UNKNOWN";
    }
}

void temp_init()
{
    // from sdk example
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
}

float temp_read()
{
    // from sdk example
    const float conversionFactor = 3.3f / (1 << 12);
    float adc = (float)adc_read() * conversionFactor;
    return 27.0f - (adc - 0.706f) / 0.001721f;
}


#define REG_AIRCR (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))

void reboot()
{
    //watchdog_reboot(0,0,0); instead: do this:
    REG_AIRCR = 0x5FA0004;
}
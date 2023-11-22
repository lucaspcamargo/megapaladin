#include "support.h"

#include <hardware/adc.h>

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
#include "defs.h"
#include "button.h"
#include "support.h"
#include "ports.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/flash.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <stdio.h>
#include <stdarg.h>


const unsigned char region_mapping[][2] = {
    {LANG_EN, FMT_NTSC}, // REGION_US
    {LANG_EN, FMT_PAL},  // REGION_EU
    {LANG_JP, FMT_NTSC}  // REGION_JP
};
enum PwrLEDState
{
    PLS_STEADY,
    PLS_BLINKING,
    PLS_DRONING
} __attribute__((packed));

bool syncing_now = false;
bool press_sync_sent = false;
enum Region region_curr = DEFAULT_REGION;
enum Region region_sel = DEFAULT_REGION;
enum PwrLEDState pwrled_state;
uint64_t pwrled_state_timer;
uint32_t pwrled_state_data;

void core1_preinit();
void core1_init();
void core1_loop();
void core1_region_commit();
void core1_region_select(enum Region r);
void core1_region_advance();
void core1_do_reset();
void core1_pwrled_update();
void core1_pwrled_blink(uint times);
void core1_pwrled_drone();
void core1_pwrled_steady();
void _core1_log_msg(const char *fmt, ...);


void core1_main()
{
    core1_init();
    for (;;)
        core1_loop();
}

void core1_preinit()
{
    // this can be called from other contexts to initialize essential signals as soon as possible

    // init region outputs and commit initial values
    gpio_init(PIN_OUT_FMT);
    gpio_set_dir(PIN_OUT_FMT, GPIO_OUT);
    gpio_init(PIN_OUT_LANG);
    gpio_set_dir(PIN_OUT_LANG, GPIO_OUT);
    core1_region_commit();

    port_preinit();
}

void core1_init()
{
    core1_preinit();
    port_init();
    flash_safe_execute_core_init();

    // reset out starts as input (hi-z)
    gpio_init(PIN_OUT_RESET);
    gpio_set_dir(PIN_OUT_RESET, GPIO_IN);
    gpio_set_pulls(PIN_OUT_RESET, false, false);

    // reset in is input with pullup
    gpio_init(PIN_IN_RESET);
    gpio_set_dir(PIN_IN_RESET, GPIO_IN);
    gpio_set_pulls(PIN_IN_RESET, true, false);

    // power led out is pwm out
    gpio_set_function(PIN_OUT_PWR_LED, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PIN_OUT_PWR_LED);
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, false);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.f);
    pwm_init(slice_num, &config, true);

    pwrled_state = PLS_STEADY;
    pwrled_state_timer = 0;
    pwrled_state_data = 0;
    pwm_set_gpio_level(PIN_OUT_PWR_LED, PWM_FULLBRIGHT);
    core1_pwrled_update();

    _core1_log_msg("Init done.");
}

void core1_loop()
{
    //_core1_log_msg("CORE 1 ITERATION @ %llu us", time_us_64());
    // handle reset button
    unsigned long now = time_us_64();
    btn_update(now);
    
    if (btn_was_released())
    {
        if(syncing_now)
        {
            // sync cancel 
            FIFOCmd req;
            req.opcode = FC_SYNC_STOP_REQ;
            fifo_push(&req);
        }
        else if(press_sync_sent)
        {
            press_sync_sent = false;
        }
        else
        {
            unsigned long pressedTime = btn_last_press_duration();
            if (pressedTime < BUTTON_LONGPRESS_US)
                core1_do_reset();
            else
                core1_region_advance();
        }
    }
    else if(btn_is_pressed() && btn_curr_press_duration(now) > BUTTON_SYNCPRESS_US && !press_sync_sent)
    {
        // sync request
        FIFOCmd req;
        req.opcode = syncing_now? FC_SYNC_STOP_REQ : FC_SYNC_START_REQ;
        fifo_push(&req);
        press_sync_sent = true;
    }

    core1_pwrled_update();

    FIFOCmd core0_cmd;
    while (fifo_pop(&core0_cmd))
    {
        switch (core0_cmd.opcode)
        {
        case FC_STATUS_REQ:
        {
            FIFOCmd reply;
            reply.opcode = FC_STATUS_REPL;
            reply.data[0] = region_curr;
            reply.data[1] = region_sel;
            reply.data[2] = port_get_evt_count() % 256;
            fifo_push(&reply);
        }
        break;
        case FC_SYNC_STATUS_REPL:
        {
            syncing_now = core0_cmd.data[0] ? true : false;
            (syncing_now? core1_pwrled_drone : core1_pwrled_steady)();
        }
        break;
        case FC_REGION_SELECT:
        {
            core1_region_select((enum Region) core0_cmd.data[0]);
        }
        break;
        case FC_REGION_COMMIT:
        {
            core1_region_commit();
        }
        break;
        case FC_RESET:
        {
            core1_do_reset();
        }
        break;
        case FC_JOY_HOST_STATUS:
        {
            for(int port = 0; port < PORT_COUNT; port++)
            {
                uint8_t bt_type = core0_cmd.data[1+port/2] & (0xf << port%2);
                uint8_t curr_type = port_type_curr(port);
                if(bt_type != curr_type)
                {
                    port_type_set(port, bt_type);
                    _core1_log_msg("Port %d set to type %s.", port, device_type_str(bt_type));
                }
            }
        }
        break;
        case FC_JOY_HOST_EVENT:
        {
            port_on_host_event(&core0_cmd);
        }
        break;
        }
    }

    port_step();
}

void core1_region_commit()
{
    region_curr = region_sel;
    gpio_put(PIN_OUT_LANG, (bool)region_mapping[region_curr][0]);
    gpio_put(PIN_OUT_FMT, (bool)region_mapping[region_curr][1]);
}

void core1_region_select(enum Region r)
{
    region_sel = r;
    core1_pwrled_blink(region_sel + 1);
}

void core1_region_advance()
{
    core1_region_select((enum Region)((region_sel + 1) % REGION_COUNT));
}

void core1_do_reset()
{
    core1_region_commit();

    gpio_set_pulls(PIN_IN_RESET, false, false);
    gpio_set_dir(PIN_OUT_RESET, GPIO_OUT);
    gpio_put(PIN_OUT_RESET, false);
    sleep_ms(RESET_DURATION_MS);          // TODO move to mainloop processing
    gpio_set_pulls(PIN_IN_RESET, true, false);
    gpio_set_dir(PIN_OUT_RESET, GPIO_IN); // back to hi-z
}

void core1_pwrled_update()
{
    static enum PwrLEDState prev_state = PLS_STEADY;
    unsigned long now = time_us_64();

    switch (pwrled_state)
    {
    case PLS_STEADY:
        if(prev_state != PLS_STEADY)
            pwm_set_gpio_level(PIN_OUT_PWR_LED, PWM_FULLBRIGHT);
        break;
    case PLS_BLINKING:
    {
        uint64_t delta = now - pwrled_state_timer;
        static const uint64_t blink_period = (BLINK_DURATION_US+BLINK_INTERVAL_US);
        uint64_t max_delta = blink_period * pwrled_state_data;
        if(delta > max_delta)
        {
            pwrled_state = syncing_now? PLS_DRONING : PLS_STEADY;
            if(!syncing_now)
                pwm_set_gpio_level(PIN_OUT_PWR_LED, PWM_FULLBRIGHT);
            pwrled_state_data = 0;
            pwrled_state_timer = now;
        }
        else
        {
            uint64_t periodic = delta % blink_period;
            pwm_set_gpio_level(PIN_OUT_PWR_LED, periodic > BLINK_DURATION_US? PWM_FULLBRIGHT : 0);
        }
    }
    break;
    case PLS_DRONING:
    {
        int ms = (time_us_64()>>10)%1024;
        ms = (ms>=512?1023-ms:ms)/2;
        pwm_set_gpio_level(PIN_OUT_PWR_LED, ms*ms);
    }
    }
    prev_state = pwrled_state;
}

void core1_pwrled_blink(uint times)
{
    pwrled_state = PLS_BLINKING;
    pwrled_state_data = times;
    pwrled_state_timer = time_us_64();
}

void core1_pwrled_drone()
{
    pwrled_state = PLS_DRONING;
    pwrled_state_data = 0;
    pwrled_state_timer = time_us_64();
}

void core1_pwrled_steady()
{
    pwrled_state = PLS_STEADY;
    pwrled_state_data = 0;
    pwrled_state_timer = time_us_64();
}


void _core1_log_msg(const char *fmt, ...)
{
    char buf[MSG_LEN_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, MSG_LEN_MAX, fmt, args);
    va_end(args);
    if(fifo_str_push(buf))
    {
        FIFOCmd cmd;
        cmd.opcode = FC_LOG_NOTIFY;
        fifo_push(&cmd);
    }
}
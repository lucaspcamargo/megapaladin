#include "defs.h"
#include "button.h"
#include "support.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

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

enum Region region_curr = DEFAULT_REGION;
enum Region region_sel = DEFAULT_REGION;
enum PwrLEDState pwrled_state;
uint64_t pwrled_state_timer;
uint32_t pwrled_state_data;

void core1_init();
void core1_loop();
void core1_region_commit();
void core1_region_select(enum Region r);
void core1_region_advance();
void core1_do_reset();
void core1_pwrled_update();
void core1_pwrled_blink(uint times);

void core1_main()
{
    core1_init();
    for (;;)
        core1_loop();
}

void core1_init()
{
    // (re)init region outputs and commit initial values
    gpio_init(PIN_OUT_FMT);
    gpio_set_dir(PIN_OUT_FMT, GPIO_OUT);
    gpio_init(PIN_OUT_LANG);
    gpio_set_dir(PIN_OUT_LANG, GPIO_OUT);
    core1_region_commit();

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
    core1_pwrled_update(0);
}

void core1_loop()
{
    // handle reset button
    unsigned long now = time_us_64();
    btn_update(now);
    if (btn_was_released())
    {
        unsigned long pressedTime = btn_last_press_duration();
        if (pressedTime < 400)
            core1_do_reset();
        else
            core1_region_advance();

        // TODO on release, clear sync sent during press flag
    }
    // TODO else if(btn_is_pressed() && btn is held for more than threshold && ! sync sent during press)
    //               send_bt_sync_command();

    core1_pwrled_update();

    FIFOCmd core0_cmd;
    while (multicore_fifo_pop_timeout_us(FIFO_TIMEOUT_US, (uint32_t *)&core0_cmd))
    {
        switch (core0_cmd.opcode)
        {
        case FC_STATUS_REQ:
        {
            FIFOCmd reply;
            reply.opcode = FC_STATUS_REPL;
            reply.data[0] = region_curr;
            reply.data[1] = region_sel;
            reply.data[2] = 0;
            multicore_fifo_push_blocking(fifo_pack(&reply));
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
        }
    }
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

    gpio_set_dir(PIN_OUT_RESET, GPIO_OUT);
    gpio_put(PIN_OUT_RESET, false);
    sleep_ms(RESET_DURATION_MS);          // TODO move to mainloop processing
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
            pwrled_state = PLS_STEADY;
            pwrled_state_data = 0;
            pwrled_state_timer = 0;
        }
        else
        {
            uint64_t periodic = delta % blink_period;
            pwm_set_gpio_level(PIN_OUT_PWR_LED, periodic > BLINK_DURATION_US? PWM_FULLBRIGHT : 0);
        }
    }
    break;
    case PLS_DRONING:
        pwm_set_gpio_level(PIN_OUT_PWR_LED, PWM_FULLBRIGHT/4); // TODO sweep

    }
    prev_state = pwrled_state;
}

void core1_pwrled_blink(uint times)
{
    // TODO setup fsm
}
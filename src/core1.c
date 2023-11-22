#include "defs.h"
#include "button.h"

#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

enum Region region_curr = DEFAULT_REGION;
enum Region region_sel = DEFAULT_REGION;

const unsigned char region_mapping[][2] = {
    {LANG_EN, FMT_NTSC}, // REGION_US
    {LANG_EN, FMT_PAL},  // REGION_EU
    {LANG_JP, FMT_NTSC}  // REGION_JP
};

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
    pwm_set_gpio_level(PIN_OUT_PWR_LED, PWM_FULLBRIGHT);
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
    //TODO else if(btn_is_pressed() && btn is held for more than threshold && ! sync sent during press) 
    //              send_bt_sync_command();

    core1_pwrled_update();

    FIFOCmd core0_cmd;
    /* TODO 
    if (rp2040.fifo.pop_nb(reinterpret_cast<uint32_t *>(&core0_cmd)))
    {
        switch (core0_cmd.opcode)
        {
        case FC_STATUS_REQ:
        {
            FIFOCmd reply;
            reply.opcode = FC_STATUS_REPL;
            rp2040.fifo.push(*reinterpret_cast<uint32_t *>(&reply));
        }
        break;
        case FC_POWER_LED_BLINK:
        {
            // TODO do this with a state machine in the loop, without blocking
            uint8_t times = core0_cmd.data[0];
            while (times)
            {
                digitalWrite(PIN_OUT_PWR_LED, LOW);
                delay(BLINK_DURATION_MS);
                digitalWrite(PIN_OUT_PWR_LED, HIGH);
                if (times)
                    delay(BLINK_INTERVAL_MS);
            }
        }
        break;
        }
    } */
}

void core1_region_commit()
{
    region_curr = region_sel;
    gpio_put(PIN_OUT_LANG, (bool) region_mapping[region_curr][0]);
    gpio_put(PIN_OUT_FMT, (bool) region_mapping[region_curr][1]);
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
    sleep_ms(RESET_DURATION_MS); // TODO move to mainloop processing
    gpio_set_dir(PIN_OUT_RESET, GPIO_IN); // back to hi-z
}


void core1_pwrled_update()
{
    // TODO step fsm
}

void core1_pwrled_blink(uint times)
{
    // TODO setup fsm
}
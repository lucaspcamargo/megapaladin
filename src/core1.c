#include "pico/stdlib.h"
#include "defs.h"
#include "button.h"

enum Region region_curr = DEFAULT_REGION;
enum Region region_sel = DEFAULT_REGION;
ButtonTemplate<PIN_IN_RESET> btn_reset;

const unsigned char region_mapping[][2] = {
    {LANG_EN, FMT_NTSC}, // REGION_US
    {LANG_EN, FMT_PAL},  // REGION_EU
    {LANG_JP, FMT_NTSC}  // REGION_JP
};

void core1_setup();
void core1_loop();
void core1_region_commit();
void core1_region_select(enum Region r);
void core1_region_advance();
void core1_do_reset();

void core1_main()
{
    core1_init();
    for (;;)
        core1_loop();
}

void core1_init()
{
    pinMode(PIN_OUT_FMT, OUTPUT);
    pinMode(PIN_OUT_LANG, OUTPUT);
    core1_region_commit();

    pinMode(PIN_OUT_RESET, INPUT);
    digitalWrite(PIN_OUT_RESET, LOW);
    pinMode(PIN_IN_RESET, INPUT_PULLUP);

    pinMode(PIN_OUT_PWR_LED, OUTPUT);
    digitalWrite(PIN_OUT_PWR_LED, HIGH);
}

void core1_loop()
{
    // handle reset button
    unsigned long now = millis();
    btn_reset.update(now);
    if (btn_reset.wasReleased())
    {
        unsigned long pressedTime = btn_reset.lastPressDuration();
        if (pressedTime < 400)
            core1_do_reset();
        else
            core1_region_advance();
    }

    FIFOCmd core0_cmd;
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
    }
}

void core1_region_commit()
{
    region_curr = region_sel;
    digitalWrite(PIN_OUT_LANG, mapping[region_curr][0]);
    digitalWrite(PIN_OUT_FMT, mapping[region_curr][1]);
}

void core1_region_select(enum Region r)
{
    region_sel = r;
    blinkPowerLed(region_sel + 1);
}

void core1_region_advance()
{
    core1_region_select(static_cast<Region>((region_sel + 1) % REGION_COUNT));
}

void core1_do_reset()
{
    core1_region_output();
    pinMode(PIN_OUT_RESET, OUTPUT);
    digitalWrite(PIN_OUT_RESET, LOW);
    delay(200);
    pinMode(PIN_OUT_RESET, INPUT);
}

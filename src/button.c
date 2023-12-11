#include "button.h"
#include "hardware/gpio.h"

static uint64_t m_lastPress;
static uint64_t m_lastPressTime;
static uint8_t m_flags;


void btn_init()
{
    gpio_init(PIN_IN_RESET);
    gpio_set_dir(PIN_IN_RESET, GPIO_IN);
    gpio_set_pulls(PIN_IN_RESET, true, false);
    m_lastPress = 0;
    m_lastPressTime = 0;
    m_flags = 0;
}

void btn_update( uint64_t now )
{
    bool pressedNow = !gpio_get( PIN_IN_RESET );
    if( pressedNow && ! (m_flags & F_PRESSED) )
    {
        // is pressed, wasn't before
        if(now - m_lastPress > BUTTON_DEBOUNCE_US)
        {
            m_flags |= (F_WAS_PRESSED | F_PRESSED);
            m_lastPress = now;
        }
    }
    else if( !pressedNow && (m_flags & F_PRESSED) )
    {
        // is not pressed, was before
        if(now - m_lastPress > BUTTON_DEBOUNCE_US)
        {
            m_flags &= ~(F_PRESSED);
            m_flags |= F_WAS_RELEASED;
            m_lastPressTime = now - m_lastPress;
        }
    }
}

bool btn_was_pressed()
{
    bool ret = m_flags & F_WAS_PRESSED;
    m_flags &= ~F_WAS_PRESSED;
    return ret;
}

bool btn_is_pressed()
{
    return m_flags & F_PRESSED;
}

uint64_t btn_last_press_duration()
{
    return m_lastPressTime;
}

uint64_t btn_curr_press_duration( uint64_t now )
{
    return btn_is_pressed()? now - m_lastPress : 0;
}

bool btn_was_released()
{
    bool ret = m_flags & F_WAS_RELEASED;
    m_flags &= ~F_WAS_RELEASED;
    return ret;
}
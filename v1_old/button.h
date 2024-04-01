#pragma once

#define BUTTON_DEBOUNCE_MILLIS 50

template <int PIN, bool PULLUP = (PIN==13) >
class ButtonTemplate
{
    enum Flags
    {
        F_PRESSED       = 0x01,
        F_WAS_PRESSED   = 0x02,
        F_WAS_RELEASED  = 0x04
    };

public:
    ButtonTemplate()
    {
        pinMode( PIN, PULLUP? INPUT_PULLUP : INPUT );
        m_lastPress = 0;
        m_lastPressTime = 0;
        m_flags = 0;
    }

    ~ButtonTemplate() { /* do nothing */ }

    void update( ulong now )
    {
        bool pressedNow = !digitalRead( PIN );
        if( pressedNow && ! (m_flags & F_PRESSED) )
        {
            // is pressed, wasn't before
            if(now - m_lastPress > BUTTON_DEBOUNCE_MILLIS)
            {
                m_flags |= (F_WAS_PRESSED | F_PRESSED);
                m_lastPress = now;
            }
        }
        else if( !pressedNow && (m_flags & F_PRESSED) )
        {
            // is not pressed, was before
            if(now - m_lastPress > BUTTON_DEBOUNCE_MILLIS)
            {
                m_flags &= ~(F_PRESSED);
                m_flags |= F_WAS_RELEASED;
                m_lastPressTime = now - m_lastPress;
            }
        }
    }

    bool wasPressed()
    {
        bool ret = m_flags & F_WAS_PRESSED;
        m_flags &= ~F_WAS_PRESSED;
        return ret;
    }

    bool isPressed() const
    {
        return m_flags & F_PRESSED;
    }

    ulong lastPressDuration() const
    {
        return m_lastPressTime;
    }

    bool wasReleased()
    {
        bool ret = m_flags & F_WAS_RELEASED;
        m_flags &= ~F_WAS_RELEASED;
        return ret;
    }

private:
    ulong m_lastPress;
    ulong m_lastPressTime;
    byte m_flags;
}; 

#pragma once

#include "defs.h"

enum ButtonFlags
{
    F_PRESSED       = 0x01,
    F_WAS_PRESSED   = 0x02,
    F_WAS_RELEASED  = 0x04
};

void btn_init();

void btn_update( uint64_t now );

bool btn_was_pressed();

bool btn_is_pressed();

uint64_t btn_last_press_duration();

uint64_t btn_curr_press_duration( uint64_t now );

bool btn_was_released();
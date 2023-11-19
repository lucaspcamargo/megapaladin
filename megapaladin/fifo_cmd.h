#pragma once

struct FIFOCmd
{
  uint8_t opcode;
  uint8_t data[3];
} __attribute__ ((aligned (4)));

enum FIFOCmdOpcode : uint8_t
{
  FC_NOP = 0,
  FC_STATUS_REQ, // 0->1
  FC_STATUS_REPL, // 1->0
  FC_POWER_LED_BLINK, // 0->1, times
};

#pragma once

#include "pico/types.h"

#define MP_VERSION 1

// CONFIGS
#define IS_PICO_W 1
#define SERIAL_INIT_TIMEOUT 1000
#define SERIAL_OUT_DELAY 300
#define BLINK_DURATION_MS 50
#define BLINK_INTERVAL_MS 100
#define CMD_BUF_SZ 64
#define DEFAULT_REGION REGION_US

#define PIN_IN_RESET 18
#define PIN_OUT_RESET 19
#define PIN_OUT_FMT 20
#define PIN_OUT_LANG 21
#define PIN_OUT_PWR_LED 22


typedef uint64_t ulong;

enum Region
{
  REGION_US,
  REGION_EU,
  REGION_JP,
  REGION_COUNT
};

enum Language
{
  LANG_JP = 0,
  LANG_EN = 1  
};

enum Format
{
  FMT_PAL = 0,
  FMT_NTSC = 1
};

typedef struct FIFOCmd_t
{
  unsigned char opcode;
  unsigned char data[3];
} __attribute__ ((aligned (4))) FIFOCmd;

enum FIFOCmdOpcode
{
  FC_NOP = 0,
  FC_STATUS_REQ, // 0->1
  FC_STATUS_REPL, // 1->0
  FC_REGION_SELECT, // 0->1
  FC_REGION_COMMIT, // 0->1
} __attribute__ ((packed));
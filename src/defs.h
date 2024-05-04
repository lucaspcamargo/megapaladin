#pragma once

#include <pico/types.h>

#define MP_VERSION 1
#define MP_REVISION 999

// CONFIGS
#define IS_PICO_W (PICO_BOARD == "pico_w")
#define DEFAULT_REGION REGION_US
#define CMD_BUF_SZ 64
#define CORE0_MAIN_TIMER_INTERVAL 50  // TODO define value. seems to be in millisseconds
#define JOY_MAX 2

#define BUTTON_DEBOUNCE_US 50000        // 0.05s
#define BUTTON_LONGPRESS_US 400000        // 0.4s
#define BUTTON_SYNCPRESS_US 2500000        // 2.5 s
#define BLINK_DURATION_US 50000         // 0.05s
#define BLINK_INTERVAL_US 100000        // 0.1s
#define SERIAL_INIT_TIMEOUT_US 1000000  // 1s
#define RESET_DURATION_MS 200           // 0.2s
#define FIFO_TIMEOUT_US 10              // 0.00001 s

#define PWM_FULLBRIGHT 0xfffe

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
} __attribute__ ((packed));

enum Language
{
  LANG_JP = 0,
  LANG_EN = 1  
} __attribute__ ((packed));

enum Format
{
  FMT_PAL = 0,
  FMT_NTSC = 1
} __attribute__ ((packed));

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
  FC_RESET, // 0->1

  FC_SYNC_START_REQ,  // 1->0
  FC_SYNC_STOP_REQ,  // 1->0
  FC_SYNC_STATUS_REPL,  // 0->1, whether sync is in progress, can stop by itself

  FC_JOY_HOST_STATUS, // 0->1, update connected devices info
  FC_JOY_HOST_EVENT,  // 0->1, information concerning controllers
} __attribute__ ((packed));

enum FIFOCmdFlags
{
  // for FC_JOY_HOST_STATUS
  FC_F_JOY_STATUS_CONNECTED = 0x01,
  FC_F_JOY_STATUS_DISCONNECTED = 0x02,
  // bytes 1 and 2 are the joy type of connected controllers, or NONE
  // 22221111 44443333

  // for FC_JOY_HOST_EVENT
  FC_F_JOY_EVENT_CURR = 0x01,
  // bytes 1 and 2 carry controller or mouse data
  // joypad: SACBRLDU ----MXYZ
  // mouse:  SACBM--- XXXXYYYY, LMB=A RMB=C MMB=B
} __attribute__ ((packed));

// Device types
#define DEVICE_TYPE_NONE      0x00
#define DEVICE_TYPE_JOY       0x01
#define DEVICE_TYPE_MOUSE     0x02
#define DEVICE_TYPE_UNKNOWN   0x0F


// Button bits
// WE USE SGDK STANDARD AROUND THESE PARTS
#define BUTTON_UP       0x0001
#define BUTTON_DOWN     0x0002
#define BUTTON_LEFT     0x0004
#define BUTTON_RIGHT    0x0008
#define BUTTON_A        0x0040
#define BUTTON_B        0x0010
#define BUTTON_C        0x0020
#define BUTTON_START    0x0080
#define BUTTON_X        0x0400
#define BUTTON_Y        0x0200
#define BUTTON_Z        0x0100
#define BUTTON_MODE     0x0800
#define BUTTON_LMB      0x0040  // is A
#define BUTTON_MMB      0x0010  // is B
#define BUTTON_RMB      0x0020  // is C 
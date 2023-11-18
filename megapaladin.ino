#include <Arduino.h>

enum Region
{
  REGION_US,
  REGION_EU,
  REGION_JP,
  REGION_COUNT
};

enum Format
{
  FMT_NTSC = 0,
  FMT_PAL = 1
};

enum Language
{
  LANG_JP = 0,
  LANG_EN = 1  
};

const uint8_t mapping[][2] = {
  {LANG_EN, FMT_NTSC},  // REGION_US
  {LANG_EN, FMT_PAL},  // REGION_EU
  {LANG_JP, FMT_NTSC}   // REGION_JP
};

#define PIN_IN_RESET 18
#define PIN_OUT_RESET 19
#define PIN_OUT_FMT 20
#define PIN_OUT_LANG 21
#define PIN_OUT_PWR_LED 22

Region curr_region = REGION_US;


void setRegionOutputs();

void setup() {
  pinMode(PIN_IN_RESET, INPUT_PULLUP);
  pinMode(PIN_OUT_RESET, OUTPUT);
  pinMode(PIN_OUT_FMT, OUTPUT);
  pinMode(PIN_OUT_LANG, OUTPUT);
  pinMode(PIN_OUT_PWR_LED, OUTPUT);

  setRegionOutputs();

}

void loop() {
  
}

void setRegionOutputs()
{
  digitalWrite(PIN_OUT_LANG, mapping[curr_region][0]);
  digitalWrite(PIN_OUT_FMT, mapping[curr_region][1]);
}

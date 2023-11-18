#include <Arduino.h>
#include "button.h"

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
ButtonTemplate<PIN_IN_RESET> btn_reset;

void setRegionOutputs();
void advanceRegion();
void doReset();
void blinkPowerLed(uint8_t times);

void setup() {
  pinMode(PIN_OUT_RESET, INPUT);
  digitalWrite(PIN_OUT_RESET, LOW);
  pinMode(PIN_OUT_FMT, OUTPUT);
  pinMode(PIN_OUT_LANG, OUTPUT);
  pinMode(PIN_OUT_PWR_LED, OUTPUT);
  digitalWrite(PIN_OUT_PWR_LED, HIGH);
  
  pinMode(PIN_IN_RESET, INPUT_PULLUP);

  setRegionOutputs();

  Serial.begin(); // init debug/reset console
}

void loop() {
  ulong now = millis();
  btn_reset.update(now);
  if(btn_reset.wasReleased())
  {
    ulong pressedTime = btn_reset.lastPressDuration();
    if(pressedTime < 400)
      doReset();
    else
      advanceRegion();
  }
}

void setRegionOutputs()
{
  digitalWrite(PIN_OUT_LANG, mapping[curr_region][0]);
  digitalWrite(PIN_OUT_FMT, mapping[curr_region][1]);
}

void advanceRegion()
{
  curr_region = static_cast<Region>((curr_region+1) % REGION_COUNT);
  blinkPowerLed(curr_region + 1);
}

void doReset()
{
  setRegionOutputs();
  pinMode(PIN_OUT_RESET, OUTPUT);
  digitalWrite(PIN_OUT_RESET, LOW);
  delay(200);  
  pinMode(PIN_OUT_RESET, INPUT);
}

void blinkPowerLed(uint8_t times)
{
  while(times--)
  {
    digitalWrite(PIN_OUT_PWR_LED, LOW);
    delay(50);  
    digitalWrite(PIN_OUT_PWR_LED, HIGH);
    if(times)
      delay(100);      
  }
}

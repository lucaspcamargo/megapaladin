#include <Arduino.h>
#define VERSION 1

// CONFIGS
#define IS_PICO_W 1
#define SERIAL_BAUD 115200
#define SERIAL_INIT_TIMEOUT 1000
#define SERIAL_OUT_DELAY 300
#define BLINK_DURATION_MS 50
#define BLINK_INTERVAL_MS 100
#define CMD_BUF_SZ 64
#define DEFAULT_REGION REGION_US

#include "button.h"
#include "fifo_cmd.h"
#if IS_PICO_W
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "secrets.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif


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

Region curr_region = DEFAULT_REGION;
Region sel_region = DEFAULT_REGION;
ButtonTemplate<PIN_IN_RESET> btn_reset;
String cmd_buf;

void setRegionOutputs();
void advanceSelRegion();
const char * regionToStr(Region r);
void doReset();
void blinkPowerLed(uint8_t times);
void processSerialCommand();
void processCore1Reply(const FIFOCmd repl);

#if IS_PICO_W
uint8_t wifi_prev_status = WL_IDLE_STATUS;
bool ota_was_setup = false;
void setupOTA();
#endif



void setup() 
{
  pinMode(PIN_OUT_RESET, INPUT);
  digitalWrite(PIN_OUT_RESET, LOW);
  pinMode(PIN_OUT_FMT, OUTPUT);
  pinMode(PIN_OUT_LANG, OUTPUT);
  
  pinMode(PIN_IN_RESET, INPUT_PULLUP);

  setRegionOutputs();

  ulong serial_setup_start = millis();
  Serial.begin(SERIAL_BAUD); // init debug/reset console
  while((millis() - serial_setup_start < SERIAL_INIT_TIMEOUT) && !Serial);
  delay(SERIAL_OUT_DELAY);
  Serial.print("\nmegaPALadin V");
  Serial.println(VERSION);
  cmd_buf.reserve(CMD_BUF_SZ);

#if IS_PICO_W
  Serial.println("WiFi: Initializing");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("megapaladin");
  wifi_prev_status = WiFi.status();
  WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PSK);
#endif

}


void loop() 
{
  ulong now = millis();
  btn_reset.update(now);
  if(btn_reset.wasReleased())
  {
    ulong pressedTime = btn_reset.lastPressDuration();
    if(pressedTime < 400)
      doReset();
    else
      advanceSelRegion();
  }

  while(Serial.available())
  {
    char in = Serial.read();
    if(in == '\n')
      processSerialCommand();
    else
      if(cmd_buf.length() < CMD_BUF_SZ && in != '\r')
        cmd_buf.concat(in);
  }

  FIFOCmd core1_repl;
  while(rp2040.fifo.pop_nb(reinterpret_cast<uint32_t*>(&core1_repl)))
  {
    // got a message from core 1
    processCore1Reply(core1_repl);
  }

#if IS_PICO_W
  uint8_t currStatus = WiFi.status();
  if(currStatus != wifi_prev_status)
  {
    if(currStatus == WL_CONNECT_FAILED)
    {
      Serial.println("WiFi: Connection failed, retrying...");
      WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PSK);
    }
    else if(currStatus == WL_CONNECTED)
    {
      Serial.println("WiFi: Connected");
      if(!ota_was_setup)
        setupOTA();
    }
    else if(currStatus == WL_NO_SSID_AVAIL)
    {
      Serial.print("WiFi: SSID '");
      Serial.print(OTA_WIFI_SSID);
      Serial.print("' not found");
    }
    wifi_prev_status = currStatus;
  }
  if(currStatus == WL_CONNECTED)
    ArduinoOTA.handle();
#endif
}


void setRegionOutputs()
{
  digitalWrite(PIN_OUT_LANG, mapping[curr_region][0]);
  digitalWrite(PIN_OUT_FMT, mapping[curr_region][1]);
}


void advanceSelRegion()
{
  sel_region = static_cast<Region>((sel_region+1) % REGION_COUNT);
  blinkPowerLed(sel_region + 1);
}


void doReset()
{
  curr_region = sel_region;
  setRegionOutputs();
  pinMode(PIN_OUT_RESET, OUTPUT);
  digitalWrite(PIN_OUT_RESET, LOW);
  delay(200);  
  pinMode(PIN_OUT_RESET, INPUT);
}


void blinkPowerLed(uint8_t times)
{
  FIFOCmd cmd;
  cmd.opcode = FC_POWER_LED_BLINK;
  cmd.data[0] = times;
  rp2040.fifo.push(*reinterpret_cast<uint32_t*>(&cmd));   
}


void processCore1Reply(const FIFOCmd repl)
{
  switch(repl.opcode)
  {
    case FC_STATUS_REPL:
      Serial.println("Auxiliary core is running.");
      break;
    default:
      Serial.print("Core 0: unknown opcode from core 1: ");
      Serial.println(repl.opcode);
  }
}


void processSerialCommand()
{
  if(cmd_buf == "status")
  {
    printStatus();
  }
  else if(cmd_buf == "reset")
  {
    doReset();
    Serial.print("Applied region, and resetted console.");
  }
  else if(cmd_buf == "us")
  {
    Serial.print("Selected US region\n");
    sel_region = REGION_US;
    blinkPowerLed(sel_region + 1);
  }
  else if(cmd_buf == "eu")
  {
    Serial.print("Selected EU region\n");
    sel_region = REGION_EU;
    blinkPowerLed(sel_region + 1);
  }
  else if(cmd_buf == "jp")
  {
    Serial.print("Selected JP region\n");
    sel_region = REGION_JP;
    blinkPowerLed(sel_region + 1);
  }
  else if(cmd_buf == "reboot")
  {
    Serial.print("Rebooting the megaPALadin and the console.");
    sel_region = DEFAULT_REGION;
    doReset();
    rp2040.restart();
  }
  else if(cmd_buf == "help" || cmd_buf == "?")
  {
    Serial.print("Available commands: status, reset, us, eu, jp, reboot, help, ?\n");
  }
  else
    Serial.print("Unrecognized command.\nTry 'help' or '?'.\n");

  cmd_buf = "";
}


void printStatus()
{
  Serial.print("Current region: ");
  Serial.println(regionToStr(curr_region));
  Serial.print("Selected region: ");
  Serial.println(regionToStr(sel_region));
  Serial.print("Heap mem: ");
  Serial.print(rp2040.getFreeHeap());
  Serial.print(" free, ");
  Serial.print(rp2040.getTotalHeap());
  Serial.println(" total");
  Serial.printf("Core temperature: %2.1fC\n", analogReadTemp());
  
  // request core 1 status, will be printed on reply
  FIFOCmd cmd;
  cmd.opcode = FC_STATUS_REQ;
  rp2040.fifo.push(*reinterpret_cast<uint32_t*>(&cmd)); 
  
}


const char * regionToStr(Region r)
{
  switch(r)
  {
    case REGION_US:
      return "US";
    case REGION_EU:
      return "EU";
    case REGION_JP:
      return "JP";
    default:
      return "??";
  }
}


#if IS_PICO_W
void setupOTA()
{
  ArduinoOTA.setHostname("megapaladin");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA: started");
    blinkPowerLed(5);
  });

  ArduinoOTA.onEnd([]() {  // do a fancy thing with our board led at end
    Serial.println("OTA: finished");
    blinkPowerLed(5);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("OTA: error #");
    Serial.println((int) error);
    Serial.println("Rebooting...");
    rp2040.restart();
  });

  /* setup the OTA server */
  ArduinoOTA.begin();
  Serial.println("OTA configured");
  ota_was_setup = true;
}
#endif


// finally, add the code for the second core
#include "core1.h"

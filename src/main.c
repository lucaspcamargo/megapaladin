#include <stdio.h>
#include "pico/stdlib.h"

#include "defs.h"
#include "utils.h"

char cmd_buf[CMD_BUF_SZ];
uint8_t cmd_buf_len;

void core0_setup();

void core0_process_core1_cmd(const FIFOCmd repl);
void core0_process_serial_cmd();

int main()
{
    // as soon as possible, get code in core 1 running (control)
    
    
    // now setup core 0 (bt and comms)
    core0_setup();
    return 0;
}

void core0_setup()
{
    uint64_t serial_setup_start = time_us_64();
    setup_default_uart();
    while ((time_us_64() - serial_setup_start < SERIAL_INIT_TIMEOUT) && uart_is_writable(PICO_DEFAULT_UART));
    delay(SERIAL_OUT_DELAY);
    printf("\nmegaPALadin V");
    printf(MP_VERSION);
    printf("\n");

    cmd_buf_len = 0;
}


void core0_process_core1_cmd(const FIFOCmd repl)
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


void core0_process_serial_cmd()
{
  if(cmd_buf == "status")
  {
    printStatus();
  }
  else if(cmd_buf == "reset")
  {
    doReset();
    printf("Applied region, and resetted console.\n");
  }
  else if(cmd_buf == "us")
  {
    printf("Selected US region\n");
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
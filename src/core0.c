#include "defs.h"
#include "support.h"

#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>


char cmd_buf[CMD_BUF_SZ];
uint8_t cmd_buf_len;

bool usb_uart_ok = false;

void core1_main();
void core0_process_core1_cmd(const FIFOCmd repl);
void core0_process_serial_cmd();
void core0_status_print();

int main() // for core 0
{
    // as soon as possible, get code in core 1 running (control)
    multicore_launch_core1(core1_main);

    // now setup core 0
    cmd_buf_len = 0;

    // setup usb uart
    uint64_t serial_setup_start = time_us_64();
    usb_uart_ok = stdio_usb_init();
    if (usb_uart_ok)
    {
        while ((time_us_64() - serial_setup_start < SERIAL_INIT_TIMEOUT_US) && !stdio_usb_connected())
            ;
        printf("\nmegaPALadin V%d\n", MP_VERSION);
    }

    // finishing touches
    temp_init();

    for(;;); // TODO tend to serial

    return 0;
}

void core0_process_core1_cmd(const FIFOCmd repl)
{
    switch (repl.opcode)
    {
    case FC_STATUS_REPL:
        printf("Auxiliary core is running.\n");
        break;
    default:
        printf("Core 0: unknown opcode from core 1: %d\n", (int)repl.opcode);
    }
}

void core0_process_serial_cmd()
{
    if (!strcmp(cmd_buf, "status"))
    {
        // TODO request status from core 1, then call with args later core0_status_print();
    }
    else if (!strcmp(cmd_buf, "reset"))
    {
        doReset();
        printf("Applied region, and resetted console.\n");
    }
    else if (!strcmp(cmd_buf, "us"))
    {
        printf("Selected US region\n");
        //sel_region = REGION_US; TODO
        //blinkPowerLed(sel_region + 1);
    }
    else if (!strcmp(cmd_buf, "eu"))
    {
        printf("Selected EU region\n");
        //sel_region = REGION_EU; TODO
        //blinkPowerLed(sel_region + 1);
    }
    else if (!strcmp(cmd_buf, "jp"))
    {
        printf("Selected JP region\n");
        //sel_region = REGION_JP; TODO
        //blinkPowerLed(sel_region + 1);
    }
    else if (!strcmp(cmd_buf, "reboot"))
    {
        printf("Rebooting the megaPALadin and the console.\n");
        // TODO sel_region = DEFAULT_REGION;
        // TODO doReset();
        // TODO rp2040.restart();
    }
    else if ((!strcmp(cmd_buf, "help")) || (!strcmp(cmd_buf, "?")))
    {
        printf("Available commands: status, reset, us, eu, jp, reboot, help, ?\n");
    }
    else
        printf("Unrecognized command.\nTry 'help' or '?'.\n");

    cmd_buf_len = 0;
}

void core0_status_print(enum Region curr_region, enum Region sel_region)
{
    printf("Current region: %s\n", regionToStr(curr_region));
    printf("Selected region: %s\n", regionToStr(sel_region));
    // TODO
    // printf("Heap mem: ", rp2040.getFreeHeap()," free, ", rp2040.getTotalHeap(), " total");
    printf("Core temperature: %2.1fC\n", temp_read());
}
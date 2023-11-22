#include "defs.h"
#include "support.h"

#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/watchdog.h>


char cmd_buf[CMD_BUF_SZ];
uint8_t cmd_buf_len;

bool usb_uart_ok = false;

void core1_main();
void core0_process_core1_cmd(const FIFOCmd repl);
void core0_process_serial_cmd();
void core0_status_print(enum Region curr_region, enum Region sel_region);

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
            tight_loop_contents();
        printf("\nmegaPALadin V%d\n", MP_VERSION);
    }

    // finishing touches
    temp_init();

    for(;;)
        tight_loop_contents(); // TODO setup bluetooth stuff

    return 0;
}

void core0_process_core1_cmd(const FIFOCmd repl)
{
    switch (repl.opcode)
    {
    case FC_STATUS_REPL:
        core0_status_print((enum Region)repl.data[0],(enum Region)repl.data[1]);
        break;
    default:
        printf("Core 0: unknown opcode from core 1: %d\n", (int)repl.opcode);
    }
}

void core0_process_serial_cmd()
{
    if (!strcmp(cmd_buf, "status"))
    {
        FIFOCmd c = {FC_STATUS_REQ, 0, 0, 0};
        multicore_fifo_push_blocking(fifo_pack(&c));
    }
    else if (!strcmp(cmd_buf, "reset"))
    {
        FIFOCmd c = {FC_RESET, 0, 0, 0};
        multicore_fifo_push_blocking(fifo_pack(&c));
        printf("Applied region, and resetted console.\n");
    }
    else if (!strcmp(cmd_buf, "us"))
    {
        FIFOCmd c = {FC_REGION_SELECT, REGION_US, 0, 0};
        multicore_fifo_push_blocking(fifo_pack(&c));
        printf("Selected US region\n");
    }
    else if (!strcmp(cmd_buf, "eu"))
    {
        FIFOCmd c = {FC_REGION_SELECT, REGION_EU, 0, 0};
        multicore_fifo_push_blocking(fifo_pack(&c));
        printf("Selected EU region\n");
    }
    else if (!strcmp(cmd_buf, "jp"))
    {
        FIFOCmd c = {FC_REGION_SELECT, REGION_JP, 0, 0};
        multicore_fifo_push_blocking(fifo_pack(&c));
        printf("Selected JP region\n");
    }
    else if (!strcmp(cmd_buf, "reboot"))
    {
        printf("Rebooting the megaPALadin and the console.\n");
        FIFOCmd c = {FC_REGION_SELECT, DEFAULT_REGION, 0, 0};
        multicore_fifo_push_blocking(fifo_pack(&c));
        c.opcode = FC_RESET;
        c.data[0] = 0;
        multicore_fifo_push_blocking(fifo_pack(&c));
        // wait for core 1 to reset, add some safety margin
        sleep_ms(RESET_DURATION_MS + (RESET_DURATION_MS/4));
        reboot();
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
    printf("Current region: %s\n", region_str(curr_region));
    printf("Selected region: %s\n", region_str(sel_region));
    // TODO port from arduino-pico
    // printf("Heap mem: ", rp2040.getFreeHeap()," free, ", rp2040.getTotalHeap(), " total");
    printf("Core temperature: %2.1fC\n", temp_read());
}
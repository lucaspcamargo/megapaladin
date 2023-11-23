#include "defs.h"
#include "support.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/flash.h>
#include <hardware/watchdog.h>

#include <btstack_run_loop.h>
#include <pico/cyw43_arch.h>
#include "sdkconfig.h"
#include "uni_init.h"
#include "uni_log.h"
#include "uni_platform.h"
#include "bt_platform.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif


btstack_timer_source_t main_timer;
char cmd_buf[CMD_BUF_SZ];
uint8_t cmd_buf_len;
bool usb_uart_ok = false;

void core0_btstack_timer(struct btstack_timer_source *);
void core0_process_core1_cmd(const FIFOCmd repl);
void core0_process_serial_cmd();
void core0_status_print(enum Region curr_region, enum Region sel_region);
void core1_preinit();
void core1_main();


int main() // for core 0
{
    // as soon as possible, get code in core 1 running (control)
    multicore_launch_core1(core1_main);
    flash_safe_execute_core_init();
    
    // now setup core 0
    cmd_buf_len = 0;

    // setup usb uart
    uint64_t serial_setup_start = time_us_64();
    stdio_init_all();
    while ((time_us_64() - serial_setup_start < SERIAL_INIT_TIMEOUT_US) && !stdio_usb_connected())
        tight_loop_contents();

    // finishing touches
    temp_init();

    // now, the wireless part
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    int bt_error = cyw43_arch_init();
    if(bt_error)
        return -1;

    // Turn-on LED. Turn it off once init is done.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(500);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(500);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(500);

    // Must be called before uni_init()
    uni_platform_set_custom(bt_platform_get());

    // Initialize BP32
    uni_init(0, NULL);

    // add application process in run loop as a timer
    memset(&main_timer, 0x00, sizeof(btstack_timer_source_t));
    main_timer.process = &core0_btstack_timer;
    btstack_run_loop_set_timer(&main_timer, CORE0_MAIN_TIMER_INTERVAL);
    btstack_run_loop_add_timer(&main_timer);

    // Does not return.
    btstack_run_loop_execute();
    
    return 0;
}


void core0_btstack_timer(struct btstack_timer_source *timer)
{
    static bool on = false;
    if(on)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        printf("ON\n");
    }
    else
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        printf("OFF\n");
    }
    on = !on;
    btstack_run_loop_set_timer(timer, CORE0_MAIN_TIMER_INTERVAL);
    btstack_run_loop_add_timer(timer);
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
    printf("\nmegaPALadin V%d\n", MP_VERSION);
    printf("Current region: %s\n", region_str(curr_region));
    printf("Selected region: %s\n", region_str(sel_region));
    struct mallinfo mi = mallinfo();
    printf("Heap mem: ", mi.fordblks," free, ", mi.arena, " total");
    printf("Core temperature: %2.1fC\n", temp_read());
}
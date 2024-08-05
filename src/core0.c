#include "defs.h"
#include "support.h"
#include "bt_platform.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/flash.h>
#include <hardware/watchdog.h>
#include "hardware/vreg.h"

#include <btstack_run_loop.h>
#include <pico/cyw43_arch.h>
#include "sdkconfig.h"
#include "uni.h"
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif


btstack_timer_source_t main_timer;
char cmd_buf[CMD_BUF_SZ];
uint8_t cmd_buf_len;
bool usb_uart_ok = false;
bool syncing = false;

void core0_btstack_timer(btstack_timer_source_t *);
void core0_process_core1_cmd(const FIFOCmd repl);
void core0_process_serial_cmd();
void core0_status_print(enum Region curr_region, enum Region sel_region);
void core1_preinit();
void core1_main();


int main() // for core 0
{
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    set_sys_clock_khz(PICO_FREQ_KHZ, false);

    core1_preinit();
    fifo_init();
    // as soon as possible, get code in core 1 running (control)
    if(!flash_safe_execute_core_init())
    {
        stdio_init_all();
        printf("Failed to intialize flash access control. Program halted.\n");
        for(;;)
            tight_loop_contents();
    }
    multicore_launch_core1(core1_main);
    
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

    // Must be called before uni_init()
    uni_platform_set_custom(bt_platform_get());

    // Initialize BP32
    uni_init(0, NULL);

    // add application callback to run loop
    memset(&main_timer, 0x00, sizeof(btstack_timer_source_t));
    main_timer.process = &core0_btstack_timer;
    btstack_run_loop_set_timer(&main_timer, CORE0_MAIN_TIMER_INTERVAL);
    btstack_run_loop_add_timer(&main_timer);

    // Does not return.
    btstack_run_loop_execute();
    
    return 0;
}


void core0_btstack_timer(btstack_timer_source_t *timer)
{
    // NOTE: whatever can be sent to core1 straight from bt_platform, we do it there for the lowest latency
    static bool initial_prompt = false;
    if(!initial_prompt)
    {
        putchar('>');
        putchar(' ');
        initial_prompt = true;
    }

    // process console commands
    int c;
    while((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT)
    {
        if(c == '\r' || c == '\n')
        {
            putchar('\r');
            putchar('\n');
            if(cmd_buf_len)
            {
                cmd_buf[cmd_buf_len] = '\0';
                core0_process_serial_cmd();
            }
            putchar('>');
            putchar(' ');
            cmd_buf_len = 0;
        }
        else if(cmd_buf_len == (CMD_BUF_SZ-1))
        {
            cmd_buf_len = 0;
        }
        else if(c > 0 && c < 256)
        {
            cmd_buf[cmd_buf_len] = (char) c;
            putchar((char)c);
            cmd_buf_len++;
        }
    }

    // process core1 commands
    FIFOCmd core1_cmd;
    while (fifo_pop(&core1_cmd))
        core0_process_core1_cmd(core1_cmd);

    // blink 
    static bool on = false;
    if(on)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }
    else
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
    on = !on;

    // reschedule
    btstack_run_loop_set_timer(timer, CORE0_MAIN_TIMER_INTERVAL);
    btstack_run_loop_add_timer(timer);
}

void core0_process_core1_cmd(const FIFOCmd repl)
{
    switch (repl.opcode)
    {
    case FC_STATUS_REPL:
        core0_status_print((enum Region)repl.data[0],(enum Region)repl.data[1]);
        printf("Port event counter at %d\n", (int)repl.data[2]);
        break;
    case FC_SYNC_START_REQ:
    {
        syncing = true;
        uni_bt_enable_new_connections_safe(syncing);
        FIFOCmd reply;
        reply.opcode = FC_SYNC_STATUS_REPL;
        reply.data[0] = 1;  // 1 means syncing
        fifo_push(&reply);
        break;
    }
    case FC_SYNC_STOP_REQ:
    {
        syncing = false;
        uni_bt_enable_new_connections_safe(syncing);
        FIFOCmd reply;
        reply.opcode = FC_SYNC_STATUS_REPL;
        reply.data[0] = 0;  // 0 means not syncing
        fifo_push(&reply);
        break;
    }
    case FC_LOG_NOTIFY:
    {
        char buf[MSG_LEN_MAX];
        if(fifo_str_pop(buf))
            printf("CORE1: %s\n", buf);
        break;
    }
    default:
        printf("Core 0: unknown opcode from core 1: %d\n", (int)repl.opcode);
    }
}

void core0_process_serial_cmd()
{
    if (!strcmp(cmd_buf, "status") || !strcmp(cmd_buf, "s"))
    {
        FIFOCmd c = {FC_STATUS_REQ, 0, 0, 0};
        fifo_push(&c);
    }
    else if (!strcmp(cmd_buf, "sync"))
    {
        syncing = !syncing;
        uni_bt_enable_new_connections_safe(syncing);
        FIFOCmd reply;
        reply.opcode = FC_SYNC_STATUS_REPL;
        reply.data[0] = syncing? 1 : 0;
        fifo_push(&reply);
        if(syncing)
            printf("Sync started.\n");
        else
            printf("Sync stopped.\n");
    }
    else if (!strcmp(cmd_buf, "reset"))
    {
        FIFOCmd c = {FC_RESET, 0, 0, 0};
        fifo_push(&c);
        printf("Applied region, and resetted console.\n");
    }
    else if (!strcmp(cmd_buf, "us"))
    {
        FIFOCmd c = {FC_REGION_SELECT, REGION_US, 0, 0};
        fifo_push(&c);
        printf("Selected US region\n");
    }
    else if (!strcmp(cmd_buf, "eu"))
    {
        FIFOCmd c = {FC_REGION_SELECT, REGION_EU, 0, 0};
        fifo_push(&c);
        printf("Selected EU region\n");
    }
    else if (!strcmp(cmd_buf, "jp"))
    {
        FIFOCmd c = {FC_REGION_SELECT, REGION_JP, 0, 0};
        fifo_push(&c);
        printf("Selected JP region\n");
    }
    else if (!strcmp(cmd_buf, "reboot"))
    {
        printf("Rebooting the megaPALadin and the console.\n");
        FIFOCmd c = {FC_REGION_SELECT, DEFAULT_REGION, 0, 0};
        fifo_push(&c);
        c.opcode = FC_RESET;
        c.data[0] = 0;
        fifo_push(&c);
        // wait for core 1 to reset, with some safety margin
        sleep_ms(RESET_DURATION_MS + (RESET_DURATION_MS/4));
        reboot();   // TODO this is panicking
    }
    else if ((!strcmp(cmd_buf, "help")) || (!strcmp(cmd_buf, "?")))
    {
        printf("Available commands: status, sync, reset, us, eu, jp, reboot, help, ?\n");
    }
    else
        printf("Unrecognized command.\nTry 'help' or '?'.\n");

    cmd_buf_len = 0;
}

void core0_status_print(enum Region curr_region, enum Region sel_region)
{
    printf("\nmegaPALadin V%d rev. %d\n", MP_VERSION, MP_REVISION);
    printf("Current region: %s\n", region_str(curr_region));
    printf("Selected region: %s\n", region_str(sel_region));
    struct mallinfo mi = mallinfo();
    printf("Heap mem: ", mi.fordblks," free, ", mi.arena, " total");
    printf("Core temperature: %2.1fC\n", temp_read());
    printf("Device dump:\n");
    uni_bt_dump_devices_safe();
    printf("\n> ");
}
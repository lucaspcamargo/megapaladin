// Example file - Public Domain
// Need help? https://tinyurl.com/bluepad32-help

#include <stdio.h>
#include <string.h>

#include <pico/cyw43_arch.h>

#include "sdkconfig.h"
#include "uni.h"
#include "defs.h"
#include "support.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif



// Defines
enum {
    TRIGGER_EFFECT_VIBRATION,
    TRIGGER_EFFECT_WEAPON,
    TRIGGER_EFFECT_FEEDBACK,
    TRIGGER_EFFECT_OFF,
    TRIGGER_EFFECT_COUNT,
};



// Data
static uint8_t bp_device_count;
static uni_hid_device_t *bp_devices[JOY_MAX];
static uint8_t bp_device_types[JOY_MAX];



// Helpers
static void _bp_trigger_event_on_gamepad(uni_hid_device_t* d);
static void _bp_device_debug_on_controller_data()
{
    /*
            // Debugging
            // Axis ry: control rumble
            if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
                d->report_parser.play_dual_rumble(d, 0, // delayed start ms
                                                    250, // duration ms
                                                    128, // weak magnitude 
                                                    0); // strong magnitude
            }
            // Buttons: Control LEDs On/Off
            if ((gp->buttons & BUTTON_B) && d->report_parser.set_player_leds != NULL) {
                d->report_parser.set_player_leds(d, leds++ & 0x0f);
            }
            // Axis: control RGB color
            if ((gp->buttons & BUTTON_X) && d->report_parser.set_lightbar_color != NULL) {
                uint8_t r = (gp->axis_x * 256) / 512;
                uint8_t g = (gp->axis_y * 256) / 512;
                uint8_t b = (gp->axis_rx * 256) / 512;
                d->report_parser.set_lightbar_color(d, r, g, b);
            }

            // Toggle Bluetooth connections
            if ((gp->buttons & BUTTON_SHOULDER_L) && enabled) {
                logi("*** Disabling Bluetooth connections\n");
                uni_bt_enable_new_connections_safe(false);
                enabled = false;
            }
            if ((gp->buttons & BUTTON_SHOULDER_R) && !enabled) {
                logi("*** Enabling Bluetooth connections\n");
                uni_bt_enable_new_connections_safe(true);
                enabled = true;
            }
            */
}

static void _bp_create_remappings()
{
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
}


void _bp_send_host_status(enum FIFOCmdFlags flags)
{
    FIFOCmd cmd;
    cmd.opcode = FC_JOY_HOST_STATUS;
    cmd.data[0] = flags;
    cmd.data[1] = bp_device_types[0] | bp_device_types[1] << 4;
#if (JOY_MAX == 4)
    cmd.data[2] = bp_device_types[2] | bp_device_types[3] << 4;
#elif (JOY_MAX == 2)
    cmd.data[2] = DEVICE_TYPE_NONE | DEVICE_TYPE_NONE << 4;
#else
#error "Weird JOY_MAX, cannot handle here"
#endif 

    fifo_push(&cmd);
}

void _bp_send_controller_data_joy(uni_hid_device_t* d, uni_controller_t* ctl, uint8_t idx) {
    uni_gamepad_t* gp = &ctl->gamepad;

    FIFOCmd cmd;
    cmd.opcode = FC_JOY_HOST_EVENT;
    cmd.data[0] = FC_F_JOY_EVENT_CURR | ((idx & 0xf) << 4);
    // TODO data
    fifo_push(&cmd);
}

void _bp_send_controller_data_mouse(uni_hid_device_t* d, uni_controller_t* ctl) {
    uni_mouse_t* m = &ctl->mouse;
}

uint8_t _bp_convert_controller_class_to_native(uni_controller_class_t type)
{
    switch(type)
    {
        case UNI_CONTROLLER_CLASS_NONE:
            return DEVICE_TYPE_NONE;
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            return DEVICE_TYPE_JOY;
        case UNI_CONTROLLER_CLASS_MOUSE:
            return DEVICE_TYPE_MOUSE;
        default:
            break;
    }
    return DEVICE_TYPE_UNKNOWN;
}

uint8_t _bp_get_index(uni_hid_device_t* d)
{
    for(int i = 0; i < JOY_MAX; i++)
        if(bp_devices[i] == d)
            return i;
    return 0xff;
}


//
// Platform Overrides
//
static void bt_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    // turn on led on init
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    logi("bt_platform: init()\n");

    bp_device_count = 0;
    memset(bp_devices, 0x00, sizeof(bp_devices));
    memset(bp_device_types, 0x00, sizeof(bp_device_types));

    // _bp_create_remappings();
}

static void bt_platform_on_init_complete(void) {
    logi("bt_platform: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Don't scan in the beginning scanning
    uni_bt_enable_new_connections_unsafe(false);

    // Based on runtime condition you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();

    // Turn off LED once init is done.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    uni_property_dump_all();
}

static void bt_platform_on_device_connected(uni_hid_device_t* d) {
    logi("bt_platform: device connected: %p\n", d);

    // we use on_device_ready to determine whether to actually keep the device
    // so, we do nothing here for now
}

static void bt_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("bt_platform: device disconnected: %p\n", d);

    for(int i = 0; i < JOY_MAX; i++)
        if(bp_devices[i] == d)
        {
            bp_devices[i] = NULL;
            bp_device_count--;

            _bp_send_host_status(FC_F_JOY_STATUS_DISCONNECTED);

            break;
        }
}

static uni_error_t bt_platform_on_device_ready(uni_hid_device_t* d) {
    logi("bt_platform: device ready: %p\n", d);


    if(bp_device_count >= JOY_MAX)
        return UNI_ERROR_NO_SLOTS;

    if(d->controller_type == k_eControllerType_Unknown || d->controller_type == k_eControllerType_None)
        return UNI_ERROR_INVALID_CONTROLLER;

    uint8_t native_type = _bp_convert_controller_class_to_native(d->controller.klass);
    if(native_type == DEVICE_TYPE_NONE || native_type == DEVICE_TYPE_UNKNOWN)
        return UNI_ERROR_INVALID_CONTROLLER;

    for(int i = 0; i < JOY_MAX; i++)
        if(!bp_devices[i])
        {
            bp_devices[i] = d;
            bp_device_types[i] = native_type;
            bp_device_count++;

            _bp_send_host_status(FC_F_JOY_STATUS_CONNECTED);

            return UNI_ERROR_SUCCESS;
        }
        
    return UNI_ERROR_NO_SLOTS;
}

static void bt_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static uint8_t leds = 0;
    static uint8_t enabled = true;
    static uni_controller_t prev = {0};
    uni_gamepad_t* gp;

    if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
        return;
    }
    prev = *ctl;
    // Print device Id before dumping gamepad.
    logi("(%p) ", d);
    uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
        {
            uint8_t idx = _bp_get_index(d);
            if(idx != 0xff)
                _bp_send_controller_data_joy(d, ctl, idx);
        }
        break;
        case UNI_CONTROLLER_CLASS_BALANCE_BOARD:
            // Do something
            uni_balance_board_dump(&ctl->balance_board);
            break;
        case UNI_CONTROLLER_CLASS_MOUSE:
            // Do something
            uni_mouse_dump(&ctl->mouse);
            _bp_send_controller_data_mouse(d, ctl);
            break;
        case UNI_CONTROLLER_CLASS_KEYBOARD:
            // Do something
            uni_keyboard_dump(&ctl->keyboard);
            break;
        default:
            loge("Unsupported controller class: %d\n", ctl->klass);
            break;
    }
}

static const uni_property_t* bt_platform_get_property(uni_property_idx_t key) {
    // Deprecated
    ARG_UNUSED(key);
    return NULL;
}

static void bt_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
            // Optional: do something when "system" button gets pressed.
            _bp_trigger_event_on_gamepad((uni_hid_device_t*)data);
            break;

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            // When the "bt scanning" is on / off. Could by triggered by different events
            // Useful to notify the user
            logi("bt_platform_on_oob_event: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("bt_platform_on_oob_event: unsupported event: 0x%04x\n", event);
    }
}

//
// Helpers
//
static void _bp_trigger_event_on_gamepad(uni_hid_device_t* d) {
    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 50 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        static uint8_t led = 0;
        led += 1;
        led &= 0xf;
        d->report_parser.set_player_leds(d, led);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        static uint8_t red = 0x10;
        static uint8_t green = 0x20;
        static uint8_t blue = 0x40;

        red += 0x10;
        green -= 0x20;
        blue += 0x40;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

//
// Entry Point
//
struct uni_platform* bt_platform_get(void) {
    static struct uni_platform plat = {
        .name = "megapaladin",
        .init = bt_platform_init,
        .on_init_complete = bt_platform_on_init_complete,
        .on_device_connected = bt_platform_on_device_connected,
        .on_device_disconnected = bt_platform_on_device_disconnected,
        .on_device_ready = bt_platform_on_device_ready,
        .on_oob_event = bt_platform_on_oob_event,
        .on_controller_data = bt_platform_on_controller_data,
        .get_property = bt_platform_get_property,
    };

    return &plat;
}
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "srctusbconfig.h"

// ===================== CONFIGURATION =====================
#define LEFT_TRIGGER_THRESHOLD  50    // 0..255
#define DEFAULT_FREQ_HZ         12.0f
#define DEFAULT_INTENSITY       50.0f

#define BTN_FREQ_UP             6
#define BTN_FREQ_DOWN           7
#define BTN_INTENSITY_UP        8
#define BTN_INTENSITY_DOWN      9

#define XINPUT_REPORT_SIZE      20

#define XINPUT_OFFSET_BUTTONS_LOW   1
#define XINPUT_OFFSET_BUTTONS_HIGH  2
#define XINPUT_OFFSET_LEFT_TRIGGER  3
#define XINPUT_OFFSET_RIGHT_TRIGGER 4
#define XINPUT_OFFSET_LEFT_X        5
#define XINPUT_OFFSET_LEFT_Y        7
#define XINPUT_OFFSET_RIGHT_X       9
#define XINPUT_OFFSET_RIGHT_Y       11

static float freq_hz = DEFAULT_FREQ_HZ;
static float intensity = DEFAULT_INTENSITY;
static bool auto_whammy_enabled = false;
static bool last_left_trigger_pressed = false;

static absolute_time_t next_toggle_time;
static bool whammy_output_state = false;

static uint8_t guitar_report[XINPUT_REPORT_SIZE] = {0};
static uint8_t device_report[XINPUT_REPORT_SIZE] = {0};
static volatile bool guitar_report_ready = false;

static void copy_guitar_to_device() {
    if (guitar_report_ready) {
        memcpy(device_report, guitar_report, XINPUT_REPORT_SIZE);
        guitar_report_ready = false;
    }
}

static inline void set_right_trigger(uint8_t value) {
    device_report[XINPUT_OFFSET_RIGHT_TRIGGER] = value;
}

static inline uint8_t get_guitar_right_trigger() {
    return guitar_report[XINPUT_OFFSET_RIGHT_TRIGGER];
}

static void toggle_auto_whammy() {
    auto_whammy_enabled = !auto_whammy_enabled;
    if (!auto_whammy_enabled) {
        if (guitar_report_ready) {
            set_right_trigger(get_guitar_right_trigger());
        } else {
            set_right_trigger(0);
        }
        whammy_output_state = false;
    } else {
        next_toggle_time = make_timeout_time_ms((int)(1000.0f / freq_hz / 2));
        whammy_output_state = true;
        set_right_trigger(255);
    }
}

static void update_whammy() {
    if (!auto_whammy_enabled) return;

    if (time_reached(next_toggle_time)) {
        float period_ms = 1000.0f / freq_hz;
        float hold_ms = period_ms * (intensity / 100.0f);
        float release_ms = period_ms - hold_ms;

        if (whammy_output_state) {
            if (intensity < 100.0f) {
                whammy_output_state = false;
                set_right_trigger(0);
                next_toggle_time = make_timeout_time_ms((int)(release_ms));
            } else {
                next_toggle_time = make_timeout_time_ms((int)(period_ms));
            }
        } else {
            if (intensity > 0.0f) {
                whammy_output_state = true;
                set_right_trigger(255);
                next_toggle_time = make_timeout_time_ms((int)(hold_ms));
            } else {
                next_toggle_time = make_timeout_time_ms((int)(period_ms));
            }
        }
    }
}

static void process_left_trigger() {
    uint8_t left_val = guitar_report[XINPUT_OFFSET_LEFT_TRIGGER];
    bool pressed = left_val > LEFT_TRIGGER_THRESHOLD;

    if (pressed && !last_left_trigger_pressed) {
        toggle_auto_whammy();
    }
    last_left_trigger_pressed = pressed;
}

void tuh_hid_mounted_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    printf("Guitar HID mounted\n");
    tuh_hid_set_protocol(dev_addr, instance, 1);
    tuh_hid_get_report(dev_addr, instance, 0, HID_REPORT_TYPE_INPUT, NULL, 0);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    if (len == XINPUT_REPORT_SIZE) {
        memcpy(guitar_report, report, XINPUT_REPORT_SIZE);
        guitar_report_ready = true;
        process_left_trigger();
    }
}

static const uint8_t desc_hid_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x10,        //   Usage Maximum (16)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // Logical Maximum (65535)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0xC0,              // End Collection
};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    uint16_t len = XINPUT_REPORT_SIZE;
    if (reqlen < len) len = reqlen;
    memcpy(buffer, device_report, len);
    return len;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {}

int main() {
    stdio_init_all();

    gpio_init(BTN_FREQ_UP); gpio_set_dir(BTN_FREQ_UP, GPIO_IN); gpio_pull_up(BTN_FREQ_UP);
    gpio_init(BTN_FREQ_DOWN); gpio_set_dir(BTN_FREQ_DOWN, GPIO_IN); gpio_pull_up(BTN_FREQ_DOWN);
    gpio_init(BTN_INTENSITY_UP); gpio_set_dir(BTN_INTENSITY_UP, GPIO_IN); gpio_pull_up(BTN_INTENSITY_UP);
    gpio_init(BTN_INTENSITY_DOWN); gpio_set_dir(BTN_INTENSITY_DOWN, GPIO_IN); gpio_pull_up(BTN_INTENSITY_DOWN);

    tusb_init();
    memset(device_report, 0, XINPUT_REPORT_SIZE);
    auto_whammy_enabled = false;
    whammy_output_state = false;

    while (true) {
        tuh_task();
        tud_task();

        if (guitar_report_ready) {
            memcpy(device_report, guitar_report, XINPUT_REPORT_SIZE);
            process_left_trigger();
            guitar_report_ready = false;
        }

        update_whammy();

        if (!gpio_get(BTN_FREQ_UP)) {
            freq_hz = min(30.0f, freq_hz + 0.5f);
            while (!gpio_get(BTN_FREQ_UP)) tight_loop_contents();
            next_toggle_time = make_timeout_time_ms(0);
        }
        if (!gpio_get(BTN_FREQ_DOWN)) {
            freq_hz = max(1.0f, freq_hz - 0.5f);
            while (!gpio_get(BTN_FREQ_DOWN)) tight_loop_contents();
            next_toggle_time = make_timeout_time_ms(0);
        }
        if (!gpio_get(BTN_INTENSITY_UP)) {
            intensity = min(100.0f, intensity + 1.0f);
            while (!gpio_get(BTN_INTENSITY_UP)) tight_loop_contents();
            next_toggle_time = make_timeout_time_ms(0);
        }
        if (!gpio_get(BTN_INTENSITY_DOWN)) {
            intensity = max(0.0f, intensity - 1.0f);
            while (!gpio_get(BTN_INTENSITY_DOWN)) tight_loop_contents();
            next_toggle_time = make_timeout_time_ms(0);
        }

        sleep_ms(10);
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {}
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* report, uint16_t len) {}
void tuh_hid_set_protocol_cb(uint8_t dev_addr, uint8_t instance, uint8_t protocol) {}

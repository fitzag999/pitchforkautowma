/*
 * Auto-Whammy for CRKD Guitar (XInput) — Adafruit Feather RP2040 USB Host
 * Simplified version – no extra TinyUSB calls.
 */

#include <stdio.h>
#include <string.h>
#include <algorithm>        // for std::min, std::max
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "tusb.h"

// Your header is named srctusbconfig.h – include it directly.
#include "srctusbconfig.h"

// ===================== CONFIGURATION =====================
#define LEFT_TRIGGER_THRESHOLD  50      // 0..255
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

// ===================== GLOBALS =====================
static float freq_hz  = DEFAULT_FREQ_HZ;
static float intensity = DEFAULT_INTENSITY;
static bool auto_whammy_enabled = false;
static bool last_left_trigger_pressed = false;

static absolute_time_t next_toggle_time;
static bool whammy_output_state = false;

static uint8_t guitar_report[XINPUT_REPORT_SIZE] = {0};
static uint8_t device_report[XINPUT_REPORT_SIZE] = {0};
static volatile bool guitar_report_ready = false;

// ===================== HELPERS =====================

static inline void set_right_trigger(uint8_t value) {
    device_report[XINPUT_OFFSET_RIGHT_TRIGGER] = value;
}

static void toggle_auto_whammy() {
    auto_whammy_enabled = !auto_whammy_enabled;
    if (!auto_whammy_enabled) {
        set_right_trigger(0);
        whammy_output_state = false;
    } else {
        whammy_output_state = true;
        set_right_trigger(255);
        next_toggle_time = make_timeout_time_ms((uint32_t)(500.0f / freq_hz));
    }
}

static void update_whammy() {
    if (!auto_whammy_enabled) return;
    if (!time_reached(next_toggle_time)) return;

    float period_ms  = 1000.0f / freq_hz;
    float hold_ms    = period_ms * (intensity / 100.0f);
    float release_ms = period_ms - hold_ms;

    if (whammy_output_state) {
        if (intensity < 100.0f) {
            whammy_output_state = false;
            set_right_trigger(0);
            next_toggle_time = make_timeout_time_ms((uint32_t)release_ms);
        } else {
            next_toggle_time = make_timeout_time_ms((uint32_t)period_ms);
        }
    } else {
        if (intensity > 0.0f) {
            whammy_output_state = true;
            set_right_trigger(255);
            next_toggle_time = make_timeout_time_ms((uint32_t)hold_ms);
        } else {
            next_toggle_time = make_timeout_time_ms((uint32_t)period_ms);
        }
    }
}

static void process_left_trigger() {
    uint8_t left_val = guitar_report[XINPUT_OFFSET_LEFT_TRIGGER];
    bool pressed = (left_val > LEFT_TRIGGER_THRESHOLD);
    if (pressed && !last_left_trigger_pressed) {
        toggle_auto_whammy();
    }
    last_left_trigger_pressed = pressed;
}

static bool button_just_pressed(uint8_t pin) {
    if (!gpio_get(pin)) {
        sleep_ms(20);
        return !gpio_get(pin);
    }
    return false;
}

// ===================== USB HOST CALLBACKS =====================

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report; (void)desc_len;
    printf("Guitar HID mounted\n");
    // Request a report – this will cause the device to send.
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr; (void)instance;
    printf("Guitar HID unmounted\n");
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                 uint8_t const* report, uint16_t len) {
    if (len >= XINPUT_REPORT_SIZE) {
        memcpy(guitar_report, report, XINPUT_REPORT_SIZE);
        guitar_report_ready = true;
    }
    // Re-queue next report
    tuh_hid_receive_report(dev_addr, instance);
}

// ===================== USB DEVICE CALLBACKS =====================

// The report descriptor (same as before)
static const uint8_t desc_hid_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Gamepad)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x10,        //   Usage Maximum (16)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x16, 0x00, 0x80,  //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    0xC0               // End Collection
};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type;
    uint16_t len = (reqlen < XINPUT_REPORT_SIZE) ? reqlen : XINPUT_REPORT_SIZE;
    memcpy(buffer, device_report, len);
    return len;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)bufsize;
}

// ===================== MAIN =====================

int main() {
    stdio_init_all();

    const uint8_t btns[] = {BTN_FREQ_UP, BTN_FREQ_DOWN,
                            BTN_INTENSITY_UP, BTN_INTENSITY_DOWN};
    for (auto pin : btns) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    tusb_init();
    memset(device_report, 0, XINPUT_REPORT_SIZE);

    while (true) {
        tuh_task();
        tud_task();

        if (guitar_report_ready) {
            guitar_report_ready = false;
            memcpy(device_report, guitar_report, XINPUT_REPORT_SIZE);
            process_left_trigger();
        }

        update_whammy();

        static bool btn_freq_up_prev = false;
        static bool btn_freq_dn_prev = false;
        static bool btn_int_up_prev  = false;
        static bool btn_int_dn_prev  = false;

        bool freq_up  = button_just_pressed(BTN_FREQ_UP);
        bool freq_dn  = button_just_pressed(BTN_FREQ_DOWN);
        bool int_up   = button_just_pressed(BTN_INTENSITY_UP);
        bool int_dn   = button_just_pressed(BTN_INTENSITY_DOWN);

        if (freq_up && !btn_freq_up_prev) {
            freq_hz = std::min(30.0f, freq_hz + 0.5f);
            next_toggle_time = get_absolute_time();
        }
        if (freq_dn && !btn_freq_dn_prev) {
            freq_hz = std::max(1.0f, freq_hz - 0.5f);
            next_toggle_time = get_absolute_time();
        }
        if (int_up && !btn_int_up_prev) {
            intensity = std::min(100.0f, intensity + 1.0f);
            next_toggle_time = get_absolute_time();
        }
        if (int_dn && !btn_int_dn_prev) {
            intensity = std::max(0.0f, intensity - 1.0f);
            next_toggle_time = get_absolute_time();
        }

        btn_freq_up_prev = freq_up;
        btn_freq_dn_prev = freq_dn;
        btn_int_up_prev  = int_up;
        btn_int_dn_prev  = int_dn;

        if (tud_hid_ready()) {
            tud_hid_report(0, device_report, XINPUT_REPORT_SIZE);
        }
    }
}

// Stubs for TinyUSB callbacks we didn't implement
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance,
                                     uint8_t report_id, hid_report_type_t report_type,
                                     uint8_t const* report, uint16_t len) {}
void tuh_hid_set_protocol_cb(uint8_t dev_addr, uint8_t instance, uint8_t protocol) {}

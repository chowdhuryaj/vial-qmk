/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "oled_display.h"
#include "per_layer_rgb.h"
#include "shared/num_word.h"
#include "shared/sentence_case.h"
#include "i2c_master.h"

/* ---------------------------------------------------------------------------
 * I2C bus recovery (hardware-observed 2026-07-06): the panel freezes on its
 * last frame within ~1 s of boot — one transient NACK/timeout leaves the
 * F103 I2C peripheral wedged and every later transfer fails. The oled
 * driver's transmit functions are weak (drivers/oled/oled_driver.c:191/238);
 * these overrides add a bus re-init + one retry on failure, with counters
 * exposed over the display HID channel (0x04/0x05) for diagnosis.
 * ------------------------------------------------------------------------- */
#ifndef OLED_DISPLAY_ADDRESS
#    define OLED_DISPLAY_ADDRESS 0x3C
#endif
#ifndef OLED_I2C_TIMEOUT
#    define OLED_I2C_TIMEOUT 100
#endif
#define NLK_I2C_DATA 0x40

static volatile uint16_t i2c_fail_count    = 0;
static volatile uint16_t i2c_recover_count = 0;
// After this many consecutive unrecovered failures the panel is considered
// dead and further transfers are skipped — a fully-failing bus otherwise
// costs 2 timeouts per block per frame and starves the main loop (USB went
// unresponsive on hardware before this cap existed).
#define NLK_I2C_MAX_CONSECUTIVE_FAILS 10
static volatile uint8_t i2c_consecutive = 0;

static bool nlk_i2c_gate(void) {
    return i2c_consecutive < NLK_I2C_MAX_CONSECUTIVE_FAILS;
}

static bool nlk_i2c_result(i2c_status_t st) {
    if (st == I2C_STATUS_SUCCESS) {
        i2c_consecutive = 0;
        i2c_recover_count++; // counts successes here (diagnostic repurpose)
        return true;
    }
    i2c_fail_count++;
    i2c_consecutive++;
    return false;
}

// NO i2c_init() in the failure path: re-initializing a locked ChibiOS I2C
// driver mid-error corrupted its state machine and hard-hung the main loop
// (hardware-observed round 7, 2026-07-06). Failures just count and gate.
bool oled_send_cmd(const uint8_t *data, uint16_t size) {
    if (!nlk_i2c_gate()) return false;
    return nlk_i2c_result(i2c_transmit((OLED_DISPLAY_ADDRESS << 1), data, size, OLED_I2C_TIMEOUT));
}

bool oled_send_data(const uint8_t *data, uint16_t size) {
    if (!nlk_i2c_gate()) return false;
    return nlk_i2c_result(i2c_write_register((OLED_DISPLAY_ADDRESS << 1), NLK_I2C_DATA, data, size, OLED_I2C_TIMEOUT));
}

/* I2C address scan (display channel 0x06): probes 7-bit addresses 0x08-0x77
 * with an empty write and records who ACKs. Run on demand from the host —
 * answers "is anything alive on the bus, and at which address". */
static volatile uint8_t scan_first_ack = 0;
static volatile uint8_t scan_ack_count = 0;

void oled_display_i2c_scan(void) {
    scan_first_ack = 0;
    scan_ack_count = 0;
    // 1-byte probe: zero-length transfers are illegal on the F1 LLD and
    // wedged the driver when tried (2026-07-06). 0x00 = a harmless SSD/SH
    // control byte for anything OLED-shaped.
    uint8_t probe = 0x00;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_transmit(addr << 1, &probe, 1, 5) == I2C_STATUS_SUCCESS) {
            if (!scan_first_ack) scan_first_ack = addr;
            scan_ack_count++;
        }
    }
}

uint16_t oled_display_i2c_scan_result(void) {
    return ((uint16_t)scan_ack_count << 8) | scan_first_ack;
}

uint16_t oled_display_i2c_fails(void) {
    return i2c_fail_count;
}

uint16_t oled_display_i2c_recovers(void) {
    return i2c_recover_count;
}

// Written from the raw-HID handler (USB task context), read from the main
// loop's oled_task — volatile so LTO can't cache the flags across contexts.
static char              push_buf[NLK_DISPLAY_LINES][NLK_DISPLAY_COLS + 1];
static volatile bool     pushed    = false;
static volatile uint32_t last_push = 0;
static volatile uint16_t hold_ms   = NLK_DISPLAY_HOLD_MS_DEFAULT;

void oled_display_push_line(uint8_t line, const uint8_t *text, uint8_t len) {
    if (line >= NLK_DISPLAY_LINES) return;
    if (len > NLK_DISPLAY_COLS) len = NLK_DISPLAY_COLS;
    // Stored space-padded to the full width. render_pushed must NOT call
    // oled_advance_page: a full-width write already wraps the cursor to the
    // next line, so advance_page would blank the FOLLOWING line, which the
    // next pass then rewrites — every block ping-pongs dirty and the driver
    // re-transmits nonstop (hardware-measured 218 tx/s vs 2 tx/s idle).
    for (uint8_t i = 0; i < NLK_DISPLAY_COLS; i++) {
        char c            = (i < len) ? (char)text[i] : ' ';
        push_buf[line][i] = (c >= 32 && c < 127) ? c : ' ';
    }
    push_buf[line][NLK_DISPLAY_COLS] = '\0';
    pushed                           = true;
    last_push                        = timer_read32();
}

void oled_display_release(void) {
    pushed = false;
}

bool oled_display_pushed_active(void) {
    return pushed && timer_elapsed32(last_push) < hold_ms;
}

uint16_t oled_display_seconds_since_push(void) {
    if (!pushed) return 0xFFFF;
    uint32_t s = timer_elapsed32(last_push) / 1000;
    return (s > 0xFFFE) ? 0xFFFE : (uint16_t)s;
}

void oled_display_set_hold_ms(uint16_t ms) {
    if (ms < NLK_DISPLAY_HOLD_MS_MIN) ms = NLK_DISPLAY_HOLD_MS_MIN;
    if (ms > NLK_DISPLAY_HOLD_MS_MAX) ms = NLK_DISPLAY_HOLD_MS_MAX;
    hold_ms = ms;
}

uint16_t oled_display_get_hold_ms(void) {
    return hold_ms;
}

static volatile uint16_t sleep_s = NLK_DISPLAY_SLEEP_S_DEFAULT;

void oled_display_set_sleep_s(uint16_t s) {
    if (s > NLK_DISPLAY_SLEEP_S_MAX) s = NLK_DISPLAY_SLEEP_S_MAX;
    sleep_s = s;
}

uint16_t oled_display_get_sleep_s(void) {
    return sleep_s;
}

static void render_pushed(void) {
    for (uint8_t l = 0; l < NLK_DISPLAY_LINES; l++) {
        oled_set_cursor(0, l);
        oled_write(push_buf[l], false);
        // Clear the remainder ONLY for short lines (never-pushed slots are
        // empty strings). A full-width line has already wrapped the cursor —
        // advance_page there would blank the NEXT line (see push_line).
        if (strlen(push_buf[l]) < NLK_DISPLAY_COLS) {
            oled_advance_page(true);
        }
    }
}

/* Fallback-screen widgets (v3): one per visible line, assigned over HID and
 * persisted by the keymap. Table initializer reproduces the pre-v3 screen. */
static uint8_t widgets[NLK_DISPLAY_VISIBLE_LINES] = NLK_DISPLAY_WIDGET_DEFAULTS;

void oled_display_set_widget(uint8_t line, uint8_t widget) {
    if (line >= NLK_DISPLAY_VISIBLE_LINES) return;
    if (widget >= NLK_WIDGET_COUNT) widget = NLK_WIDGET_COUNT - 1;
    widgets[line] = widget;
}

uint8_t oled_display_get_widget(uint8_t line) {
    return (line < NLK_DISPLAY_VISIBLE_LINES) ? widgets[line] : NLK_WIDGET_BLANK;
}

uint8_t *oled_display_widget_table(void) {
    return widgets;
}

// Draws one widget at the current cursor, ≤5 chars, then clears the rest of
// the line. Widgets never reach canvas column 10, so the advance_page can't
// hit the full-width wrap trap render_pushed dodges.
static void render_widget_line(uint8_t widget) {
    switch (widget) {
        case NLK_WIDGET_LAYER:
            oled_write_P(PSTR("LYR "), false);
            oled_write_char('0' + get_highest_layer(layer_state | default_layer_state), true);
            break;
        case NLK_WIDGET_UPTIME: {
            uint32_t secs = timer_read32() / 1000;
            oled_write_P(PSTR("T "), false);
            oled_write_char('0' + (secs / 100) % 10, false);
            oled_write_char('0' + (secs / 10) % 10, false);
            oled_write_char('0' + secs % 10, false);
            break;
        }
        case NLK_WIDGET_MODS: {
            uint8_t mods = get_mods() | get_weak_mods();
            oled_write_char('C', mods & MOD_MASK_CTRL);
            oled_write_char('S', mods & MOD_MASK_SHIFT);
            oled_write_char('A', mods & MOD_MASK_ALT);
            oled_write_char('G', mods & MOD_MASK_GUI);
            break;
        }
        case NLK_WIDGET_OSM: {
            uint8_t mods = get_oneshot_mods() | get_oneshot_locked_mods();
            oled_write_char('o', false);
            oled_write_char('C', mods & MOD_MASK_CTRL);
            oled_write_char('S', mods & MOD_MASK_SHIFT);
            oled_write_char('A', mods & MOD_MASK_ALT);
            oled_write_char('G', mods & MOD_MASK_GUI);
            break;
        }
        case NLK_WIDGET_OSL: {
            bool active = is_oneshot_layer_active();
            oled_write_P(PSTR("OSL "), false);
            oled_write_char(active ? (char)('0' + get_oneshot_layer()) : '-', active);
            break;
        }
        case NLK_WIDGET_LOCKS: {
            led_t leds = host_keyboard_led_state();
            oled_write_char('C', leds.caps_lock);
            oled_write_char(' ', false);
            oled_write_char('N', leds.num_lock);
            oled_write_char(' ', false);
            oled_write_char('S', leds.scroll_lock);
            break;
        }
        case NLK_WIDGET_CAPS:
            oled_write_P(PSTR("CAP"), host_keyboard_led_state().caps_lock);
            break;
        case NLK_WIDGET_NUMLOCK:
            oled_write_P(PSTR("NLK"), host_keyboard_led_state().num_lock);
            break;
        case NLK_WIDGET_SCROLLLOCK:
            oled_write_P(PSTR("SLK"), host_keyboard_led_state().scroll_lock);
            break;
        case NLK_WIDGET_RGBMAP:
            oled_write_P(PSTR("MAP"), per_layer_rgb_get_enabled());
            break;
        case NLK_WIDGET_NUMWORD:
            oled_write_P(PSTR("NUM"), num_word_is_active());
            break;
        case NLK_WIDGET_SENTENCE:
            oled_write_P(PSTR("SC"), is_sentence_case_on());
            break;
        case NLK_WIDGET_BLANK:
        default:
            break;
    }
    oled_advance_page(true);
}

// Portrait canvas 10 x 16 (SSD1306 128x64 + ROTATION_90), but the GLASS is a
// 64x32 window: lines 4..11, text columns 0..4 — see NLK_DISPLAY_VISIBLE_* in
// keyboards/nlofin/nlkb16_02/config.h. Every widget reads within 5 characters;
// the off-glass lines are kept blank.
static void render_fallback(void) {
    for (uint8_t l = 0; l < NLK_DISPLAY_VISIBLE_FIRST; l++) {
        oled_set_cursor(0, l);
        oled_write_ln_P(PSTR(""), false);
    }
    for (uint8_t i = 0; i < NLK_DISPLAY_VISIBLE_LINES; i++) {
        oled_set_cursor(0, NLK_DISPLAY_VISIBLE_FIRST + i);
        render_widget_line(widgets[i]);
    }
    for (uint8_t l = NLK_DISPLAY_VISIBLE_FIRST + NLK_DISPLAY_VISIBLE_LINES; l < NLK_DISPLAY_LINES; l++) {
        oled_set_cursor(0, l);
        oled_write_ln_P(PSTR(""), false);
    }
}

/* Panel-probe hooks (2026-07-06, freeze diagnosis): the panel ACKs every
 * transfer but stops applying RAM writes to the glass within ~2 s of boot.
 * These let the host re-run oled_init() and inject arbitrary command bytes
 * to find the un-wedge/wedge triggers without a reflash per experiment. */
static volatile bool     reinit_req    = false;
static volatile uint8_t  raw_cmd[8];
static volatile uint8_t  raw_cmd_len   = 0;
static volatile uint16_t raw_cmd_state = 0xFFFF; // 0xFFFF none, 1 ACK, 0 fail

void oled_display_queue_raw_cmd(const uint8_t *bytes, uint8_t len) {
    if (len == 0 || len > sizeof(raw_cmd)) return;
    for (uint8_t i = 0; i < len; i++) {
        raw_cmd[i] = bytes[i];
    }
    raw_cmd_len = len;
}

uint16_t oled_display_raw_cmd_result(void) {
    return raw_cmd_state;
}

void oled_display_request_reinit(void) {
    reinit_req = true;
}

bool oled_display_task(void) {
    if (reinit_req) {
        reinit_req = false;
        oled_init(OLED_ROTATION_90); // clears + dirties all; content repaints below
    }
    if (raw_cmd_len) {
        uint8_t frame[1 + sizeof(raw_cmd)];
        frame[0] = 0x00; // control byte: command stream
        for (uint8_t i = 0; i < raw_cmd_len; i++) {
            frame[1 + i] = raw_cmd[i];
        }
        raw_cmd_state = oled_send_cmd(frame, raw_cmd_len + 1) ? 1 : 0;
        raw_cmd_len   = 0;
    }
    if (oled_display_pushed_active()) {
        render_pushed();
    } else if (sleep_s != 0 && last_input_activity_elapsed() > (uint32_t)sleep_s * 1000) {
        // Idle: stop drawing BEFORE switching off — any dirty block makes
        // oled_render_dirty re-assert oled_on, which is exactly how the
        // ticking uptime widget kept the panel lit forever.
        oled_off();
    } else {
        render_fallback();
    }
    return false;
}

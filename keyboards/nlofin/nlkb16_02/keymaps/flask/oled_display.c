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
static char              push_buf[NLK_DISPLAY_BIG_LINES][NLK_DISPLAY_BIG_COLS + 1];
static volatile bool     pushed    = false;
static volatile uint32_t last_push = 0;
static volatile uint16_t hold_ms   = NLK_DISPLAY_HOLD_MS_DEFAULT;

void oled_display_push_line(uint8_t line, const uint8_t *text, uint8_t len) {
    if (line >= NLK_DISPLAY_BIG_LINES) return;
    if (len > NLK_DISPLAY_BIG_COLS) len = NLK_DISPLAY_BIG_COLS;
    for (uint8_t i = 0; i < NLK_DISPLAY_BIG_COLS; i++) {
        char c            = (i < len) ? (char)text[i] : ' ';
        push_buf[line][i] = (c >= 32 && c < 127) ? c : ' ';
    }
    push_buf[line][NLK_DISPLAY_BIG_COLS] = '\0';
    pushed                               = true;
    last_push                            = timer_read32();
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


/* ---------------------------------------------------------------------------
 * Big-line renderer (v5): 4 lines × 5 chars, glyphs pixel-doubled vertically
 * (6×8 font → 6×16). We include our own copy of the oled font — the driver's
 * `font[]` is static to oled_driver.c (costs ~1.6 KB flash, fine on 128K).
 * Coordinates are LOGICAL post-rotation: the glass window is x 0..31,
 * y 32..95 (visible lines 4-11 of the 16-line canvas) — big line i sits at
 * y = 32 + i*16, column c at x = c*6.
 *
 * oled_write_pixel only dirties changed pixels, but the per-pixel loop
 * itself costs ~0.25 ms/line — a per-line content cache skips unchanged
 * lines so a steady screen costs nothing.
 * ------------------------------------------------------------------------- */
#include "glcdfont.c" // static const unsigned char font[] PROGMEM, 6 B/char

static char    line_cache[NLK_DISPLAY_BIG_LINES][NLK_DISPLAY_BIG_COLS];
static uint8_t mask_cache[NLK_DISPLAY_BIG_LINES];
static bool    cache_valid = false;

static void draw_big_char(uint8_t line, uint8_t col, char c, bool invert) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = &font[(uint16_t)c * 6];
    uint8_t        x0    = col * 6;
    uint8_t        y0    = NLK_DISPLAY_VISIBLE_FIRST * 8 + line * 16;
    for (uint8_t gx = 0; gx < 6; gx++) {
        uint8_t bits = pgm_read_byte(glyph + gx);
        for (uint8_t gy = 0; gy < 8; gy++) {
            bool on = ((bits >> gy) & 1) != 0;
            if (invert) on = !on;
            oled_write_pixel(x0 + gx, y0 + gy * 2, on);
            oled_write_pixel(x0 + gx, y0 + gy * 2 + 1, on);
        }
    }
}

// text = up to 5 chars (shorter is space-padded); invert_mask bit c inverts
// column c's glyph.
static void render_big_line(uint8_t line, const char *text, uint8_t invert_mask) {
    char padded[NLK_DISPLAY_BIG_COLS];
    bool ended = false;
    for (uint8_t c = 0; c < NLK_DISPLAY_BIG_COLS; c++) {
        char ch = ended ? ' ' : text[c];
        if (ch == '\0') {
            ended = true;
            ch    = ' ';
        }
        padded[c] = ch;
    }
    if (cache_valid && mask_cache[line] == invert_mask && memcmp(line_cache[line], padded, sizeof(padded)) == 0) {
        return;
    }
    memcpy(line_cache[line], padded, sizeof(padded));
    mask_cache[line] = invert_mask;
    for (uint8_t c = 0; c < NLK_DISPLAY_BIG_COLS; c++) {
        draw_big_char(line, c, padded[c], (invert_mask >> c) & 1);
    }
}

bool oled_display_rendered_line(uint8_t line, uint8_t *out) {
    if (line >= NLK_DISPLAY_BIG_LINES) return false;
    out[0] = cache_valid ? mask_cache[line] : 0;
    if (cache_valid) {
        memcpy(&out[1], line_cache[line], NLK_DISPLAY_BIG_COLS);
    } else {
        memset(&out[1], ' ', NLK_DISPLAY_BIG_COLS);
    }
    return true;
}

bool oled_display_panel_on(void) {
    return is_oled_on();
}

/* Fallback-screen widgets: one per big line, assigned over HID and persisted
 * by the keymap. */
static uint8_t widgets[NLK_DISPLAY_BIG_LINES] = NLK_DISPLAY_WIDGET_DEFAULTS;
static char    custom_buf[NLK_DISPLAY_BIG_LINES][NLK_DISPLAY_BIG_COLS + 1];

void oled_display_set_widget(uint8_t line, uint8_t widget) {
    if (line >= NLK_DISPLAY_BIG_LINES) return;
    if (widget >= NLK_WIDGET_COUNT) widget = NLK_WIDGET_COUNT - 1;
    widgets[line] = widget;
}

uint8_t oled_display_get_widget(uint8_t line) {
    return (line < NLK_DISPLAY_BIG_LINES) ? widgets[line] : NLK_WIDGET_BLANK;
}

uint8_t *oled_display_widget_table(void) {
    return widgets;
}

void oled_display_set_custom(uint8_t line, const uint8_t *text, uint8_t len) {
    if (line >= NLK_DISPLAY_BIG_LINES) return;
    if (len > NLK_DISPLAY_BIG_COLS) len = NLK_DISPLAY_BIG_COLS;
    for (uint8_t i = 0; i < NLK_DISPLAY_BIG_COLS; i++) {
        char c             = (i < len) ? (char)text[i] : ' ';
        custom_buf[line][i] = (c >= 32 && c < 127) ? c : ' ';
    }
    custom_buf[line][NLK_DISPLAY_BIG_COLS] = '\0';
}

const char *oled_display_get_custom(uint8_t line) {
    return (line < NLK_DISPLAY_BIG_LINES) ? custom_buf[line] : "";
}

// Fills text[5] (space-padded, no terminator) + per-char invert mask.
static void widget_content(uint8_t line, uint8_t widget, char *t, uint8_t *mask) {
    memset(t, ' ', NLK_DISPLAY_BIG_COLS);
    *mask = 0;
    switch (widget) {
        case NLK_WIDGET_LAYER:
            memcpy(t, "LYR ", 4);
            t[4]  = '0' + get_highest_layer(layer_state | default_layer_state);
            *mask = 1 << 4;
            break;
        case NLK_WIDGET_UPTIME: {
            uint32_t secs = timer_read32() / 1000;
            t[0]          = 'T';
            t[2]          = '0' + (secs / 100) % 10;
            t[3]          = '0' + (secs / 10) % 10;
            t[4]          = '0' + secs % 10;
            break;
        }
        case NLK_WIDGET_MODS: {
            uint8_t mods = get_mods() | get_weak_mods();
            memcpy(t, "CSAG", 4);
            if (mods & MOD_MASK_CTRL) *mask |= 1 << 0;
            if (mods & MOD_MASK_SHIFT) *mask |= 1 << 1;
            if (mods & MOD_MASK_ALT) *mask |= 1 << 2;
            if (mods & MOD_MASK_GUI) *mask |= 1 << 3;
            break;
        }
        case NLK_WIDGET_OSM: {
            uint8_t mods = get_oneshot_mods() | get_oneshot_locked_mods();
            memcpy(t, "oCSAG", 5);
            if (mods & MOD_MASK_CTRL) *mask |= 1 << 1;
            if (mods & MOD_MASK_SHIFT) *mask |= 1 << 2;
            if (mods & MOD_MASK_ALT) *mask |= 1 << 3;
            if (mods & MOD_MASK_GUI) *mask |= 1 << 4;
            break;
        }
        case NLK_WIDGET_OSL: {
            bool active = is_oneshot_layer_active();
            memcpy(t, "OSL ", 4);
            t[4] = active ? (char)('0' + get_oneshot_layer()) : '-';
            if (active) *mask = 1 << 4;
            break;
        }
        case NLK_WIDGET_LOCKS: {
            led_t leds = host_keyboard_led_state();
            t[0]       = 'C';
            t[2]       = 'N';
            t[4]       = 'S';
            if (leds.caps_lock) *mask |= 1 << 0;
            if (leds.num_lock) *mask |= 1 << 2;
            if (leds.scroll_lock) *mask |= 1 << 4;
            break;
        }
        case NLK_WIDGET_CAPS:
            memcpy(t, "CAP", 3);
            if (host_keyboard_led_state().caps_lock) *mask = 0x07;
            break;
        case NLK_WIDGET_NUMLOCK:
            memcpy(t, "NLK", 3);
            if (host_keyboard_led_state().num_lock) *mask = 0x07;
            break;
        case NLK_WIDGET_SCROLLLOCK:
            memcpy(t, "SLK", 3);
            if (host_keyboard_led_state().scroll_lock) *mask = 0x07;
            break;
        case NLK_WIDGET_RGBMAP:
            memcpy(t, "MAP", 3);
            if (per_layer_rgb_get_enabled()) *mask = 0x07;
            break;
        case NLK_WIDGET_NUMWORD:
            memcpy(t, "NUM", 3);
            if (num_word_is_active()) *mask = 0x07;
            break;
        case NLK_WIDGET_SENTENCE:
            memcpy(t, "SC", 2);
            if (is_sentence_case_on()) *mask = 0x03;
            break;
        case NLK_WIDGET_CUSTOM:
            memcpy(t, custom_buf[line], NLK_DISPLAY_BIG_COLS);
            break;
        case NLK_WIDGET_BLANK:
        default:
            break;
    }
}

static void render_pushed(void) {
    for (uint8_t l = 0; l < NLK_DISPLAY_BIG_LINES; l++) {
        render_big_line(l, push_buf[l], 0);
    }
    cache_valid = true;
}

static void render_fallback(void) {
    for (uint8_t i = 0; i < NLK_DISPLAY_BIG_LINES; i++) {
        // Space-padded raw 5 bytes, no terminator — render_big_line reads at
        // most NLK_DISPLAY_BIG_COLS chars, so that's safe.
        char    t[NLK_DISPLAY_BIG_COLS];
        uint8_t mask;
        widget_content(i, widgets[i], t, &mask);
        render_big_line(i, t, mask);
    }
    cache_valid = true;
}

/* ---------------------------------------------------------------------------
 * Transient overlays (v5): volume / RGB brightness / autoscroll speed.
 * Values render LIVE each frame during the overlay window, so turning a
 * knob keeps updating the glass while the window keeps extending.
 * ------------------------------------------------------------------------- */
#include "rgb_matrix.h"
#include "shared/autoscroll.h"

static volatile nlk_overlay_t overlay_type  = NLK_OVERLAY_NONE;
static volatile char          overlay_aux   = 0;
static volatile uint32_t      overlay_since = 0;
static volatile uint16_t      overlay_ms    = NLK_DISPLAY_OVERLAY_MS_DEFAULT;

void oled_display_overlay(nlk_overlay_t type, char aux) {
    if (overlay_ms == 0) return; // disabled
    overlay_type  = type;
    overlay_aux   = aux;
    overlay_since = timer_read32();
}

void oled_display_set_overlay_ms(uint16_t ms) {
    if (ms > NLK_DISPLAY_OVERLAY_MS_MAX) ms = NLK_DISPLAY_OVERLAY_MS_MAX;
    overlay_ms = ms;
}

uint16_t oled_display_get_overlay_ms(void) {
    return overlay_ms;
}

static bool overlay_active(void) {
    return overlay_type != NLK_OVERLAY_NONE && overlay_ms != 0 && timer_elapsed32(overlay_since) < overlay_ms;
}

static void render_overlay(void) {
    char label[NLK_DISPLAY_BIG_COLS + 1] = "     ";
    char value[NLK_DISPLAY_BIG_COLS + 1] = "     ";
    switch (overlay_type) {
        case NLK_OVERLAY_VOLUME:
            memcpy(label, " VOL ", 5);
            if (overlay_aux == 'M') {
                memcpy(value, "MUTE ", 5);
            } else {
                value[2] = overlay_aux; // '+' or '-'
            }
            break;
        case NLK_OVERLAY_RGB_VAL: {
            memcpy(label, " BRI ", 5);
            uint16_t pct = (uint16_t)rgb_matrix_get_val() * 100 / 255;
            value[0]     = ' ';
            value[1]     = pct >= 100 ? '1' : ' ';
            value[2]     = pct >= 10 ? ('0' + (pct / 10) % 10) : ' ';
            value[3]     = '0' + pct % 10;
            value[4]     = '%';
            break;
        }
        case NLK_OVERLAY_AUTOSCROLL: {
            memcpy(label, "SCRL ", 5);
            int8_t level = autoscroll_get_level();
            if (level == 0) {
                memcpy(value, " OFF ", 5);
            } else {
                value[1] = level > 0 ? '+' : '-';
                value[2] = '0' + (level > 0 ? level : -level);
            }
            break;
        }
        default:
            break;
    }
    render_big_line(0, "     ", 0);
    render_big_line(1, label, 0);
    render_big_line(2, value, 0);
    render_big_line(3, "     ", 0);
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
        cache_valid = false;         // buffer was wiped — force full redraw
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
    // Priority: transient overlay (device-driven feedback) > pushed content
    // (app-driven) > widgets. Idle sleep applies below both — an overlay or
    // push writes pixels, dirties blocks, and thereby wakes the panel.
    if (overlay_active()) {
        render_overlay();
    } else if (oled_display_pushed_active()) {
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

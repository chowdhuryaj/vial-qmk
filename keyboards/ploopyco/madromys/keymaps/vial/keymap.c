/* Copyright 2023 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H
#include <stdio.h>

#include "pd_accel.h"
#include "pointing_device_smoothing.h"
#include "pd_gestures.h"
#include "drag_scroll.h"

/* ---------------------------------------------------------------------------
 * Layers
 * ------------------------------------------------------------------------- */
enum madromys_layers {
    _BASE = 0, // default mouse buttons
    _MOUSE,    // momentary mouse layer (hold LT on _BASE Top-Right-Right)
    _SCRL,     // drag-scroll oriented layer
    _FN,       // DPI / tuning / settings
};

/* ---------------------------------------------------------------------------
 * Custom keycodes.
 *
 * These map 1:1 to the Vial GUI "customKeycodes" list, which is indexed from
 * QK_KB_0. The order here MUST match the order in vial.json.
 * ------------------------------------------------------------------------- */
enum madromys_keycodes {
    DPI_CONFIG = QK_KB_0, // 0  cycle through the DPI options
    DPI_UP,               // 1  next (higher) DPI
    DPI_DOWN,             // 2  previous (lower) DPI
    DRG_TOG,              // 3  drag scroll: toggle
    DRG_MO,               // 4  drag scroll: momentary (hold)
    STATUS,               // 5  type out current settings as plain text
    DRG_INV,              // 6  drag scroll: invert direction (toggle)
    GRA_TOG,              // 7  gesture set A "arrows": toggle (latching)
    GRA_HLD,              // 8  gesture set A "arrows": momentary hold
    GRB_TOG,              // 9  gesture set B "editing": toggle (latching)
    GRB_HLD,              // 10 gesture set B "editing": momentary hold
    GRC_TOG,              // 11 gesture set C "media": toggle (latching)
    GRC_HLD,              // 12 gesture set C "media": momentary hold
    GRD_TOG,              // 13 gesture set D "PACS nav": toggle (latching)
    GRD_HLD,              // 14 gesture set D "PACS nav": momentary hold
};

/* ---------------------------------------------------------------------------
 * Keymap
 *
 * LAYOUT() order follows info.json:
 *   ( Top Left Left, Top Left, Top Right, Top Right Right, Bottom Left, Bottom Right )
 *   = matrix ( [0,1], [0,2], [0,3], [0,4], [0,0], [0,5] )
 * ------------------------------------------------------------------------- */
// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Base: drag-scroll toggle + mouse buttons. Three positions are simple
     * layer-tap mod-taps (tap = the mouse button, hold = a layer); the rest are
     * plain single presses:
     *   Top Left Left    DRG_TOG              drag-scroll toggle
     *   Top Left         LT(_SCRL,  KC_BTN4)  tap BTN4 / hold Scroll layer
     *   Top Right        LT(_FN,    KC_BTN5)  tap BTN5 / hold Function layer
     *   Top Right Right  LT(_MOUSE, KC_BTN2)  tap BTN2 / hold Mouse layer
     *   Bottom Left      KC_BTN1              left click
     *   Bottom Right     KC_BTN3              middle click */
    [_BASE] = LAYOUT(
        DRG_TOG, LT(_SCRL, KC_BTN4), LT(_FN, KC_BTN5), LT(_MOUSE, KC_BTN2), KC_BTN1, KC_BTN3
    ),

    /* Mouse: momentary layer, entered by holding LT(_MOUSE, KC_BTN2) on _BASE's
     * Top Right Right. Top Left Left carries DRG_TOG here too — the SAME physical
     * key as _BASE's drag-scroll toggle — so the toggle stays reachable without
     * leaving the mouse layer. Middle click (BTN3) on Top Right Right; DPI down/up
     * fill the middle two slots. */
    [_MOUSE] = LAYOUT(
        DRG_TOG, DPI_DOWN, DPI_UP, KC_BTN3, KC_BTN1, KC_BTN2
    ),

    /* Scroll: wheel up/down plus a drag-scroll toggle fill the top slots;
     * clicks keep the same BTN1/BTN2/BTN3 positions as _BASE/_MOUSE. */
    [_SCRL] = LAYOUT(
        KC_BTN3, MS_WHLU, MS_WHLD, DRG_TOG, KC_BTN1, KC_BTN2
    ),

    /* Function: PACS-nav gestures, DPI cycle, a Vial macro, debug-console toggle,
     * and bootloader. GRD_TOG (Top Left) latches the set-D "PACS nav" flick
     * gestures on/off; MC_0 (Top Right Right) is Vial macro M0 — its sequence is
     * authored in the Vial GUI's Macros tab, not here. DB_TOGG (Bottom Left) is
     * QMK's built-in debug-console toggle — flip it on before a `qmk console`
     * tuning session (see POINTING_DEVICE_DEBUG in config.h), off after, so the
     * console isn't spammed during normal use. Top Left Left is free (KC_NO) —
     * bind anything in Vial. */
    [_FN] = LAYOUT(
        KC_NO, GRD_TOG, DPI_CONFIG, MC_0, DB_TOGG, QK_BOOT
    ),

    /* ----- Spare layers (4-7) ----------------------------------------------
     * Blank placeholders so Vial exposes 8 dynamic layers for future GUI-side
     * use. All transparent (KC_TRNS) rather than KC_NO so an accidental
     * MO()/TO() into one falls through to _BASE instead of killing every
     * button. Nothing in firmware references these layers. */
    [4] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [5] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [6] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [7] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};
// clang-format on

/* ---------------------------------------------------------------------------
 * DPI / CPI management (persisted in user EEPROM)
 * ------------------------------------------------------------------------- */
static const uint16_t dpi_options[] = MADROMYS_DPI_OPTIONS;
#define DPI_COUNT ARRAY_SIZE(dpi_options)

typedef union {
    uint32_t raw;
    struct {
        uint8_t dpi_index;
    };
} user_config_t;

static user_config_t user_config;

static void dpi_apply(void) {
    pointing_device_set_cpi(dpi_options[user_config.dpi_index]);
}

static void dpi_save_and_apply(void) {
    eeconfig_update_user(user_config.raw);
    dpi_apply();
}

static void dpi_cycle(void) {
    user_config.dpi_index = (user_config.dpi_index + 1) % DPI_COUNT;
    dpi_save_and_apply();
}

static void dpi_up(void) {
    if (user_config.dpi_index < DPI_COUNT - 1) {
        user_config.dpi_index++;
        dpi_save_and_apply();
    }
}

static void dpi_down(void) {
    if (user_config.dpi_index > 0) {
        user_config.dpi_index--;
        dpi_save_and_apply();
    }
}

/* ---------------------------------------------------------------------------
 * Gesture direction tables. 4-way, index order: 0=E 1=S 2=W 3=N (East, then
 * clockwise). The keymap hands one of these to the pd_gestures engine when a
 * ratchet gesture keycode starts; the engine taps from it once per RATCHET_STEP
 * of travel. Held modifiers pass through (tap_code16), so e.g. Shift + an arrow
 * flick selects text. See pd_gestures.h.
 * ------------------------------------------------------------------------- */
// Set A "arrows" (GRA_*): arrow keys.
static const uint16_t gesture_ratchet_a[PD_GESTURES_NUM_DIRECTIONS] = {
    KC_RIGHT, KC_DOWN, KC_LEFT, KC_UP,
};
// Set B "editing" (GRB_*): forward-delete, ctrl+backspace, backspace, ctrl+delete.
static const uint16_t gesture_ratchet_b[PD_GESTURES_NUM_DIRECTIONS] = {
    KC_DEL, C(KC_BSPC), KC_BSPC, C(KC_DEL),
};
// Set C "media" (GRC_*): next track, vol down, prev track, vol up.
static const uint16_t gesture_ratchet_c[PD_GESTURES_NUM_DIRECTIONS] = {
    KC_MNXT, KC_VOLD, KC_MPRV, KC_VOLU,
};
// Set D "PACS nav" (GRD_*): flick to drive the PACS viewer's study/series
// hotkeys. Directions in {E,S,W,N} order: E=right F8 (next study), S=down
// Shift+F10 (prev series), W=left F7 (prev study), N=up Shift+F9 (next series).
// Emitted via tap_code16, so the shifted F-keys pass their modifier through.
static const uint16_t gesture_ratchet_d[PD_GESTURES_NUM_DIRECTIONS] = {
    KC_F8, S(KC_F10), KC_F7, S(KC_F9),
};

/* ---------------------------------------------------------------------------
 * Pointing device pipeline
 *
 * smoothing -> gestures -> drag scroll -> acceleration
 *
 * While a gesture is open it swallows movement (zeroes x/y); while drag scroll
 * is active it converts movement into wheel events and zeroes X/Y (so
 * acceleration, which skips zero reports, leaves scrolling untouched).
 * ------------------------------------------------------------------------- */
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    mouse_report = pointing_device_smoothing_apply(mouse_report);
    mouse_report = pd_gestures_apply(mouse_report);
    mouse_report = drag_scroll_apply(mouse_report);
    mouse_report = pd_accel_apply(mouse_report);
    return mouse_report;
}

/* ---------------------------------------------------------------------------
 * Status report: types the current settings as plain text into whatever has
 * focus (a text editor, chat box, etc.) via send_string. There's no display
 * on this device, so this is the status output.
 * ------------------------------------------------------------------------- */
// Formats a float as "[-]D.DD" using integer math only, since this build's
// libc snprintf doesn't reliably support %f on this platform.
static void fmt2(char *buf, size_t bufsize, float val) {
    bool neg = val < 0;
    if (neg) val = -val;
    int whole = (int)val;
    int frac  = (int)((val - whole) * 100.0f + 0.5f);
    if (frac >= 100) {
        frac -= 100;
        whole += 1;
    }
    // Clamp so the compiler can prove "%d.%02d" always fits the caller's buffer
    // (these are tuning-curve values, never anywhere near this range anyway).
    if (whole < 0) whole = 0;
    if (whole > 999) whole = 999;
    if (frac < 0) frac = 0;
    if (frac > 99) frac = 99;
    snprintf(buf, bufsize, "%s%d.%02d", neg ? "-" : "", whole, frac);
}

static void print_status(void) {
    char tko[10], grw[10], ofs[10], lmt[10], smf[10];
    fmt2(tko, sizeof(tko), pd_accel_get_takeoff());
    fmt2(grw, sizeof(grw), pd_accel_get_growth_rate());
    fmt2(ofs, sizeof(ofs), pd_accel_get_offset());
    fmt2(lmt, sizeof(lmt), pd_accel_get_limit());
    fmt2(smf, sizeof(smf), pointing_device_smoothing_get_factor());

    char buf[512];
    snprintf(buf, sizeof(buf),
             "=== Adept status ===\n"
             "DPI: %u (%u/%u)\n"
             "Accel: %s takeoff=%s growth=%s offset=%s limit=%s\n"
             "Smooth: %s factor=%s timeout=%ums\n"
             "Gestures: ratchet=%u active=%s\n"
             "DragScroll: h=%d v=%d active=%s inverted=%s\n",
             (unsigned)dpi_options[user_config.dpi_index], (unsigned)user_config.dpi_index + 1, (unsigned)DPI_COUNT,
             pd_accel_get_enabled() ? "ON" : "OFF", tko, grw, ofs, lmt,
             pointing_device_smoothing_get_enabled() ? "ON" : "OFF", smf,
             (unsigned)pointing_device_smoothing_get_reset_timeout(),
             (unsigned)PD_GESTURES_RATCHET_STEP, pd_gestures_is_active() ? "ON" : "OFF",
             (int)get_drag_scroll_h_divisor(), (int)get_drag_scroll_v_divisor(),
             get_drag_scroll_scrolling() ? "ON" : "OFF", get_drag_scroll_inverted() ? "ON" : "OFF");
    send_string(buf);
}

/* ---------------------------------------------------------------------------
 * Keycode handling
 * ------------------------------------------------------------------------- */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef DRAG_SCROLL_AUTO_EXIT
    // Any button press (except the DRG_* scroll controls) drops out of drag
    // scroll. The press is not consumed here, so it still does its normal job.
    if (record->event.pressed && get_drag_scroll_scrolling()) {
        switch (keycode) {
            case DRG_TOG:
            case DRG_MO:
            case DRG_INV:
                break; // scroll controls must not cancel scrolling
            default:
                set_drag_scroll_scrolling(false);
                break;
        }
    }
#endif
#ifdef GESTURE_AUTO_EXIT
    // Safe exit: any button press except the gesture controls cancels an active
    // gesture. The press is not consumed, so it still does its normal job. The
    // gesture controls keep their own semantics (toggle off / switch set / hold).
    if (record->event.pressed && pd_gestures_is_active()) {
        switch (keycode) {
            case GRA_TOG:
            case GRA_HLD:
            case GRB_TOG:
            case GRB_HLD:
            case GRC_TOG:
            case GRC_HLD:
            case GRD_TOG:
            case GRD_HLD:
                break; // gesture controls keep their own behavior
            default:
                pd_gestures_cancel();
                break;
        }
    }
#endif
    switch (keycode) {
        case DPI_CONFIG:
            if (record->event.pressed) dpi_cycle();
            return false;
        case DPI_UP:
            if (record->event.pressed) dpi_up();
            return false;
        case DPI_DOWN:
            if (record->event.pressed) dpi_down();
            return false;
        case DRG_TOG:
            if (record->event.pressed) set_drag_scroll_scrolling(!get_drag_scroll_scrolling());
            return false;
        case DRG_MO:
            set_drag_scroll_scrolling(record->event.pressed);
            return false;
        case STATUS:
            if (record->event.pressed) print_status();
            return false;
        case DRG_INV:
            if (record->event.pressed) drag_scroll_toggle_inverted();
            return false;
        case GRA_TOG:
            if (record->event.pressed) pd_gestures_toggle(gesture_ratchet_a);
            return false;
        case GRA_HLD:
            if (record->event.pressed) {
                pd_gestures_begin(gesture_ratchet_a);
            } else {
                pd_gestures_end();
            }
            return false;
        case GRB_TOG:
            if (record->event.pressed) pd_gestures_toggle(gesture_ratchet_b);
            return false;
        case GRB_HLD:
            if (record->event.pressed) {
                pd_gestures_begin(gesture_ratchet_b);
            } else {
                pd_gestures_end();
            }
            return false;
        case GRC_TOG:
            if (record->event.pressed) pd_gestures_toggle(gesture_ratchet_c);
            return false;
        case GRC_HLD:
            if (record->event.pressed) {
                pd_gestures_begin(gesture_ratchet_c);
            } else {
                pd_gestures_end();
            }
            return false;
        case GRD_TOG:
            if (record->event.pressed) pd_gestures_toggle(gesture_ratchet_d);
            return false;
        case GRD_HLD:
            if (record->event.pressed) {
                pd_gestures_begin(gesture_ratchet_d);
            } else {
                pd_gestures_end();
            }
            return false;
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * Layer-bound drag scroll: turn drag scroll on when entering DRAG_SCROLL_LAYER
 * and off when leaving it. Edge-triggered on that layer's membership so ordinary
 * layer changes don't clobber a manually toggled scroll elsewhere.
 * ------------------------------------------------------------------------- */
#ifdef DRAG_SCROLL_LAYER
layer_state_t layer_state_set_user(layer_state_t state) {
    static bool was_on_scroll_layer = false;
    bool        on_scroll_layer     = layer_state_cmp(state, DRAG_SCROLL_LAYER);
    if (on_scroll_layer != was_on_scroll_layer) {
        set_drag_scroll_scrolling(on_scroll_layer);
        was_on_scroll_layer = on_scroll_layer;
    }
    return state;
}
#endif

/* ---------------------------------------------------------------------------
 * Init
 * ------------------------------------------------------------------------- */
void keyboard_pre_init_user(void) {
    // Ground the unused RP2040 pins for additional ground pathways (Ploopy HW).
#ifdef UNUSABLE_PINS
    const pin_t unused_pins[] = UNUSABLE_PINS;
    for (uint8_t i = 0; i < ARRAY_SIZE(unused_pins); i++) {
        gpio_set_pin_output_push_pull(unused_pins[i]);
        gpio_write_pin_low(unused_pins[i]);
    }
#endif
}

void eeconfig_init_user(void) {
    user_config.raw       = 0;
    user_config.dpi_index = MADROMYS_DPI_DEFAULT_INDEX;
    eeconfig_update_user(user_config.raw);
}

void keyboard_post_init_user(void) {
    user_config.raw = eeconfig_read_user();
    if (user_config.dpi_index >= DPI_COUNT) {
        user_config.raw       = 0;
        user_config.dpi_index = MADROMYS_DPI_DEFAULT_INDEX;
        eeconfig_update_user(user_config.raw);
    }
    dpi_apply();

    pd_accel_init();
}

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
#include "wiggle_ball.h"
#include "am_tuning.h"

/* ---------------------------------------------------------------------------
 * Layers
 * ------------------------------------------------------------------------- */
enum madromys_layers {
    _BASE = 0, // default mouse buttons (right hand)
    _MOUSE,    // auto-mouse target (activated by trackball movement)
    _SCRL,     // drag-scroll oriented layer
    _FN,       // DPI / tuning / settings
    // Left-hand mirror set: structurally identical to _BASE.._FN but with the
    // click buttons re-laid-out for left-hand use (see the keymap below). The
    // ambidextrous toggle (AMBI_TOG) flips the persisted default layer between
    // _BASE and _BASE_L; the gap between the two sets is HAND_LAYER_COUNT (4),
    // used as a runtime "hand offset" so the shared layer-switching logic can
    // target the correct half without duplicating it.
    _BASE_L,   // 4: mirror of _BASE
    _MOUSE_L,  // 5: mirror of _MOUSE
    _SCRL_L,   // 6: mirror of _SCRL
    _FN_L,     // 7: mirror of _FN
};

// Offset between the right-hand set (_BASE..) and the left-hand set (_BASE_L..).
#define HAND_LAYER_COUNT (_BASE_L - _BASE)

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
    AMBI_TOG,             // 13 ambidextrous: toggle right-hand / left-hand mode
    AM_THR,               // 14 auto-mouse: adjust activation threshold (Ctrl x10, Shift inverts)
    AM_TIME,              // 15 auto-mouse: adjust activation timeout (Ctrl x10, Shift inverts)
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

    /* Mouse: auto-mouse target. Top Left Left is tap = middle click (BTN3) /
     * hold = Scroll layer (mod-tap); DPI + momentary drag scroll fill the
     * other top slots. */
    [_MOUSE] = LAYOUT(
        LT(_SCRL, KC_BTN3), DPI_DOWN, DPI_UP, DRG_MO, KC_BTN1, KC_BTN2
    ),

    /* Scroll: wheel up/down plus a drag-scroll toggle fill the top slots;
     * clicks keep the same BTN1/BTN2/BTN3 positions as _BASE/_MOUSE. */
    [_SCRL] = LAYOUT(
        KC_BTN3, MS_WHLU, MS_WHLD, DRG_TOG, KC_BTN1, KC_BTN2
    ),

    /* Function: DPI cycle, ambidextrous toggle, debug-console toggle, and
     * bootloader. The remaining slots are free — bind gesture sets here via
     * Vial if you want them on a permanent layer. AMBI_TOG (Top Left Left)
     * flips right-/left-hand mode; it is mirrored into _FN_L at the same
     * position so you can always toggle back. DB_TOGG (Bottom Left) is QMK's
     * built-in debug-console toggle — flip it on before a `qmk console`
     * tuning session (see POINTING_DEVICE_DEBUG in config.h), off after, so
     * the console isn't spammed during normal use. */
    [_FN] = LAYOUT(
        AMBI_TOG, KC_NO, DPI_CONFIG, KC_NO, DB_TOGG, QK_BOOT
    ),

    /* ----- Left-hand mirror set (_BASE_L .. _FN_L) -------------------------
     * Each layer mirrors its right-hand counterpart with ONLY the click buttons
     * changed (everything else — gesture/scroll/DPI keys — is identical). Only
     * BTN1/BTN2/BTN3 move; each keeps its identity and is placed at the
     * physical mirror of its right-hand position:
     *   BTN1 (left-click)   Bottom Left  (right) <-> Bottom Right (left)
     *   BTN2 (right-click)  Bottom Right (right) <-> Bottom Left  (left)
     *   BTN3 (middle-click) Top Left Left (right) <-> Top Right Right (left)
     * The hold side of each mod-tap rides along with its button (Fn-hold with
     * BTN2, Scroll-hold with BTN3), and the LT() targets point at the
     * left-hand layers (_FN_L / _SCRL_L). The runtime hand offset
     * (HAND_LAYER_COUNT) retargets auto-mouse and the drag-scroll-layer
     * binding for this half — see hand_offset() below. */
    [_BASE_L] = LAYOUT(
        KC_BTN4, KC_BTN5, DRG_TOG, LT(_FN_L, KC_BTN3), KC_BTN2, KC_BTN1
    ),
    [_MOUSE_L] = LAYOUT(
        DPI_DOWN, DPI_UP, DRG_MO, KC_BTN3, KC_BTN2, LT(_SCRL_L, KC_BTN1)
    ),
    [_SCRL_L] = LAYOUT(
        MS_WHLU, MS_WHLD, DRG_TOG, KC_BTN3, KC_BTN2, KC_BTN1
    ),
    [_FN_L] = LAYOUT(
        AMBI_TOG, KC_NO, DPI_CONFIG, KC_NO, DB_TOGG, QK_BOOT
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

/* ---------------------------------------------------------------------------
 * Pointing device pipeline
 *
 * smoothing -> wiggle detection -> gestures -> drag scroll -> acceleration
 *
 * Wiggle detection runs BEFORE gestures (and before drag scroll) on purpose: a
 * live gesture swallows movement (zeroes x/y), so if wiggle ran after it, the
 * shake would already be gone and could never be seen. Running wiggle first
 * lets a shake cancel an active gesture (see wiggle_ball.c) as well as toggle
 * drag scroll. Its input is still the smoothed, pre-drag-scroll ball axes -
 * same as before the reorder - so the shake thresholds don't need re-tuning.
 *
 * While a gesture is open it swallows movement; while drag scroll is active it
 * converts movement into wheel events and zeroes X/Y (so acceleration, which
 * skips zero reports, leaves scrolling untouched).
 * ------------------------------------------------------------------------- */
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    mouse_report = pointing_device_smoothing_apply(mouse_report);
    mouse_report = wiggle_ball_apply(mouse_report);
    mouse_report = pd_gestures_apply(mouse_report);
    mouse_report = drag_scroll_apply(mouse_report);
    mouse_report = pd_accel_apply(mouse_report);
    return mouse_report;
}

/* ---------------------------------------------------------------------------
 * Ambidextrous (hand) mode
 *
 * Right-hand mode keeps the default layer at _BASE (0); left-hand mode moves it
 * to _BASE_L (4). The two layer sets are structural mirrors (see the keymap), so
 * the shared layer-switching logic just adds a runtime "hand offset" of 0 or
 * HAND_LAYER_COUNT rather than duplicating the gesture/layer code for 4-7.
 *
 * Persistence is plain QMK: set_single_persistent_default_layer() stores the
 * default layer in EEPROM and the core restores it on boot (see quantum.c /
 * keyboard.c), so the active hand survives power cycles with no EEPROM field of
 * our own.
 * ------------------------------------------------------------------------- */
static bool hand_is_left(void) {
    return get_highest_layer(default_layer_state) >= _BASE_L;
}

static uint8_t hand_offset(void) {
    return hand_is_left() ? HAND_LAYER_COUNT : 0;
}

// Auto-mouse target layer for a given hand's base layer. Right hand (base
// _BASE) gets the dedicated _MOUSE layer, as before. The left hand's base layer
// (_BASE_L) already carries the mouse buttons, so its auto-mouse target IS its
// own base layer - moving the ball never switches layers in left-hand mode.
static uint8_t auto_mouse_target(uint8_t base_layer) {
    return (base_layer >= _BASE_L) ? base_layer : _MOUSE;
}

// Switch hand mode: persist the new default layer and retarget auto-mouse to the
// matching half. base_layer is _BASE (right) or _BASE_L (left).
static void apply_hand_mode(uint8_t base_layer) {
    set_single_persistent_default_layer(base_layer);
    set_auto_mouse_layer(auto_mouse_target(base_layer));
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
             "Hand: %s (default layer %u)\n"
             "DPI: %u (%u/%u)\n"
             "AutoMouse: %s layer=%u timeout=%ums thresh=%u debounce=%ums\n"
             "Accel: %s takeoff=%s growth=%s offset=%s limit=%s\n"
             "Smooth: %s factor=%s timeout=%ums\n"
             "Gestures: ratchet=%u active=%s\n"
             "DragScroll: h=%d v=%d active=%s inverted=%s forced=%s\n",
             hand_is_left() ? "LEFT" : "RIGHT", (unsigned)get_highest_layer(default_layer_state),
             (unsigned)dpi_options[user_config.dpi_index], (unsigned)user_config.dpi_index + 1, (unsigned)DPI_COUNT,
             get_auto_mouse_enable() ? "ON" : "OFF", (unsigned)get_auto_mouse_layer(), (unsigned)get_auto_mouse_timeout(),
             (unsigned)am_tuning_get_threshold(), (unsigned)get_auto_mouse_debounce(),
             pd_accel_get_enabled() ? "ON" : "OFF", tko, grw, ofs, lmt,
             pointing_device_smoothing_get_enabled() ? "ON" : "OFF", smf,
             (unsigned)pointing_device_smoothing_get_reset_timeout(),
             (unsigned)PD_GESTURES_RATCHET_STEP, pd_gestures_is_active() ? "ON" : "OFF",
             (int)get_drag_scroll_h_divisor(), (int)get_drag_scroll_v_divisor(),
             get_drag_scroll_scrolling() ? "ON" : "OFF", get_drag_scroll_inverted() ? "ON" : "OFF",
             get_drag_scroll_force() ? "ON" : "OFF");
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
        case AMBI_TOG:
            // Toggle right-hand (_BASE) <-> left-hand (_BASE_L) and persist it.
            if (record->event.pressed) {
                apply_hand_mode(hand_is_left() ? _BASE : _BASE_L);
            }
            return false;
        case AM_THR:
            if (record->event.pressed) am_tuning_threshold_increment();
            return false;
        case AM_TIME:
            if (record->event.pressed) am_tuning_timeout_increment();
            return false;
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * Layer-bound drag scroll: turn drag scroll on when entering DRAG_SCROLL_LAYER
 * and off when leaving it. Edge-triggered on that layer's membership so ordinary
 * layer changes (and auto-mouse's own layer flips) don't clobber a manually
 * toggled scroll elsewhere. remove_auto_mouse_layer() masks the auto-mouse target
 * layer out of the check (see the note in pointing_device_auto_mouse.c); harmless
 * here since DRAG_SCROLL_LAYER != the auto-mouse layer.
 * ------------------------------------------------------------------------- */
#ifdef DRAG_SCROLL_LAYER
layer_state_t layer_state_set_user(layer_state_t state) {
    static bool was_on_scroll_layer = false;
    // Offset by the active hand so the left-hand scroll layer (_SCRL_L) drives
    // drag scroll exactly as _SCRL does in right-hand mode.
    bool        on_scroll_layer     = layer_state_cmp(remove_auto_mouse_layer(state, false), DRAG_SCROLL_LAYER + hand_offset());
    if (on_scroll_layer != was_on_scroll_layer) {
        set_drag_scroll_scrolling(on_scroll_layer);
        was_on_scroll_layer = on_scroll_layer;
    }
    return state;
}
#endif

/* ---------------------------------------------------------------------------
 * Host lock LEDs -> drag scroll. While any of caps / num / scroll lock is active
 * on the host, drag scroll is forced on, and manual turn-off (a button press,
 * DRG_TOG, releasing DRG_MO, or leaving the scroll layer) is ignored — the force
 * flag in drag_scroll.c vetoes it. When all three locks clear, the force
 * releases and drag scroll turns back off, handing control back to the manual
 * keycodes / layer binding. Independent of hand mode.
 * ------------------------------------------------------------------------- */
bool led_update_user(led_t led_state) {
    bool any_lock = led_state.caps_lock || led_state.num_lock || led_state.scroll_lock;
    set_drag_scroll_force(any_lock);
    if (!any_lock) {
        set_drag_scroll_scrolling(false);
    }
    return true;
}

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

    // Auto-mouse target for whichever hand mode was restored from EEPROM (the
    // default layer is already applied by core at this point). Right hand -> the
    // dedicated _MOUSE layer; left hand -> its own base layer (auto_mouse_target).
    set_auto_mouse_layer(auto_mouse_target(get_highest_layer(default_layer_state)));
    set_auto_mouse_enable(true);
}

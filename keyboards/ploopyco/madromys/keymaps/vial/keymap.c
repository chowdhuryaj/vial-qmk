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
#include <math.h>
#include <string.h>

#include "via.h"

#include "pd_accel.h"
#include "pointing_device_smoothing.h"
#include "pd_gestures.h"
#include "drag_scroll.h"
#include "wiggle_ball.h"
#include "custom_shift_keys.h"
#include "select_word.h"
#include "sentence_case.h"
#include "autoscroll.h"
#include "wheel_chords.h"
#include "os_shortcuts.h"
#include "os_detection.h"
#include "pipeline_diag.h"

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
    // Gesture set toggles (latching). Set CONTENTS are dynamic — edited live
    // over raw HID by the companion app and persisted in the EEPROM datablock.
    // Firmware defaults: 1=arrows 2=editing 3=media 4=app/tab-nav, 5-8 empty.
    // GR1-GR4 occupy the exact slots of the old GRA/GRB/GRC/GRN keycodes
    // (same QK_KB_7..10 values, seeded with the same bindings).
    GR1_TOG,              // 7  gesture set 1: toggle
    GR2_TOG,              // 8  gesture set 2: toggle
    GR3_TOG,              // 9  gesture set 3: toggle
    GR4_TOG,              // 10 gesture set 4: toggle
    GR5_TOG,              // 11 gesture set 5: toggle
    GR6_TOG,              // 12 gesture set 6: toggle
    GR7_TOG,              // 13 gesture set 7: toggle
    GR8_TOG,              // 14 gesture set 8: toggle
    // getreuer typing modules (2026-07-03)
    SELWORD,              // 15 select word fwd (Shift: line); repeat extends
    SELWDBK,              // 16 select word backward
    SELLINE,              // 17 select line fwd
    SELLNUP,              // 18 select line upward
    SC_TOG,               // 19 sentence case: toggle (state persists)
    CSK_TOG,              // 20 custom shift keys: toggle
    // Autoscroll (2026-07-03)
    ASC_JOG,               // 21 autoscroll: jog mode toggle (ball = jog wheel)
    ASC_UP,                // 22 autoscroll: step speed up (through zero stops)
    ASC_DOWN,              // 23 autoscroll: step speed down
    // OS-aware editing shortcuts (2026-07-03): mac mode = Cmd hotkeys,
    // pc mode = Ctrl; mode follows USB OS detection unless pinned (0x1D).
    OS_CUT,                // 24 cut (⌘X / ^X)
    OS_COPY,               // 25 copy (⌘C / ^C)
    OS_PSTE,               // 26 paste (⌘V / ^V)
    OS_UNDO,               // 27 undo (⌘Z / ^Z)
    OS_REDO,               // 28 redo (⇧⌘Z / ^Y)
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

    /* Function: DPI cycle, a Vial macro, debug-console toggle, and bootloader.
     * MC_0 (Top Right Right) is Vial macro M0 — its sequence is authored in the
     * Vial GUI's Macros tab, not here. DB_TOGG (Bottom Left) is QMK's built-in
     * debug-console toggle — flip it on before a `qmk console` tuning session
     * (see POINTING_DEVICE_DEBUG in config.h), off after, so the console isn't
     * spammed during normal use. Top Left Left and Top Left are both free
     * (KC_NO) — bind anything in Vial. */
    [_FN] = LAYOUT(
        KC_NO, KC_NO, DPI_CONFIG, MC_0, DB_TOGG, QK_BOOT
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
 * Gesture direction tables. 4-way, index order: 0=E 1=S 2=W 3=N (East, then
 * clockwise). The keymap hands one of these to the pd_gestures engine when a
 * ratchet gesture keycode starts; the engine taps from it once per ratchet
 * step of travel. Held modifiers pass through (tap_code16), so e.g. Shift +
 * an arrow flick selects text. See pd_gestures.h.
 *
 * The live tables are RAM (gesture_sets), edited over raw HID by the
 * companion app and persisted in the EEPROM datablock. Slots are fired via
 * tap_code16, so legal contents are basic keycodes + modifier combos
 * (C()/S()/A()/G() wraps) — NOT Vial macros, layer keys, or QK_KB_* customs
 * (those need the engine rerouted through vial_keycode_tap; deliberately not
 * done, per user choice 2026-07-02, to avoid touching a proven pipeline).
 * ------------------------------------------------------------------------- */
static uint16_t gesture_sets[MAD_GESTURE_SET_COUNT][PD_GESTURES_NUM_DIRECTIONS];

// Firmware defaults for sets 1-4 (sets 5-8 default empty). 8-direction order
// since 2026-07-02: E, SE, S, SW, W, NW, N, NE (diagonals default KC_NO — the
// engine then falls back to the nearest cardinal, preserving the old 4-way
// feel). Set 4 nav history: E=Ctrl+Tab (next browser tab), S=Space (page-down
// in browsers/PACS), W=Ctrl+Shift+Tab (prev tab), N=Tab (focus next field).
// Cmd+Tab was tried and dropped: each ratchet step is a full tap (Cmd released
// per step), so it only bounced between the two most recent apps instead of
// walking the switcher.
static const uint16_t gesture_set_defaults[4][PD_GESTURES_NUM_DIRECTIONS] = {
    //  E          SE     S        SW     W             NW     N       NE
    {KC_RIGHT,     KC_NO, KC_DOWN, KC_NO, KC_LEFT,      KC_NO, KC_UP,  KC_NO}, // set 1 "arrows"
    {KC_DEL,       KC_NO, KC_ENT,  KC_NO, KC_BSPC,      KC_NO, KC_ESC, KC_NO}, // set 2 "editing"
    {KC_MNXT,      KC_NO, KC_VOLD, KC_NO, KC_MPRV,      KC_NO, KC_VOLU, KC_NO}, // set 3 "media"
    {C(KC_TAB),    KC_NO, KC_SPC,  KC_NO, C(S(KC_TAB)), KC_NO, KC_TAB, KC_NO}, // set 4 "app/tab nav"
};

static bool gesture_set_is_empty(uint8_t set) {
    for (uint8_t d = 0; d < PD_GESTURES_NUM_DIRECTIONS; d++) {
        if (gesture_sets[set][d] != KC_NO) return false;
    }
    return true;
}

static void gesture_toggle_set(uint8_t set) {
    if (set >= MAD_GESTURE_SET_COUNT) return;
    // Empty guard: never START an all-KC_NO set (it would swallow ball
    // movement and fire nothing — a cursor freeze). A re-press of the
    // currently-active set must still reach the engine so it can toggle OFF,
    // since a set can be emptied over HID while latched.
    if (gesture_set_is_empty(set) && !pd_gestures_is_active_table(gesture_sets[set])) {
        return;
    }
    pd_gestures_toggle(gesture_sets[set]);
}

/* ---------------------------------------------------------------------------
 * Leader sequences (dynamic). QK_LEAD starts a sequence; QMK core
 * (quantum/leader.c) collects up to 5 keys and calls leader_end_user() on
 * timeout/overflow. Slots live in RAM, are edited over raw HID by the
 * companion app, and persist in the EEPROM datablock. A slot is
 * [key0..key4, output]; unused key positions are KC_NO (so the used length is
 * the KC_NO-free prefix), and output == KC_NO means the slot is empty.
 * Output fires via tap_code16 — same contract as gesture slots: basic
 * keycodes + C()/S()/A()/G() combos only, NO macros/layer keys/QK_KB_*.
 * ------------------------------------------------------------------------- */
static uint16_t leader_seqs[MAD_LEADER_SEQ_COUNT][MAD_LEADER_SEQ_KEYS + 1];

// Globals defined in quantum/leader.c but not declared in leader.h (it only
// exposes fixed-arity helpers like leader_sequence_two_keys, which compare a
// key prefix without checking the typed length — an exact length + content
// compare needs the raw buffer).
extern uint16_t leader_sequence[5];
extern uint8_t  leader_sequence_size;

static bool leader_seq_matches(const uint16_t *seq) {
    uint8_t len = 0;
    while (len < MAD_LEADER_SEQ_KEYS && seq[len] != KC_NO) {
        len++;
    }
    if (len == 0 || leader_sequence_size != len) return false;
    for (uint8_t i = 0; i < len; i++) {
        if (leader_sequence[i] != seq[i]) return false;
    }
    return true;
}

// Weak default in quantum/leader.c:22.
void leader_end_user(void) {
    for (uint8_t s = 0; s < MAD_LEADER_SEQ_COUNT; s++) {
        if (leader_seqs[s][MAD_LEADER_SEQ_KEYS] != KC_NO && leader_seq_matches(leader_seqs[s])) {
            tap_code16(leader_seqs[s][MAD_LEADER_SEQ_KEYS]);
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Persisted settings (EEPROM user datablock)
 *
 * RAM mirror of the EECONFIG_USER_DATA_SIZE datablock. The core validity word
 * is EECONFIG_USER_DATA_VERSION (config.h) — bump it whenever this struct's
 * layout changes; the datablock then reads invalid on next boot and re-seeds
 * from the config.h defaults. Scaled-integer fields (x100 for the float curve
 * parameters) use the same encoding as the raw HID wire format so EEPROM,
 * wire, and app agree. mad_config is the LAST-SAVED state; the modules' own
 * runtime variables are the live state — the two only meet at boot (apply)
 * and on an explicit per-channel save.
 * ------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint8_t  dpi_index;
    uint16_t wiggle_interval;      // ms (direction-switch timeout)
    uint16_t wiggle_cooldown;      // ms (post-toggle lockout)
    uint8_t  wiggle_threshold;     // sensor counts (perpendicular noise)
    uint8_t  accel_enabled;
    uint16_t accel_takeoff_x100;
    uint16_t accel_growth_x100;
    int16_t  accel_offset_x100;    // signed — offset may be negative
    uint16_t accel_limit_x100;
    uint8_t  smooth_enabled;
    uint8_t  smooth_factor_x100;   // 0..100
    uint16_t smooth_timeout_ms;
    uint16_t gesture_ratchet_step; // sensor counts
    uint8_t  drag_div_h;
    uint8_t  drag_div_v;
    uint8_t  drag_inverted;
    // Dynamic gesture set contents (raw QMK keycodes, [set][direction]).
    // v2 appended this at 8x4; v4 grew it in place to 8x8 (E,SE,S,SW,W,NW,N,NE)
    // — an in-place growth is a layout change, hence the version bump.
    uint16_t gesture_sets[MAD_GESTURE_SET_COUNT][PD_GESTURES_NUM_DIRECTIONS];
    // Wiggle-ball shake-detection kill switch (v4).
    uint8_t wiggle_enabled;
    // getreuer typing modules + leader (v5).
    uint8_t            csk_enabled;
    custom_shift_key_t csk_table[CUSTOM_SHIFT_KEYS_SLOTS];
    uint8_t            selword_mac;
    uint8_t            sentence_case_on;
    uint16_t           leader_seqs[MAD_LEADER_SEQ_COUNT][MAD_LEADER_SEQ_KEYS + 1];
    // Autoscroll (v6).
    uint8_t  as_inverted;
    uint16_t as_speed_scale_x100;
    uint8_t  as_jog_deadzone;
    uint16_t as_jog_range;
    // Auto-mouse (v7).
    uint8_t  am_enabled;
    uint16_t am_timeout_ms;
    uint8_t  am_threshold;
    // Wheel chords (v7): [button][direction] gesture keycodes + tunables.
    uint8_t  wc_enabled;
    uint16_t wc_step;
    uint16_t wc_table[WHEEL_CHORDS_BUTTONS][WHEEL_CHORDS_DIRECTIONS];
    // OS-aware shortcuts (v8): follow-detection switch + pinned mac/pc mode.
    uint8_t  os_follow;
    uint8_t  os_mac;
    // Wheel chords hold delay (v9): ms held before capture engages.
    uint16_t wc_hold_ms;
} mad_config_t;
_Static_assert(sizeof(mad_config_t) <= EECONFIG_USER_DATA_SIZE, "mad_config_t exceeds EECONFIG_USER_DATA_SIZE");

static mad_config_t mad_config;

// Wire/EEPROM scaling helpers. Inputs are clamped to <= |327| by the setter
// ranges, so the x100 result always fits.
static uint16_t f_to_x100(float val) {
    return (uint16_t)lroundf(val * 100.0f);
}
static int16_t f_to_x100_signed(float val) {
    return (int16_t)lroundf(val * 100.0f);
}

static void mad_config_write(void) {
    // eeconfig_update_user_datablock also (re)writes the validity word, so a
    // save on a fresh/invalid block marks it valid in the same call.
    eeconfig_update_user_datablock(&mad_config, 0, sizeof(mad_config));
}

/* ---------------------------------------------------------------------------
 * DPI / CPI management
 * ------------------------------------------------------------------------- */
static const uint16_t dpi_options[] = MADROMYS_DPI_OPTIONS;
#define DPI_COUNT ARRAY_SIZE(dpi_options)

static void dpi_apply(void) {
    pointing_device_set_cpi(dpi_options[mad_config.dpi_index]);
}

// DPI keycodes persist immediately (pre-datablock behavior, kept): stepping
// through options should survive a power cycle without a separate save.
static void dpi_save_and_apply(void) {
    mad_config_write();
    dpi_apply();
}

static void dpi_cycle(void) {
    mad_config.dpi_index = (mad_config.dpi_index + 1) % DPI_COUNT;
    dpi_save_and_apply();
}

static void dpi_up(void) {
    if (mad_config.dpi_index < DPI_COUNT - 1) {
        mad_config.dpi_index++;
        dpi_save_and_apply();
    }
}

static void dpi_down(void) {
    if (mad_config.dpi_index > 0) {
        mad_config.dpi_index--;
        dpi_save_and_apply();
    }
}

/* ---------------------------------------------------------------------------
 * Auto-mouse runtime state (QMK core feature; re-added 2026-07-03)
 *
 * Enable + timeout use the core's own runtime setters. Threshold has no core
 * runtime API (AUTO_MOUSE_THRESHOLD is compile-time), so keymap-side we
 * override the weak auto_mouse_activation (default at
 * quantum/pointing_device/pointing_device_auto_mouse.c:238) with our own
 * accumulator checked against a runtime threshold. The accumulator resets
 * after a still gap so slow drift doesn't eventually trip it.
 * ------------------------------------------------------------------------- */
static bool    mad_am_enabled   = MAD_AUTOMOUSE_ENABLED_DEFAULT;
static uint8_t mad_am_threshold = MAD_AUTOMOUSE_THRESHOLD_DEFAULT;

static void mad_automouse_set_enabled(bool on) {
    mad_am_enabled = on;
    set_auto_mouse_enable(on);
}

static uint16_t mad_automouse_clamp_timeout(uint16_t ms) {
    if (ms < MAD_AUTOMOUSE_TIMEOUT_MIN) return MAD_AUTOMOUSE_TIMEOUT_MIN;
    if (ms > MAD_AUTOMOUSE_TIMEOUT_MAX) return MAD_AUTOMOUSE_TIMEOUT_MAX;
    return ms;
}

// Live mirror of the core timeout (the core has a setter but no getter).
static uint16_t mad_am_timeout_ms = MAD_AUTOMOUSE_TIMEOUT_DEFAULT;

static void mad_automouse_set_timeout(uint16_t ms) {
    mad_am_timeout_ms = mad_automouse_clamp_timeout(ms);
    set_auto_mouse_timeout(mad_am_timeout_ms);
}

bool auto_mouse_activation(report_mouse_t mouse_report) {
    static int16_t  acc_x, acc_y;
    static uint16_t last_motion;
    if (timer_elapsed(last_motion) > 200) { // still gap: restart the burst
        acc_x = 0;
        acc_y = 0;
    }
    if (mouse_report.x != 0 || mouse_report.y != 0) {
        last_motion = timer_read();
    }
    acc_x += mouse_report.x;
    acc_y += mouse_report.y;
    bool trip = abs(acc_x) > mad_am_threshold || abs(acc_y) > mad_am_threshold || mouse_report.buttons;
    if (trip) {
        acc_x = 0;
        acc_y = 0;
    }
    return trip;
}

static void mad_automouse_apply(void) {
    set_auto_mouse_layer(_MOUSE);
    mad_am_threshold = mad_config.am_threshold;
    mad_automouse_set_timeout(mad_config.am_timeout_ms);
    mad_automouse_set_enabled(mad_config.am_enabled != 0);
}

/* ---------------------------------------------------------------------------
 * Defaults / boot application of the persisted config
 * ------------------------------------------------------------------------- */
static void mad_config_set_defaults(void) {
    mad_config = (mad_config_t){
        .dpi_index            = MADROMYS_DPI_DEFAULT_INDEX,
        .wiggle_interval      = WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT,
        .wiggle_cooldown      = WIGGLE_BALL_TIMEOUT,
        .wiggle_threshold     = WIGGLE_BALL_MOVEMENT_THRESHOLD,
        .accel_enabled        = 1,
        .accel_takeoff_x100   = (uint16_t)(POINTING_DEVICE_ACCEL_TAKEOFF * 100.0f + 0.5f),
        .accel_growth_x100    = (uint16_t)(POINTING_DEVICE_ACCEL_GROWTH_RATE * 100.0f + 0.5f),
        .accel_offset_x100    = (int16_t)(POINTING_DEVICE_ACCEL_OFFSET * 100.0f + (POINTING_DEVICE_ACCEL_OFFSET >= 0 ? 0.5f : -0.5f)),
        .accel_limit_x100     = (uint16_t)(POINTING_DEVICE_ACCEL_LIMIT * 100.0f + 0.5f),
        .smooth_enabled       = 1,
        .smooth_factor_x100   = (uint8_t)(POINTING_DEVICE_SMOOTHING_FACTOR * 100.0f + 0.5f),
        .smooth_timeout_ms    = POINTING_DEVICE_SMOOTHING_RESET_TIMEOUT_MS,
        .gesture_ratchet_step = PD_GESTURES_RATCHET_STEP,
        .drag_div_h           = SCROLL_DIVISOR_H,
        .drag_div_v           = SCROLL_DIVISOR_V,
        .drag_inverted        = DRAG_SCROLL_DEFAULT_INVERTED ? 1 : 0,
        .wiggle_enabled       = WIGGLE_BALL_ENABLED_DEFAULT ? 1 : 0,
        .csk_enabled          = CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT ? 1 : 0,
        .selword_mac          = SELECT_WORD_MAC_DEFAULT ? 1 : 0,
        .sentence_case_on     = SENTENCE_CASE_ON_DEFAULT ? 1 : 0,
        .as_inverted          = AUTOSCROLL_INVERTED_DEFAULT ? 1 : 0,
        .as_speed_scale_x100  = AUTOSCROLL_SPEED_SCALE_X100,
        .as_jog_deadzone      = AUTOSCROLL_JOG_DEADZONE,
        .as_jog_range         = AUTOSCROLL_JOG_RANGE,
        .am_enabled           = MAD_AUTOMOUSE_ENABLED_DEFAULT ? 1 : 0,
        .am_timeout_ms        = MAD_AUTOMOUSE_TIMEOUT_DEFAULT,
        .am_threshold         = MAD_AUTOMOUSE_THRESHOLD_DEFAULT,
        .wc_enabled           = WHEEL_CHORDS_ENABLED_DEFAULT ? 1 : 0,
        .wc_step              = WHEEL_CHORDS_STEP_DEFAULT,
        // wc_table zero-fills (all slots empty) via the struct literal.
        .os_follow            = OS_SHORTCUTS_FOLLOW_DEFAULT ? 1 : 0,
        .os_mac               = OS_SHORTCUTS_MAC_DEFAULT ? 1 : 0,
        .wc_hold_ms           = WHEEL_CHORDS_HOLD_MS_DEFAULT,
    };
    // The struct literal above zero-fills gesture_sets (KC_NO everywhere) and
    // likewise the custom-shift-key table and leader sequences (all empty);
    // seed gesture sets 1-4 with the firmware defaults, leave 5-8 empty.
    for (uint8_t s = 0; s < 4; s++) {
        for (uint8_t d = 0; d < PD_GESTURES_NUM_DIRECTIONS; d++) {
            mad_config.gesture_sets[s][d] = gesture_set_defaults[s][d];
        }
    }
}

// Push every persisted value into the modules' runtime state. Module setters
// clamp their own ranges, so mildly out-of-range EEPROM content degrades to a
// clamped value rather than misbehavior.
static void mad_config_apply(void) {
    if (mad_config.dpi_index >= DPI_COUNT) {
        mad_config.dpi_index = MADROMYS_DPI_DEFAULT_INDEX;
    }
    dpi_apply();
    set_wiggle_ball_direction_switch_timeout(mad_config.wiggle_interval);
    set_wiggle_ball_timeout(mad_config.wiggle_cooldown);
    set_wiggle_ball_movement_threshold(mad_config.wiggle_threshold);
    set_wiggle_ball_enabled(mad_config.wiggle_enabled != 0);
    pd_accel_set_enabled(mad_config.accel_enabled != 0);
    pd_accel_set_takeoff((float)mad_config.accel_takeoff_x100 / 100.0f);
    pd_accel_set_growth_rate((float)mad_config.accel_growth_x100 / 100.0f);
    pd_accel_set_offset((float)mad_config.accel_offset_x100 / 100.0f);
    pd_accel_set_limit((float)mad_config.accel_limit_x100 / 100.0f);
    pointing_device_smoothing_set_enabled(mad_config.smooth_enabled != 0);
    pointing_device_smoothing_set_factor((float)mad_config.smooth_factor_x100 / 100.0f);
    pointing_device_smoothing_set_reset_timeout(mad_config.smooth_timeout_ms);
    pd_gestures_set_ratchet_step(mad_config.gesture_ratchet_step);
    memcpy(gesture_sets, mad_config.gesture_sets, sizeof(gesture_sets));
    set_drag_scroll_h_divisor((int8_t)mad_config.drag_div_h);
    set_drag_scroll_v_divisor((int8_t)mad_config.drag_div_v);
    set_drag_scroll_inverted(mad_config.drag_inverted != 0);
    custom_shift_keys_set_enabled(mad_config.csk_enabled != 0);
    memcpy(custom_shift_keys_table(), mad_config.csk_table, sizeof(mad_config.csk_table));
    select_word_set_mac(mad_config.selword_mac != 0);
    if (mad_config.sentence_case_on) {
        sentence_case_on();
    } else {
        sentence_case_off();
    }
    memcpy(leader_seqs, mad_config.leader_seqs, sizeof(leader_seqs));
    set_autoscroll_inverted(mad_config.as_inverted != 0);
    set_autoscroll_speed_scale(mad_config.as_speed_scale_x100);
    set_autoscroll_jog_deadzone(mad_config.as_jog_deadzone);
    set_autoscroll_jog_range(mad_config.as_jog_range);
    mad_automouse_apply();
    wheel_chords_set_enabled(mad_config.wc_enabled != 0);
    wheel_chords_set_step(mad_config.wc_step);
    wheel_chords_set_hold_ms(mad_config.wc_hold_ms);
    memcpy(wheel_chords_table(), mad_config.wc_table, sizeof(mad_config.wc_table));
    // Order matters: set the pinned mode first, then follow — enabling
    // follow re-applies any detection that fired before this ran.
    os_shortcuts_set_mac(mad_config.os_mac != 0);
    os_shortcuts_set_follow(mad_config.os_follow != 0);
}

/* ---------------------------------------------------------------------------
 * Pointing device pipeline
 *
 * smoothing -> wiggle -> gestures -> drag scroll -> acceleration
 *
 * Wiggle must see raw x/y, so it runs before gestures (which zero x/y while a
 * gesture is open) and drag scroll (which divides x/y into wheel output).
 * While a gesture is open it swallows movement (zeroes x/y); while drag scroll
 * is active it converts movement into wheel events and zeroes X/Y (so
 * acceleration, which skips zero reports, leaves scrolling untouched).
 * ------------------------------------------------------------------------- */
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    pipeline_diag_tick(); // freeze diagnostic: gap watermark (channel 0x1F)
    mouse_report = pointing_device_smoothing_apply(mouse_report);
    mouse_report = wiggle_ball_apply(mouse_report);
    // Wheel chords before gestures: a held button is a more deliberate
    // intent than a latched gesture set, so it wins the motion.
    mouse_report = wheel_chords_apply(mouse_report);
    mouse_report = pd_gestures_apply(mouse_report);
    // Autoscroll before drag scroll: jog mode swallows raw ball motion the
    // same way drag scroll does; stepped mode only adds timed wheel ticks.
    mouse_report = autoscroll_apply(mouse_report);
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
             "DragScroll: h=%d v=%d active=%s inverted=%s\n"
             "Wiggle: interval=%ums cooldown=%ums thresh=%u\n"
             "Typing: csk=%s sentcase=%s selword=%s\n",
             (unsigned)dpi_options[mad_config.dpi_index], (unsigned)mad_config.dpi_index + 1, (unsigned)DPI_COUNT,
             pd_accel_get_enabled() ? "ON" : "OFF", tko, grw, ofs, lmt,
             pointing_device_smoothing_get_enabled() ? "ON" : "OFF", smf,
             (unsigned)pointing_device_smoothing_get_reset_timeout(),
             (unsigned)pd_gestures_get_ratchet_step(), pd_gestures_is_active() ? "ON" : "OFF",
             (int)get_drag_scroll_h_divisor(), (int)get_drag_scroll_v_divisor(),
             get_drag_scroll_scrolling() ? "ON" : "OFF", get_drag_scroll_inverted() ? "ON" : "OFF",
             (unsigned)get_wiggle_ball_direction_switch_timeout(),
             (unsigned)get_wiggle_ball_timeout(), (unsigned)get_wiggle_ball_movement_threshold(),
             custom_shift_keys_get_enabled() ? "ON" : "OFF",
             is_sentence_case_on() ? "ON" : "OFF",
             select_word_get_mac() ? "mac" : "win");
    send_string(buf);
}

/* ---------------------------------------------------------------------------
 * Keycode handling
 * ------------------------------------------------------------------------- */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Wheel chords track physical BTN1..BTN8 state, including the tap half
    // of layer-taps (LT(x, BTNn) resolves here with tap.count set). Tracked
    // before anything can consume the event so held state can't go stale.
    {
        uint16_t base = keycode;
        if ((keycode & 0xF000) == QK_LAYER_TAP && record->tap.count) {
            base = keycode & 0xFF;
        } else if ((keycode & 0xE000) == QK_MOD_TAP && record->tap.count) {
            base = keycode & 0xFF;
        }
        if (base >= MS_BTN1 && base <= MS_BTN8) { // KC_BTN aliases stop at 5
            wheel_chords_on_button(base - MS_BTN1, record->event.pressed);
        }
    }

    // Ported getreuer modules see every key event (after the auto-exit blocks
    // below would be wrong — they must observe even events those blocks react
    // to, and none of the three consumes trackball control keycodes anyway).
    // custom_shift_keys may consume the event (Shift+key replacement).
    if (!process_record_custom_shift_keys(keycode, record)) {
        return false;
    }
    process_record_sentence_case(keycode, record); // state tracking; never consumes
    select_word_on_record(keycode, record);        // state tracking; never consumes
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
    // Autoscroll auto-exit: any key press except its own controls stops it
    // (same convention as drag scroll and gestures below). The press still
    // performs its normal action.
    if (record->event.pressed && autoscroll_is_active()) {
        switch (keycode) {
            case ASC_JOG:
            case ASC_UP:
            case ASC_DOWN:
                break; // autoscroll controls keep their own behavior
            default:
                autoscroll_stop();
                break;
        }
    }
#ifdef GESTURE_AUTO_EXIT
    // Safe exit: any button press except the gesture controls cancels an active
    // gesture. The press is not consumed, so it still does its normal job. The
    // gesture controls keep their own semantics (toggle off / switch set / hold).
    if (record->event.pressed && pd_gestures_is_active()) {
        switch (keycode) {
            case GR1_TOG:
            case GR2_TOG:
            case GR3_TOG:
            case GR4_TOG:
            case GR5_TOG:
            case GR6_TOG:
            case GR7_TOG:
            case GR8_TOG:
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
        // One explicit case per gesture toggle (not a `case A ... B` range) so
        // check.sh's per-keycode handler-coverage grep keeps working.
        case GR1_TOG:
            if (record->event.pressed) gesture_toggle_set(0);
            return false;
        case GR2_TOG:
            if (record->event.pressed) gesture_toggle_set(1);
            return false;
        case GR3_TOG:
            if (record->event.pressed) gesture_toggle_set(2);
            return false;
        case GR4_TOG:
            if (record->event.pressed) gesture_toggle_set(3);
            return false;
        case GR5_TOG:
            if (record->event.pressed) gesture_toggle_set(4);
            return false;
        case GR6_TOG:
            if (record->event.pressed) gesture_toggle_set(5);
            return false;
        case GR7_TOG:
            if (record->event.pressed) gesture_toggle_set(6);
            return false;
        case GR8_TOG:
            if (record->event.pressed) gesture_toggle_set(7);
            return false;
        case SELWORD: {
            // Shift+SELWORD selects the whole line (upstream behavior).
            if (record->event.pressed) {
                const bool shifted = (get_mods() | get_weak_mods() | get_oneshot_mods()) & MOD_MASK_SHIFT;
                select_word_register(shifted ? 'L' : 'W');
            } else {
                select_word_unregister();
            }
            return false;
        }
        case SELWDBK:
            if (record->event.pressed) select_word_register('B');
            else select_word_unregister();
            return false;
        case SELLINE:
            if (record->event.pressed) select_word_register('L');
            else select_word_unregister();
            return false;
        case SELLNUP:
            if (record->event.pressed) select_word_register('U');
            else select_word_unregister();
            return false;
        case SC_TOG:
            if (record->event.pressed) sentence_case_toggle();
            return false;
        case CSK_TOG:
            if (record->event.pressed) custom_shift_keys_set_enabled(!custom_shift_keys_get_enabled());
            return false;
        case ASC_JOG:
            if (record->event.pressed) autoscroll_jog_toggle();
            return false;
        case ASC_UP:
            if (record->event.pressed) autoscroll_step(1);
            return false;
        case ASC_DOWN:
            if (record->event.pressed) autoscroll_step(-1);
            return false;
        case OS_CUT:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_CUT);
            return false;
        case OS_COPY:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_COPY);
            return false;
        case OS_PSTE:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_PASTE);
            return false;
        case OS_UNDO:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_UNDO);
            return false;
        case OS_REDO:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_REDO);
            return false;
    }
    return true;
}

// OS detection result (weak default at quantum/os_detection.c:124). Feeds
// the os_shortcuts mode and, while follow is on, mirrors the mac/pc answer
// into select word's hotkey style so one detection drives both.
bool process_detected_host_os_kb(os_variant_t detected_os) {
    if (!process_detected_host_os_user(detected_os)) return false;
    if (os_shortcuts_on_detect(detected_os)) {
        select_word_set_mac(os_shortcuts_get_mac());
    }
    return true;
}

// Idle-timeout ticks for the ported getreuer modules (no other
// housekeeping_task_user existed in this keymap before this).
void housekeeping_task_user(void) {
    select_word_task();
    sentence_case_task();
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

// Called on a full eeconfig reset (EE_CLR / first boot after eeconfig magic
// mismatch). The core zeroes the datablock and writes its validity word first;
// this then seeds our defaults over the zeros.
void eeconfig_init_user(void) {
    mad_config_set_defaults();
    mad_config_write();
}

void keyboard_post_init_user(void) {
    pd_accel_init(); // module's own init; mad_config_apply overwrites with persisted values

    if (eeconfig_is_user_datablock_valid()) {
        eeconfig_read_user_datablock(&mad_config, 0, sizeof(mad_config));
        if (mad_config.dpi_index >= DPI_COUNT) {
            // Wholesale-garbage canary: version word matched but content is
            // implausible — re-seed everything rather than trust the rest.
            mad_config_set_defaults();
            mad_config_write();
        }
    } else {
        // Invalid = fresh chip, EECONFIG_USER_DATA_VERSION bump, or layout
        // migration. Seed defaults; the write marks the block valid.
        mad_config_set_defaults();
        mad_config_write();
    }
    mad_config_apply();
}

/* ---------------------------------------------------------------------------
 * Companion-app raw HID tuning protocol
 *
 * VIA_CUSTOM_LIGHTING_ENABLE (config.h) makes quantum/via.c route command IDs
 * 0x07/0x08/0x09 (id_custom_set_value / id_custom_get_value / id_custom_save)
 * here. Frame: data[0]=command, data[1]=channel, data[2]=value_id,
 * data[3..]=value (u16 big-endian, x100-scaled for float parameters, offset
 * is the one signed field). Unknown channel/value/command → data[0] is set to
 * id_unhandled (0xFF) so the host can tell. via.c echoes the buffer back to
 * the host after this returns — do NOT call raw_hid_send() here
 * (quantum/via.c:188 defines the weak default this overrides).
 *
 * Channels 0x10+ dodge VIA's reserved channel IDs 0-5. GET/SET always act on
 * the module's live runtime state; SAVE snapshots that live state into
 * mad_config and persists the whole datablock in one write (so unrelated
 * fields never go stale relative to what's actually running).
 * ------------------------------------------------------------------------- */
// v2 = added gesture set slots (channel 0x11, value_ids 0x10..0x2F).
// v3 = gestures grew to 8 directions (diagonal slots appended at 0x30..0x4F;
//      cardinal ids unchanged) + gestures active-set (0x11/0x02, settable) +
//      wiggle enabled kill switch (0x12/0x04) + drag-scroll live state
//      (0x15/0x04, settable, not persisted).
// v4 = getreuer-module channels: custom shift keys (0x16), select word
//      (0x17), sentence case (0x18), leader sequences (0x19).
// v5 = autoscroll channel (0x1A).
// v6 = auto-mouse channel (0x1B) + wheel-chords channel (0x1C).
// v7 = OS-aware shortcuts channel (0x1D: follow, mac mode, detected OS RO)
//      + freeze-diagnostic channel (0x1F: pointing-gap watermark, uptime).
//      (0x1E is num word, Svalboard-only — unhandled here.)
// v8 = wheel-chords hold delay (0x1C/0x03: ms held before capture engages).
#define MAD_HID_PROTOCOL_VERSION 8

enum mad_hid_channel {
    mad_ch_meta       = 0x00, // 0x01: protocol version (read-only)
    mad_ch_accel      = 0x10,
    mad_ch_gestures   = 0x11,
    mad_ch_wiggle     = 0x12,
    mad_ch_smoothing  = 0x13,
    mad_ch_dpi        = 0x14,
    mad_ch_dragscroll = 0x15,
    mad_ch_csk        = 0x16, // custom shift keys
    mad_ch_selword    = 0x17, // select word
    mad_ch_sentence   = 0x18, // sentence case
    mad_ch_leader     = 0x19, // leader sequences
    mad_ch_autoscroll = 0x1A, // autoscroll (jog + stepped)
    mad_ch_automouse  = 0x1B, // QMK core auto-mouse (v6)
    mad_ch_wheelchords = 0x1C, // button-held ball gestures (v6)
    mad_ch_os         = 0x1D, // OS-aware shortcuts (v7)
    mad_ch_diag       = 0x1F, // freeze diagnostic (v7; nothing persists)
};

enum mad_hid_meta_value {
    mad_meta_protocol_version = 0x01,
};

enum mad_hid_accel_value {
    mad_accel_enabled = 0x01,
    mad_accel_takeoff = 0x02,
    mad_accel_growth  = 0x03,
    mad_accel_offset  = 0x04, // signed
    mad_accel_limit   = 0x05,
};

enum mad_hid_gestures_value {
    mad_gestures_ratchet_step = 0x01,
    // Currently latched set. GET: set index, or 0xFF when none. SET: 0xFF
    // cancels any active gesture; a set index toggles that set (same path and
    // empty-set guard as the GR#_TOG keycodes). Echoes the resulting state.
    mad_gestures_active_set = 0x02,
    // Gesture set slot keycodes. Wire ids are append-only across protocol
    // versions, so the v2 cardinal ids kept their meaning when v3 added
    // diagonals in a second block:
    //   0x10 + set*4 + c   (c: 0=E 1=S 2=W 3=N)     -> internal dir c*2
    //   0x30 + set*4 + d   (d: 0=SE 1=SW 2=NW 3=NE) -> internal dir d*2+1
    // Payload is the raw QMK keycode (u16 BE), unclamped — any 16-bit code is
    // stored; what actually FIRES is limited by tap_code16 (see gesture_sets).
    mad_gestures_slot_base = 0x10,
    mad_gestures_diag_base = 0x30,
};

// Decodes a gesture-slot wire id into (set, internal direction). Returns
// false for ids outside both slot blocks.
static bool mad_gesture_slot_decode(uint8_t value_id, uint8_t *set, uint8_t *dir) {
    if (value_id >= mad_gestures_slot_base && value_id < mad_gestures_slot_base + MAD_GESTURE_SET_COUNT * 4) {
        uint8_t slot = value_id - mad_gestures_slot_base;
        *set         = slot / 4;
        *dir         = (slot % 4) * 2; // cardinals sit at even internal indices
        return true;
    }
    if (value_id >= mad_gestures_diag_base && value_id < mad_gestures_diag_base + MAD_GESTURE_SET_COUNT * 4) {
        uint8_t slot = value_id - mad_gestures_diag_base;
        *set         = slot / 4;
        *dir         = (slot % 4) * 2 + 1; // diagonals at odd indices
        return true;
    }
    return false;
}

// Index of the currently latched gesture set, or 0xFF when none is active.
static uint8_t gesture_active_set(void) {
    for (uint8_t s = 0; s < MAD_GESTURE_SET_COUNT; s++) {
        if (pd_gestures_is_active_table(gesture_sets[s])) return s;
    }
    return 0xFF;
}

enum mad_hid_wiggle_value {
    mad_wiggle_interval  = 0x01,
    mad_wiggle_cooldown  = 0x02,
    mad_wiggle_threshold = 0x03,
    mad_wiggle_enabled   = 0x04, // v3: shake-detection kill switch
};

enum mad_hid_smoothing_value {
    mad_smoothing_enabled = 0x01,
    mad_smoothing_factor  = 0x02,
    mad_smoothing_timeout = 0x03,
};

enum mad_hid_dpi_value {
    mad_dpi_index = 0x01,
};

enum mad_hid_dragscroll_value {
    mad_dragscroll_div_h    = 0x01,
    mad_dragscroll_div_v    = 0x02,
    mad_dragscroll_inverted = 0x03,
    // v3: live scrolling state. GET answers "is drag scroll on right now" —
    // the diagnostic for the accidental-freeze reports. SET forces it on/off
    // (rescue switch). Deliberately NOT persisted by save.
    mad_dragscroll_active   = 0x04,
};

enum mad_hid_csk_value {
    mad_csk_enabled    = 0x01,
    mad_csk_slot_count = 0x02, // read-only: CUSTOM_SHIFT_KEYS_SLOTS
    // Slot keycodes (raw QMK keycodes, u16 BE). Two blocks, same append-only
    // spirit as the gesture slot ids: base keycode at 0x10+slot, its shifted
    // replacement at 0x30+slot. KC_NO base = empty slot.
    mad_csk_key_base   = 0x10,
    mad_csk_shift_base = 0x30,
};

enum mad_hid_selword_value {
    mad_selword_mac = 0x01, // 1 = macOS hotkeys (Alt/Cmd), 0 = Ctrl/Home/End
};

enum mad_hid_sentence_value {
    mad_sentence_enabled = 0x01, // live on/off; persisted by save
};

enum mad_hid_leader_value {
    // Sequence slots: 0x10 + seq*8 + pos. pos 0..4 = sequence keys (KC_NO
    // pads the unused tail), pos 5 = output keycode (KC_NO = slot empty).
    // pos 6..7 reserved. Raw QMK keycodes, u16 BE, unclamped — what actually
    // FIRES is limited by tap_code16 (see leader_seqs).
    mad_leader_slot_base = 0x10,
};

enum mad_hid_autoscroll_value {
    mad_as_inverted    = 0x01,
    mad_as_speed_scale = 0x02, // x100; 100 = Ben White's table as-is
    mad_as_deadzone    = 0x03, // jog: counts ignored around center
    mad_as_range       = 0x04, // jog: counts past deadzone to full speed
    // Live state (never persisted): GET = signed stepped level as i16, or
    // ±100 while jogging, 0 idle; SET 0 force-stops (rescue switch, same
    // spirit as dragscroll 0x04).
    mad_as_state       = 0x05,
};

enum mad_hid_automouse_value {
    mad_am_enabled_id  = 0x01,
    mad_am_timeout_id  = 0x02, // ms, clamped [MAD_AUTOMOUSE_TIMEOUT_MIN, MAX]
    mad_am_threshold_id = 0x03, // accumulated counts, clamped [0, MAX]
};

enum mad_hid_wheelchords_value {
    mad_wc_enabled_id = 0x01,
    mad_wc_step_id    = 0x02, // counts per gesture fire
    mad_wc_hold_ms_id = 0x03, // v8: ms held before capture engages (0 = immediate)
    // Slot keycodes: 0x10 + button*8 + dir (button 0..7 = BTN1..BTN8,
    // dir = internal pd_gestures order E SE S SW W NW N NE). Raw QMK
    // keycode payload, unclamped — what fires is limited by tap_code16.
    mad_wc_slot_base  = 0x10,
};

enum mad_hid_os_value {
    mad_os_follow_id   = 0x01, // bool: OS detection drives the mac/pc mode
    mad_os_mac_id      = 0x02, // bool: live mac/pc mode (manual set while follow is on lasts until the next detection)
    mad_os_detected_id = 0x03, // RO: raw os_variant_t (0 unsure, 1 linux, 2 windows, 3 macos, 4 ios)
};

enum mad_hid_diag_value {
    mad_diag_max_gap_id = 0x01, // GET: largest ms gap between pointing passes since reset; SET (any) resets
    mad_diag_uptime_id  = 0x02, // RO: seconds since boot (u16, wraps ~18 h)
};

// Decodes a leader-slot wire id into (sequence, position). pos 0..4 are the
// sequence keys, 5 is the output. Returns false outside the slot block.
static bool mad_leader_slot_decode(uint8_t value_id, uint8_t *seq, uint8_t *pos) {
    if (value_id < mad_leader_slot_base) return false;
    uint8_t slot = value_id - mad_leader_slot_base;
    uint8_t s    = slot / 8;
    uint8_t p    = slot % 8;
    if (s >= MAD_LEADER_SEQ_COUNT || p > MAD_LEADER_SEQ_KEYS) return false;
    *seq = s;
    *pos = p;
    return true;
}

static void mad_hid_write_u16(uint8_t *payload, uint16_t value) {
    payload[0] = value >> 8;
    payload[1] = value & 0xFF;
}

static uint16_t mad_hid_read_u16(const uint8_t *payload) {
    return ((uint16_t)payload[0] << 8) | payload[1];
}

static void mad_hid_write_i16(uint8_t *payload, int16_t value) {
    mad_hid_write_u16(payload, (uint16_t)value);
}

static int16_t mad_hid_read_i16(const uint8_t *payload) {
    return (int16_t)mad_hid_read_u16(payload);
}

// GET: read straight from the module's live runtime state (not mad_config —
// that's only the last-saved snapshot). Returns true if handled.
static bool mad_hid_get(uint8_t channel, uint8_t value_id, uint8_t *payload) {
    switch (channel) {
        case mad_ch_meta:
            if (value_id == mad_meta_protocol_version) {
                mad_hid_write_u16(payload, MAD_HID_PROTOCOL_VERSION);
                return true;
            }
            return false;

        case mad_ch_accel:
            switch (value_id) {
                case mad_accel_enabled: mad_hid_write_u16(payload, pd_accel_get_enabled() ? 1 : 0); return true;
                case mad_accel_takeoff: mad_hid_write_u16(payload, f_to_x100(pd_accel_get_takeoff())); return true;
                case mad_accel_growth: mad_hid_write_u16(payload, f_to_x100(pd_accel_get_growth_rate())); return true;
                case mad_accel_offset: mad_hid_write_i16(payload, f_to_x100_signed(pd_accel_get_offset())); return true;
                case mad_accel_limit: mad_hid_write_u16(payload, f_to_x100(pd_accel_get_limit())); return true;
                default: return false;
            }

        case mad_ch_gestures: {
            if (value_id == mad_gestures_ratchet_step) {
                mad_hid_write_u16(payload, pd_gestures_get_ratchet_step());
                return true;
            }
            if (value_id == mad_gestures_active_set) {
                mad_hid_write_u16(payload, gesture_active_set());
                return true;
            }
            uint8_t set, dir;
            if (mad_gesture_slot_decode(value_id, &set, &dir)) {
                mad_hid_write_u16(payload, gesture_sets[set][dir]);
                return true;
            }
            return false;
        }

        case mad_ch_wiggle:
            switch (value_id) {
                case mad_wiggle_interval: mad_hid_write_u16(payload, get_wiggle_ball_direction_switch_timeout()); return true;
                case mad_wiggle_cooldown: mad_hid_write_u16(payload, get_wiggle_ball_timeout()); return true;
                case mad_wiggle_threshold: mad_hid_write_u16(payload, get_wiggle_ball_movement_threshold()); return true;
                case mad_wiggle_enabled: mad_hid_write_u16(payload, get_wiggle_ball_enabled() ? 1 : 0); return true;
                default: return false;
            }

        case mad_ch_smoothing:
            switch (value_id) {
                case mad_smoothing_enabled: mad_hid_write_u16(payload, pointing_device_smoothing_get_enabled() ? 1 : 0); return true;
                case mad_smoothing_factor: mad_hid_write_u16(payload, f_to_x100(pointing_device_smoothing_get_factor())); return true;
                case mad_smoothing_timeout: mad_hid_write_u16(payload, pointing_device_smoothing_get_reset_timeout()); return true;
                default: return false;
            }

        case mad_ch_dpi:
            if (value_id == mad_dpi_index) {
                mad_hid_write_u16(payload, mad_config.dpi_index);
                return true;
            }
            return false;

        case mad_ch_dragscroll:
            switch (value_id) {
                case mad_dragscroll_div_h: mad_hid_write_u16(payload, (uint8_t)get_drag_scroll_h_divisor()); return true;
                case mad_dragscroll_div_v: mad_hid_write_u16(payload, (uint8_t)get_drag_scroll_v_divisor()); return true;
                case mad_dragscroll_inverted: mad_hid_write_u16(payload, get_drag_scroll_inverted() ? 1 : 0); return true;
                case mad_dragscroll_active: mad_hid_write_u16(payload, get_drag_scroll_scrolling() ? 1 : 0); return true;
                default: return false;
            }

        case mad_ch_csk:
            if (value_id == mad_csk_enabled) {
                mad_hid_write_u16(payload, custom_shift_keys_get_enabled() ? 1 : 0);
                return true;
            }
            if (value_id == mad_csk_slot_count) {
                mad_hid_write_u16(payload, CUSTOM_SHIFT_KEYS_SLOTS);
                return true;
            }
            if (value_id >= mad_csk_key_base && value_id < mad_csk_key_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                mad_hid_write_u16(payload, custom_shift_keys_get_slot_keycode(value_id - mad_csk_key_base));
                return true;
            }
            if (value_id >= mad_csk_shift_base && value_id < mad_csk_shift_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                mad_hid_write_u16(payload, custom_shift_keys_get_slot_shifted(value_id - mad_csk_shift_base));
                return true;
            }
            return false;

        case mad_ch_selword:
            if (value_id == mad_selword_mac) {
                mad_hid_write_u16(payload, select_word_get_mac() ? 1 : 0);
                return true;
            }
            return false;

        case mad_ch_sentence:
            if (value_id == mad_sentence_enabled) {
                mad_hid_write_u16(payload, is_sentence_case_on() ? 1 : 0);
                return true;
            }
            return false;

        case mad_ch_leader: {
            uint8_t seq, pos;
            if (mad_leader_slot_decode(value_id, &seq, &pos)) {
                mad_hid_write_u16(payload, leader_seqs[seq][pos]);
                return true;
            }
            return false;
        }

        case mad_ch_autoscroll:
            switch (value_id) {
                case mad_as_inverted: mad_hid_write_u16(payload, get_autoscroll_inverted() ? 1 : 0); return true;
                case mad_as_speed_scale: mad_hid_write_u16(payload, get_autoscroll_speed_scale()); return true;
                case mad_as_deadzone: mad_hid_write_u16(payload, get_autoscroll_jog_deadzone()); return true;
                case mad_as_range: mad_hid_write_u16(payload, get_autoscroll_jog_range()); return true;
                case mad_as_state:
                    mad_hid_write_i16(payload, autoscroll_is_jogging() ? 100 : autoscroll_get_level());
                    return true;
                default: return false;
            }

        case mad_ch_automouse:
            switch (value_id) {
                case mad_am_enabled_id: mad_hid_write_u16(payload, mad_am_enabled ? 1 : 0); return true;
                case mad_am_timeout_id: mad_hid_write_u16(payload, mad_am_timeout_ms); return true;
                case mad_am_threshold_id: mad_hid_write_u16(payload, mad_am_threshold); return true;
                default: return false;
            }

        case mad_ch_wheelchords: {
            if (value_id == mad_wc_enabled_id) {
                mad_hid_write_u16(payload, wheel_chords_get_enabled() ? 1 : 0);
                return true;
            }
            if (value_id == mad_wc_step_id) {
                mad_hid_write_u16(payload, wheel_chords_get_step());
                return true;
            }
            if (value_id == mad_wc_hold_ms_id) {
                mad_hid_write_u16(payload, wheel_chords_get_hold_ms());
                return true;
            }
            if (value_id >= mad_wc_slot_base && value_id < mad_wc_slot_base + WHEEL_CHORDS_BUTTONS * 8) {
                uint8_t slot = value_id - mad_wc_slot_base;
                uint8_t btn = slot / 8, dir = slot % 8;
                if (dir < WHEEL_CHORDS_DIRECTIONS) {
                    mad_hid_write_u16(payload, wheel_chords_table()[btn][dir]);
                    return true;
                }
            }
            return false;
        }

        case mad_ch_os:
            switch (value_id) {
                case mad_os_follow_id: mad_hid_write_u16(payload, os_shortcuts_get_follow() ? 1 : 0); return true;
                case mad_os_mac_id: mad_hid_write_u16(payload, os_shortcuts_get_mac() ? 1 : 0); return true;
                case mad_os_detected_id: mad_hid_write_u16(payload, os_shortcuts_detected()); return true;
                default: return false;
            }

        case mad_ch_diag:
            switch (value_id) {
                case mad_diag_max_gap_id: mad_hid_write_u16(payload, pipeline_diag_max_gap_ms()); return true;
                case mad_diag_uptime_id: mad_hid_write_u16(payload, pipeline_diag_uptime_s()); return true;
                default: return false;
            }

        default:
            return false;
    }
}

// SET: writes straight into the module's live runtime state via its own
// clamped setter, then echoes back what actually took effect (the setter may
// have clamped the requested value). Returns true if handled.
static bool mad_hid_set(uint8_t channel, uint8_t value_id, uint8_t *payload) {
    switch (channel) {
        case mad_ch_accel:
            switch (value_id) {
                case mad_accel_enabled: pd_accel_set_enabled(mad_hid_read_u16(payload) != 0); mad_hid_write_u16(payload, pd_accel_get_enabled() ? 1 : 0); return true;
                case mad_accel_takeoff: pd_accel_set_takeoff((float)mad_hid_read_u16(payload) / 100.0f); mad_hid_write_u16(payload, f_to_x100(pd_accel_get_takeoff())); return true;
                case mad_accel_growth: pd_accel_set_growth_rate((float)mad_hid_read_u16(payload) / 100.0f); mad_hid_write_u16(payload, f_to_x100(pd_accel_get_growth_rate())); return true;
                case mad_accel_offset: pd_accel_set_offset((float)mad_hid_read_i16(payload) / 100.0f); mad_hid_write_i16(payload, f_to_x100_signed(pd_accel_get_offset())); return true;
                case mad_accel_limit: pd_accel_set_limit((float)mad_hid_read_u16(payload) / 100.0f); mad_hid_write_u16(payload, f_to_x100(pd_accel_get_limit())); return true;
                default: return false;
            }

        case mad_ch_gestures: {
            if (value_id == mad_gestures_ratchet_step) {
                pd_gestures_set_ratchet_step(mad_hid_read_u16(payload));
                mad_hid_write_u16(payload, pd_gestures_get_ratchet_step());
                return true;
            }
            if (value_id == mad_gestures_active_set) {
                // 0xFF = cancel; a set index toggles that set through the same
                // guarded path as the GR#_TOG keycodes. Echo the result so the
                // app sees what actually happened (guard may refuse a start).
                uint16_t requested = mad_hid_read_u16(payload);
                if (requested == 0xFF) {
                    pd_gestures_cancel();
                } else if (requested < MAD_GESTURE_SET_COUNT) {
                    gesture_toggle_set((uint8_t)requested);
                }
                mad_hid_write_u16(payload, gesture_active_set());
                return true;
            }
            uint8_t set, dir;
            if (mad_gesture_slot_decode(value_id, &set, &dir)) {
                // In-place edit — the engine holds the table by pointer, so
                // an active gesture picks the new keycode up on its next fire.
                gesture_sets[set][dir] = mad_hid_read_u16(payload);
                return true;
            }
            return false;
        }

        case mad_ch_wiggle:
            switch (value_id) {
                case mad_wiggle_interval: set_wiggle_ball_direction_switch_timeout(mad_hid_read_u16(payload)); mad_hid_write_u16(payload, get_wiggle_ball_direction_switch_timeout()); return true;
                case mad_wiggle_cooldown: set_wiggle_ball_timeout(mad_hid_read_u16(payload)); mad_hid_write_u16(payload, get_wiggle_ball_timeout()); return true;
                case mad_wiggle_threshold: {
                    // Clamp in u16 space BEFORE narrowing: a bare (uint8_t)
                    // cast wraps at 256, sneaking a wrong value past the
                    // setter's clamp (same wrap class as the divisor bug
                    // found in Phase 2 hardware verification).
                    uint16_t requested = mad_hid_read_u16(payload);
                    if (requested > WIGGLE_BALL_THRESHOLD_MAX) requested = WIGGLE_BALL_THRESHOLD_MAX;
                    set_wiggle_ball_movement_threshold((uint8_t)requested);
                    mad_hid_write_u16(payload, get_wiggle_ball_movement_threshold());
                    return true;
                }
                case mad_wiggle_enabled:
                    set_wiggle_ball_enabled(mad_hid_read_u16(payload) != 0);
                    mad_hid_write_u16(payload, get_wiggle_ball_enabled() ? 1 : 0);
                    return true;
                default: return false;
            }

        case mad_ch_smoothing:
            switch (value_id) {
                case mad_smoothing_enabled: pointing_device_smoothing_set_enabled(mad_hid_read_u16(payload) != 0); mad_hid_write_u16(payload, pointing_device_smoothing_get_enabled() ? 1 : 0); return true;
                case mad_smoothing_factor: pointing_device_smoothing_set_factor((float)mad_hid_read_u16(payload) / 100.0f); mad_hid_write_u16(payload, f_to_x100(pointing_device_smoothing_get_factor())); return true;
                case mad_smoothing_timeout: pointing_device_smoothing_set_reset_timeout(mad_hid_read_u16(payload)); mad_hid_write_u16(payload, pointing_device_smoothing_get_reset_timeout()); return true;
                default: return false;
            }

        case mad_ch_dpi:
            // DPI persists immediately (matches DPI_UP/DOWN/CONFIG keycode
            // behavior) rather than waiting for an explicit save.
            if (value_id == mad_dpi_index) {
                uint16_t requested = mad_hid_read_u16(payload);
                if (requested >= DPI_COUNT) requested = DPI_COUNT - 1;
                mad_config.dpi_index = (uint8_t)requested;
                dpi_save_and_apply();
                mad_hid_write_u16(payload, mad_config.dpi_index);
                return true;
            }
            return false;

        case mad_ch_dragscroll:
            switch (value_id) {
                // Divisors: clamp in u16 space BEFORE the int8 cast. A bare
                // cast wraps values > 127 negative (200 -> -56), which the
                // module clamp then pins to MIN instead of MAX — found on
                // hardware in Phase 2 verification.
                case mad_dragscroll_div_h: {
                    uint16_t requested = mad_hid_read_u16(payload);
                    if (requested > SCROLL_DIVISOR_MAX) requested = SCROLL_DIVISOR_MAX;
                    set_drag_scroll_h_divisor((int8_t)requested);
                    mad_hid_write_u16(payload, (uint8_t)get_drag_scroll_h_divisor());
                    return true;
                }
                case mad_dragscroll_div_v: {
                    uint16_t requested = mad_hid_read_u16(payload);
                    if (requested > SCROLL_DIVISOR_MAX) requested = SCROLL_DIVISOR_MAX;
                    set_drag_scroll_v_divisor((int8_t)requested);
                    mad_hid_write_u16(payload, (uint8_t)get_drag_scroll_v_divisor());
                    return true;
                }
                case mad_dragscroll_inverted: set_drag_scroll_inverted(mad_hid_read_u16(payload) != 0); mad_hid_write_u16(payload, get_drag_scroll_inverted() ? 1 : 0); return true;
                case mad_dragscroll_active:
                    // Live force on/off (rescue switch); never persisted.
                    set_drag_scroll_scrolling(mad_hid_read_u16(payload) != 0);
                    mad_hid_write_u16(payload, get_drag_scroll_scrolling() ? 1 : 0);
                    return true;
                default: return false;
            }

        case mad_ch_csk:
            if (value_id == mad_csk_enabled) {
                custom_shift_keys_set_enabled(mad_hid_read_u16(payload) != 0);
                mad_hid_write_u16(payload, custom_shift_keys_get_enabled() ? 1 : 0);
                return true;
            }
            // slot_count is read-only. Slot keycodes are raw and unclamped
            // (same policy as gesture slots).
            if (value_id >= mad_csk_key_base && value_id < mad_csk_key_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                custom_shift_keys_set_slot_keycode(value_id - mad_csk_key_base, mad_hid_read_u16(payload));
                return true;
            }
            if (value_id >= mad_csk_shift_base && value_id < mad_csk_shift_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                custom_shift_keys_set_slot_shifted(value_id - mad_csk_shift_base, mad_hid_read_u16(payload));
                return true;
            }
            return false;

        case mad_ch_selword:
            if (value_id == mad_selword_mac) {
                select_word_set_mac(mad_hid_read_u16(payload) != 0);
                mad_hid_write_u16(payload, select_word_get_mac() ? 1 : 0);
                return true;
            }
            return false;

        case mad_ch_sentence:
            if (value_id == mad_sentence_enabled) {
                if (mad_hid_read_u16(payload) != 0) {
                    sentence_case_on();
                } else {
                    sentence_case_off();
                }
                mad_hid_write_u16(payload, is_sentence_case_on() ? 1 : 0);
                return true;
            }
            return false;

        case mad_ch_leader: {
            uint8_t seq, pos;
            if (mad_leader_slot_decode(value_id, &seq, &pos)) {
                leader_seqs[seq][pos] = mad_hid_read_u16(payload);
                return true;
            }
            return false;
        }

        case mad_ch_autoscroll:
            switch (value_id) {
                case mad_as_inverted:
                    set_autoscroll_inverted(mad_hid_read_u16(payload) != 0);
                    mad_hid_write_u16(payload, get_autoscroll_inverted() ? 1 : 0);
                    return true;
                case mad_as_speed_scale:
                    set_autoscroll_speed_scale(mad_hid_read_u16(payload));
                    mad_hid_write_u16(payload, get_autoscroll_speed_scale());
                    return true;
                case mad_as_deadzone: {
                    // Clamp in u16 space BEFORE the u8 narrowing (wrap class
                    // found in Phase 2 hardware verification).
                    uint16_t requested = mad_hid_read_u16(payload);
                    if (requested > AUTOSCROLL_JOG_DEADZONE_MAX) requested = AUTOSCROLL_JOG_DEADZONE_MAX;
                    set_autoscroll_jog_deadzone((uint8_t)requested);
                    mad_hid_write_u16(payload, get_autoscroll_jog_deadzone());
                    return true;
                }
                case mad_as_range:
                    set_autoscroll_jog_range(mad_hid_read_u16(payload));
                    mad_hid_write_u16(payload, get_autoscroll_jog_range());
                    return true;
                case mad_as_state:
                    // Rescue switch: any SET stops autoscroll (never persisted).
                    autoscroll_stop();
                    mad_hid_write_i16(payload, 0);
                    return true;
                default: return false;
            }

        case mad_ch_automouse:
            switch (value_id) {
                case mad_am_enabled_id:
                    mad_automouse_set_enabled(mad_hid_read_u16(payload) != 0);
                    mad_hid_write_u16(payload, mad_am_enabled ? 1 : 0);
                    return true;
                case mad_am_timeout_id:
                    mad_automouse_set_timeout(mad_hid_read_u16(payload));
                    mad_hid_write_u16(payload, mad_am_timeout_ms);
                    return true;
                case mad_am_threshold_id: {
                    // Clamp in u16 space BEFORE the u8 narrowing (wrap class
                    // found in Phase 2 hardware verification).
                    uint16_t requested = mad_hid_read_u16(payload);
                    if (requested > MAD_AUTOMOUSE_THRESHOLD_MAX) requested = MAD_AUTOMOUSE_THRESHOLD_MAX;
                    mad_am_threshold = (uint8_t)requested;
                    mad_hid_write_u16(payload, mad_am_threshold);
                    return true;
                }
                default: return false;
            }

        case mad_ch_wheelchords: {
            if (value_id == mad_wc_enabled_id) {
                wheel_chords_set_enabled(mad_hid_read_u16(payload) != 0);
                mad_hid_write_u16(payload, wheel_chords_get_enabled() ? 1 : 0);
                return true;
            }
            if (value_id == mad_wc_step_id) {
                wheel_chords_set_step(mad_hid_read_u16(payload));
                mad_hid_write_u16(payload, wheel_chords_get_step());
                return true;
            }
            if (value_id == mad_wc_hold_ms_id) {
                wheel_chords_set_hold_ms(mad_hid_read_u16(payload));
                mad_hid_write_u16(payload, wheel_chords_get_hold_ms());
                return true;
            }
            if (value_id >= mad_wc_slot_base && value_id < mad_wc_slot_base + WHEEL_CHORDS_BUTTONS * 8) {
                uint8_t slot = value_id - mad_wc_slot_base;
                uint8_t btn = slot / 8, dir = slot % 8;
                if (dir < WHEEL_CHORDS_DIRECTIONS) {
                    // Raw keycode, unclamped (same policy as gesture slots);
                    // what fires is limited by tap_code16.
                    wheel_chords_table()[btn][dir] = mad_hid_read_u16(payload);
                    return true;
                }
            }
            return false;
        }

        case mad_ch_os:
            switch (value_id) {
                case mad_os_follow_id:
                    os_shortcuts_set_follow(mad_hid_read_u16(payload) != 0);
                    mad_hid_write_u16(payload, os_shortcuts_get_follow() ? 1 : 0);
                    return true;
                case mad_os_mac_id:
                    // Mirror into select word too — same coupling as the
                    // detection path, so the app's one switch moves both.
                    os_shortcuts_set_mac(mad_hid_read_u16(payload) != 0);
                    select_word_set_mac(os_shortcuts_get_mac());
                    mad_hid_write_u16(payload, os_shortcuts_get_mac() ? 1 : 0);
                    return true;
                // mad_os_detected_id is read-only.
                default: return false;
            }

        case mad_ch_diag:
            if (value_id == mad_diag_max_gap_id) {
                pipeline_diag_reset(); // any SET resets the watermark
                mad_hid_write_u16(payload, 0);
                return true;
            }
            return false;

        default:
            return false;
    }
}

// SAVE: snapshots the module's current live state into mad_config and
// persists the whole datablock in one write, so channels never drift apart
// in EEPROM relative to what's actually running.
static bool mad_hid_save(uint8_t channel) {
    switch (channel) {
        case mad_ch_accel:
            mad_config.accel_enabled      = pd_accel_get_enabled() ? 1 : 0;
            mad_config.accel_takeoff_x100 = f_to_x100(pd_accel_get_takeoff());
            mad_config.accel_growth_x100  = f_to_x100(pd_accel_get_growth_rate());
            mad_config.accel_offset_x100  = f_to_x100_signed(pd_accel_get_offset());
            mad_config.accel_limit_x100   = f_to_x100(pd_accel_get_limit());
            break;
        case mad_ch_gestures:
            mad_config.gesture_ratchet_step = pd_gestures_get_ratchet_step();
            memcpy(mad_config.gesture_sets, gesture_sets, sizeof(mad_config.gesture_sets));
            break;
        case mad_ch_wiggle:
            mad_config.wiggle_interval  = get_wiggle_ball_direction_switch_timeout();
            mad_config.wiggle_cooldown  = get_wiggle_ball_timeout();
            mad_config.wiggle_threshold = get_wiggle_ball_movement_threshold();
            mad_config.wiggle_enabled   = get_wiggle_ball_enabled() ? 1 : 0;
            break;
        case mad_ch_smoothing:
            mad_config.smooth_enabled     = pointing_device_smoothing_get_enabled() ? 1 : 0;
            mad_config.smooth_factor_x100 = (uint8_t)f_to_x100(pointing_device_smoothing_get_factor());
            mad_config.smooth_timeout_ms  = pointing_device_smoothing_get_reset_timeout();
            break;
        case mad_ch_dpi:
            // Already persisted immediately by mad_hid_set/dpi_save_and_apply;
            // a save call still succeeds (no-op) so the app doesn't need to
            // special-case this channel.
            break;
        case mad_ch_dragscroll:
            mad_config.drag_div_h    = (uint8_t)get_drag_scroll_h_divisor();
            mad_config.drag_div_v    = (uint8_t)get_drag_scroll_v_divisor();
            mad_config.drag_inverted = get_drag_scroll_inverted() ? 1 : 0;
            break;
        case mad_ch_csk:
            mad_config.csk_enabled = custom_shift_keys_get_enabled() ? 1 : 0;
            memcpy(mad_config.csk_table, custom_shift_keys_table(), sizeof(mad_config.csk_table));
            break;
        case mad_ch_selword:
            mad_config.selword_mac = select_word_get_mac() ? 1 : 0;
            break;
        case mad_ch_sentence:
            mad_config.sentence_case_on = is_sentence_case_on() ? 1 : 0;
            break;
        case mad_ch_leader:
            memcpy(mad_config.leader_seqs, leader_seqs, sizeof(mad_config.leader_seqs));
            break;
        case mad_ch_autoscroll:
            mad_config.as_inverted         = get_autoscroll_inverted() ? 1 : 0;
            mad_config.as_speed_scale_x100 = get_autoscroll_speed_scale();
            mad_config.as_jog_deadzone     = get_autoscroll_jog_deadzone();
            mad_config.as_jog_range        = get_autoscroll_jog_range();
            break;
        case mad_ch_automouse:
            mad_config.am_enabled    = mad_am_enabled ? 1 : 0;
            mad_config.am_timeout_ms = mad_am_timeout_ms;
            mad_config.am_threshold  = mad_am_threshold;
            break;
        case mad_ch_wheelchords:
            mad_config.wc_enabled = wheel_chords_get_enabled() ? 1 : 0;
            mad_config.wc_step    = wheel_chords_get_step();
            mad_config.wc_hold_ms = wheel_chords_get_hold_ms();
            memcpy(mad_config.wc_table, wheel_chords_table(), sizeof(mad_config.wc_table));
            break;
        case mad_ch_os:
            mad_config.os_follow = os_shortcuts_get_follow() ? 1 : 0;
            mad_config.os_mac    = os_shortcuts_get_mac() ? 1 : 0;
            break;
        default:
            return false;
    }
    mad_config_write();
    return true;
}

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command  = &data[0];
    uint8_t  channel  = data[1];
    uint8_t  value_id = data[2];
    uint8_t *payload  = &data[3];

    bool handled = false;
    switch (*command) {
        case id_custom_get_value:
            handled = mad_hid_get(channel, value_id, payload);
            break;
        case id_custom_set_value:
            handled = mad_hid_set(channel, value_id, payload);
            break;
        case id_custom_save:
            handled = mad_hid_save(channel);
            break;
    }
    if (!handled) {
        *command = id_unhandled;
    }
}

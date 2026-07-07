/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/* ---------------------------------------------------------------------------
 * Vial
 * ------------------------------------------------------------------------- */
#define VIAL_KEYBOARD_UID {0x3D, 0x23, 0x68, 0xC4, 0x1F, 0x4E, 0x55, 0x8C}

// Hold Top Right (0,3) + Bottom Right (0,5) to unlock the Vial security lock.
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {3, 5}

// Emulated-EEPROM size (RP2040 wear-leveling in flash). Defaults are
// 8K backing / 4K logical; doubled 2026-07-03 so Vial's dynamic-keymap area
// (macros especially) gets ~6.5 KB instead of ~2.5 KB. Changing these
// relocates/reformats the EEPROM region — every persisted setting re-seeds on
// the next boot (same effect as an EECONFIG_USER_DATA_VERSION bump).
#define WEAR_LEVELING_BACKING_SIZE 16384
#define WEAR_LEVELING_LOGICAL_SIZE 8192

// Vial dynamic-entry capacities. The auto-tier picks these same values when
// TOTAL_EEPROM_BYTE_COUNT > 4000, but pinned explicitly so a future EEPROM
// size change can't silently shrink them (they size EEPROM slots AND the
// counts the GUI/companion app reads over HID).
#define VIAL_TAP_DANCE_ENTRIES 32
// 64 combos since 2026-07-03: first 32 = the general Combos editor, last 32
// reserved by Flask's Mouse Chords section (app-side partition).
#define VIAL_COMBO_ENTRIES 64
#define VIAL_KEY_OVERRIDE_ENTRIES 32

/* ---------------------------------------------------------------------------
 * Companion-app raw HID tuning (see raw_hid_receive_kb in keymap.c)
 * ------------------------------------------------------------------------- */
// Routes VIA command IDs 0x07/0x08/0x09 (id_custom_set_value/get_value/save)
// from quantum/via.c to raw_hid_receive_kb instead of dead-ending them.
// Safe here: no lighting feature is compiled in this build and vial.json says
// "lighting": "none", so nothing else uses those IDs. This carries the VIA
// v3-style custom-UI protocol the macOS companion app speaks.
#define VIA_CUSTOM_LIGHTING_ENABLE

// EEPROM datablock holding every persisted tunable (mad_config_t in keymap.c).
// Replaces the legacy 4-byte eeconfig user word (that word now stores the
// validity/version marker). Bump the VERSION whenever mad_config_t's layout
// changes — the datablock then reads invalid and re-seeds from defaults.
// v2 = appended the 8x4 dynamic gesture set table (64 bytes).
// v3 = no layout change; bumped to flush verify-script test values saved to
//      EEPROM during the Phase 3 hardware pass and re-seed from the defaults
//      below (reviewed 2026-07-02).
// v4 = gesture sets grew 8x4 → 8x8 (diagonals; +64 bytes) and appended the
//      wiggle_enabled kill switch (+1 byte). SIZE 96 → 160.
// v5 = appended the getreuer-module block: custom-shift-keys enable + 16-slot
//      table (+65 bytes), select-word mac flag (+1), sentence-case enable
//      (+1), and 8 leader sequences of 5 keys + output (+96). SIZE 160 → 352.
// v6 = appended autoscroll tunables (+6 bytes: inverted, speed scale x100,
//      jog deadzone, jog range). SIZE unchanged (352 had headroom).
// v7 = appended auto-mouse tunables (+4 bytes: enabled, timeout ms u16,
//      threshold) AND the wheel-chords block (+131 bytes: enabled, step u16,
//      8x8 u16 slot table). SIZE 352 → 512.
// v8 = appended OS-aware shortcuts (+2 bytes: follow-detection switch,
//      pinned mac/pc mode). SIZE unchanged.
// v9 = appended wheel-chords hold delay (+2 bytes). SIZE unchanged.
// v10 = appended per-combo layer masks (+128 bytes: u16 x 64 combos, bit N =
//       combo may fire while layer N is highest). SIZE 512 → 768.
// v11 = appended raw DPI CPI (u16, 0 = legacy table), wiggle action/target
//       set/source (3 bytes), auto-mouse target layer (1), num word timeout
//       + layer (3). SIZE unchanged (768 has room).
#define EECONFIG_USER_DATA_SIZE 768
#define EECONFIG_USER_DATA_VERSION 11

// Per-combo layer gating (ZMK-style "layers = [...]"): compiles the
// combo_should_trigger() hook into quantum/process_combo.c; the
// implementation in keymap.c consults the combo_layer_masks table
// (HID channel 0x20, persisted in mad_config).
#define COMBO_SHOULD_TRIGGER

/* ---------------------------------------------------------------------------
 * Auto-mouse (QMK core POINTING_DEVICE_AUTO_MOUSE_ENABLE, re-added 2026-07-03
 * on a fresh user ask — the 2026-07-01 removal note in CLAUDE.md stands as
 * history, this is the sanctioned re-add)
 *
 * Ball motion past a threshold activates _MOUSE momentarily; the layer drops
 * after the timeout with no further motion. Runtime state ships DISABLED so
 * "ball motion never changes layers" stays true until the user flips it in
 * Flask (HID channel 0x1B). Threshold is enforced by the
 * auto_mouse_activation override in keymap.c (own accumulator), not the
 * core's compile-time AUTO_MOUSE_THRESHOLD.
 * ------------------------------------------------------------------------- */
#define AUTO_MOUSE_DEFAULT_LAYER 1 // _MOUSE
#define MAD_AUTOMOUSE_ENABLED_DEFAULT false
#define MAD_AUTOMOUSE_TIMEOUT_DEFAULT 650  // ms of stillness before _MOUSE drops
#define MAD_AUTOMOUSE_TIMEOUT_MIN 100
#define MAD_AUTOMOUSE_TIMEOUT_MAX 5000
#define MAD_AUTOMOUSE_THRESHOLD_DEFAULT 10 // accumulated counts to trigger
#define MAD_AUTOMOUSE_THRESHOLD_MAX 60
#define MAD_AUTOMOUSE_LAYER_DEFAULT 1      // _MOUSE (runtime-selectable, v10)

// Raw DPI (v10): Flask sets CPI directly in this range; 0 = legacy 5-entry
// table. The PMW3360 quantizes to its native 100-count steps internally.
#define MADROMYS_DPI_CPI_MIN 200
#define MADROMYS_DPI_CPI_MAX 4000
#define MADROMYS_DPI_CPI_STEP 50

// Num word (v10, shared module): number layer with caps-word-style hold +
// idle timeout. Layer default = 5 — the user's "Number" layer (digit tap
// dances, baked 2026-07-05); retarget to any layer 0-7 in Flask (0x1E/0x02).
#define NUM_WORD_LAYER_DEFAULT 5
#define NUM_WORD_IDLE_TIMEOUT_DEFAULT 5000
#define NUM_WORD_IDLE_TIMEOUT_MAX 30000
#define AUTO_MOUSE_TIME MAD_AUTOMOUSE_TIMEOUT_DEFAULT

/* ---------------------------------------------------------------------------
 * Wheel chords (wheel_chords.c/.h, 2026-07-03)
 *
 * Hold BTN1..BTN8 + move the ball → 8-direction gesture keycodes, one tap
 * per step of travel (pd_gestures math, button-gated). The click itself is
 * never suppressed; motion is swallowed only while the held button has at
 * least one configured slot. Slots live in RAM + the EEPROM datablock,
 * edited over HID channel 0x1C.
 * ------------------------------------------------------------------------- */
#define WHEEL_CHORDS_ENABLED_DEFAULT true
#define WHEEL_CHORDS_STEP_DEFAULT 200
#define WHEEL_CHORDS_STEP_MIN 50
#define WHEEL_CHORDS_STEP_MAX 2000
// Hold delay before capture engages (quick click-drag stays a normal drag).
#define WHEEL_CHORDS_HOLD_MS_DEFAULT 200
#define WHEEL_CHORDS_HOLD_MS_MAX 1000

/* ---------------------------------------------------------------------------
 * OS-aware shortcuts (os_shortcuts.c/.h, 2026-07-03)
 *
 * OS_CUT/OS_COPY/OS_PSTE/OS_UNDO/OS_REDO keycodes send ⌘-based hotkeys in
 * mac mode, ^-based in pc mode. Mode follows QMK OS detection (rules.mk
 * OS_DETECTION_ENABLE, USB descriptor fingerprinting) while "follow" is on;
 * the detection also mirrors into select word's mac/win style. HID 0x1D.
 * ------------------------------------------------------------------------- */
#define OS_SHORTCUTS_MAC_DEFAULT true
#define OS_SHORTCUTS_FOLLOW_DEFAULT true

/* ---------------------------------------------------------------------------
 * Pointing device
 * ------------------------------------------------------------------------- */
// 16-bit mouse reports so the acceleration curve has headroom on fast flicks.
#define MOUSE_EXTENDED_REPORT

// Enables pd_dprintf() call sites (quantum/pointing_device_internal.h) across
// core sensor drivers. No-op until CONSOLE_ENABLE (rules.mk) is also on and
// debug_enable is toggled at runtime (DB_TOGG). Live-debug workflow: `qmk
// console`, press DB_TOGG, watch the sensor trace while nudging the ball.
// Comment out if console traffic isn't wanted day-to-day - purely a
// debug-output flag, no functional effect.
#define POINTING_DEVICE_DEBUG

/* ---------------------------------------------------------------------------
 * DPI / CPI options (cycled / stepped by the DPI keycodes)
 * ------------------------------------------------------------------------- */
#define MADROMYS_DPI_OPTIONS \
    { 400, 600, 800, 1200, 1600 }
#define MADROMYS_DPI_DEFAULT_INDEX 4

/* ---------------------------------------------------------------------------
 * Pointing device acceleration (pd_accel)
 * ------------------------------------------------------------------------- */
// Curve defaults reviewed 2026-07-02: canonical maccel/drashna starting values
// (takeoff 2.0 / growth 0.25 / offset 2.2 / limit 0.2). Growth was 0.30 —
// returned to 0.25 for a gentler, more predictable mid-speed ramp. Live-tune
// from the companion app; these only matter on EEPROM re-seed.
#define POINTING_DEVICE_ACCEL_TAKEOFF 2.0f
#define POINTING_DEVICE_ACCEL_GROWTH_RATE 0.25f
#define POINTING_DEVICE_ACCEL_OFFSET 2.2f
#define POINTING_DEVICE_ACCEL_LIMIT 0.2f
// HID tuning clamp ranges (user-approved 2026-07-02; the raw HID handler in
// keymap.c clamps incoming values to these before calling the module setters).
// Takeoff floor matches pd_accel's own >= 0.5 guard; limit ceiling matches
// POINTING_DEVICE_ACCEL_LIMIT_UPPER (1); growth/offset floors/ceilings are the
// approved UI ranges.
#define POINTING_DEVICE_ACCEL_TAKEOFF_MIN 0.5f
#define POINTING_DEVICE_ACCEL_TAKEOFF_MAX 10.0f
#define POINTING_DEVICE_ACCEL_GROWTH_RATE_MAX 2.0f
#define POINTING_DEVICE_ACCEL_OFFSET_MIN -10.0f
#define POINTING_DEVICE_ACCEL_OFFSET_MAX 10.0f
#define POINTING_DEVICE_ACCEL_LIMIT_MAX 1.0f

/* ---------------------------------------------------------------------------
 * Pointing device smoothing (pointing_device_smoothing)
 * ------------------------------------------------------------------------- */
#define POINTING_DEVICE_SMOOTHING_FACTOR 0.4f
#define POINTING_DEVICE_SMOOTHING_RESET_TIMEOUT_MS 200
// HID tuning clamp ceiling for the reset timeout (factor is already clamped
// 0..1 by the module setter).
#define POINTING_DEVICE_SMOOTHING_TIMEOUT_MAX 1000

/* ---------------------------------------------------------------------------
 * Pointing device gestures (pd_gestures) — ratchet only
 * ------------------------------------------------------------------------- */
// Ratchet mode: ball travel (sensor counts) per repeated key. Lower = fires more
// often. DPI-relative: at 1600 CPI, 200 counts ~= 0.125" of ball travel per key.
// Default only — runtime value lives in pd_gestures.c, persisted in EEPROM,
// tuned live by the companion app over raw HID. Clamped to MIN/MAX below.
#define PD_GESTURES_RATCHET_STEP 200
#define PD_GESTURES_RATCHET_STEP_MIN 50
#define PD_GESTURES_RATCHET_STEP_MAX 1000
// Number of dynamic gesture sets (GR1_TOG..GR8_TOG keycodes; contents edited
// live by the companion app over raw HID, persisted in the EEPROM datablock).
// Changing this changes mad_config_t's layout — bump EECONFIG_USER_DATA_VERSION.
#define MAD_GESTURE_SET_COUNT 8
// Safe exit: any button press except the gesture controls (GR?_TOG/GR?_HLD)
// cancels an active gesture; the press still performs its normal action. Pressing
// a different set's toggle still switches sets. Comment out to disable.
#define GESTURE_AUTO_EXIT

/* ---------------------------------------------------------------------------
 * Wiggle ball (wiggle_ball) — shake left-right to toggle drag scroll
 *
 * All three detection parameters are defaults only — the runtime values live
 * in wiggle_ball.c, persist in EEPROM, and are tuned live by the companion
 * app over raw HID. Clamp ranges user-approved 2026-07-02.
 * ------------------------------------------------------------------------- */
// Cooldown after a toggle before another wiggle is recognized.
#define WIGGLE_BALL_TIMEOUT 250
#define WIGGLE_BALL_COOLDOWN_MIN 50
#define WIGGLE_BALL_COOLDOWN_MAX 2000
// Max gap between direction reversals to still count as the same shake ("interval").
#define WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT 150
#define WIGGLE_BALL_INTERVAL_MIN 10
#define WIGGLE_BALL_INTERVAL_MAX 2000
// Perpendicular-axis movement allowed before a reversal is rejected as noise
// ("distance", sensor counts; floor is 0).
#define WIGGLE_BALL_MOVEMENT_THRESHOLD 3
#define WIGGLE_BALL_THRESHOLD_MAX 20
// Shake detection on/off at boot (runtime kill switch, HID value 0x12/0x04).
// Added 2026-07-02: accidental toggles during fast normal movement froze the
// cursor — the app can now disable detection outright instead of only
// detuning it.
#define WIGGLE_BALL_ENABLED_DEFAULT true

/* ---------------------------------------------------------------------------
 * Drag scroll (drag_scroll) — larger divisor = slower scroll
 * ------------------------------------------------------------------------- */
// H deliberately higher than V: no axis lock exists, so diagonal drift during a
// vertical roll leaks horizontal ticks — the stiffer H divisor damps that.
#define SCROLL_DIVISOR_H 40
#define SCROLL_DIVISOR_V 32
// Step size (and clamp range) for the DRG_DIV live-tuning keycode.
#define SCROLL_DIVISOR_STEP 1
#define SCROLL_DIVISOR_MIN 1
#define SCROLL_DIVISOR_MAX 64
// Invert scroll output by default (negate both axes). DRG_INV toggles at runtime.
#define DRAG_SCROLL_DEFAULT_INVERTED true
// Auto-exit: any button press (except the DRG_* scroll controls) drops out of drag
// scroll; the press still performs its normal action. Comment out to disable.
#define DRAG_SCROLL_AUTO_EXIT
// Bind drag scroll to a layer: entering this layer starts drag scroll, leaving it
// stops. 2 = _SCRL. Comment out to unbind. (Auto-exit can still drop it mid-layer.)
#define DRAG_SCROLL_LAYER 2

/* ---------------------------------------------------------------------------
 * Tap-hold (mod-tap / layer-tap) and combos
 * ------------------------------------------------------------------------- */
#define TAPPING_TERM 200
#define COMBO_TERM 50

/* ---------------------------------------------------------------------------
 * Custom shift keys (custom_shift_keys, ported from getreuer/qmk-modules)
 * Redefines what a key types while Shift is held (e.g. Shift+. = ?). Slots
 * are edited live by the companion app over raw HID and persisted in the
 * EEPROM datablock; they default empty.
 * ------------------------------------------------------------------------- */
#define CUSTOM_SHIFT_KEYS_SLOTS 16
#define CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT true

/* ---------------------------------------------------------------------------
 * Select word (select_word, ported from getreuer/qmk-modules)
 * SELWORD selects the current word (repeat extends); with Shift, whole lines.
 * SELWDBK/SELLINE/SELLNUP are the backward/line/line-up variants.
 * ------------------------------------------------------------------------- */
// Reset an in-progress selection after this idle time (ms).
#define SELECT_WORD_TIMEOUT 5000
// Hotkey style at boot: true = macOS (Alt/Cmd+arrows), false = Win/Linux
// (Ctrl/Home/End). Runtime value is HID-tunable + persisted.
#define SELECT_WORD_MAC_DEFAULT true

/* ---------------------------------------------------------------------------
 * Sentence case (sentence_case, ported from getreuer/qmk-modules)
 * Auto-capitalizes the first letter after ". " / "! " / "? ". SC_TOG toggles;
 * the on/off state persists in the EEPROM datablock. Off by default so typing
 * behaves stock until explicitly enabled.
 * ------------------------------------------------------------------------- */
#define SENTENCE_CASE_TIMEOUT 5000
#define SENTENCE_CASE_BUFFER_SIZE 8
#define SENTENCE_CASE_ON_DEFAULT false

/* ---------------------------------------------------------------------------
 * Leader key (QMK core feature; sequences are dynamic)
 * QK_LEAD starts a sequence (place it on a key via the companion app). The 8
 * sequence slots (up to 5 keys -> 1 output keycode) live in RAM, are edited
 * over raw HID, and persist in the EEPROM datablock. Output fires via
 * tap_code16 — same contract as gesture slots: basic keycodes + mod combos
 * only, no macros/layer keys/QK_KB_* customs.
 * ------------------------------------------------------------------------- */
#define LEADER_TIMEOUT 400
// Timeout applies per key, not to the whole sequence — friendlier pacing.
#define LEADER_PER_KEY_TIMING
#define MAD_LEADER_SEQ_COUNT 8
#define MAD_LEADER_SEQ_KEYS 5

/* ---------------------------------------------------------------------------
 * Autoscroll (autoscroll) — hands-free continuous scrolling
 * Modeled on Ben White's radiology AHK autoscroller + the Contour Shuttle
 * jog wheel. AS_UP/AS_DOWN step a signed speed level (intervals below, ms per
 * wheel tick); AS_JOG turns the ball into a jog wheel (deflection = speed,
 * motion swallowed). Any other key press stops either mode (auto-exit).
 * All four tunables runtime + HID (channel 0x1A) + persisted.
 * ------------------------------------------------------------------------- */
#define AUTOSCROLL_STEP_INTERVALS { 1000, 500, 200, 100, 67, 50, 40, 33, 25 }
#define AUTOSCROLL_SPEED_SCALE_X100 100
#define AUTOSCROLL_JOG_DEADZONE 15
#define AUTOSCROLL_JOG_RANGE 300
#define AUTOSCROLL_INVERTED_DEFAULT false

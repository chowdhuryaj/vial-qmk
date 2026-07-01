/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/* ---------------------------------------------------------------------------
 * Vial
 * ------------------------------------------------------------------------- */
#define VIAL_KEYBOARD_UID {0x3D, 0x23, 0x68, 0xC4, 0x1F, 0x4E, 0x55, 0x8C}

// Hold Top Right (0,3) + Bottom Right (0,5) to unlock the Vial security lock.
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {3, 5}

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
#define POINTING_DEVICE_ACCEL_TAKEOFF 2.0f
#define POINTING_DEVICE_ACCEL_GROWTH_RATE 0.30f
#define POINTING_DEVICE_ACCEL_OFFSET 2.2f
#define POINTING_DEVICE_ACCEL_LIMIT 0.2f

/* ---------------------------------------------------------------------------
 * Pointing device smoothing (pointing_device_smoothing)
 * ------------------------------------------------------------------------- */
#define POINTING_DEVICE_SMOOTHING_FACTOR 0.4f
#define POINTING_DEVICE_SMOOTHING_RESET_TIMEOUT_MS 200

/* ---------------------------------------------------------------------------
 * Pointing device gestures (pd_gestures) — ratchet only
 * ------------------------------------------------------------------------- */
// Ratchet mode: ball travel (sensor counts) per repeated key. Lower = fires more
// often. DPI-relative: at 1600 CPI, 200 counts ~= 0.125" of ball travel per key.
#define PD_GESTURES_RATCHET_STEP 200
// Safe exit: any button press except the gesture controls (GR?_TOG/GR?_HLD)
// cancels an active gesture; the press still performs its normal action. Pressing
// a different set's toggle still switches sets. Comment out to disable.
#define GESTURE_AUTO_EXIT

/* ---------------------------------------------------------------------------
 * Drag scroll (drag_scroll) — larger divisor = slower scroll
 * ------------------------------------------------------------------------- */
#define SCROLL_DIVISOR_H 16
#define SCROLL_DIVISOR_V 16
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

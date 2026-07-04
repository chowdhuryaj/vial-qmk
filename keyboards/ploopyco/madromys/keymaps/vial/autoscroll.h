/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Autoscroll: hands-free continuous scrolling, modeled on Ben White's
 * radiology AutoHotkey autoscroller (stepped speeds, ~1000ms..25ms per wheel
 * tick) and the Contour Shuttle jog wheel (analog deflection = speed).
 *
 * Two modes, mutually exclusive:
 *  - STEPPED: AS_UP / AS_DOWN keycodes move a signed speed level (+ = up,
 *    - = down, 0 = stopped). Each |level| picks a tick interval from
 *    AUTOSCROLL_STEP_INTERVALS. Stepping through zero stops. Ball still
 *    moves the cursor normally.
 *  - JOG: AS_JOG toggles. Ball vertical travel accumulates into a
 *    "deflection" (the virtual jog-wheel position, clamped to
 *    ±AUTOSCROLL_JOG_RANGE); scroll direction/speed follow it continuously,
 *    with a deadzone around center. Ball motion is swallowed while jogging
 *    (like drag scroll). Toggle again to exit and reset.
 *
 * Any other key press stops either mode (same auto-exit convention as drag
 * scroll and gestures — see autoscroll_stop() call in process_record_user).
 */
#pragma once

#include "quantum.h"

// Tick intervals (ms per wheel notch) for stepped |level| 1..9 — Ben White's
// canonical table. Scaled at runtime by the speed-scale tunable.
#ifndef AUTOSCROLL_STEP_INTERVALS
#    define AUTOSCROLL_STEP_INTERVALS \
        { 1000, 500, 200, 100, 67, 50, 40, 33, 25 }
#endif

// Runtime tunable defaults (HID channel 0x1A, persisted in mad_config).
// Speed scale x100: 100 = the table as-is; 200 = twice as fast (half the
// interval); clamped 25..400.
#ifndef AUTOSCROLL_SPEED_SCALE_X100
#    define AUTOSCROLL_SPEED_SCALE_X100 100
#endif
#define AUTOSCROLL_SPEED_SCALE_MIN 25
#define AUTOSCROLL_SPEED_SCALE_MAX 400

// Jog: ball travel (sensor counts) ignored around center before scrolling
// starts — keeps a resting ball from creeping.
#ifndef AUTOSCROLL_JOG_DEADZONE
#    define AUTOSCROLL_JOG_DEADZONE 15
#endif
#define AUTOSCROLL_JOG_DEADZONE_MAX 200

// Jog: deflection (counts past deadzone) that reaches full speed. Interval
// interpolates from the slowest to the fastest stepped interval across this
// range — full deflection scrolls exactly as fast as stepped level 9.
#ifndef AUTOSCROLL_JOG_RANGE
#    define AUTOSCROLL_JOG_RANGE 300
#endif
#define AUTOSCROLL_JOG_RANGE_MIN 50
#define AUTOSCROLL_JOG_RANGE_MAX 2000

// Invert scroll direction (natural vs classic), like drag scroll's invert.
#ifndef AUTOSCROLL_INVERTED_DEFAULT
#    define AUTOSCROLL_INVERTED_DEFAULT false
#endif

// Pipeline stage: runs after gestures, before drag scroll (jog must swallow
// raw ball motion the same way drag scroll would).
report_mouse_t autoscroll_apply(report_mouse_t report);

// Keycode handlers.
void autoscroll_step(int8_t direction); // +1 = AS_UP, -1 = AS_DOWN
void autoscroll_jog_toggle(void);
void autoscroll_stop(void); // auto-exit hook; safe to call when idle

// State for STATUS / HID diagnostics: signed stepped level (-9..9), or
// jog deflection sign * 100 when jogging; 0 = idle.
bool   autoscroll_is_active(void);
int8_t autoscroll_get_level(void);
bool   autoscroll_is_jogging(void);

// Tunables (clamped setters, live values).
void     set_autoscroll_speed_scale(uint16_t x100);
uint16_t get_autoscroll_speed_scale(void);
void     set_autoscroll_jog_deadzone(uint8_t counts);
uint8_t  get_autoscroll_jog_deadzone(void);
void     set_autoscroll_jog_range(uint16_t counts);
uint16_t get_autoscroll_jog_range(void);
void     set_autoscroll_inverted(bool inverted);
bool     get_autoscroll_inverted(void);

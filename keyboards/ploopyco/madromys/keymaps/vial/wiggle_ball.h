// Copyright 2024 @pandrr
// Copyright 2025 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `wiggle_ball` community module: rapidly shaking the
// ball left-right toggles drag scroll on/off, hands-free. Community-module
// plumbing and its own task dispatch stripped; the keymap pipes mouse reports
// through wiggle_ball_apply() each cycle (see pointing_device_task_user in
// keymap.c), same as every other ported module.
//
// Re-added 2026-07-02 (user request, companion-app project) after the
// 2026-07-01 removal. The old unresolved quirk stands: scroll engaged BY a
// wiggle sometimes produced no wheel output (see CLAUDE.md "Drag-scroll
// history"). All three detection parameters are runtime variables now,
// exposed over raw HID for the companion app — the first live
// debugging/tuning path this module has had.

#pragma once

#include <stdint.h>
#include "report.h"

// Default cooldown (ms) after a toggle before another wiggle can be recognized.
// Runtime-adjustable via set_wiggle_ball_timeout().
#ifndef WIGGLE_BALL_TIMEOUT
#    define WIGGLE_BALL_TIMEOUT 250
#endif
// Default max gap (ms) between direction reversals to still count as the same
// shake. Runtime-adjustable via set_wiggle_ball_direction_switch_timeout().
#ifndef WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT
#    define WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT 150
#endif
// Default perpendicular-axis movement (sensor counts per report) allowed
// before a reversal is rejected as noise. Runtime-adjustable via
// set_wiggle_ball_movement_threshold().
#ifndef WIGGLE_BALL_MOVEMENT_THRESHOLD
#    define WIGGLE_BALL_MOVEMENT_THRESHOLD 3
#endif
// Clamp range for the runtime direction-switch timeout ("interval": lower =
// must shake faster, higher = lazier shakes count).
#ifndef WIGGLE_BALL_INTERVAL_MIN
#    define WIGGLE_BALL_INTERVAL_MIN 10
#endif
#ifndef WIGGLE_BALL_INTERVAL_MAX
#    define WIGGLE_BALL_INTERVAL_MAX 2000
#endif
// Clamp range for the runtime cooldown.
#ifndef WIGGLE_BALL_COOLDOWN_MIN
#    define WIGGLE_BALL_COOLDOWN_MIN 50
#endif
#ifndef WIGGLE_BALL_COOLDOWN_MAX
#    define WIGGLE_BALL_COOLDOWN_MAX 2000
#endif
// Ceiling for the runtime movement threshold ("distance"; floor is 0).
#ifndef WIGGLE_BALL_THRESHOLD_MAX
#    define WIGGLE_BALL_THRESHOLD_MAX 20
#endif
// Whether shake detection starts enabled at boot (runtime + HID-tunable kill
// switch; added 2026-07-02 after accidental toggles froze the cursor mid-move).
#ifndef WIGGLE_BALL_ENABLED_DEFAULT
#    define WIGGLE_BALL_ENABLED_DEFAULT true
#endif

// Detects a left-right shake (4 direction reversals within the timing windows
// above). If a gesture is active, the shake cancels it (pd_gestures_cancel())
// and is consumed there; otherwise it toggles drag scroll via
// get/set_drag_scroll_scrolling(). Always reads the raw ball axes (x/y) - the
// same shake must work to turn scrolling on and off AND to escape a gesture -
// so this must run BEFORE both pd_gestures_apply() (which zeroes x/y while a
// gesture is open) and drag_scroll_apply() (which divides them into wheel
// output) in the pipeline.
report_mouse_t wiggle_ball_apply(report_mouse_t mouse_report);

// Runtime "interval": max ms between direction reversals. Setter clamps to
// [WIGGLE_BALL_INTERVAL_MIN, WIGGLE_BALL_INTERVAL_MAX].
uint16_t get_wiggle_ball_direction_switch_timeout(void);
void     set_wiggle_ball_direction_switch_timeout(uint16_t ms);

// Runtime cooldown after a toggle. Setter clamps to
// [WIGGLE_BALL_COOLDOWN_MIN, WIGGLE_BALL_COOLDOWN_MAX].
uint16_t get_wiggle_ball_timeout(void);
void     set_wiggle_ball_timeout(uint16_t ms);

// Runtime "distance": perpendicular noise tolerance in sensor counts.
// Setter clamps to [0, WIGGLE_BALL_THRESHOLD_MAX].
uint8_t get_wiggle_ball_movement_threshold(void);
void    set_wiggle_ball_movement_threshold(uint8_t counts);

// Runtime kill switch for shake detection (true = active).
bool get_wiggle_ball_enabled(void);
void set_wiggle_ball_enabled(bool enabled);

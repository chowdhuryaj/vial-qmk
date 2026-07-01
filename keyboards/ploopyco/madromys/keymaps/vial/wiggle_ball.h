// Copyright 2024 @pandrr
// Copyright 2025 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `wiggle_ball` community module: rapidly shaking the
// ball left-right toggles drag scroll on/off, hands-free. Community-module
// plumbing and its own task dispatch stripped; the keymap pipes mouse reports
// through wiggle_ball_apply() each cycle (see pointing_device_task_user in
// keymap.c), same as every other ported module.

#pragma once

#include "report.h"

// Cooldown (ms) after a toggle before another wiggle can be recognized.
#ifndef WIGGLE_BALL_TIMEOUT
#    define WIGGLE_BALL_TIMEOUT 250
#endif
// Max gap (ms) between direction reversals to still count as the same shake.
#ifndef WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT
#    define WIGGLE_BALL_DIRECTION_SWITCH_TIMEOUT 150
#endif
// Perpendicular-axis movement allowed before a reversal is rejected as noise.
#ifndef WIGGLE_BALL_MOVEMENT_THRESHOLD
#    define WIGGLE_BALL_MOVEMENT_THRESHOLD 3
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

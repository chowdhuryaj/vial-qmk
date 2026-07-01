// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `drag_scroll` community module into a self-contained
// keymap module. Community-module plumbing and custom keycodes removed; the
// keymap drives this via drag_scroll_apply() and set/get_drag_scroll_scrolling().

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "report.h"

// Larger divisor = slower scrolling.
#ifndef SCROLL_DIVISOR_H
#    define SCROLL_DIVISOR_H 8
#endif
#ifndef SCROLL_DIVISOR_V
#    define SCROLL_DIVISOR_V 8
#endif

// Step applied per keypress when adjusting the divisor. Shift inverts, Ctrl x10.
#ifndef SCROLL_DIVISOR_STEP
#    define SCROLL_DIVISOR_STEP 1
#endif
// Divisor is clamped to this range (it's a denominator, so it can never reach 0).
#ifndef SCROLL_DIVISOR_MIN
#    define SCROLL_DIVISOR_MIN 1
#endif
#ifndef SCROLL_DIVISOR_MAX
#    define SCROLL_DIVISOR_MAX 64
#endif

// Whether scroll output starts inverted at boot. DRG_INV toggles it at runtime.
#ifndef DRAG_SCROLL_DEFAULT_INVERTED
#    define DRAG_SCROLL_DEFAULT_INVERTED false
#endif

// Convert pointer movement into wheel movement while drag scroll is active.
report_mouse_t drag_scroll_apply(report_mouse_t mouse_report);

bool get_drag_scroll_scrolling(void);
void set_drag_scroll_scrolling(bool scrolling);

// Force drag scroll on regardless of manual control. While forced, drag scroll
// stays on and any set_drag_scroll_scrolling(false) is ignored (button presses,
// DRG_TOG, leaving the scroll layer, etc. cannot turn it off). Releasing the
// force does NOT turn scrolling off by itself — the caller decides. Used by the
// keymap to bind drag scroll to the host lock LEDs.
void set_drag_scroll_force(bool force);
bool get_drag_scroll_force(void);

// Invert scroll output (negates both axes). Default: off (normal direction).
bool get_drag_scroll_inverted(void);
void set_drag_scroll_inverted(bool inverted);
void drag_scroll_toggle_inverted(void);

int8_t get_drag_scroll_h_divisor(void);
int8_t get_drag_scroll_v_divisor(void);
void   set_drag_scroll_h_divisor(int8_t divisor);
void   set_drag_scroll_v_divisor(int8_t divisor);
void   set_drag_scroll_divisor(int8_t divisor);
// Step both H and V divisors together, clamped to [SCROLL_DIVISOR_MIN, SCROLL_DIVISOR_MAX].
void   drag_scroll_divisor_increment(void);

// Optional user hook fired whenever the drag-scroll state changes.
bool set_drag_scroll_scrolling_user(bool scrolling);

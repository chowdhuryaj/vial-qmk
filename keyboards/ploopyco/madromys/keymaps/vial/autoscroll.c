/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Autoscroll implementation — see autoscroll.h for the model. */
#include "autoscroll.h"

static const uint16_t step_intervals[9] = AUTOSCROLL_STEP_INTERVALS;

// Mode state. level != 0 = stepped mode; jogging = jog mode. Never both.
static int8_t  level      = 0;
static bool    jogging    = false;
static int32_t deflection = 0; // jog-wheel position, sensor counts, +down
static uint16_t last_tick = 0;

// Tunables (live values; persisted snapshot lives in keymap.c's mad_config).
static uint16_t speed_scale_x100 = AUTOSCROLL_SPEED_SCALE_X100;
static uint8_t  jog_deadzone     = AUTOSCROLL_JOG_DEADZONE;
static uint16_t jog_range        = AUTOSCROLL_JOG_RANGE;
static bool     inverted         = AUTOSCROLL_INVERTED_DEFAULT;

void set_autoscroll_speed_scale(uint16_t x100) {
    if (x100 < AUTOSCROLL_SPEED_SCALE_MIN) x100 = AUTOSCROLL_SPEED_SCALE_MIN;
    if (x100 > AUTOSCROLL_SPEED_SCALE_MAX) x100 = AUTOSCROLL_SPEED_SCALE_MAX;
    speed_scale_x100 = x100;
}
uint16_t get_autoscroll_speed_scale(void) { return speed_scale_x100; }

void set_autoscroll_jog_deadzone(uint8_t counts) {
    if (counts > AUTOSCROLL_JOG_DEADZONE_MAX) counts = AUTOSCROLL_JOG_DEADZONE_MAX;
    jog_deadzone = counts;
}
uint8_t get_autoscroll_jog_deadzone(void) { return jog_deadzone; }

void set_autoscroll_jog_range(uint16_t counts) {
    if (counts < AUTOSCROLL_JOG_RANGE_MIN) counts = AUTOSCROLL_JOG_RANGE_MIN;
    if (counts > AUTOSCROLL_JOG_RANGE_MAX) counts = AUTOSCROLL_JOG_RANGE_MAX;
    jog_range = counts;
}
uint16_t get_autoscroll_jog_range(void) { return jog_range; }

void set_autoscroll_inverted(bool inv) { inverted = inv; }
bool get_autoscroll_inverted(void) { return inverted; }

bool   autoscroll_is_active(void) { return level != 0 || jogging; }
int8_t autoscroll_get_level(void) { return level; }
bool   autoscroll_is_jogging(void) { return jogging; }

void autoscroll_stop(void) {
    level      = 0;
    jogging    = false;
    deflection = 0;
}

void autoscroll_step(int8_t direction) {
    if (jogging) { // stepping while jogging exits jog first
        autoscroll_stop();
    }
    int8_t next = level + direction;
    if (next > 9) next = 9;
    if (next < -9) next = -9;
    level = next;
    last_tick = timer_read(); // full interval before the first tick
}

void autoscroll_jog_toggle(void) {
    if (jogging) {
        autoscroll_stop();
    } else {
        autoscroll_stop(); // clears any stepped level
        jogging   = true;
        last_tick = timer_read();
    }
}

// Effective interval for stepped |level| 1..9, speed-scale applied.
static uint16_t stepped_interval(uint8_t magnitude) {
    uint32_t base = step_intervals[magnitude - 1];
    uint32_t scaled = base * 100 / speed_scale_x100;
    return scaled < 5 ? 5 : (uint16_t)scaled;
}

// Jog interval: linear interpolation from the slowest to the fastest stepped
// interval across [deadzone .. deadzone+range] deflection.
static uint16_t jog_interval(uint32_t magnitude) {
    uint32_t slowest = stepped_interval(1);
    uint32_t fastest = stepped_interval(9);
    if (magnitude >= jog_range) return (uint16_t)fastest;
    return (uint16_t)(slowest - (slowest - fastest) * magnitude / jog_range);
}

report_mouse_t autoscroll_apply(report_mouse_t report) {
    int8_t   scroll_dir = 0; // +1 = wheel up, -1 = wheel down
    uint16_t interval   = 0;

    if (jogging) {
        // Ball vertical travel winds the virtual jog wheel; motion is
        // swallowed (cursor frozen), same contract as drag scroll.
        deflection += report.y;
        if (deflection > (int32_t)(jog_deadzone + jog_range)) deflection = jog_deadzone + jog_range;
        if (deflection < -(int32_t)(jog_deadzone + jog_range)) deflection = -(int32_t)(jog_deadzone + jog_range);
        report.x = 0;
        report.y = 0;

        int32_t past = (deflection < 0 ? -deflection : deflection) - jog_deadzone;
        if (past > 0) {
            // Ball down (y+) scrolls down — wheel negative in HID convention.
            scroll_dir = deflection > 0 ? -1 : 1;
            interval   = jog_interval((uint32_t)past);
        }
    } else if (level != 0) {
        scroll_dir = level > 0 ? 1 : -1;
        interval   = stepped_interval(level > 0 ? level : -level);
    }

    if (scroll_dir != 0 && timer_elapsed(last_tick) >= interval) {
        last_tick = timer_read();
        int8_t v = inverted ? -scroll_dir : scroll_dir;
        // Saturating add on top of whatever drag scroll etc. contribute.
        int16_t sum = (int16_t)report.v + v;
        if (sum > 127) sum = 127;
        if (sum < -127) sum = -127;
        report.v = (int8_t)sum;
    }
    return report;
}

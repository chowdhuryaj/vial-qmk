// Copyright 2024-2025 @tzarc
// Copyright 2025 Christopher Courtney, aka Drashna Jael're (@drashna)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from drashna's `pointing_device_gestures` community module. See
// pd_gestures.h. Ratchet-only: while a gesture is active it swallows ball
// movement and taps one key per PD_GESTURES_RATCHET_STEP of accumulated
// travel, in the dominant of 8 directions.

#include QMK_KEYBOARD_H
#include "pd_gestures.h"
#include <math.h>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

static bool            pdg_active = false;
static const uint16_t *pdg_table  = NULL;
static int32_t         pdg_acc_x  = 0;
static int32_t         pdg_acc_y  = 0;

// Runtime ratchet step. Seeded from the compile-time default;
// keyboard_post_init_user overwrites it with the EEPROM-persisted value, and
// the companion app adjusts it live over raw HID.
static uint16_t pdg_ratchet_step = PD_GESTURES_RATCHET_STEP;

uint16_t pd_gestures_get_ratchet_step(void) {
    return pdg_ratchet_step;
}

void pd_gestures_set_ratchet_step(uint16_t step) {
    if (step < PD_GESTURES_RATCHET_STEP_MIN) step = PD_GESTURES_RATCHET_STEP_MIN;
    if (step > PD_GESTURES_RATCHET_STEP_MAX) step = PD_GESTURES_RATCHET_STEP_MAX;
    pdg_ratchet_step = step;
}

bool pd_gestures_is_active(void) {
    return pdg_active;
}

bool pd_gestures_is_active_table(const uint16_t *table) {
    return pdg_active && pdg_table == table;
}

static void pdg_reset_accum(void) {
    pdg_acc_x = 0;
    pdg_acc_y = 0;
}

// True once accumulated travel reaches `thresh` (compared squared, avoids sqrt
// and the int overflow a 32-bit x*x+y*y could hit on a long-open gesture).
static bool pdg_reached(int32_t x, int32_t y, uint16_t thresh) {
    int64_t d2 = (int64_t)x * (int64_t)x + (int64_t)y * (int64_t)y;
    int64_t t2 = (int64_t)thresh * (int64_t)thresh;
    return d2 >= t2;
}

// 8-way bin of the accumulated vector, 45° sectors. Mouse coordinates have +y
// pointing south, so atan2's positive angles run E→SE→S: 0=E 1=SE 2=S 3=SW
// 4=W 5=NW 6=N 7=NE (East, then clockwise).
static uint8_t pdg_direction(int32_t x, int32_t y) {
    float     r     = atan2f((float)y, (float)x);
    float     d     = 180.0f * r / (float)M_PI;
    int16_t   id    = (int16_t)d;
    const int split = PD_GESTURES_NUM_DIRECTIONS;
    return ((id + 360 + 360 / split / 2) % 360 / (360 / split)) % split;
}

static void pdg_fire(uint8_t direction, int32_t x, int32_t y) {
    if (pdg_table == NULL || direction >= PD_GESTURES_NUM_DIRECTIONS) {
        return;
    }
    uint16_t keycode = pdg_table[direction];
    // Empty diagonal falls back to the nearest cardinal by dominant axis, so a
    // set that only fills E/S/W/N behaves exactly like the old 90°-sector
    // binning (a rough-east flick that lands in SE still fires E).
    if (keycode == KC_NO && (direction & 1)) {
        uint8_t cardinal;
        int64_t ax = x < 0 ? -(int64_t)x : (int64_t)x;
        int64_t ay = y < 0 ? -(int64_t)y : (int64_t)y;
        if (ax >= ay) {
            cardinal = (x >= 0) ? 0 : 4; // E or W
        } else {
            cardinal = (y >= 0) ? 2 : 6; // S or N (mouse +y = south)
        }
        keycode = pdg_table[cardinal];
    }
    if (keycode != KC_NO) {
        tap_code16(keycode);
    }
}

void pd_gestures_cancel(void) {
    pdg_active = false;
    pdg_reset_accum();
}

static void pdg_begin_internal(const uint16_t *table) {
    pdg_table  = table;
    pdg_active = true;
    pdg_reset_accum();
}

void pd_gestures_begin(const uint16_t *table) {
    pdg_begin_internal(table);
}

// Hold-style release: the ratchet has already fired incrementally, so just stop.
void pd_gestures_end(void) {
    pd_gestures_cancel();
}

void pd_gestures_toggle(const uint16_t *table) {
    if (pdg_active && pdg_table == table) {
        pd_gestures_cancel(); // re-press of the same gesture turns it off
    } else {
        pdg_begin_internal(table); // start, or switch from another set
    }
}

report_mouse_t pd_gestures_apply(report_mouse_t mouse_report) {
    if (!pdg_active) {
        return mouse_report;
    }

    // While a gesture is open, swallow movement and accumulate travel.
    int16_t dx     = mouse_report.x;
    int16_t dy     = mouse_report.y;
    mouse_report.x = 0;
    mouse_report.y = 0;
    mouse_report.h = 0;
    mouse_report.v = 0;

    pdg_acc_x += dx;
    pdg_acc_y += dy;

    // Fire one key per ratchet step of travel. A fast pass can cover several
    // steps in a single sensor report — fire for each of them and keep the
    // remainder, instead of the old fire-once-and-reset which discarded the
    // surplus and made fast rolls feel like the output lagged the ball.
    // Safety cap bounds the burst if a huge delta ever arrives.
    uint8_t burst = 8;
    while (pdg_reached(pdg_acc_x, pdg_acc_y, pdg_ratchet_step) && burst--) {
        pdg_fire(pdg_direction(pdg_acc_x, pdg_acc_y), pdg_acc_x, pdg_acc_y);
        float dist = sqrtf((float)pdg_acc_x * (float)pdg_acc_x + (float)pdg_acc_y * (float)pdg_acc_y);
        if (dist <= (float)pdg_ratchet_step) {
            pdg_reset_accum();
            break;
        }
        float keep = (dist - (float)pdg_ratchet_step) / dist;
        pdg_acc_x  = (int32_t)((float)pdg_acc_x * keep);
        pdg_acc_y  = (int32_t)((float)pdg_acc_y * keep);
    }

    return mouse_report;
}

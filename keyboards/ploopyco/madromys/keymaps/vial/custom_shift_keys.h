/* SPDX-License-Identifier: Apache-2.0 */
/* Ported from getreuer/qmk-modules custom_shift_keys (Copyright 2021-2025
 * Google LLC, Apache 2.0) for this Vial fork — community modules don't work
 * here, so the module lives in the keymap per the porting strategy in
 * /CLAUDE.md. Differences from upstream: the const introspection table is
 * replaced by a RAM slot table (edited live over raw HID by the companion
 * app, persisted in the EEPROM datablock) plus a runtime enable toggle.
 * NEGMODS / LAYER_MASK compile options were dropped (unused here).
 *
 * A "custom shift key" redefines what a key types while Shift is held, e.g.
 * . -> ? or , -> ! — see <https://getreuer.info/posts/keyboards/custom-shift-keys>.
 */
#pragma once

#include "quantum.h"

// Number of dynamic slots. Changing this changes mad_config_t's layout —
// bump EECONFIG_USER_DATA_VERSION.
#ifndef CUSTOM_SHIFT_KEYS_SLOTS
#    define CUSTOM_SHIFT_KEYS_SLOTS 16
#endif

// Whether the feature starts enabled (CSK_TOG keycode / HID toggle at runtime).
#ifndef CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT
#    define CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT true
#endif

typedef struct __attribute__((packed)) {
    uint16_t keycode;         // key as it appears in the layout; KC_NO = empty slot
    uint16_t shifted_keycode; // what it types while Shift is held
} custom_shift_key_t;

// Runs on every key event; returns false when it consumed the event.
bool process_record_custom_shift_keys(uint16_t keycode, keyrecord_t *record);

void custom_shift_keys_set_enabled(bool on);
bool custom_shift_keys_get_enabled(void);

// Live slot table (RAM). Bounds-checked accessors for the HID handler; the
// bulk pointer is for the EEPROM snapshot/restore in keymap.c.
uint16_t            custom_shift_keys_get_slot_keycode(uint8_t index);
uint16_t            custom_shift_keys_get_slot_shifted(uint8_t index);
void                custom_shift_keys_set_slot_keycode(uint8_t index, uint16_t keycode);
void                custom_shift_keys_set_slot_shifted(uint8_t index, uint16_t shifted);
custom_shift_key_t *custom_shift_keys_table(void);

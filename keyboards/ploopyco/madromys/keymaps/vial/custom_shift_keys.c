/* SPDX-License-Identifier: Apache-2.0 */
/* Ported from getreuer/qmk-modules custom_shift_keys — see the header for
 * what changed relative to upstream. Core press/release logic is verbatim. */
#include "custom_shift_keys.h"

static custom_shift_key_t csk_table[CUSTOM_SHIFT_KEYS_SLOTS];
static bool               csk_enabled = CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT;

void custom_shift_keys_set_enabled(bool on) {
    csk_enabled = on;
}

bool custom_shift_keys_get_enabled(void) {
    return csk_enabled;
}

uint16_t custom_shift_keys_get_slot_keycode(uint8_t index) {
    return (index < CUSTOM_SHIFT_KEYS_SLOTS) ? csk_table[index].keycode : KC_NO;
}

uint16_t custom_shift_keys_get_slot_shifted(uint8_t index) {
    return (index < CUSTOM_SHIFT_KEYS_SLOTS) ? csk_table[index].shifted_keycode : KC_NO;
}

void custom_shift_keys_set_slot_keycode(uint8_t index, uint16_t keycode) {
    if (index < CUSTOM_SHIFT_KEYS_SLOTS) csk_table[index].keycode = keycode;
}

void custom_shift_keys_set_slot_shifted(uint8_t index, uint16_t shifted) {
    if (index < CUSTOM_SHIFT_KEYS_SLOTS) csk_table[index].shifted_keycode = shifted;
}

custom_shift_key_t *custom_shift_keys_table(void) {
    return csk_table;
}

bool process_record_custom_shift_keys(uint16_t keycode, keyrecord_t *record) {
    static uint16_t registered_keycode = KC_NO;

    // If a custom shift key is registered, then this event is either releasing
    // it or manipulating another key at the same time. Either way, we release
    // the currently registered key.
    if (registered_keycode != KC_NO) {
        unregister_code16(registered_keycode);
        registered_keycode = KC_NO;
    }

    if (!csk_enabled) {
        return true;
    }

    if (record->event.pressed) { // Press event.
        const uint8_t saved_mods = get_mods();
        const uint8_t mods       = saved_mods | get_weak_mods() | get_oneshot_mods();
        if ((mods & MOD_MASK_SHIFT) != 0) { // Shift is held.
            // Continue default handling if this is a tap-hold key being held.
            if ((IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode)) && record->tap.count == 0) {
                return true;
            }

            // Search for a custom shift key whose keycode is `keycode`.
            for (uint8_t i = 0; i < CUSTOM_SHIFT_KEYS_SLOTS; ++i) {
                if (csk_table[i].keycode == KC_NO || keycode != csk_table[i].keycode) {
                    continue;
                }
                registered_keycode = csk_table[i].shifted_keycode;
                if (IS_QK_MODS(registered_keycode) && // Should keycode be shifted?
                    (QK_MODS_GET_MODS(registered_keycode) & MOD_LSFT) != 0) {
                    register_code16(registered_keycode); // If so, press it directly.
                } else {
                    // Otherwise cancel shift mods, press the key, and restore mods.
                    del_weak_mods(MOD_MASK_SHIFT);
                    del_oneshot_mods(MOD_MASK_SHIFT);
                    unregister_mods(MOD_MASK_SHIFT);
                    register_code16(registered_keycode);
                    set_mods(saved_mods);
                }
                return false;
            }
        }
    }

    return true; // Continue with default handling.
}

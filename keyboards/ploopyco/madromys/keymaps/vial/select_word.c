/* SPDX-License-Identifier: Apache-2.0 */
/* Ported from getreuer/qmk-modules select_word — see the header for what
 * changed relative to upstream. Selection mechanics are verbatim. */
#include "select_word.h"

#if SELECT_WORD_TIMEOUT > 0 && (SELECT_WORD_TIMEOUT < 100 || SELECT_WORD_TIMEOUT > 30000)
// With the 16-bit timer, the longest representable timeout is 32768 ms.
#    error "select_word: SELECT_WORD_TIMEOUT must be between 100 and 30000 ms"
#endif

static int8_t  selection_dir           = 0;
static bool    reset_before_next_event = false;
static uint8_t registered_hotkey       = KC_NO;
static bool    sw_is_mac               = SELECT_WORD_MAC_DEFAULT;

void select_word_set_mac(bool mac) {
    sw_is_mac = mac;
}

bool select_word_get_mac(void) {
    return sw_is_mac;
}

#if SELECT_WORD_TIMEOUT > 0
// Idle timeout timer to reset Select Word after a period of inactivity.
static uint16_t idle_timer = 0;

static void restart_idle_timer(void) {
    idle_timer = (timer_read() + SELECT_WORD_TIMEOUT) | 1;
}
#endif

void select_word_task(void) {
#if SELECT_WORD_TIMEOUT > 0
    if (idle_timer && timer_expired(timer_read(), idle_timer)) {
        idle_timer    = 0;
        selection_dir = 0;
    }
#endif
}

static void clear_all_mods(void) {
    clear_mods();
    clear_weak_mods();
    clear_oneshot_mods();
}

static uint8_t select_init(void) {
    const uint8_t saved_mods = get_mods();
    clear_all_mods();
    reset_before_next_event = false;
    return saved_mods;
}

static void select_word_in_dir(int8_t dir) {
    // Windows/Linux: word selection via Ctrl+arrows; Mac uses Alt (Opt).
    // To extend an existing selection: Ctrl/Alt+Shift+Left/Right.
    const uint8_t saved_mods = select_init();

    if (selection_dir && (selection_dir < 0) != (dir < 0)) { // Reversal.
        send_keyboard_report();
        tap_code_delay((dir < 0) ? KC_RGHT : KC_LEFT, TAP_CODE_DELAY);
    }

    add_mods(sw_is_mac ? MOD_BIT_LALT : MOD_BIT_LCTRL);

    if (selection_dir == 0) { // Initial selection.
        send_keyboard_report();
        send_string_with_delay_P((dir < 0) ? PSTR(SS_TAP(X_LEFT) SS_TAP(X_RGHT))
                                           : PSTR(SS_TAP(X_RGHT) SS_TAP(X_LEFT)),
                                 TAP_CODE_DELAY);
    }

    register_mods(MOD_BIT_LSHIFT);
    registered_hotkey = (dir < 0) ? KC_LEFT : KC_RGHT;
    register_code(registered_hotkey);

    set_mods(saved_mods);
    selection_dir = dir;
}

static void select_line(int8_t dir) {
    // Windows/Linux: line selection via Home/End; Mac uses GUI (Cmd)+arrows.
    // To extend an existing selection: Shift+Up / Shift+Down.
    const uint8_t saved_mods = select_init();

    if (selection_dir != dir) {
        send_keyboard_report();

        if (selection_dir && (selection_dir < 0) != (dir < 0)) { // Reversal.
            tap_code_delay((dir < 0) ? KC_LEFT : KC_RGHT, TAP_CODE_DELAY);
            selection_dir = 0;
        }

        if (selection_dir == 0) {
            tap_code16_delay( // Move cursor to the start/end of the line.
                sw_is_mac ? ((dir < 0) ? G(KC_RGHT) : G(KC_LEFT))
                          : ((dir < 0) ? KC_END : KC_HOME),
                TAP_CODE_DELAY);
        }

        tap_code16_delay( // Select to the opposite end of the line.
            sw_is_mac ? ((dir < 0) ? S(G(KC_LEFT)) : S(G(KC_RGHT)))
                      : ((dir < 0) ? S(KC_HOME) : S(KC_END)),
            TAP_CODE_DELAY);
    } else {
        register_mods(MOD_BIT_LSHIFT);
        registered_hotkey = (dir < 0) ? KC_UP : KC_DOWN;
        register_code(registered_hotkey);
    }

    set_mods(saved_mods);
    selection_dir = dir;
}

void select_word_register(char action) {
    if (registered_hotkey) {
        select_word_unregister();
    }

    switch (action) {
        case 'W':
            select_word_in_dir(1);
            break;
        case 'B':
            select_word_in_dir(-1);
            break;
        case 'L':
            select_line(2);
            break;
        case 'U':
            select_line(-2);
            break;
    }

#if SELECT_WORD_TIMEOUT > 0
    idle_timer = 0;
#endif
}

void select_word_unregister(void) {
    reset_before_next_event = false;
    unregister_code(registered_hotkey);

    const uint8_t saved_mods = get_mods();
    clear_all_mods();

    switch (registered_hotkey) {
        case KC_DOWN:
            // Extending a multi-line selection: on release, tap Shift+End (Mac:
            // GUI+Shift+Right) so the selection reaches the end of the line.
            send_keyboard_report();
            send_string_with_delay_P(sw_is_mac ? PSTR(SS_LGUI(SS_LSFT(SS_TAP(X_RGHT))))
                                               : PSTR(SS_LSFT(SS_TAP(X_END))),
                                     TAP_CODE_DELAY);
            break;

        case KC_UP:
            // Upward multi-line selection: extend to the beginning of the line.
            send_keyboard_report();
            send_string_with_delay_P(sw_is_mac ? PSTR(SS_LGUI(SS_LSFT(SS_TAP(X_LEFT))))
                                               : PSTR(SS_LSFT(SS_TAP(X_HOME))),
                                     TAP_CODE_DELAY);
            break;
    }

    set_mods(saved_mods);
    registered_hotkey = KC_NO;
#if SELECT_WORD_TIMEOUT > 0
    restart_idle_timer();
#endif
}

void select_word_on_record(uint16_t keycode, keyrecord_t *record) {
    (void)record;
    if (selection_dir) {
        if (reset_before_next_event) {
            selection_dir = 0;
        }

        // Ignore most modifier and layer switch keys.
        switch (keycode) {
            case MODIFIER_KEYCODE_RANGE:
            case QK_MOMENTARY ... QK_MOMENTARY_MAX:
            case QK_LAYER_MOD ... QK_LAYER_MOD_MAX:
            case QK_LAYER_TAP_TOGGLE ... QK_LAYER_TAP_TOGGLE_MAX:
            case QK_TO ... QK_TO_MAX:
            case QK_TOGGLE_LAYER ... QK_TOGGLE_LAYER_MAX:
            case QK_ONE_SHOT_LAYER ... QK_ONE_SHOT_LAYER_MAX:
            case QK_ONE_SHOT_MOD ... QK_ONE_SHOT_MOD_MAX:
                return;
            // Ignore hold events on mod-tap and layer-tap keys.
            case QK_MOD_TAP ... QK_MOD_TAP_MAX:
            case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
                if (record->tap.count == 0) {
                    return;
                }
                break;
        }

        reset_before_next_event = true;
    }

#if SELECT_WORD_TIMEOUT > 0
    if (idle_timer) {
        restart_idle_timer();
    }
#endif
}

/* SPDX-License-Identifier: Apache-2.0 */
/* Ported from getreuer/qmk-modules sentence_case — see the header for what
 * changed relative to upstream. The state machine is verbatim. */
#include "sentence_case.h"

#include <string.h>

#if defined(NO_ACTION_ONESHOT)
// Sentence Case capitalizes via a one-shot Shift; one-shot keys must be on.
#    error "sentence_case: Please enable oneshot."
#endif

#if SENTENCE_CASE_TIMEOUT > 0 && (SENTENCE_CASE_TIMEOUT < 100 || SENTENCE_CASE_TIMEOUT > 30000)
// With the 16-bit timer, the longest representable timeout is 32768 ms.
#    error "sentence_case: SENTENCE_CASE_TIMEOUT must be between 100 and 30000 ms"
#endif

// Number of keys of state history to retain for backspacing.
#define STATE_HISTORY_SIZE 6

/** States in matching the beginning of a sentence. */
enum {
    STATE_INIT,     /**< Initial enabled state. */
    STATE_WORD,     /**< Within a word. */
    STATE_ABBREV,   /**< Within an abbreviation like "e.g.". */
    STATE_ENDING,   /**< Sentence ended. */
    STATE_PRIMED,   /**< "Primed" state, in the space following an ending. */
    STATE_DISABLED, /**< Sentence Case is disabled. */
};

#if SENTENCE_CASE_TIMEOUT > 0
static uint16_t idle_timer = 0;
#endif
#if SENTENCE_CASE_BUFFER_SIZE > 1
static uint16_t key_buffer[SENTENCE_CASE_BUFFER_SIZE] = {0};
#endif
static uint8_t  state_history[STATE_HISTORY_SIZE];
static uint16_t suppress_key   = KC_NO;
static uint8_t  sentence_state = SENTENCE_CASE_ON_DEFAULT ? STATE_INIT : STATE_DISABLED;

static void set_sentence_state(uint8_t new_state) {
    sentence_state = new_state;
}

static void clear_state_history(void) {
#if SENTENCE_CASE_TIMEOUT > 0
    idle_timer = 0;
#endif
    memset(state_history, STATE_INIT, sizeof(state_history));
    if (sentence_state != STATE_DISABLED) {
        set_sentence_state(STATE_INIT);
    }
}

void sentence_case_clear(void) {
    clear_state_history();
    suppress_key = KC_NO;
#if SENTENCE_CASE_BUFFER_SIZE > 1
    memset(key_buffer, 0, sizeof(key_buffer));
#endif
}

void sentence_case_on(void) {
    if (sentence_state == STATE_DISABLED) {
        sentence_state = STATE_INIT;
        sentence_case_clear();
    }
}

void sentence_case_off(void) {
    if (sentence_state != STATE_DISABLED) {
        set_sentence_state(STATE_DISABLED);
    }
}

void sentence_case_toggle(void) {
    if (sentence_state != STATE_DISABLED) {
        sentence_case_off();
    } else {
        sentence_case_on();
    }
}

bool is_sentence_case_on(void) {
    return sentence_state != STATE_DISABLED;
}

void sentence_case_task(void) {
#if SENTENCE_CASE_TIMEOUT > 0
    if (idle_timer && timer_expired(timer_read(), idle_timer)) {
        clear_state_history(); // Timed out; clear all state.
    }
#endif
}

#if SENTENCE_CASE_BUFFER_SIZE > 1
static bool just_typed(const uint16_t *pattern, int8_t pattern_len) {
    const uint16_t *buffer = key_buffer + SENTENCE_CASE_BUFFER_SIZE - pattern_len;
    for (int8_t i = 0; i < pattern_len; ++i) {
        if (buffer[i] != pattern[i]) {
            return false;
        }
    }
    return true;
}

static bool sentence_case_check_ending(void) {
#    if SENTENCE_CASE_BUFFER_SIZE >= 5
    // Don't consider the abbreviations "vs." and "etc." to end the sentence.
    static const uint16_t vs_pattern[]  = {KC_SPC, KC_V, KC_S, KC_DOT};
    static const uint16_t etc_pattern[] = {KC_SPC, KC_E, KC_T, KC_C, KC_DOT};
    if (just_typed(vs_pattern, 4) || just_typed(etc_pattern, 5)) {
        return false; // Not a real sentence ending.
    }
#    endif
    return true; // Real sentence ending; capitalize next letter.
}
#endif

// Classifies a key press: 'a' letter, '.' sentence-ending punctuation,
// '#' symbol, ' ' space, '\'' quote, '\0' ignore-and-reset.
static char sentence_case_press(uint16_t keycode, uint8_t mods) {
    if ((mods & ~(MOD_MASK_SHIFT | MOD_BIT(KC_RALT))) == 0) {
        const bool shifted = mods & MOD_MASK_SHIFT;
        switch (keycode) {
            case KC_A ... KC_Z:
                return 'a'; // Letter key.

            case KC_DOT: // . is punctuation, Shift . is a symbol (>)
                return !shifted ? '.' : '#';
            case KC_1:
            case KC_SLSH:
                return shifted ? '.' : '#';
            case KC_EXLM:
            case KC_QUES:
                return '.';
            case KC_2 ... KC_0:       // 2 3 4 5 6 7 8 9 0
            case KC_AT ... KC_RPRN:   // @ # $ % ^ & * ( )
            case KC_MINS ... KC_SCLN: // - = [ ] backslash ;
            case KC_UNDS ... KC_COLN: // _ + { } | :
            case KC_GRV:
            case KC_COMM:
                return '#'; // Symbol key.

            case KC_SPC:
                return ' '; // Space key.

            case KC_QUOT:
                return '\''; // Quote key.
        }
    }

    // Otherwise clear Sentence Case to initial state.
    sentence_case_clear();
    return '\0';
}

bool process_record_sentence_case(uint16_t keycode, keyrecord_t *record) {
    // Only process press events, and only while enabled.
    if (!record->event.pressed || sentence_state == STATE_DISABLED) {
        return true;
    }

#if SENTENCE_CASE_TIMEOUT > 0
    idle_timer = (record->event.time + SENTENCE_CASE_TIMEOUT) | 1;
#endif

    switch (keycode) {
        case KC_LCTL ... KC_RGUI: // Ignore mod keys.
        case QK_ONE_SHOT_MOD ... QK_ONE_SHOT_MOD_MAX:
        // Ignore MO, TO, TG, TT, OSL layer switch keys.
        case QK_MOMENTARY ... QK_MOMENTARY_MAX:
        case QK_TO ... QK_TO_MAX:
        case QK_TOGGLE_LAYER ... QK_TOGGLE_LAYER_MAX:
        case QK_LAYER_TAP_TOGGLE ... QK_LAYER_TAP_TOGGLE_MAX:
        case QK_ONE_SHOT_LAYER ... QK_ONE_SHOT_LAYER_MAX:
            return true;

        case QK_MOD_TAP ... QK_MOD_TAP_MAX:
            if (record->tap.count == 0) {
                return true;
            }
            keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
            break;
        case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:
            if (record->tap.count == 0) {
                return true;
            }
            keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
            break;
    }

    if (keycode == KC_BSPC) {
        // Backspace key pressed. Rewind the state and key buffers.
        set_sentence_state(state_history[STATE_HISTORY_SIZE - 1]);

        memmove(state_history + 1, state_history, STATE_HISTORY_SIZE - 1);
        state_history[0] = STATE_INIT;
#if SENTENCE_CASE_BUFFER_SIZE > 1
        memmove(key_buffer + 1, key_buffer, (SENTENCE_CASE_BUFFER_SIZE - 1) * sizeof(uint16_t));
        key_buffer[0] = KC_NO;
#endif
        return true;
    }

    const uint8_t mods      = get_mods() | get_weak_mods() | get_oneshot_mods();
    uint8_t       new_state = STATE_INIT;

    // We search for sentence beginnings using a simple finite state machine.
    // It matches things like "a. a" and "a.  a" but not "a.. a" or "a.a. a".
    // The state transition matrix is:
    //
    //             'a'       '.'      ' '      '\''
    //           +-------------------------------------
    //   INIT    | WORD      INIT     INIT     INIT
    //   WORD    | WORD      ENDING   INIT     WORD
    //   ABBREV  | ABBREV    ABBREV   INIT     ABBREV
    //   ENDING  | ABBREV    INIT     PRIMED   ENDING
    //   PRIMED  | match!    INIT     PRIMED   PRIMED
    char code = sentence_case_press(keycode, mods);
    switch (code) {
        case '\0': // Current key should be ignored.
            return true;

        case 'a': // Current key is a letter.
            switch (sentence_state) {
                case STATE_ABBREV:
                case STATE_ENDING:
                    new_state = STATE_ABBREV;
                    break;

                case STATE_PRIMED:
                    // This is the start of a sentence.
                    if (keycode != suppress_key) {
                        suppress_key = keycode;
                        set_oneshot_mods(MOD_BIT(KC_LSFT)); // Shift mod to capitalize.
                        new_state = STATE_WORD;
                    }
                    break;

                default:
                    new_state = STATE_WORD;
            }
            break;

        case '.': // Current key is sentence-ending punctuation.
            switch (sentence_state) {
                case STATE_WORD:
                    new_state = STATE_ENDING;
                    break;

                default:
                    new_state = STATE_ABBREV;
            }
            break;

        case ' ': // Current key is a space.
            if (sentence_state == STATE_PRIMED ||
                (sentence_state == STATE_ENDING
#if SENTENCE_CASE_BUFFER_SIZE > 1
                 && sentence_case_check_ending()
#endif
                     )) {
                new_state    = STATE_PRIMED;
                suppress_key = KC_NO;
            }
            break;

        case '\'': // Current key is a quote.
            new_state = sentence_state;
            break;
    }

    // Slide key_buffer and state_history buffers one element to the left.
#if SENTENCE_CASE_BUFFER_SIZE > 1
    for (int8_t i = 0; i < SENTENCE_CASE_BUFFER_SIZE - 1; ++i) {
        key_buffer[i] = key_buffer[i + 1];
    }
#endif
    for (int8_t i = 0; i < STATE_HISTORY_SIZE - 1; ++i) {
        state_history[i] = state_history[i + 1];
    }

#if SENTENCE_CASE_BUFFER_SIZE > 1
    key_buffer[SENTENCE_CASE_BUFFER_SIZE - 1] = keycode;
    if (new_state == STATE_ENDING && !sentence_case_check_ending()) {
        new_state = STATE_INIT;
    }
#endif
    state_history[STATE_HISTORY_SIZE - 1] = sentence_state;

    set_sentence_state(new_state);
    return true;
}

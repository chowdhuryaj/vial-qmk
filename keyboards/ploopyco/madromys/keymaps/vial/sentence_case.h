/* SPDX-License-Identifier: Apache-2.0 */
/* Ported from getreuer/qmk-modules sentence_case (Copyright 2022-2025 Google
 * LLC, Apache 2.0) for this Vial fork — community modules don't work here,
 * so the module lives in the keymap per the porting strategy in /CLAUDE.md.
 * Differences from upstream: the module's own ON/OFF/TOGGLE keycodes are gone
 * (the keymap's SC_TOG custom keycode calls sentence_case_toggle()), the weak
 * callbacks are static (no other keymap piece overrides them), and the
 * primed-indicator hook was dropped (no LED on this board).
 *
 * Sentence Case: automatically capitalizes the first letter of a sentence —
 * it watches for ". " (also "! ", "? ") and one-shot-shifts the next letter.
 * See <https://getreuer.info/posts/keyboards/sentence-case>.
 */
#pragma once

#include "quantum.h"

// Clear state after this long with no key events (ms). 100..30000; 0 disables.
#ifndef SENTENCE_CASE_TIMEOUT
#    define SENTENCE_CASE_TIMEOUT 5000
#endif

// Keycode history depth for the abbreviation check ("vs.", "etc." don't end a
// sentence). Needs >= 5 for that check; 0/1 disables it.
#ifndef SENTENCE_CASE_BUFFER_SIZE
#    define SENTENCE_CASE_BUFFER_SIZE 8
#endif

// Whether the feature starts enabled at boot. Runtime state is toggled by the
// SC_TOG keycode / HID and persisted in the EEPROM datablock.
#ifndef SENTENCE_CASE_ON_DEFAULT
#    define SENTENCE_CASE_ON_DEFAULT false
#endif

// Runs on every key event (state machine tracking). Always returns true.
bool process_record_sentence_case(uint16_t keycode, keyrecord_t *record);

// Idle timeout tick. Call from housekeeping_task_user.
void sentence_case_task(void);

void sentence_case_on(void);
void sentence_case_off(void);
void sentence_case_toggle(void);
bool is_sentence_case_on(void);

// Clears the matching state (e.g. when the host focus likely changed).
void sentence_case_clear(void);

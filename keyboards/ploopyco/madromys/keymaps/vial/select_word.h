/* SPDX-License-Identifier: Apache-2.0 */
/* Ported from getreuer/qmk-modules select_word (Copyright 2021-2026 Google
 * LLC, Apache 2.0) for this Vial fork — community modules don't work here,
 * so the module lives in the keymap per the porting strategy in /CLAUDE.md.
 * Differences from upstream: the module's own keycodes are gone (the keymap's
 * SELWORD/SELWDBK/SELLINE/SELLNUP custom keycodes call the register/unregister
 * API from process_record_user), and the compile-time Mac/Windows hotkey
 * choice is a runtime flag (HID-tunable, persisted) since the host OS can't
 * be detected on this build.
 *
 * Select Word: one key selects the current word; repeat to extend the
 * selection word-by-word. With Shift (or the LINE keycodes) it selects whole
 * lines. See <https://getreuer.info/posts/keyboards/select-word>.
 */
#pragma once

#include "quantum.h"

// Reset an in-progress selection after this long with no key events (ms).
// 100..30000 (16-bit timer limit); 0 disables the timeout.
#ifndef SELECT_WORD_TIMEOUT
#    define SELECT_WORD_TIMEOUT 5000
#endif

// Whether hotkeys default to macOS (Alt/GUI+arrows) or Windows/Linux
// (Ctrl/Home/End) style at boot. Runtime value is HID-tunable + persisted.
#ifndef SELECT_WORD_MAC_DEFAULT
#    define SELECT_WORD_MAC_DEFAULT true
#endif

// Starts a selection action: 'W' word forward, 'B' word backward,
// 'L' line forward, 'U' line upward. Call on keycode press.
void select_word_register(char action);

// Ends the action started by select_word_register. Call on keycode release.
void select_word_unregister(void);

// State tracking: must see every key event so a non-select key resets the
// selection direction. Call unconditionally from process_record_user.
void select_word_on_record(uint16_t keycode, keyrecord_t *record);

// Idle timeout tick. Call from housekeeping_task_user.
void select_word_task(void);

// Mac vs Windows/Linux hotkey style (runtime).
void select_word_set_mac(bool mac);
bool select_word_get_mac(void);

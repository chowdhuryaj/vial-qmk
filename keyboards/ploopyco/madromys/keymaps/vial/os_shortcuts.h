// OS-aware editing shortcuts (2026-07-03).
//
// One keycode per editing action; what it types depends on a runtime
// mac/pc mode: mac mode sends Cmd-based hotkeys (⌘X/⌘C/⌘V/⌘Z/⇧⌘Z),
// pc mode sends Ctrl-based ones (^X/^C/^V/^Z/^Y). The mode can follow
// QMK's OS detection (USB descriptor fingerprinting) or be pinned by
// the user; both knobs live on HID channel 0x1D and persist.
//
// Same file ships in the Ploopy Adept vial keymap and the Svalboard
// flask keymap — keep them identical (module convention: all defaults
// are #ifndef-guarded here and mirrored in the keymap's config.h).
#pragma once

#include "quantum.h"

// Editing actions, in custom-keycode order (OS_CUT..OS_REDO).
typedef enum {
    OS_SHORTCUT_CUT = 0,
    OS_SHORTCUT_COPY,
    OS_SHORTCUT_PASTE,
    OS_SHORTCUT_UNDO,
    OS_SHORTCUT_REDO,
} os_shortcut_t;

#ifndef OS_SHORTCUTS_MAC_DEFAULT
#    define OS_SHORTCUTS_MAC_DEFAULT true
#endif

// Follow = OS detection drives the mac/pc mode (and the keymap may mirror
// the result into other mac/pc booleans, e.g. select word's hotkey style).
#ifndef OS_SHORTCUTS_FOLLOW_DEFAULT
#    define OS_SHORTCUTS_FOLLOW_DEFAULT true
#endif

// Taps the mode-appropriate hotkey for the action.
void os_shortcuts_tap(os_shortcut_t action);

// Live mac/pc mode. Setting it manually does NOT clear follow — the next
// detection event will overwrite a manual value while follow is on.
void os_shortcuts_set_mac(bool mac);
bool os_shortcuts_get_mac(void);

// Follow-detection switch. Turning it on re-applies the last detection
// result immediately (if there was one).
void os_shortcuts_set_follow(bool follow);
bool os_shortcuts_get_follow(void);

// Feed a detection result in (from process_detected_host_os_kb). Returns
// true if the mac/pc mode changed as a result.
bool os_shortcuts_on_detect(os_variant_t os);

// Last detection result, raw os_variant_t (OS_UNSURE until detected) —
// exposed read-only over HID so the app can display it.
uint8_t os_shortcuts_detected(void);

/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/* ---------------------------------------------------------------------------
 * Vial
 * ------------------------------------------------------------------------- */
#define VIAL_KEYBOARD_UID {0xB5, 0x9E, 0xFB, 0x87, 0x97, 0x74, 0x0A, 0x9D}

// Hold top-left key (0,0) + second-row-third key (1,2) to unlock — same
// combo the stock NLOFIN firmware shipped with.
#define VIAL_UNLOCK_COMBO_ROWS {0, 1}
#define VIAL_UNLOCK_COMBO_COLS {0, 2}

// Emulated-EEPROM size (STM32F103 EFL wear-leveling, 1 KiB flash pages).
// SMALLER than the Adept's 16K/8K on purpose: the wear-leveling driver keeps
// a LOGICAL_SIZE RAM mirror, and the F103 has only 20 KB RAM — 8 KB logical
// left just 560 bytes of heap (measured 2026-07-06). 4 KB logical leaves
// ~4.6 KB free and still fits every carve-out below plus ~0.5 KB of Vial
// macros. Changing these relocates/reformats the EEPROM region — every
// persisted setting re-seeds on the next boot.
#define WEAR_LEVELING_BACKING_SIZE 8192
#define WEAR_LEVELING_LOGICAL_SIZE 4096

// Vial dynamic-entry capacities, pinned explicitly (Adept convention) so an
// EEPROM size change can't silently shrink them.
#define VIAL_TAP_DANCE_ENTRIES 32
#define VIAL_COMBO_ENTRIES 32
#define VIAL_KEY_OVERRIDE_ENTRIES 32

/* ---------------------------------------------------------------------------
 * Flask raw HID tuning (see raw_hid_receive_kb in keymap.c)
 *
 * VIA_CUSTOM_LIGHTING_ENABLE routes VIA command IDs 0x07/0x08/0x09 to
 * raw_hid_receive_kb. UNLIKE the Adept, this board ALSO compiles VIALRGB —
 * quantum/via.c calls vialrgb_*_value() first and raw_hid_receive_kb second
 * on the SAME buffer, so the handler must leave frames whose data[1] is a
 * VialRGB value ID (0x40..0x44) untouched or it would stomp the response
 * vialrgb just wrote. Flask channels start at 0x10 — the ranges are disjoint.
 * ------------------------------------------------------------------------- */
#define VIA_CUSTOM_LIGHTING_ENABLE

// EEPROM datablock holding every persisted tunable (nlk_config_t in keymap.c).
// Bump the VERSION whenever nlk_config_t's layout changes — the datablock
// then reads invalid and re-seeds from the defaults below.
// v1 = initial: getreuer module block (custom shift keys, select word,
//      sentence case), leader sequences, OS shortcuts, num word, per-combo
//      layer masks, per-layer per-key RGB map (8 layers x 23 LEDs x HSV),
//      display hold time. ~790 bytes used.
// v2 = appended disp_widgets[8] (per-line fallback-screen widget ids) +
//      disp_sleep_s (idle seconds before the panel switches off).
// v3 = appended autoscroll (+4 bytes: inverted, speed scale x100 u16,
//      stop-on-key). SIZE unchanged.
// v4 = display goes 4 big lines: disp_widgets shrank 8 -> 4; appended custom
//      widget text (4 x 5 chars), overlay duration u16, leader timeout u16.
#define EECONFIG_USER_DATA_SIZE 1024
#define EECONFIG_USER_DATA_VERSION 4

// Per-combo layer gating (Adept parity): compiles the combo_should_trigger()
// hook into quantum/process_combo.c; masks table edited over HID channel
// 0x20, persisted in nlk_config.
#define COMBO_SHOULD_TRIGGER

/* ---------------------------------------------------------------------------
 * Per-layer per-key RGB (per_layer_rgb.c/.h — this board's headline feature)
 *
 * A persisted HSV map, 8 layers x 23 LEDs. When enabled it repaints every
 * frame from rgb_matrix_indicators_user, so the active layer's colors win
 * over whatever VialRGB effect is running; when disabled the board behaves
 * like stock (VialRGB effects). Edited over HID channel 0x21; RGBMAP_TOG
 * toggles at the keyboard.
 * ------------------------------------------------------------------------- */
#define NLK_RGBMAP_LAYERS 8
#define NLK_RGBMAP_LEDS 23
#define NLK_RGBMAP_ENABLED_DEFAULT false

/* ---------------------------------------------------------------------------
 * OLED display (oled_display.c/.h)
 *
 * Panel: SSD1306 128x64, glass mounted portrait — OLED_ROTATION_90 in
 * keymap.c makes the logical canvas 64x128 = 10 chars x 16 lines.
 *
 * Fallback screen: layer readout plus feature indicators, drawn whenever
 * Flask isn't pushing content. Flask pushes text lines over HID channel
 * 0x22; pushed content sticks until it goes stale (no push for hold_ms) or
 * Flask releases the display.
 * ------------------------------------------------------------------------- */
#define NLK_DISPLAY_LINES 16
#define NLK_DISPLAY_COLS 10
// Glass window (hardware-mapped 2026-07-06 by pushing boundary probes): the
// glass is a 64x32 window into the SSD1306's 128x64 RAM — RAM columns 32..95
// and rows 0..31. On the 16-line portrait canvas that is logical lines 4..11
// and text columns 0..4 (a 6th column shows its first pixel sliver). The full
// 16x10 push address space stays (protocol stability); the fallback screen
// draws inside the window only.
#define NLK_DISPLAY_VISIBLE_FIRST 4
#define NLK_DISPLAY_VISIBLE_LINES 8
#define NLK_DISPLAY_VISIBLE_COLS 5
#define NLK_DISPLAY_HOLD_MS_DEFAULT 3000
#define NLK_DISPLAY_HOLD_MS_MIN 500
#define NLK_DISPLAY_HOLD_MS_MAX 60000
// Idle sleep: seconds without key/encoder input before the status screen
// stops drawing and the panel turns off (0 = never). Any key wakes it.
// Without this the panel never slept — the ticking uptime widget re-armed
// the driver's OLED_TIMEOUT every second (burn-in). HID 0x22/0x0A.
#define NLK_DISPLAY_SLEEP_S_DEFAULT 120
#define NLK_DISPLAY_SLEEP_S_MAX 3600
// Transient overlays (v5): volume / RGB brightness / autoscroll level flash
// on the glass for this long (0 = disabled). HID 0x22/0x0B.
#define NLK_DISPLAY_OVERLAY_MS_DEFAULT 2000
#define NLK_DISPLAY_OVERLAY_MS_MAX 10000
// Layer names for the LAYERNAME widget (v8) — matches the baked RadKit
// layer plan in keymap.c (5 chars max).
#define NLK_LAYER_NAMES \
    { "BASE", "RGB", "DICT", "CALL", "UTIL", "PACS", "SITE", "CONF" }

/* ---------------------------------------------------------------------------
 * RGB boot/wake holdoff (keymap.c)
 *
 * Hardware-bisected 2026-07-06: the OLED panel wedges (ACKs I2C, stops
 * applying writes) ONLY when the 23-LED strip lights at full brightness
 * during the USB power-up window — the same animation at the same brightness
 * is harmless once enumeration has settled (pre-configuration 100 mA budget
 * vs the 500 mA grant; the sag latches the panel's charge pump). So: LEDs
 * held dark for this long after boot AND after USB wake, then restored from
 * EEPROM, with a defensive panel re-init at the moment of release.
 * ------------------------------------------------------------------------- */
#define NLK_RGB_HOLDOFF_MS 3000

/* RGB defaults dark (v7, user ask): the Maple bootloader mass-erases the
 * EEPROM on every flash, so whatever this seeds IS the post-flash state —
 * boot with the strip off instead of the stock animation. RGB_TOG (or
 * Flask's RGB tab) turns it on, and THAT persists across power cycles
 * (but not across reflashes — bootloader erase, see CLAUDE.md). */
#define RGB_MATRIX_DEFAULT_ON false

/* ---------------------------------------------------------------------------
 * Tap-hold and combos
 * ------------------------------------------------------------------------- */
#define TAPPING_TERM 200
#define COMBO_TERM 50

/* ---------------------------------------------------------------------------
 * Custom shift keys (shared/custom_shift_keys, getreuer port)
 * ------------------------------------------------------------------------- */
#define CUSTOM_SHIFT_KEYS_SLOTS 16
#define CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT true

/* ---------------------------------------------------------------------------
 * Select word (shared/select_word, getreuer port)
 * ------------------------------------------------------------------------- */
#define SELECT_WORD_TIMEOUT 5000
// pc default (v8): deployment target is a locked-down Windows workstation
// and the bootloader erase re-seeds these on every flash — if OS detection
// ever fails there, the fallback must already be Windows behavior. Follow
// mode still flips to mac automatically on a Mac host.
#define SELECT_WORD_MAC_DEFAULT false

/* ---------------------------------------------------------------------------
 * Sentence case (shared/sentence_case, getreuer port)
 * ------------------------------------------------------------------------- */
#define SENTENCE_CASE_TIMEOUT 5000
#define SENTENCE_CASE_BUFFER_SIZE 8
#define SENTENCE_CASE_ON_DEFAULT false

/* ---------------------------------------------------------------------------
 * OS-aware shortcuts (shared/os_shortcuts)
 * ------------------------------------------------------------------------- */
// pc default (v8) — same rationale as SELECT_WORD_MAC_DEFAULT above.
#define OS_SHORTCUTS_MAC_DEFAULT false
#define OS_SHORTCUTS_FOLLOW_DEFAULT true

/* ---------------------------------------------------------------------------
 * Num word (shared/num_word)
 * ------------------------------------------------------------------------- */
#define NUM_WORD_LAYER_DEFAULT 1
#define NUM_WORD_IDLE_TIMEOUT_DEFAULT 5000
#define NUM_WORD_IDLE_TIMEOUT_MAX 30000

/* ---------------------------------------------------------------------------
 * Autoscroll (shared/autoscroll) — stepped mode only: ASC_UP/ASC_DOWN move a
 * signed speed level (bind to a knob = scroll-speed dial). No ball → jog
 * mode can never engage; jog defines stay at module defaults. Rides a
 * "custom" pointing driver whose weak defaults are no-ops (rules.mk).
 * ------------------------------------------------------------------------- */
#define AUTOSCROLL_SPEED_SCALE_X100 100
#define AUTOSCROLL_INVERTED_DEFAULT false
#define AUTOSCROLL_STOP_ON_KEY_DEFAULT true

/* ---------------------------------------------------------------------------
 * Leader key (QMK core feature; sequences are dynamic, Adept pattern)
 * ------------------------------------------------------------------------- */
#define LEADER_TIMEOUT 400
#define LEADER_PER_KEY_TIMING
#define NLK_LEADER_SEQ_COUNT 8
#define NLK_LEADER_SEQ_KEYS 5
// Live-tunable timeout bounds (v5, HID 0x19/0x01): output fires this many ms
// after the last sequence key — lower = snappier, less typing margin.
#define NLK_LEADER_TIMEOUT_MIN 100
#define NLK_LEADER_TIMEOUT_MAX 2000

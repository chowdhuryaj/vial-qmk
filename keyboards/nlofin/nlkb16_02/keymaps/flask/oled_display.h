/* SPDX-License-Identifier: GPL-2.0-or-later */
/* OLED display: Flask-driven text with a layer-status fallback.
 *
 * Flask pushes up to 4 lines x 21 chars over HID channel 0x22; pushed
 * content is shown while fresh (re-pushed within hold_ms) and the display
 * falls back to the built-in layer/status screen when it goes stale or is
 * explicitly released. Nothing here persists except hold_ms (nlk_config).
 */

#pragma once

#include "quantum.h"

#ifndef NLK_DISPLAY_LINES
#    define NLK_DISPLAY_LINES 4
#endif
#ifndef NLK_DISPLAY_COLS
#    define NLK_DISPLAY_COLS 21
#endif
#ifndef NLK_DISPLAY_VISIBLE_FIRST
#    define NLK_DISPLAY_VISIBLE_FIRST 0
#endif
#ifndef NLK_DISPLAY_VISIBLE_LINES
#    define NLK_DISPLAY_VISIBLE_LINES NLK_DISPLAY_LINES
#endif
#ifndef NLK_DISPLAY_VISIBLE_COLS
#    define NLK_DISPLAY_VISIBLE_COLS NLK_DISPLAY_COLS
#endif
#ifndef NLK_DISPLAY_HOLD_MS_DEFAULT
#    define NLK_DISPLAY_HOLD_MS_DEFAULT 3000
#endif
#ifndef NLK_DISPLAY_HOLD_MS_MIN
#    define NLK_DISPLAY_HOLD_MS_MIN 500
#endif
#ifndef NLK_DISPLAY_HOLD_MS_MAX
#    define NLK_DISPLAY_HOLD_MS_MAX 60000
#endif

// Idle sleep (v3): seconds of no key/encoder input before the status screen
// stops drawing and the panel is switched off (0 = never sleep). Without
// this the panel NEVER slept: the driver's OLED_TIMEOUT is re-armed by any
// dirty block, and the ticking uptime widget dirtied one every second —
// permanent-on OLED = burn-in. Any key wakes it (quantum calls oled_on on
// input activity); a Flask push wakes it too (pushed content re-renders).
#ifndef NLK_DISPLAY_SLEEP_S_DEFAULT
#    define NLK_DISPLAY_SLEEP_S_DEFAULT 120
#endif
#ifndef NLK_DISPLAY_SLEEP_S_MAX
#    define NLK_DISPLAY_SLEEP_S_MAX 3600
#endif

// Big-line model (v5, 2026-07-07): the glass renders 4 lines of 5 characters
// in a double-height font (each 6×8 glyph drawn 6×16 — our own copy of the
// oled font, pixel-doubled vertically). Widgets, pushed content and the
// transient overlays all draw through this one renderer.
#define NLK_DISPLAY_BIG_LINES 4
#define NLK_DISPLAY_BIG_COLS 5

// Fallback-screen widgets: each of the 4 lines renders one widget,
// assignable over HID (display channel, 0x20 + line) and persisted in
// nlk_config. IDs are wire values — append-only, same spirit as HID ids.
typedef enum {
    NLK_WIDGET_BLANK = 0,
    NLK_WIDGET_LAYER,      // "LYR n" — active layer, digit inverted
    NLK_WIDGET_UPTIME,     // "T nnn" — uptime seconds (liveness tick)
    NLK_WIDGET_MODS,       // "CSAG" — held modifiers, each lit while down
    NLK_WIDGET_OSM,        // "oCSAG" — pending one-shot mods (incl locked)
    NLK_WIDGET_OSL,        // "OSL n" — one-shot layer while active, else "-"
    NLK_WIDGET_LOCKS,      // "C N S" — caps/num/scroll lock, each lit
    NLK_WIDGET_CAPS,       // "CAP" inverted while caps lock on
    NLK_WIDGET_NUMLOCK,    // "NLK" inverted while num lock on
    NLK_WIDGET_SCROLLLOCK, // "SLK" inverted while scroll lock on
    NLK_WIDGET_RGBMAP,     // "MAP" inverted while the per-layer RGB map is on
    NLK_WIDGET_NUMWORD,    // "NUM" inverted while num word is active
    NLK_WIDGET_SENTENCE,   // "SC" inverted while sentence case is on
    NLK_WIDGET_CUSTOM,     // v5: the line's stored custom text (HID 0x30+line)
    NLK_WIDGET_LAYERNAME,  // v8: active layer's 5-char name (NLK_LAYER_NAMES)
    NLK_WIDGET_COUNT
} nlk_widget_t;

// 5-char name per layer for NLK_WIDGET_LAYERNAME (shorter names space-pad).
#ifndef NLK_LAYER_NAMES
#    define NLK_LAYER_NAMES \
        { "L0", "L1", "L2", "L3", "L4", "L5", "L6", "L7" }
#endif

// Boot/reseed assignment for the 4 lines. LAYERNAME on top: post-flash on a
// host with no Flask, the glass itself must say which work layer is active.
#define NLK_DISPLAY_WIDGET_DEFAULTS \
    { NLK_WIDGET_LAYERNAME, NLK_WIDGET_MODS, NLK_WIDGET_LOCKS, NLK_WIDGET_RGBMAP }

// Per-line widget assignment (line 0-3, top to bottom). Setter clamps
// out-of-range widget ids to the last valid one.
void     oled_display_set_widget(uint8_t line, uint8_t widget);
uint8_t  oled_display_get_widget(uint8_t line);
// Live table pointer (NLK_DISPLAY_BIG_LINES bytes) for config save/apply.
uint8_t *oled_display_widget_table(void);

// Custom text per line (shown by NLK_WIDGET_CUSTOM). Stored space-padded to
// NLK_DISPLAY_BIG_COLS; non-printable bytes become spaces.
void        oled_display_set_custom(uint8_t line, const uint8_t *text, uint8_t len);
const char *oled_display_get_custom(uint8_t line); // NUL-terminated, 5 chars

// Rendered-line mirror (HID GET 0x0C, protocol v6): copies what the glass
// currently shows straight from the renderer's per-line cache, so pushed
// content, widgets and overlays all mirror truthfully. out[0] = the line's
// per-char invert mask, out[1..5] = its 5 chars (space-padded, no
// terminator). Returns false for an out-of-range line.
bool oled_display_rendered_line(uint8_t line, uint8_t *out);
// Panel power state (false while the idle sleep / driver timeout has it off).
bool oled_display_panel_on(void);

// Transient overlays (v5): keymap hooks call these on volume / RGB
// brightness / autoscroll events; the glass shows a big label + live value
// for overlay_ms (0 = overlays disabled), then falls back.
typedef enum {
    NLK_OVERLAY_NONE = 0,
    NLK_OVERLAY_VOLUME,     // aux: '+' / '-' / 'M' (mute) — host owns the value
    NLK_OVERLAY_RGB_VAL,    // value read live from rgb_matrix_get_val()
    NLK_OVERLAY_AUTOSCROLL, // value read live from autoscroll_get_level()
} nlk_overlay_t;

void oled_display_overlay(nlk_overlay_t type, char aux);

#ifndef NLK_DISPLAY_OVERLAY_MS_DEFAULT
#    define NLK_DISPLAY_OVERLAY_MS_DEFAULT 2000
#endif
#ifndef NLK_DISPLAY_OVERLAY_MS_MAX
#    define NLK_DISPLAY_OVERLAY_MS_MAX 10000
#endif
void     oled_display_set_overlay_ms(uint16_t ms); // clamps; 0 disables
uint16_t oled_display_get_overlay_ms(void);

// Store one pushed line (v5: line is a BIG line 0-3, len capped to
// NLK_DISPLAY_BIG_COLS; non-printable bytes become spaces). Refreshes the
// staleness timer.
void oled_display_push_line(uint8_t line, const uint8_t *text, uint8_t len);
// Drop pushed content immediately (back to the fallback screen).
void oled_display_release(void);
bool oled_display_pushed_active(void);
// Diagnostics: 0xFFFF when nothing pushed, else seconds since the last push.
uint16_t oled_display_seconds_since_push(void);
// I2C failure/success counters (transmit overrides in oled_display.c).
uint16_t oled_display_i2c_fails(void);
uint16_t oled_display_i2c_recovers(void);
// On-demand bus scan: run + result as (ack_count << 8) | first_ack_addr.
void     oled_display_i2c_scan(void);
uint16_t oled_display_i2c_scan_result(void);

void     oled_display_set_hold_ms(uint16_t ms); // clamps to MIN..MAX
uint16_t oled_display_get_hold_ms(void);
void     oled_display_set_sleep_s(uint16_t s); // clamps to 0..SLEEP_S_MAX
uint16_t oled_display_get_sleep_s(void);

// Panel-probe hooks (HID SET 0x07/0x08). Both run deferred inside
// oled_display_task — raw_hid_receive and oled_task share the main-loop
// context, but deferring keeps every panel transfer on the oled_task path.
// Queue up to 8 raw command bytes (sent with a 0x00 control-byte prefix).
void oled_display_queue_raw_cmd(const uint8_t *bytes, uint8_t len);
// Result of the last raw command: 0xFFFF = none sent yet, 1 = ACK, 0 = fail.
uint16_t oled_display_raw_cmd_result(void);
// Re-run the full oled_init() sequence (panel un-wedge probe).
void oled_display_request_reinit(void);

// Call from oled_task_user. Draws either the pushed content or the fallback
// layer/status screen. Always returns false (rendering fully handled).
bool oled_display_task(void);

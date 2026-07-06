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

// Store one pushed line (non-printable bytes become spaces; len capped to
// NLK_DISPLAY_COLS). Refreshes the staleness timer.
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

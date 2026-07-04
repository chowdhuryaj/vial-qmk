// Freeze diagnostic (2026-07-03).
//
// The user reports the cursor randomly freezing for 1-2 s on BOTH devices,
// even with the companion app closed. This watermark discriminates the
// firmware from the host: pipeline_diag_tick() runs once per pointing-task
// pass, and the module keeps the LARGEST gap between passes since the last
// reset. After a freeze:
//   watermark ≈ the freeze length  → the firmware main loop stalled
//                                    (flash write, blocking code) — ours.
//   watermark stays single-digit   → firmware kept running; the stall is
//                                    the sensor reporting zeros or the host
//                                    (macOS/USB) — not a main-loop hang.
//
// HID channel 0x1F: 0x01 GET watermark / SET resets, 0x02 uptime seconds
// (wraps ~18 h — a small value after a freeze means the board rebooted,
// which is its own answer). Nothing here persists.
//
// Same file ships in the Adept vial keymap and the Svalboard flask keymap.
#pragma once

#include "quantum.h"

// Call once per pointing-device task pass (top of the pipeline).
void pipeline_diag_tick(void);

uint16_t pipeline_diag_max_gap_ms(void);
void     pipeline_diag_reset(void);
uint16_t pipeline_diag_uptime_s(void);

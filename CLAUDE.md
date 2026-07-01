# Ploopy Adept Vial-QMK Firmware Project

Status: built and working. This file is the editing contract for this
keymap — read the relevant section before changing anything, and **run
`keyboards/ploopyco/madromys/keymaps/vial/check.sh` after every edit**
before calling a change done. It catches the mistake classes a clean
`make` won't (keycode-list desync, unwired source files, unhandled
keycodes) and then runs the real build.

## Device
- Ploopy Adept Trackball (codename: madromys)
- Board path: keyboards/ploopyco/madromys/rev1_001
- Chip: RP2040, sensor: PMW3360, 6 buttons, no scroll wheel
- Flashes via .uf2 drag-and-drop (hold Bottom Left button on USB plug-in for bootloader)

## Constraints
- This is the VIAL fork of QMK, not mainline QMK. Community modules
  (qmk_modules.json) do NOT work here — ported code lives directly in the keymap.
- This entire ploopyco keyboard family ships headless in this fork (keymaps
  only, no keyboard-level definition). The madromys keyboard-level files
  (config.h, info.json, post_rules.mk, rev1_001/keyboard.json) were
  reconstructed from mainline QMK — see git history if a sibling board
  (mouse/trackball/trackball_thumb) needs the same treatment.
- Leader Key is NOT supported by Vial.

## File map — exact responsibility per file
| File | Responsibility |
|---|---|
| `keyboards/ploopyco/madromys/config.h` | Keyboard-level hardware wiring only (RP2040/PMW3360 SPI pins, UNUSABLE_PINS). Not for keymap tuning. |
| `keyboards/ploopyco/madromys/info.json` | USB VID/PID, physical layout (`LAYOUT()` shape), `dynamic_keymap.layer_count` (8). |
| `keyboards/ploopyco/madromys/post_rules.mk` | `POINTING_DEVICE_DRIVER = pmw3360`. |
| `keyboards/ploopyco/madromys/rev1_001/keyboard.json` | Matrix pins, diode direction, ws2812/rgblight wiring for this PCB rev. |
| `keymaps/vial/config.h` | **Every tuning parameter and feature define lives here.** First file to check for "change a default" requests. Also carries `POINTING_DEVICE_DEBUG` (live-tuning console output — see below). |
| `keymaps/vial/keymap.c` | Layers, the custom keycode enum, `process_record_user`, the `pointing_device_task_user` pipeline, DPI/EEPROM state, status report. |
| `keymaps/vial/vial.json` | Vial GUI layout + `customKeycodes[]`. Must stay length-matched with the enum — see the rule below. |
| `keymaps/vial/rules.mk` | Feature enables (VIA/VIAL/COMBO/TAP_DANCE/DEFERRED_EXEC) + `SRC +=` for every ported module `.c`. |
| `keymaps/vial/pd_accel.c/.h` | Acceleration curve (ported from drashna `pointing_device_accel`). |
| `keymaps/vial/pointing_device_smoothing.c/.h` | EMA smoothing (ported from drashna `pointing_device_smoothing`). |
| `keymaps/vial/pd_gestures.c/.h` | Directional flick gestures (ported from drashna `pointing_device_gestures`; renamed `pd_gestures_*` because QMK core already has a *different* built-in `pointing_device_gestures` — cursor glide — and the names would collide). |
| `keymaps/vial/drag_scroll.c/.h` | Drag-to-scroll (ported from drashna `drag_scroll`). |
| `keymaps/vial/wiggle_ball.c/.h` | Shake-to-toggle drag scroll (ported from drashna `wiggle_ball`). Runs BEFORE both `pd_gestures_apply()` and `drag_scroll_apply()` in the pipeline so it always reads raw ball axes (a live gesture zeroes them; scrolling divides them down) — same shake must work to toggle scrolling both ways AND to escape a gesture. On a shake it cancels an active gesture (consumed there, no scroll toggle) else toggles drag scroll. |
| `keymaps/vial/am_tuning.c/.h` | Overrides QMK core's weak `auto_mouse_activation()` so the activation *threshold* is runtime-tunable (core only exposes enable/layer/timeout/debounce as live-settable; threshold is normally compile-time only). |
| `keymaps/vial/check.sh` | Validator. Run after every edit. |

## THE keycode rule — read before adding/removing any custom keycode
`vial.json`'s `customKeycodes[i]` always maps to firmware keycode `QK_KB_0 + i`
(`0x7E00 + i`). Concretely:

1. `enum madromys_keycodes` in `keymap.c` and `customKeycodes[]` in
   `vial.json` must have **identical length and identical order** — index 0
   in one is the same logical keycode as index 0 in the other.
2. **Adding** a keycode: append to the **end** of both lists. Never insert
   in the middle — that silently reassigns every keycode after it.
3. **Removing**: either remove from the end, or renumber everything below
   the removed entry in both files identically.
4. Ceiling: 32 slots (`QK_KB_0..QK_KB_31`). `check.sh` reports current usage.

`check.sh` verifies the two lists are the same length and that every enum
entry has a `process_record_user` case. It cannot verify *semantic* order
match (it can't map a C identifier to a display string) — get that right by hand.

For the live, current keycode table (index / enum name / vial.json name),
run `check.sh` — it parses both files and prints the table fresh every time.
Don't hand-copy it into a doc; it changes too often to keep in sync that way.

## Tap dances (not currently used — gotcha if you add them)
The base layer uses plain keycodes + simple `LT()` layer-taps, not tap dances.
A firmware-seeded tap-dance base row was built and then reverted (plain
layer-taps were simpler for the user). If you ever add tap dances back: this
fork's Vial **owns** `tap_dance_actions[]` (`quantum/vial.c`) as a strong symbol,
so a keymap **cannot** define its own static `tap_dance_actions[]` (duplicate
strong symbol = link error). Use Vial's *dynamic* (EEPROM) tap dances instead —
four keycode slots each (`{on_tap, on_hold, on_double_tap, on_tap_hold}` +
`custom_tapping_term`), set in the GUI or seeded from firmware via
`dynamic_keymap_set_tap_dance()`. Slot keycodes may be custom (`QK_KB_*`) or
`MO(layer)` — both fire (Vial routes slots > `QK_MODS_MAX` through `action_exec`
→ `process_record_user`). `check.sh` does not validate any of this.

## Conventions for adding a new tunable
1. Every live-adjustable numeric parameter follows the same step pattern: an
   `_increment()` function reads `get_mods()`, multiplies the base step by
   10 if Ctrl is held, negates it if Shift is held, then clamps the result
   to a sane floor/ceiling before storing. Mirror this exactly.
2. Step sizes and floors/ceilings are `#ifndef`-guarded in the module's own
   `.h`, AND mirrored as explicit `#define`s in `keymaps/vial/config.h` (even
   though redundant) so config.h stays the one place to look for every
   tunable's current value.
3. New module files are paired `.c`/`.h` in `keymaps/vial/`, named after the
   feature (snake_case), and **must** be added to `rules.mk`'s `SRC +=` —
   `check.sh` catches a forgotten one, but a plain `make` will not (it just
   silently doesn't compile the file in; the keycode no-ops at runtime).
4. If overriding a QMK core weak function (like `am_tuning.c` does for
   `auto_mouse_activation`), comment which file defines the weak default —
   a future edit shouldn't redefine it again (duplicate strong symbol = link
   error) or assume no override exists.
5. Never format floats with `snprintf`'s `%f` in this codebase — this
   build's libc doesn't reliably support it on RP2040. Use the existing
   `fmt2()` helper in `keymap.c` (integer math), and clamp any new value to
   a provably bounded range first, or `-Werror=format-truncation` fails the
   build (GCC can't see through a float→int cast to know the runtime value
   is small).

## Build & validate
```
make ploopyco/madromys/rev1_001:vial                      # build only
keyboards/ploopyco/madromys/keymaps/vial/check.sh          # build + consistency checks (recommended)
keyboards/ploopyco/madromys/keymaps/vial/check.sh --skip-build   # fast structural checks only
```

## Layer plan (implemented)
- `_BASE` (0): right hand. `DRG_TOG` on Top-Left-Left; plain clicks BTN1=Bottom-Left
  and BTN3=Bottom-Right; three `LT()` layer-tap mod-taps on the other top keys
  (tap = the mouse button, hold = a layer): Top-Left `LT(_SCRL, BTN4)`, Top-Right
  `LT(_FN, BTN5)`, Top-Right-Right `LT(_MOUSE, BTN2)`.
- `_MOUSE` (1): auto-mouse target layer (right hand only; in left-hand mode
  auto-mouse targets the left base itself — see the auto-mouse note below).
  BTN1=Bottom-Left, BTN2=Bottom-Right, `LT(_SCRL, KC_BTN3)` mod-tap on
  Top-Left-Left (tap=BTN3, hold=Scroll layer).
- `_SCRL` (2): drag-scroll oriented. Same BTN1/BTN2/BTN3 positions as `_BASE`/`_MOUSE`
  (Bottom-Left/Bottom-Right/Top-Left-Left), no mod-tap.
- `_FN` (3): DPI / ambidextrous toggle / bootloader.
- `_BASE_L`/`_MOUSE_L`/`_SCRL_L`/`_FN_L` (4–7): left-hand mirror of 0–3 —
  structurally identical but with BTN1/BTN2/BTN3 each placed at the physical
  mirror of its right-hand position (identity unchanged, position flipped):
  BTN1 Bottom-Right, BTN2 Bottom-Left, BTN3 Top-Right-Right. The hold side of
  each mod-tap rides along with its button (Fn-hold with BTN2, Scroll-hold
  with BTN3), and `LT()` targets are +4. `AMBI_TOG` flips the *persisted*
  default layer between `_BASE` (right) and `_BASE_L` (left); persistence is
  plain QMK default-layer EEPROM state. The shared layer-switching logic adds
  a runtime hand offset (`HAND_LAYER_COUNT`) instead of duplicating per-half
  logic — only the keymap *data* (rows 4–7) is duplicated, not the
  gesture/layer code. The right-hand `_BASE` `LT()` layer-taps are not mirrored
  onto `_BASE_L` (which keeps its own `LT(_FN_L, …)`/`LT(_SCRL_L, …)` targets).
- **Auto-mouse target by hand:** right hand → `_MOUSE` (1); left hand → its own
  base `_BASE_L` (4), which already carries mouse buttons, so ball motion never
  switches layers in left-hand mode. `auto_mouse_target()` in `keymap.c` encodes
  this; `AUTO_MOUSE_DEFAULT_LAYER 1` (config.h) is only the pre-runtime default.

## Ambidextrous + lock-LED notes
- `AMBI_TOG` (custom keycode) is the only hand-mode control; bind it via Vial.
  `_FN`/`_FN_L` carry it by default (Top-Left-Left) so left mode is escapable.
- Drag scroll is forced on while any host lock LED (caps/num/scroll) is active and
  released when all three clear — `led_update_user` drives a force flag in
  `drag_scroll.c` that vetoes manual turn-off while engaged. The lock condition
  overrides manual control; manual `DRG_*`/scroll-layer control resumes once all
  locks are off.
- Drag scroll also toggles hands-free: shaking the ball left-right (4 direction
  reversals inside the timing windows) flips it via `wiggle_ball.c`, which always
  reads the raw ball axes (x/y) regardless of scrolling state — it runs first in
  the pipeline (before `pd_gestures_apply()` and `drag_scroll_apply()`) for
  exactly this reason. (Reading the wheel output while scrolling was tried first
  and was a bug: divided down by `SCROLL_DIVISOR_H`/`V`, it almost never exceeded
  the shake's magnitude check, so scrolling could turn on via wiggle but not back
  off.)
- A shake **also escapes an active gesture**: since a live gesture zeroes x/y in
  `pd_gestures_apply()`, wiggle must (and does) run before it to still see the
  shake. On a shake with a gesture open, `wiggle_ball.c` calls
  `pd_gestures_cancel()` and consumes the shake (no drag-scroll toggle);
  otherwise it toggles drag scroll. Pipeline order is now
  `smoothing → wiggle → gestures → drag_scroll → accel`; wiggle's input is still
  the smoothed, pre-drag-scroll axes, so the shake thresholds didn't need
  re-tuning. (Watch for false cancels if a gesture flick reverses L↔R fast enough
  to look like a shake.)

## Live tuning via qmk console
`POINTING_DEVICE_DEBUG` (config.h) + `CONSOLE_ENABLE = yes` (rules.mk) turn on
QMK's `pd_dprintf()` call sites (`quantum/pointing_device_internal.h`) — used
by core sensor drivers and this keymap's `am_tuning.c` (`AM: x=.. y=.. h=.. v=..
thr=..` while accumulating, `AM: ACTIVATED ...` on trigger, and the new value
whenever `AM_THR`/`AM_TIME` is pressed). Nothing prints until QMK's runtime
`debug_enable` flag is also on — toggle it with `DB_TOGG` (Bottom Left on
`_FN`/`_FN_L`) so normal use isn't spammed with console traffic. Workflow: flash,
run `qmk console` in a terminal, press `DB_TOGG`, nudge the ball / press
`AM_THR`/`AM_TIME` and watch the live values, press `DB_TOGG` again when done.
This is compile-time gated (`POINTING_DEVICE_DEBUG`) rather than always-on so it
can be stripped later with a one-line config.h change if unwanted.

## Porting strategy (for any future drashna module)
1. Read the module's `.c`/`.h` in `~/drashna-modules-reference/<module>/`.
2. Strip community-module plumbing (`ASSERT_COMMUNITY_MODULES_MIN_API_VERSION`,
   `_kb` call chaining, VIA/EEPROM integration, its own custom keycodes).
3. Rename anything that could collide with a QMK-core built-in of a similar
   name (this bit us with `pointing_device_gestures` — check first).
4. Wire keycodes through `keymap.c`'s `process_record_user` and pipeline
   through `pointing_device_task_user`, not the module's own dispatch.
5. Move all `#define` configuration into `keymaps/vial/config.h`.
6. Add custom keycodes to the enum **and** `vial.json`, same order, same length.
7. Run `check.sh`.

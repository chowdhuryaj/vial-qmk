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
| `keyboards/ploopyco/madromys/info.json` | USB VID/PID, physical layout (`LAYOUT()` shape), `dynamic_keymap.layer_count` (8 — 0-3 real, 4-7 spare blanks). |
| `keyboards/ploopyco/madromys/post_rules.mk` | `POINTING_DEVICE_DRIVER = pmw3360`. |
| `keyboards/ploopyco/madromys/rev1_001/keyboard.json` | Matrix pins, diode direction, ws2812/rgblight wiring for this PCB rev. |
| `keymaps/vial/config.h` | **Every tuning parameter and feature define lives here.** First file to check for "change a default" requests. Also carries `POINTING_DEVICE_DEBUG` (live-tuning console output — see below). |
| `keymaps/vial/keymap.c` | Layers, the custom keycode enum, `process_record_user`, the `pointing_device_task_user` pipeline, DPI/EEPROM state, status report. |
| `keymaps/vial/vial.json` | Vial GUI layout + `customKeycodes[]`. Must stay length-matched with the enum — see the rule below. |
| `keymaps/vial/rules.mk` | Feature enables (VIA/VIAL/COMBO/TAP_DANCE/DEFERRED_EXEC) + `SRC +=` for every ported module `.c`. |
| `keymaps/vial/pd_accel.c/.h` | Acceleration curve (ported from drashna `pointing_device_accel`). |
| `keymaps/vial/pointing_device_smoothing.c/.h` | EMA smoothing (ported from drashna `pointing_device_smoothing`). |
| `keymaps/vial/pd_gestures.c/.h` | Directional flick gestures (ported from drashna `pointing_device_gestures`; renamed `pd_gestures_*` because QMK core already has a *different* built-in `pointing_device_gestures` — cursor glide — and the names would collide). |
| `keymaps/vial/drag_scroll.c/.h` | Drag-to-scroll (ported from drashna `drag_scroll`). No wiggle/shake control and no host-lock-LED force — both were removed 2026-07-01; manual keycodes (`DRG_TOG`/`DRG_MO`/`DRG_INV`) and the `DRAG_SCROLL_LAYER` binding are the only controls now. |
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
4. If you override a QMK core weak function, comment which file defines the
   weak default — a future edit shouldn't redefine it again (duplicate strong
   symbol = link error) or assume no override exists. (No keymap file currently
   does this; auto-mouse's `am_tuning.c` override was removed 2026-07-01.)
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
Four real layers (0-3), single-hand (right), plus four spare blank layers (4-7,
all `KC_TRNS`) kept so Vial exposes 8 dynamic layers for GUI-side use — nothing
in firmware references the spares. The ambidextrous / left-hand mirror set and
auto-mouse were both removed 2026-07-01 (see "Removed features" below).
- `_BASE` (0): `DRG_TOG` on Top-Left-Left; plain clicks BTN1=Bottom-Left
  and BTN3=Bottom-Right; three `LT()` layer-tap mod-taps on the other top keys
  (tap = the mouse button, hold = a layer): Top-Left `LT(_SCRL, BTN4)`, Top-Right
  `LT(_FN, BTN5)`, Top-Right-Right `LT(_MOUSE, BTN2)`.
- `_MOUSE` (1): momentary mouse layer, reached ONLY by **holding**
  `LT(_MOUSE, BTN2)` on `_BASE` Top-Right-Right (no auto-mouse — ball motion
  never switches layers). `DRG_TOG` on Top-Left-Left (same key as `_BASE`, kept
  so the toggle stays reachable while the layer is held); BTN3 on Top-Right-Right,
  BTN1=Bottom-Left, BTN2=Bottom-Right, DPI down/up on Top-Left/Top-Right.
- `_SCRL` (2): drag-scroll oriented. `DRG_TOG` on Top-Right-Right (not Top-Left-Left
  here); wheel up/down on Top-Left/Top-Right; same BTN1/BTN2 positions as
  `_BASE`/`_MOUSE`, BTN3 on Top-Left-Left. This is `DRAG_SCROLL_LAYER` — entering
  it turns drag scroll on, leaving turns it off (`layer_state_set_user`).
- `_FN` (3): gesture-set-D ("PACS nav") latch (`GRD_TOG`), DPI cycle, a Vial
  macro slot (`MC_0`, authored in the Vial GUI), debug-console toggle (`DB_TOGG`),
  bootloader. Top-Left-Left is free (`KC_NO`) — was `AMBI_TOG` before the
  ambidextrous removal; bind anything via Vial.

## Removed features — auto-mouse & ambidextrous (removed 2026-07-01)
Both stripped at user request to cut complexity; don't re-add without a fresh ask.
- **Auto-mouse** (`POINTING_DEVICE_AUTO_MOUSE_ENABLE` + `am_tuning.c/.h` +
  `AM_THR`/`AM_TIME` keycodes + `auto_mouse_target()` + all `AUTO_MOUSE_*`
  defines): gone. `_MOUSE` is now a plain manual momentary layer (hold
  `LT(_MOUSE, BTN2)`), not motion-activated. Ball movement no longer changes layers.
- **Ambidextrous / left-hand mode** (`AMBI_TOG` keycode; `_BASE_L`/`_MOUSE_L`/
  `_SCRL_L`/`_FN_L` layers 4–7; `HAND_LAYER_COUNT`; `hand_is_left()`/`hand_offset()`/
  `apply_hand_mode()`): gone. Single-hand (right) keymap now. Layers 4–7 were
  then restored as *blank spares* (all `KC_TRNS`, `layer_count` 8) — placeholders
  for Vial GUI use, NOT the old mirrors. The freed `_FN` Top-Left-Left is `KC_NO`.

## Drag-scroll history — removed features (do not re-add without a plan)
Two drag-scroll control paths were built, hardware-tested as unreliable, and
**removed 2026-07-01** — don't reintroduce either without addressing why they
failed:
- **Wiggle-to-toggle** (`wiggle_ball.c/.h`, ported from drashna `wiggle_ball`):
  shake detection and the toggle itself worked (confirmed on hardware — a second
  shake correctly un-froze the cursor), but scroll engaged *by a wiggle*
  produced no wheel output, while the exact same `set_drag_scroll_scrolling()`
  call from the `DRG_TOG` keycode scrolled fine. Every pipeline stage was read
  and traced; both paths are provably identical from `set_drag_scroll_scrolling`
  through `drag_scroll_apply()`, so the divergence was never root-caused — no
  console output was obtainable to see live values (`DB_TOGG` + `qmk console`
  produced nothing, twice). Removed rather than debugged further per user
  request. If revisited: get console output working FIRST (that was the actual
  blocker), then re-port from `~/drashna-modules-reference/wiggle_ball/`.
- **Host lock-LED force-on** (`set_drag_scroll_force`/`get_drag_scroll_force` in
  `drag_scroll.c/.h`, `led_update_user` in `keymap.c`): forced drag scroll on
  while caps/num/scroll lock was active. Removed alongside wiggle at the same
  time, not because it was independently broken.
- **Known open quirk, intentionally NOT being chased further:** holding
  `DRG_TOG` down, or a fast double-tap of it, can also freeze the cursor
  without scrolling — same symptom family as the wiggle bug above. A single
  clean tap works correctly (hardware-confirmed). Root cause not found (same
  console-output blocker as above). User has asked to leave this alone; do not
  attempt a blind fix. If it comes up again, get `qmk console` output working
  first — every fix attempt this project made without live values was wrong.

## Live tuning via qmk console
`POINTING_DEVICE_DEBUG` (config.h) + `CONSOLE_ENABLE = yes` (rules.mk) turn on
QMK's `pd_dprintf()` call sites (`quantum/pointing_device_internal.h`), used by
the core sensor drivers to trace live sensor values. Nothing prints until QMK's
runtime `debug_enable` flag is also on — toggle it with `DB_TOGG` (Bottom Left on
`_FN`) so normal use isn't spammed with console traffic. Workflow: flash, run
`qmk console` in a terminal, press `DB_TOGG`, nudge the ball and watch the trace,
press `DB_TOGG` again when done. This is compile-time gated (`POINTING_DEVICE_DEBUG`)
rather than always-on so it can be stripped later with a one-line config.h change
if unwanted. (Historically this also fed `am_tuning.c`'s auto-mouse trace, removed
2026-07-01.)

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

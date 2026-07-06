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
- Vial dynamic entries are pinned in keymap config.h since 2026-07-03: **32
  tap dances, 32 combos, 32 key overrides** (`VIAL_*_ENTRIES`;
  `KEY_OVERRIDE_ENABLE`/`LEADER_ENABLE` in rules.mk). Emulated EEPROM doubled:
  `WEAR_LEVELING_BACKING_SIZE 16384` / `LOGICAL_SIZE 8192` — changing these
  reformats the EEPROM region (every persisted setting re-seeds on boot).
- Leader Key: the old "NOT supported by Vial" note meant the **Vial GUI**.
  QMK core `LEADER_ENABLE` is ON since 2026-07-03, with **dynamic sequences**
  (8 slots × up to 5 keys → 1 output keycode) stored in `mad_config`, edited
  over HID channel `0x19`, matched in `leader_end_user` (keymap.c). `QK_LEAD`
  = 0x7C58, placeable from Flask. Gotcha: `leader_sequence[5]` /
  `leader_sequence_size` are extern'd in keymap.c — quantum/leader.h only
  exposes fixed-arity prefix matchers, which can't length-check.

## File map — exact responsibility per file
| File | Responsibility |
|---|---|
| `keyboards/ploopyco/madromys/config.h` | Keyboard-level hardware wiring only (RP2040/PMW3360 SPI pins, UNUSABLE_PINS). Not for keymap tuning. |
| `keyboards/ploopyco/madromys/info.json` | USB VID/PID, physical layout (`LAYOUT()` shape), `dynamic_keymap.layer_count` (8 — 0-3 real, 4-7 spare blanks). |
| `keyboards/ploopyco/madromys/post_rules.mk` | `POINTING_DEVICE_DRIVER = pmw3360`. |
| `keyboards/ploopyco/madromys/rev1_001/keyboard.json` | Matrix pins, diode direction, ws2812/rgblight wiring for this PCB rev. |
| `keymaps/vial/config.h` | **Every tuning parameter and feature define lives here.** First file to check for "change a default" requests. Also carries `POINTING_DEVICE_DEBUG` (live-tuning console output — see below), `VIA_CUSTOM_LIGHTING_ENABLE` + `EECONFIG_USER_DATA_SIZE/VERSION` (companion-app raw HID protocol + EEPROM datablock — see that section), and the HID clamp-range mirrors. |
| `keymaps/vial/keymap.c` | Layers, the custom keycode enum, `process_record_user`, the `pointing_device_task_user` pipeline, status report, **plus**: the `mad_config_t` EEPROM datablock (every persisted tunable incl. DPI), the 8 dynamic gesture set tables (`gesture_sets`), and the companion-app raw HID handler (`raw_hid_receive_kb`). |
| `keymaps/vial/vial.json` | Vial GUI layout + `customKeycodes[]`. Must stay length-matched with the enum — see the rule below. |
| `keymaps/vial/rules.mk` | Feature enables (VIA/VIAL/COMBO/TAP_DANCE/KEY_OVERRIDE/LEADER/DEFERRED_EXEC) + `SRC +=` for every ported module `.c`. |
| `keymaps/vial/shared/pd_accel.c/.h` | Acceleration curve (ported from drashna `pointing_device_accel`). |
| `keymaps/vial/shared/pointing_device_smoothing.c/.h` | EMA smoothing (ported from drashna `pointing_device_smoothing`). |
| `keymaps/vial/pd_gestures.c/.h` | Directional flick gestures (ported from drashna `pointing_device_gestures`; renamed `pd_gestures_*` because QMK core already has a *different* built-in `pointing_device_gestures` — cursor glide — and the names would collide). **8 directions since 2026-07-02** (E SE S SW W NW N NE); empty diagonals fall back to the nearest cardinal. |
| `keymaps/vial/drag_scroll.c/.h` | Drag-to-scroll (ported from drashna `drag_scroll`). Controls: manual keycodes (`DRG_TOG`/`DRG_MO`/`DRG_INV`), the `DRAG_SCROLL_LAYER` binding, and (re-added 2026-07-02) wiggle_ball's shake-to-toggle. Host-lock-LED force stays removed. |
| `keymaps/vial/wiggle_ball.c/.h` | Shake-to-toggle drag scroll (drashna `wiggle_ball`; removed 2026-07-01, **re-added 2026-07-02** on fresh user ask for the companion-app project — old quirk stands, see Drag-scroll history). All three detection params are runtime + HID-tunable, plus an **enabled kill switch** (HID `0x12/0x04`, persisted) added after accidental shake-toggles froze the cursor during normal fast movement. |
| `keymaps/vial/shared/custom_shift_keys.c/.h` | Per-key Shift replacements — Shift+key types something else (ported 2026-07-03 from **getreuer**/qmk-modules, not drashna). Dynamic 16-slot RAM table + enable flag; HID channel `0x16`; persisted. `CSK_TOG` keycode. |
| `keymaps/vial/shared/select_word.c/.h` | Word/line selection (getreuer port). Keycodes `SELWORD`/`SELWDBK`/`SELLINE`/`SELLNUP`; Mac-vs-Win hotkey style is a runtime bool (HID `0x17/0x01`, persisted, default mac). Needs `select_word_on_record` + `select_word_task` called from keymap.c (they are). |
| `keymaps/vial/shared/sentence_case.c/.h` | Auto-capitalize after ". "/"! "/"? " (getreuer port). `SC_TOG` keycode; on/off at HID `0x18/0x01`, persisted. Requires one-shot keys — never define `NO_ACTION_ONESHOT`. |
| `keymaps/vial/shared/autoscroll.c/.h` | Hands-free continuous scroll (Ben White radiology AHK + Contour Shuttle jog model, added 2026-07-03). `ASC_JOG` = ball becomes jog wheel (deflection = speed, motion swallowed); `ASC_UP`/`ASC_DOWN` step ±9 speed levels through zero. Any other key press auto-exits. HID `0x1A`; 4 persisted tunables + live-state rescue value. **Gotcha: `AS_UP`/`AS_DOWN` collide with QMK core Auto Shift keycode aliases (keycodes.h:1526) — hence the `ASC_` prefix.** |
| `keymaps/vial/shared/os_shortcuts.c/.h` | OS-aware Cut/Copy/Paste/Undo/Redo (2026-07-03): mac mode = ⌘ hotkeys, pc = ^; follows QMK OS detection (`OS_DETECTION_ENABLE`, a generic feature — plain enable works) unless pinned. HID `0x1D`. `process_detected_host_os_kb` override in keymap.c also mirrors detection into select word. |
| `keymaps/vial/shared/pipeline_diag.c/.h` | Freeze diagnostic (2026-07-03): watermark of the largest gap between pointing-task passes, HID `0x1F` (GET reads, SET resets; uptime at `0x02`). Built to localize the reported random 1-2 s cursor freezes (firmware loop vs sensor/host). Flask shows it as the Mouse tab's "Health" module. |
| `keymaps/vial/shared/wheel_chords.c/.h` | Button-held ball gestures (2026-07-03): hold BTN1..8 + roll → 8-direction keycodes, pd_gestures ratchet feel; click never suppressed, motion swallowed only while the held button has ≥1 slot. HID `0x1C`. Physical BTN state tracked in `process_record_user` incl. LT/MT tap halves. |
| `keymaps/vial/shared/` | **Git submodule** (added 2026-07-04), not a plain subdir — see "Shared module submodule" section below. All 9 rows above live here now; edit them in place (the submodule checkout) or in the standalone `~/qmk-flask-modules` repo, never re-create a local copy directly in this keymap dir. |
| `keymaps/vial/check.sh` | Validator. Run after every edit. |
| `~/AdeptCompanion/` (outside this repo) | **"Flask"** — the macOS SwiftUI companion app (SPM: `AdeptCore` lib + `Flask` executable target; dir name kept). Speaks the raw HID protocol below **and** (since 2026-07-02) the stock VIA+Vial protocol — it is a full Vial editor (keymap/macros/tap dance/combos/gestures/mouse chords/QMK settings/matrix tester/unlock), replacing the Vial GUI for this device. UI is a Swift port of Pipette's design (darakuneko/pipette-desktop). `AdeptCore/KeycodeDB.swift`'s `customKeys` mirrors the custom-keycode enum — THE keycode rule applies to it too. Gotcha: Vial dynamic-entry SET frames are `[0xFE,0x0D,op,idx,entry…]` — entry at byte 4, NO pad byte (a pad byte shipped garbage tap dances once). `swift run` to launch from source, `./make-app.sh` to build `Flask.app`. |

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
4. Ceiling: 64 slots (`QK_KB_0..QK_KB_63`, 0x7E00-0x7E3F — corrected from 32
   on 2026-07-04; the Svalboard had shipped past 32 since its v7). `check.sh`
   reports current usage (34 in use as of v1.1.0).

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
   symbol = link error) or assume no override exists. (Current overrides:
   `raw_hid_receive_kb` in `keymap.c`, weak default at `quantum/via.c:188`.)
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
- `_FN` (3): DPI cycle, a Vial macro slot (`MC_0`, authored in the Vial GUI),
  debug-console toggle (`DB_TOGG`), bootloader. Top-Left-Left and Top-Left are
  both free (`KC_NO`) — Top-Left-Left was `AMBI_TOG` before the ambidextrous
  removal, Top-Left was gesture-set-D ("PACS nav") latch (`GRD_TOG`) before
  that set's removal (2026-07-01); bind anything via Vial.

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
- **Wiggle-to-toggle** (`wiggle_ball.c/.h`, ported from drashna `wiggle_ball`)
  — **RE-ADDED 2026-07-02** on an explicit fresh user ask (the condition below
  was informed consent, not a fix): restored verbatim from git `d6838c042e`,
  with all three detection parameters made runtime + HID-tunable for the
  companion app. The original bug report stands unresolved: shake detection
  and the toggle itself worked (confirmed on hardware — a second shake
  correctly un-froze the cursor), but scroll engaged *by a wiggle* sometimes
  produced no wheel output, while the exact same `set_drag_scroll_scrolling()`
  call from the `DRG_TOG` keycode scrolled fine. Every pipeline stage was read
  and traced; both paths are provably identical from `set_drag_scroll_scrolling`
  through `drag_scroll_apply()`, so the divergence was never root-caused — no
  console output was obtainable to see live values (`DB_TOGG` + `qmk console`
  produced nothing, twice). If it resurfaces, the raw HID channel is now the
  live-values path the qmk console never provided.
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

## Companion-app raw HID tuning protocol (added Phases 0-3, 2026-07-02)
A native macOS app (`~/AdeptCompanion/`, SwiftUI + IOKit) live-tunes every
module parameter over QMK raw HID — the thing Vial's GUI can't do. Firmware
side lives entirely in `keymap.c` (`raw_hid_receive_kb` + `mad_config_t`).

**Transport:** `VIA_CUSTOM_LIGHTING_ENABLE` (keymap config.h) makes
`quantum/via.c` route command IDs `0x07`/`0x08`/`0x09` (VIA v3-style
custom_set/get/save) to `raw_hid_receive_kb` instead of dead-ending them.
Verified safe: no lighting feature compiled, vial.json `"lighting": "none"`,
all of Vial's own traffic is behind the `0xFE` prefix. Frame:
`[cmd, channel, value_id, payload...]`, u16 big-endian payloads (×100 for
float params; accel offset is the one signed field; bools 0/1). Unknown
anything → firmware sets `data[0] = 0xFF` (id_unhandled). The handler must
NOT call `raw_hid_send()` — via.c echoes the buffer itself.

**Channels** (0x10+ dodges VIA's reserved 0–5; full value table lives in
`keymap.c`'s `mad_hid_*` enums and mirrors `Sources/AdeptCore/AdeptProtocol.swift`):
`0x00` meta (protocol version, currently **10**; `0x02` active layer RO,
v10) · `0x10` accel · `0x11`
gestures (`0x01` ratchet step; `0x02` active set — GET index/0xFF, SET 0xFF
cancels or index toggles through the GR#_TOG guard path; **slots**: cardinals
`0x10 + set*4 + c` (c 0=E 1=S 2=W 3=N → internal dir c*2), diagonals
`0x30 + set*4 + d` (d 0=SE 1=SW 2=NW 3=NE → internal dir d*2+1); raw QMK
keycode payload, unclamped) · `0x12` wiggle (+`0x04` enabled kill switch,
v3) · `0x13` smoothing · `0x14` dpi (table index; persists immediately,
save is a no-op) · `0x15` dragscroll (+`0x04` live scrolling state, v3 —
GET = freeze diagnostic, SET = force on/off, never persisted) · **v4
channels (2026-07-03):** `0x16` custom shift keys (`0x01` enabled, `0x02`
slot count RO, keycode `0x10+slot`, shifted `0x30+slot`, 16 slots) · `0x17`
select word (`0x01` mac-hotkeys bool) · `0x18` sentence case (`0x01` enabled)
· `0x19` leader (slots `0x10 + seq*8 + pos`; pos 0-4 = keys, 5 = output;
8 sequences) · **v5:** `0x1A` autoscroll (`0x01` inverted, `0x02` speed scale
x100 [25,400], `0x03` jog deadzone [0,200], `0x04` jog range [50,2000],
`0x05` live state — GET signed level/±100 jogging, SET force-stops, never
persisted) · **v6 (2026-07-03):** `0x1B` auto-mouse (`0x01` enabled, `0x02`
timeout ms [100,5000], `0x03` threshold counts [0,60]; core feature hand-wired
in rules.mk — plain `POINTING_DEVICE_AUTO_MOUSE_ENABLE = yes` is inert in this
fork) + `0x1C` wheel chords (`0x01` enabled, `0x02` step [50,2000], slots
`0x10 + button*8 + dir`, 8 buttons × 8 dirs, raw keycodes, fires via
tap_code16) · **v7 (2026-07-03):** `0x1D` OS-aware shortcuts (`0x01` follow
detection, `0x02` mac/pc mode — SET also mirrors into select word, `0x03`
detected os_variant_t RO; keycodes OS_CUT/OS_COPY/OS_PSTE/OS_UNDO/OS_REDO,
os_shortcuts.c + OS_DETECTION_ENABLE) + `0x1F` freeze diagnostic (`0x01`
pointing-gap watermark ms — GET reads, SET resets; `0x02` uptime seconds RO;
pipeline_diag.c, nothing persists) · **v8-v10 (2026-07-04, v1.1.0):** `0x1E`
num word (now on the Adept too — default target layer 4), raw DPI 200-4000
step 50 on `0x14` (CPI value id; 0 = legacy table/index mode, writing an
index re-arms it), auto-mouse target layer on `0x1B`, wiggle
action/target/source knobs on `0x12`, 4 more OS shortcuts
(OS_APPSW/OS_SELALL/OS_NEWTAB/OS_CLOSE) + NUMWORD = keycodes 29-33.
Protocol currently **10**; EEPROM datablock VERSION **11**.
Gestures are **8-direction** since v3 (`PD_GESTURES_NUM_DIRECTIONS 8`,
internal order E SE S SW W NW N NE); an empty diagonal slot falls back to
the nearest cardinal by dominant axis, so 4-way sets keep their old feel.

**Semantics:** GET/SET act on live module runtime state (setters clamp);
SAVE snapshots live state into `mad_config` per-channel and writes the whole
datablock. Persistence = `EECONFIG_USER_DATA_SIZE`/`VERSION` datablock
(config.h) — **bump VERSION on any `mad_config_t` layout change** (append-only
growth otherwise), which re-seeds all tunables from config.h defaults on next
boot. Value IDs are append-only per channel, same spirit as THE keycode rule.

**Hard-won rules (hardware-verified 2026-07-02):**
- Clamp in wire-width (u16) space BEFORE any narrowing cast — a bare
  `(int8_t)` cast wrapped 200 → −56 → clamped to MIN instead of MAX on real
  hardware. Same class of bug existed for u8 casts.
- Module setters CLAMP, never reject — smoothing's inherited
  reject-if-out-of-range guard silently ignored writes until fixed.
- Verify every new value on hardware with the throwaway script pattern
  (get → set in-range → set out-of-range expecting clamp → save → power-cycle
  → re-get); the two bugs above were invisible to a clean `-Werror` build.

**Dynamic gesture sets (2026-07-02):** 8 sets × 4 directions (E/S/W/N), RAM
tables in `keymap.c` (`gesture_sets`), seeded 1=arrows 2=editing 3=media
4=app/tab-nav, 5–8 empty. Toggle keycodes `GR1_TOG..GR8_TOG` (replaced
GRA/GRB/GRC/GRN in-place at indices 7–10 + appended 11–14). Guard: an
all-KC_NO set can't be toggled ON (would freeze the cursor), but a latched
set can always toggle OFF. Slots fire via `tap_code16` → basic keycodes +
C()/S()/A()/G() combos only; NO Vial macros / layer keys / QK_KB_* customs in
slots (rerouting through `vial_keycode_tap` was declined 2026-07-02 to avoid
touching a proven pipeline — revisit only on user ask).

**App as Vial editor (2026-07-02):** the companion app now ALSO speaks the
stock VIA/Vial protocol (keymap, macros, tap dance, combos, QMK settings,
matrix tester, unlock) — no firmware surface was added; it just talks to
`quantum/via.c`/`vial.c` like the Vial GUI does. Facts that matter firmware-side:
unlock-gated = macro writes (silently ignored while locked), matrix-state read,
bootloader jump; keymap and dynamic-entry writes need NO unlock; **once
`vial_unlock_start` fires, the device only answers unlock commands until the
combo completes — there is no abort** (replug recovers). RGB via VIA lighting
IDs is permanently off the table on this firmware: `VIA_CUSTOM_LIGHTING_ENABLE`
routes 0x07–0x09 to the tuning handler. Don't run Vial GUI and the app together.

## Shared module submodule (added 2026-07-04)

`keymaps/vial/shared/` is a **git submodule**, not a plain subdirectory. It
points at the standalone repo `~/qmk-flask-modules`, which holds the 12 ported
module `.c`/`.h` pairs that are byte-identical across every Flask-speaking
firmware — currently this repo (Adept) and `~/svalboard-vial-qmk`
(Svalboard, `keyboards/svalboard/keymaps/flask/shared/` — same submodule,
same commit, added the same way). Goal: any future QMK-Vial device that wants
the Flask feature set clones the same submodule instead of re-porting or
copy-pasting.

**Current shared set (12 pairs, 24 files — v1.1.0, 2026-07-04):**
`os_shortcuts`, `pipeline_diag`, `pd_accel`, `pointing_device_smoothing`,
`select_word`, `sentence_case`, `custom_shift_keys`, `wheel_chords`,
`autoscroll`, **plus (moved in for Sval parity)** `pd_gestures`,
`wiggle_ball` (rewritten as a pure observer: `wiggle_ball_observe()` +
`wiggle_ball_triggered()`, fed raw deltas FIRST in the pipeline), `num_word`.
**Not shared** (Adept-only: `drag_scroll.c/.h`; Svalboard-only:
`support_flask.c`, `mad_hid.c`, `flask_tuning.h`) — these stay
local to each keymap, either because only one device uses them or because
they've diverged (drag_scroll has Adept-specific history, see
"Drag-scroll history" below).

**Mechanics, current state:** the submodule's `url` in `.gitmodules` is a bare
local path (`/Users/aj/qmk-flask-modules`) — works on this machine only, no
remote pushed yet. `git submodule add` itself is blocked by this environment's
sandboxing for `file://` transports; the submodule was wired by hand (plain
`git clone` of the local repo into the target path + hand-written
`.gitmodules` entry + `git add` of the resulting embedded repo, which git
auto-detects as a gitlink). If `~/qmk-flask-modules` ever needs to be cloned
on another machine, or CI needs it, push it to a real remote first and update
both `.gitmodules` files' `url` to that remote.

**Editing a shared module:** edit the files under either firmware's
`keymaps/*/shared/` (they're the same submodule checkout in spirit — commit
inside `~/qmk-flask-modules` directly, or `cd` into either repo's `shared/`
dir, since both currently point at the same local clone target). Bump the
submodule's own commit and update both firmware repos' recorded submodule SHA
so they don't drift. Never fork the content — that reintroduces the exact
duplication this submodule exists to remove. `SRC += shared/<name>.c` in
`rules.mk` and `#include "shared/<name>.h"` in the consuming `.c` files are
the only firmware-repo-side changes a shared-module edit needs.

## Porting strategy (for any future drashna module)
0. If the module will ship on more than one Flask-speaking device (currently
   Adept + Svalboard), build it directly in `~/qmk-flask-modules` and consume
   it via the `shared/` submodule (see above) instead of this keymap dir —
   don't port it locally first and move it later.
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

## NLOFIN NLKB16-02 — second Flask device in this repo (added 2026-07-06)

`keyboards/nlofin/nlkb16_02/` — a rebadged DOIO KB16 rev2 macro pad (16 keys,
3 encoders, 23-LED RGB, tiny OLED; STM32F103, Maple/stm32duino bootloader,
app at 0x8002000). Board files reconstructed from mainline `doio/kb16/rev2` +
values read off the live device; stock firmware backed up at
`~/nlkb16-02-stock-firmware-2026-07-06.bin` (restore:
`dfu-util -d 1eaf:0003 -a 2 -D <bin>`). Everything hardware-verified
2026-07-06. Its own validator: `keyboards/nlofin/nlkb16_02/keymaps/flask/check.sh`
— same contract as the Adept's, run it after every edit.

**Keymap** (`keymaps/flask/`): Vial (8 layers, 32 tap dances/combos/key
overrides) + VialRGB (stock protocol untouched) + Flask raw HID protocol
(**v2**; EEPROM datablock v1) with channels: `0x00` meta · `0x16` custom shift
keys · `0x17` select word · `0x18` sentence case · `0x19` leader · `0x1D` OS
shortcuts · `0x1E` num word · `0x20` per-combo layer masks · `0x21` per-layer
per-key RGB map (8 layers × 23 LEDs × HSV, `RGBMAP_TOG`) · `0x22` OLED
(push lines `0x10`, release `0x11`, hold `0x01`, diagnostics `0x02-0x06`,
**raw panel cmd inject `0x07` + full re-init `0x08`** — the display-debug
probes, v2). Shares the `qmk-flask-modules` submodule (getreuer set only —
no pointing device). 17 custom keycodes; THE keycode rule applies
(enum ↔ `vial.json`).

**Hard-won hardware facts — read before touching the display or RGB:**
- **The glass is a 64×32 window into the SSD1306's 128×64 RAM** (SEG columns
  32-95 × COM rows 0-31). Panel is a real SSD1306 128×64 electrically (init
  table decoded from the stock dump at flash offset 0xD4EF), mounted portrait
  → `OLED_ROTATION_90`, and of the 16×10 logical canvas only **lines 4-11 ×
  columns 0-4** are visible (`NLK_DISPLAY_VISIBLE_*` in keymap config.h). A
  whole session was lost to "frozen display" symptoms that were really
  dynamic content rendering off-glass. Boundary-probe pushes over HID are
  how to re-map this if the glass is ever questioned again.
- **RGB boot holdoff is load-bearing** (`NLK_RGB_HOLDOFF_MS`, keymap.c): the
  panel wedges (ACKs every I2C transfer, applies none — fail counters stay 0)
  if the LED strip lights at full brightness during the USB power-up window.
  Same brightness/animation is harmless seconds later (hardware-bisected:
  boot-bright froze; off/dim/bright at steady-state all clean). LEDs are held
  dark 3 s after boot and after USB wake, then restored from EEPROM with a
  defensive `oled_display_request_reinit()`. Recovery from a wedge = HID
  `0x22/0x08` re-init; it survives without a replug.
- **ws2812 PWM backend is impossible on this board** — PA10 = TIM1_CH3, the
  driver DMAs off TIM1_UP, and on the F103's fixed DMA map TIM1_UP shares
  DMA1 ch5 with I2C2_RX, which the OLED holds from `i2cStart` on. The build
  boots to a ChibiOS halt before USB. Bitbang stays (comment in board
  config.h). Don't retry.
- **The Maple bootloader mass-erases the EEPROM region on every flash** —
  persisted settings reverting to defaults after a reflash is the bootloader,
  not a firmware regression (same-session power cycles DO persist).
- **Big knob (matrix 2,4) is not pushable** — vendor never wired the switch;
  don't bind anything there.
- Full-width (10-char) pushed lines must NOT be followed by
  `oled_advance_page()` — the cursor has already wrapped and the call blanks
  the next line, ping-ponging every block dirty (218 tx/s measured vs 2 idle).
  `oled_display_push_line` stores lines space-padded; `render_pushed` only
  advances on short lines. Don't "simplify" that back.
- Host-side: python-hidapi goes stale across replug/reflash cycles — probe
  with the `hidcli` Swift one-shot (scratchpad; rebuild:
  `swiftc -O hidcli.swift -o hidcli`) or vial.rocks before believing the
  firmware is dead.
- EEPROM sizing is RAM-constrained (F103, 20 KB): 8 K backing / 4 K logical.
  8 K logical left 560 bytes of heap; don't grow it back.

**Still pending** (deferred, not forgotten): Flask macOS app support for this
device (encoder editor, RGB painter, display push panel — mirror the Adept UI
patterns) and exercising `.vil` save/load as the bootloader-erase mitigation.

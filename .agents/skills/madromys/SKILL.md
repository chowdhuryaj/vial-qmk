---
name: madromys
description: RETIRED 2026-08-14 — reference only. Was the workflow for editing the Ploopy Adept trackball's Vial-QMK keymap (keyboards/ploopyco/madromys/keymaps/vial/). The Adept is out of use, ~/vial-qmk is a retired tree, and Flask app support for the device was removed. Use this ONLY when explicitly digging into Adept history or recovering the hand-reconstructed headless-ploopyco board files from git. For any live keyboard-firmware work use the flask-ecosystem skill and ~/svalboard-vial-qmk.
---

> ## ⛔ RETIRED 2026-08-14 — reference only
>
> The Ploopy Adept is out of use and this tree (`~/vial-qmk`) is retired. The
> keymap directories remain on disk only so the git history keeps the
> hand-reconstructed board files for the headless ploopyco family.
>
> **The live QMK firmware is `~/svalboard-vial-qmk` (branch `flask-port`)** —
> protocol 23, EEPROM v18, flashed and working 2026-08-18. For anything live,
> read `.Codex/skills/flask-ecosystem/SKILL.md` instead of this file.
>
> Everything below is accurate as of 2026-08-09 and describes a device that is
> no longer connected.

# Madromys Vial keymap workflow

## Why this exists

Vial addresses custom keycodes positionally: `vial.json`'s `customKeycodes[i]`
always maps to firmware keycode `QK_KB_0 + i`. A length or order mismatch
between `vial.json` and `keymap.c`'s `enum madromys_keycodes` **compiles
cleanly** but silently misassigns every keycode from the divergence point on.
The same is true for a couple of other mistakes in this keymap — forgetting to
wire a new module's `.c` file into `rules.mk`'s `SRC +=`, or adding an enum
entry with no matching `process_record_user` case. None of these produce a
compiler error, so a clean `make` is not proof the change is correct. That's
why this keymap carries its own validator instead of relying on the build alone.

## Before editing

Read `/AGENTS.md` (repo root) — specifically the **File map**, **THE keycode
rule**, and **Conventions for adding a new tunable** sections. It documents
exactly which file owns which kind of change (`config.h` for tuning values,
`keymap.c` for layers/keycodes/the pointing pipeline, `rules.mk` for wiring
new source files, `vial.json` for the Vial GUI surface) and the established
patterns already in use — e.g. the Shift-inverts/Ctrl-x10 step pattern every
live-tunable parameter follows, where step-size defines live, and a
float-formatting gotcha with `snprintf` on this RP2040 build. Follow the
existing conventions rather than inventing new ones; consistency is what
keeps a keymap this dense maintainable by hand.

## Scope, and what is *not* local to this keymap

This workflow applies to anything under `keyboards/ploopyco/madromys/`. Two
important exceptions live inside that tree but are **not** Adept-only:

- **`keymaps/vial/shared/` is a git submodule** (`qmk-flask-modules`), shared
  byte-for-byte with the Svalboard firmware. Editing a file there changes the
  Svalboard too. Never edit a shared module as if it were local — see the
  flask-ecosystem skill for the commit/pull/re-`git add` dance.
- Only `drag_scroll.c/.h` is genuinely Adept-local among the pointing modules.

## THE keycode rule spans three files, but check.sh only validates two

`keymap.c`'s enum and `vial.json`'s `customKeycodes[]` must stay identical in
length and order — `check.sh` enforces that. The **third** mirror is
app-side: `KeycodeDB.customKeys` in `~/AdeptCompanion/Sources/AdeptCore/
KeycodeDB.swift`, indexed `kbBase + i` against the same enum. Nothing
validates that one automatically. If you add or remove a keycode, update it by
hand or the Flask app renders the new keycode as raw hex and pastes the wrong
thing. Ceiling is 64 slots (`QK_KB_0..QK_KB_63`); `check.sh` prints current
usage (37 as of 2026-07-19).

When adding a keycode, append to the **end** of all three lists — never insert
in the middle, since every keycode after an insertion point silently shifts.

## Version bumps that a keymap edit can trigger

- Changing `mad_config_t`'s layout → bump `EECONFIG_USER_DATA_VERSION`
  (config.h, currently 12). This reseeds every persisted tunable from config.h
  defaults on the next boot, so warn the user their settings will reset and
  suggest a `.vil` export first.
- Adding or changing a raw-HID value id → bump `MAD_HID_PROTOCOL_VERSION`
  (keymap.c, currently 11) **and** `expectedProtocolVersion` in the app's
  `AdeptProtocol.swift`, or the app shows a version-mismatch warning on
  connect. Value ids are append-only per channel, same spirit as the keycode rule.

## After editing — always run the validator

```
keyboards/ploopyco/madromys/keymaps/vial/check.sh
```

This checks the `vial.json`/`keymap.c` keycode-list sync, confirms every
custom keycode has a handler, confirms every ported module `.c` file is wired
into `rules.mk`, and then runs the actual firmware build — failing on any
compiler warning, not just errors.

**Only report the task as done if `check.sh` exits 0.** If it fails, the
output tells you exactly what's wrong (a printed keycode table with
mismatches flagged, a missing `SRC +=` line, or the build output) — fix that
and re-run rather than declaring success.

While iterating, `check.sh --skip-build` runs just the fast structural checks
(skips the slow full firmware build) — useful for quick feedback, but always
run the full version (no flag) before considering the change finished.

`check.sh` cannot see runtime behavior, the app-side keycode mirror, or
anything on real hardware. A PASS means "safe to flash", not "verified".

## Standing user instructions for this keymap

- **Do not blind-fix the `DRG_TOG` freeze quirk.** Holding or fast
  double-tapping it can freeze the cursor without scrolling. Root cause was
  never found and every fix attempted without live values was wrong. The user
  has asked to leave it alone; only revisit if they raise it *and* live
  console/HID values are obtainable first.
- Auto-mouse and the ambidextrous/left-hand layer set were both removed at
  user request, then auto-mouse was re-added hand-wired in `rules.mk` (a plain
  `POINTING_DEVICE_AUTO_MOUSE_ENABLE = yes` is inert in this fork). Don't
  re-add the ambidextrous set without a fresh ask.
- Macro *content* (the `MC_0` slot) is authored in the GUI, not firmware. The
  user deferred this — don't invent macro content unprompted.

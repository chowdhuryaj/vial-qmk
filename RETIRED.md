# This repo is retired (2026-08-14)

It held exactly two devices, and neither is in use any more:

- `keyboards/ploopyco/madromys/` — Ploopy Adept Trackball
- `keyboards/nlofin/nlkb16_02/` — NLOFIN NLKB16-02 macro pad

Both were also removed from the Flask companion app on the same date.
Nothing here is being developed further.

## Where the work moved

| What | Where |
|---|---|
| Firmware (the only live board) | `~/svalboard-vial-qmk` — Svalboard, branch `flask-port` |
| Shared ported modules | `~/qmk-flask-modules` (git submodule of the above) |
| Companion app | `~/AdeptCompanion` — "Flask", macOS SwiftUI |
| Web configurator | `~/flask-web` |
| Active plan / spec | `~/svalboard-vial-qmk/keyboards/svalboard/keymaps/flask/FLASK-PLAN.md` |

Launch new sessions from `~/svalboard-vial-qmk`, not here. The `vialqmk`
shell alias points at this directory and should be repointed or dropped.

## Why the directory still exists

Deleting it was not necessary to retire it, and the git history is the only
copy of some of this work — notably the reconstructed keyboard-level board
files for the headless `ploopyco` family and the NLKB16-02, both of which
were rebuilt by hand from mainline QMK plus values read off live hardware.

There are unpushed commits on the `flask` branch. If you want this
preserved off-machine before deleting anything:

```bash
cd ~/vial-qmk && git push origin flask
```

To delete it once you're satisfied it's pushed:

```bash
rm -rf ~/vial-qmk
```

## Things worth remembering from here

These outlived the devices and are documented in the Svalboard repo's
CLAUDE.md, but the originals are in this history:

- Payload-addressed HID frames (`[layer, led, H, S, V]`, not u16 frames) —
  invented for the NLKB16 RGB map; now the shape any bulk channel uses.
- `.vil` extension-key round-trip (`flask_rgbmap`) — the pattern for
  persisting anything the bootloader erase would otherwise destroy.
- Clamp in wire width **before** narrowing casts; setters clamp, never
  reject; verify every new value on hardware (get → set in-range → set
  out-of-range expecting clamp → save → power-cycle → re-get).
- The NLKB16 display saga: a 64×32 glass on a 128×64 SSD1306 controller,
  the RGB-boot-brightness I2C wedge, and the full-width-line ping-pong.
  See git history for `keyboards/nlofin/nlkb16_02/keymaps/flask/`.

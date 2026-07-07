# vial-qmk (Flask Edition)

Fork of [vial-kb/vial-qmk](https://github.com/vial-kb/vial-qmk) carrying the firmware side of the **Flask** ecosystem — a macOS companion app (AdeptCompanion/Flask) that acts as a full Vial editor plus a live-tuning surface over QMK raw HID.

## Devices in this repo

| Board | Path | Build target |
|-------|------|--------------|
| Ploopy Adept Trackball (madromys) | `keyboards/ploopyco/madromys/` | `make ploopyco/madromys/rev1_001:vial` |
| NLOFIN NLKB16-02 macro pad (DOIO KB16 rev2 clone) | `keyboards/nlofin/nlkb16_02/` | `make nlofin/nlkb16_02:flask` |

A third Flask device, the Svalboard, lives in its own vial-qmk fork.

Each keymap has its own validator script (`check.sh`) — run it after any edit; it catches keycode-list desync, unwired source files, and unhandled keycodes before doing the real build.

## Shared modules

The ported feature modules (acceleration, smoothing, gestures, drag scroll, autoscroll, select word, sentence case, custom shift keys, wheel chords, OS-aware shortcuts, num word, pipeline diagnostics) live in the [qmk-flask-modules](https://github.com/chowdhuryaj/qmk-flask-modules) repo, consumed here as the `shared/` git submodule inside each keymap.

## Documentation

`CLAUDE.md` at the repo root is the living editing contract: file responsibilities, the custom-keycode rule, the raw HID tuning protocol (channels, value IDs, persistence semantics), and the hardware-verified gotchas for each board.

## Credits

- [vial-kb/vial-qmk](https://github.com/vial-kb/vial-qmk) — Vial QMK fork this is based on
- [QMK Firmware](https://github.com/qmk/qmk_firmware) — The QMK project
- drashna's and getreuer's QMK community modules — origin of several ported features

## License

This project is licensed under the [GNU General Public License v2.0](LICENSE).

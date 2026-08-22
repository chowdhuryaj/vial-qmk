---
name: hid-remapper-vial
description: Continue work on the HID Remapper Vial-style GUI configurator — a no-RPN, QMK/Vial-like front-end for the Elecom Huge Plus trackball on an Adafruit Feather RP2040 USB Host. Use this when the user asks to resume, extend, or debug the config tool at ~/hid-remapper/config-tool-vial/.
---

# HID Remapper — Vial-style GUI: resume workflow

## What this project is

A custom configuration front-end (`~/hid-remapper/config-tool-vial/`) for the
[HID Remapper](https://github.com/jfedor2/hid-remapper) firmware. It gives the
device (Adafruit Feather RP2040 USB Host driving an Elecom Huge Plus trackball,
VID 0x056E / PID 0x01AB) a QMK/Vial-style UI — layer tabs, searchable keycode
picker, per-key tap/hold/sticky flags, and no-RPN behavior builders. Talks to
**stock, unmodified HID Remapper firmware** over the same WebHID wire protocol
as the official tool; no reflash needed.

**Why not vial-qmk:** this fork's RP2040 path is ChibiOS with no PIO USB host
stack, so a port was rejected as too costly. HID Remapper already solves the
hard part; the work here is entirely a GUI layer.

**The user explicitly does NOT want Chrome or Edge** — the desktop app
(`config-tool-vial/desktop/app.py`) uses pywebview (native WKWebView on macOS)
+ hidapi.

## File map

All files live at `~/hid-remapper/config-tool-vial/` unless noted.

| File | Responsibility |
|---|---|
| `crc.js` | CRC-32 (verbatim from stock tool). Required for 32-byte config packet integrity. |
| `protocol.js` | Wire protocol constants, `sendFeatureCommand`, `readConfigFeature`, all command/usage-page constants. |
| `expr.js` | RPN opcode table (55 ops), `exprToElems`, `elemToToken`. |
| `model.js` | `defaultConfig()` (v18), `migrateConfig()`, `newMapping()`, `usageToHex()`. |
| `device.js` | `RemapperDevice` — `requestAndOpen()` (WebHID) / `openNative()` (Python bridge). Transport stored as `this.io`; `PyHidDevice` inner class wraps the bridge. |
| `profiles.js` | `ELECOM_HUGE_PLUS` profile (8 buttons + 3 axis groups). Matched by VID/PID. |
| `keycodes.js` | `targetCategories()` → Mouse/Keyboard/Media/Layers/Macros/Special. `sourceCategories()` → Buttons/Pointer/Keyboard/Detected. Name resolvers. |
| `usages.js` | Vendored copy of stock tool's usage tables (sibling `../config-tool-web/usages.js` is unreachable from this server root). |
| `keymap.js` | Multi-action per (source, layer): `getActions`, `addAction`, `removeAction`, `clearActions`, `explodeLayers`. |
| `behaviors.js` | Behavior compiler — `compile(baseConfig, behaviors)` → device config. Five compilers: `compileDpiShift`, `compileCursorKeys`, `compileChordSet`, `compileScrollText`, `compileTapDance`. |
| `project.js` | `defaultProject()`, `compileProject()`, `projectFromJson()`, `newBehaviorId()`. Source of truth: `{format:1, profile, base, behaviors:[]}`. |
| `index.html` | Full markup — tabs: Keymap / Behaviors / Macros / Settings. |
| `vial.js` | Main controller. State: `project`, `currentLayer`, `selected`, `focusedAction`, `pickerTarget`. Key functions: `renderKeyOptions`, `sourceButton`, `layerField`, `openFieldPicker` (picker works in source *or* target mode), `textToMacroSteps`. |
| `desktop/app.py` | `HidBridge` (pywebview API): `open()`, `send_feature()`, `get_feature()`. Finds device by `usage_page==0xFF00 and usage==0x0020`. Spawns localhost HTTP server, opens pywebview at `index.html?native=1`. |
| `desktop/requirements.txt` | `pywebview>=4.4`, `hid>=1.0.5` |

## Current status (as of last session, 2026-06-29)

**Browser-verified, not yet hardware-tested.** All UI is complete:
- Keymap tab: list of inputs (B1–B8, cursor X/Y, scroll wheel, tilt), 8 layer tabs, per-key action panel with Sticky/Tap/Hold checkboxes, full searchable keycode picker.
- Behaviors tab: 5 builders (DPI shift, Cursor→keys, Chording, Scroll-text, Tap-dance), each with "Active on layers" L0–L7 row, trigger field using the source picker (any input, not just 8 buttons).
- Macros tab: inline text field + "Add as keystrokes" button (no `window.prompt`), `textToMacroSteps()` converts ASCII→HID with shift handling.
- Settings tab: tap-hold threshold, other global HID Remapper settings.
- Desktop app: code-complete, not yet run against real hardware.

**Still needs (pending hardware test results):**
- On-device tuning of tap-dance window timing, cursor→keys sensitivity, chord release feel — need user feedback from actual hardware.
- Desktop app runtime test: `pip install -r requirements.txt && brew install hidapi && python3 desktop/app.py`.

## Key design decisions

- **Keycode rule:** `customKeycodes[i]` ↔ `QK_KB_0 + i`. This GUI doesn't use QMK custom keycodes — it uses HID Remapper's native mapping system where any usage (including `0xFFF5000N` registers) can be the source of a mapping.
- **Behavior model:** a "behavior" is a high-level struct that compiles to ordinary mappings + RPN expressions. The device only stores the compiled result (like Vial's `.vil` → compiled flash). `project.js` is the source of truth.
- **Expression channels:** 8 total (`0xFFF30001`–`0xFFF30008`). Each complex behavior consumes 1–5 channels. Channels are allocated in order by `makeAllocator`.
- **No `%f` in snprintf:** RP2040 build's libc doesn't support it. Use integer math. (This constraint is firmware-side; the GUI has no such limit.)
- **`../config-tool-web/usages.js` is unreachable:** the preview server roots at `config-tool-vial/`. `usages.js` was vendored into this folder to fix 404s.
- **Module cache gotcha:** if testing in a long-lived browser tab, add `?cb=Date.now()` to imports when debugging — the ES module cache can hide edits.

## Wire protocol summary

- Feature report ID 100, 32 bytes, CRC-32 in last 4 bytes.
- `GET_CONFIG = 3`, `SET_CONFIG = 2`, `ADD_MAPPING = 5`.
- Fixed-point x1000: a value of 1.0 is stored as 1000. Scaling of 1× = 1000.
- Usage pages: `0xFFF10000` = layers, `0xFFF20000` = macros, `0xFFF30000` = expressions, `0xFFF50000` = registers.
- `NLAYERS = 8`. `NMACROS_ASSIGNABLE = 32`.

## How to run the preview (no device needed)

```
# preview server is configured in ~/vial-qmk/.Codex/launch.json → port 8731
cd ~/hid-remapper/config-tool-vial
python3 -m http.server 8731
# open http://localhost:8731/ in a browser (WebHID won't connect without a device, but UI renders fully)
```

## How to run the desktop app (requires device)

```
cd ~/hid-remapper/config-tool-vial/desktop
pip install -r requirements.txt   # plus: brew install hidapi (macOS)
python3 app.py
# opens a native window at index.html?native=1 — no browser needed
```

## Adding a new behavior type

1. Add a compiler function `compileMyBehavior(b, config, alloc)` to `behaviors.js`, following the existing pattern:
   - Use `alloc.layer()`, `alloc.channel()`, `alloc.reg(n)` to claim resources.
   - Write expressions as plain strings; separate lines with ` eol `.
   - Use `layersOf(b)` for all mapping layer arguments.
   - Surface outputs via `registerUsage(r)` → real keycodes via mapping.
2. Add the `case 'my_behavior':` to `compile()`'s switch in `behaviors.js`.
3. Add a `[data-add="my_behavior"]` button to the Behaviors tab in `index.html`.
4. Add a `renderMyBehavior(b)` card renderer in `vial.js` and wire it into `renderBehaviorCard()`.
5. Add a `defaultMyBehavior()` to `project.js` and call it from `addBehavior()` in `vial.js`.

## Debugging expressions

```js
// In browser console after page loads:
const {compile} = window.vialDebug;
const project = window.vialDebug.project;
const result = compile(project.base, project.behaviors);
console.log(result.expressions);
console.log(result.mappings);
```

## Elecom Huge Plus button map (profile)

B1=Left click, B2=Right click, B3=Middle/wheel click, B4=Back, B5=Forward,
B6=Fn1 (top-left front), B7=Fn2 (top-left back), B8=Fn3 (top-right).
Thumb side: scroll wheel (B3) + tilt, flanked by B1 (large) and B4/B5 above.
Top: B6 front-left, B7 back-left, B2 right-of-ball, B8 far-right.

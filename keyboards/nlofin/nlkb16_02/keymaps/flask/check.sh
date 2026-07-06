#!/usr/bin/env bash
# Validates the NLKB16-02 flask keymap before calling a change "done".
# Ported from keyboards/ploopyco/madromys/keymaps/vial/check.sh.
#
# Catches the mistake classes that a clean `make` won't:
#   1. vial.json customKeycodes[] and keymap.c's `enum nlkb16_keycodes` are
#      different lengths (Vial addresses customs positionally from QK_KB_0).
#   2. A custom keycode is in the enum but has no `case` in keymap.c.
#   3. A local module .c exists here but isn't in rules.mk's SRC += list.
#   4. A shared/ module is #included but its .c isn't in SRC +=.
# Then runs the real build and fails on ANY compiler warning.
#
# Usage: keyboards/nlofin/nlkb16_02/keymaps/flask/check.sh [--skip-build]

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR" && git rev-parse --show-toplevel)"
KEYMAP_DIR="$SCRIPT_DIR"

PASS=1
note() { printf '\n=== %s ===\n' "$1"; }
ok()   { printf '  OK    %s\n' "$1"; }
fail() { printf '  FAIL  %s\n' "$1"; PASS=0; }

note "Keycode sync: vial.json customKeycodes[] vs keymap.c enum, and process_record_user coverage"
python3 - "$KEYMAP_DIR/vial.json" "$KEYMAP_DIR/keymap.c" <<'PYEOF'
import json, re, sys

vial_path, keymap_path = sys.argv[1], sys.argv[2]
src = open(keymap_path).read()

vial = json.load(open(vial_path))
vial_names = [kc["name"] for kc in vial.get("customKeycodes", [])]

m = re.search(r"enum\s+nlkb16_keycodes\s*\{(.*?)\};", src, re.S)
if not m:
    print("  FAIL  could not find 'enum nlkb16_keycodes { ... };' in keymap.c")
    sys.exit(1)

enum_names = []
for line in m.group(1).splitlines():
    line = re.sub(r"//.*", "", line).strip().rstrip(",")
    if not line:
        continue
    ident = re.match(r"[A-Za-z_][A-Za-z0-9_]*", line)
    if ident:
        enum_names.append(ident.group(0))

width = max([len(n) for n in enum_names] + [10])
print(f"  {'idx':>3}  {'enum (keymap.c)':<{width}}  vial.json name")
for i in range(max(len(enum_names), len(vial_names))):
    e = enum_names[i] if i < len(enum_names) else "<missing>"
    v = vial_names[i] if i < len(vial_names) else "<missing>"
    flag = "  <-- LENGTH MISMATCH" if (i >= len(enum_names) or i >= len(vial_names)) else ""
    print(f"  {i:>3}  {e:<{width}}  {v}{flag}")

ok_so_far = True
print()
if len(enum_names) != len(vial_names):
    print(f"  FAIL  {len(enum_names)} enum entries vs {len(vial_names)} vial.json entries (must match)")
    ok_so_far = False
elif len(enum_names) > 64:
    print(f"  FAIL  {len(enum_names)} custom keycodes exceeds the 64-slot keyboard range (0x7E00-0x7E3F)")
    ok_so_far = False
else:
    print(f"  OK    {len(enum_names)}/64 custom keycode slots used, counts match")

unhandled = [n for n in enum_names if not re.search(r"\bcase\s+" + re.escape(n) + r"\s*:", src)]
if unhandled:
    print(f"  FAIL  no 'case {{kc}}:' in keymap.c for: {', '.join(unhandled)} (keycode would silently no-op)")
    ok_so_far = False
else:
    print(f"  OK    every enum entry has a process_record_user case")

sys.exit(0 if ok_so_far else 1)
PYEOF
[ $? -eq 0 ] || PASS=0

note "rules.mk: every local module .c is wired into SRC +="
shopt -s nullglob
for f in "$KEYMAP_DIR"/*.c; do
    base="$(basename "$f")"
    [ "$base" = "keymap.c" ] && continue
    if grep -q "SRC[[:space:]]*+=[[:space:]]*$base" "$KEYMAP_DIR/rules.mk"; then
        ok "$base referenced in rules.mk"
    else
        fail "$base exists but is NOT in rules.mk's SRC += list (it will not compile in)"
    fi
done

note "rules.mk: every #included shared/ module has its .c in SRC +="
for mod in $(grep -ho '#include "shared/[a-z_]*\.h"' "$KEYMAP_DIR"/*.c "$KEYMAP_DIR"/*.h 2>/dev/null | sed 's/.*shared\/\(.*\)\.h.*/\1/' | sort -u); do
    if grep -q "SRC[[:space:]]*+=[[:space:]]*shared/$mod\.c" "$KEYMAP_DIR/rules.mk"; then
        ok "shared/$mod.c referenced in rules.mk"
    else
        fail "shared/$mod.h is #included but shared/$mod.c is NOT in rules.mk's SRC += list"
    fi
done

if [ "${1:-}" = "--skip-build" ]; then
    note "Build: skipped (--skip-build)"
else
    note "Build: make nlofin/nlkb16_02:flask"
    BUILD_LOG="$(mktemp)"
    # lto-wrapper's "serial compilation" notice is toolchain noise on this
    # board (LTO enabled keyboard-wide), not a code warning — filter it.
    if ( cd "$REPO_ROOT" && make nlofin/nlkb16_02:flask ) >"$BUILD_LOG" 2>&1; then
        if grep -iE "warning:|error:" "$BUILD_LOG" | grep -vq "lto-wrapper: warning: using serial compilation"; then
            fail "build succeeded but emitted warnings (treat as bugs in this keymap):"
            grep -iE "warning:|error:" "$BUILD_LOG" | grep -v "lto-wrapper: warning: using serial compilation"
        else
            ok "clean build, no warnings"
        fi
    else
        fail "build failed:"
        tail -40 "$BUILD_LOG"
    fi
    rm -f "$BUILD_LOG"
fi

note "Result"
if [ "$PASS" -eq 1 ]; then
    echo "  PASS -- safe to consider this change done."
    exit 0
else
    echo "  FAIL -- fix the items above before considering this change done."
    exit 1
fi

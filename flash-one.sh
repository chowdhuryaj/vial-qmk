#!/usr/bin/env bash
# Flash exactly ONE target and exit. Use this instead of flash-all.sh whenever
# a board needs recovering, or whenever you only want to touch one device.
#
# flash-all.sh walks three targets back to back, and each step just waits for
# the next RPI-RP2 mount. If a board re-enters its bootloader right after being
# flashed -- which is exactly what happens when the bootloader button is still
# held as it reboots -- the NEXT image in the sequence lands on it. That is how
# a Svalboard image ends up on an Adept (symptom: the board spams one letter
# forever, because the Svalboard matrix does not exist on Adept hardware and
# the floating pins read as a stuck key).
#
# This script writes one image and stops, so that cannot happen.
#
# Usage: ./flash-one.sh adept | sval-left | sval-right

set -uo pipefail

BOOT_VOL="/Volumes/RPI-RP2"

case "${1:-}" in
    adept)
        LABEL="Ploopy Adept"
        UF2="/Users/aj/vial-qmk/ploopyco_madromys_rev1_001_vial.uf2"
        ENTRY="unplug, hold the Bottom-Left button, plug back in"
        ;;
    sval-left)
        LABEL="Svalboard LEFT"
        UF2="/Users/aj/svalboard-vial-qmk/svalboard_trackball_pmw3389_left_flask.uf2"
        ENTRY="double-tap the reset button on the LEFT half (USB connected to it)"
        ;;
    sval-right)
        LABEL="Svalboard RIGHT"
        UF2="/Users/aj/svalboard-vial-qmk/svalboard_trackball_pmw3389_right_flask.uf2"
        ENTRY="double-tap the reset button on the RIGHT half (USB connected to it)"
        ;;
    *)
        echo "Usage: $0 adept | sval-left | sval-right"
        exit 2
        ;;
esac

if [ ! -f "$UF2" ]; then
    echo "!! $UF2 is missing — build it first."
    exit 1
fi

echo "Target : $LABEL"
echo "Image  : $(basename "$UF2") ($(du -h "$UF2" | cut -f1))"
echo "Entry  : $ENTRY"
echo

if [ -d "$BOOT_VOL" ]; then
    echo "RPI-RP2 is already mounted — using it."
else
    echo "Waiting for RPI-RP2 to appear… (Ctrl-C to abort)"
    while [ ! -d "$BOOT_VOL" ]; do sleep 0.5; done
fi

echo "Mounted — writing…"
# The board ejects the drive the instant the write completes, so cp almost
# always reports an I/O error even on success. The drive disappearing is the
# real success signal, not cp's exit code.
cp "$UF2" "$BOOT_VOL/" 2>/dev/null

waited=0
while [ -d "$BOOT_VOL" ] && [ "$waited" -lt 30 ]; do
    sleep 0.5
    waited=$((waited + 1))
done

if [ -d "$BOOT_VOL" ]; then
    echo "?? RPI-RP2 still mounted after 15s — the flash may have failed."
    exit 1
fi

echo "✓ $LABEL flashed (drive ejected, board rebooting)."
echo
echo "LET GO of the bootloader button now. Holding it through boot puts the"
echo "board straight back into the bootloader, where the next thing written"
echo "to the drive — whatever that is — becomes its firmware."

#!/usr/bin/env bash
# Guided UF2 flasher for the three RP2040 targets (Adept + Svalboard L/R).
# All three enumerate the same RPI-RP2 bootloader drive, so they can't be
# told apart — this walks them one at a time: prompt → wait for the drive →
# copy → confirm the drive vanished (= write done, board rebooted) → next.
#
# Enter the bootloader per device:
#   Adept        : unplug, hold Bottom-Left button, plug in
#   Svalboard L/R: DOUBLE-TAP the reset button (RP2040_BOOTLOADER_DOUBLE_TAP_
#                  RESET is compiled in, 500ms window) on the half whose USB is
#                  connected. The keymapped QK_BOOT key only works when the
#                  CORRECT-side firmware is already on that half — double-tap
#                  reset is keymap-independent, so use it especially when
#                  recovering a wrong-side flash.
set -u

BOOT_VOL="/Volumes/RPI-RP2"

# NOTE the Svalboard targets: the CORRECT build is the PMW3389 trackball
# variant (svalboard/trackball/pmw3389/{left,right}:flask). The plain
# svalboard/{left,right}:flask target compiles NO pointing driver → dead
# trackball (hardware-verified wrong flash 2026-07-03 and again 07-04).
# Left fw and right fw are NOT interchangeable — each half needs its own side.
ADEPT_UF2="/Users/aj/vial-qmk/ploopyco_madromys_rev1_001_vial.uf2"
SVAL_L_UF2="/Users/aj/svalboard-vial-qmk/svalboard_trackball_pmw3389_left_flask.uf2"
SVAL_R_UF2="/Users/aj/svalboard-vial-qmk/svalboard_trackball_pmw3389_right_flask.uf2"

flash_one() {
    local label="$1" uf2="$2"
    echo
    echo "=================================================================="
    echo "  NEXT: $label"
    echo "  file: $(basename "$uf2") ($(du -h "$uf2" | cut -f1))"
    echo "=================================================================="
    if [ ! -f "$uf2" ]; then
        echo "  !! uf2 missing — build first. Skipping."
        return 1
    fi

    # If a bootloader drive is already mounted from a previous step, wait for
    # it to clear so we don't flash the wrong image onto a lingering mount.
    if [ -d "$BOOT_VOL" ]; then
        echo "  (waiting for the previous RPI-RP2 to eject…)"
        while [ -d "$BOOT_VOL" ]; do sleep 0.5; done
    fi

    echo "  Put $label into bootloader now. Waiting for RPI-RP2…"
    echo "  (Ctrl-C to abort the whole run.)"
    while [ ! -d "$BOOT_VOL" ]; do sleep 0.5; done

    echo "  RPI-RP2 mounted — flashing…"
    # The board ejects the drive itself the instant the write completes, so
    # cp almost always reports an I/O error even on success. Treat the drive
    # disappearing as the real success signal, not cp's exit code.
    cp "$uf2" "$BOOT_VOL/" 2>/dev/null

    local waited=0
    while [ -d "$BOOT_VOL" ] && [ "$waited" -lt 30 ]; do
        sleep 0.5
        waited=$((waited + 1))
    done

    if [ -d "$BOOT_VOL" ]; then
        echo "  ?? RPI-RP2 still mounted after 15s — flash may have failed."
        echo "     Check the drive manually. Continuing."
        return 1
    fi
    echo "  ✓ $label flashed (drive ejected, board rebooting)."
    return 0
}

echo "Guided flash: Adept + Svalboard left + Svalboard right."
echo "Order is up to you — each step waits for the next bootloader mount."

flash_one "Ploopy Adept"      "$ADEPT_UF2"
flash_one "Svalboard LEFT"    "$SVAL_L_UF2"
flash_one "Svalboard RIGHT"   "$SVAL_R_UF2"

echo
echo "All three done. First boot re-seeds tunables (Adept EEPROM v12 / Sval"
echo "user v7) — Vial keymap/macros/combos are untouched."

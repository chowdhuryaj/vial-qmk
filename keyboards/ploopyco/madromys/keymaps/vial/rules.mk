# Vial / VIA
VIA_ENABLE = yes
VIAL_ENABLE = yes

# HID console (qmk console) for live pd_dprintf tuning output. See
# POINTING_DEVICE_DEBUG in config.h.
CONSOLE_ENABLE = yes

# Keymap features
COMBO_ENABLE = yes
TAP_DANCE_ENABLE = yes
DEFERRED_EXEC_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes
LEADER_ENABLE = yes

# QMK core auto-mouse (re-added 2026-07-03 on fresh user ask; removed
# 2026-07-01 — see CLAUDE.md "Removed features"). Ships DISABLED at runtime;
# Flask channel 0x1B turns it on / tunes it.
# GOTCHA: this fork's builddefs/common_features.mk predates the feature — the
# core .c/.h exist (quantum/pointing_device/) and quantum.c/pointing_device.c
# carry the #ifdef hooks, but no make plumbing. Wire it by hand; a plain
# `POINTING_DEVICE_AUTO_MOUSE_ENABLE = yes` is silently inert here.
OPT_DEFS += -DPOINTING_DEVICE_AUTO_MOUSE_ENABLE
SRC += $(QUANTUM_DIR)/pointing_device/pointing_device_auto_mouse.c

# Ported drashna pointing-device modules
SRC += shared/pd_accel.c
SRC += shared/pointing_device_smoothing.c
SRC += shared/pd_gestures.c
SRC += drag_scroll.c
SRC += shared/wiggle_ball.c

# Ported getreuer typing modules
SRC += shared/custom_shift_keys.c
SRC += shared/select_word.c
SRC += shared/sentence_case.c

# Autoscroll (Ben White / Contour Shuttle model)
SRC += shared/autoscroll.c

# Wheel chords (button-held ball gestures, 2026-07-03)
SRC += shared/wheel_chords.c

# OS-aware editing shortcuts (2026-07-03). OS detection is a generic feature
# (builddefs/generic_features.mk) — plain enable works, no hand-wiring needed
# (unlike auto-mouse above).
OS_DETECTION_ENABLE = yes
SRC += shared/os_shortcuts.c

# Freeze diagnostic (2026-07-03): pointing-pipeline gap watermark, HID 0x1F.
SRC += shared/pipeline_diag.c

# Num word (v10 parity with the Svalboard): caps-word-for-numbers layer hold.
SRC += shared/num_word.c

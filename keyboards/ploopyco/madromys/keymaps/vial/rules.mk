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

# Ported drashna pointing-device modules
SRC += pd_accel.c
SRC += pointing_device_smoothing.c
SRC += pd_gestures.c
SRC += drag_scroll.c

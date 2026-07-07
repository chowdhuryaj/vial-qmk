# Vial / VIA / VialRGB (stock-protocol RGB control coexists with the Flask
# tuning channels — see VIA_CUSTOM_LIGHTING_ENABLE in config.h)
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes

# Encoders are Vial-remappable per layer (CCW/CW) via the dynamic encoder map.
ENCODER_MAP_ENABLE = yes

# Keymap features
COMBO_ENABLE = yes
TAP_DANCE_ENABLE = yes
DEFERRED_EXEC_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes
LEADER_ENABLE = yes

# OS-aware editing shortcuts; detection is a generic feature (plain enable
# works — unlike the Adept's hand-wired auto-mouse, no plumbing needed).
OS_DETECTION_ENABLE = yes

# Autoscroll needs a mouse report to ride on. No sensor here — the "custom"
# driver's weak defaults are all no-ops (quantum/pointing_device.c:95), so
# pointing_device_task runs each loop with an empty report and autoscroll
# injects wheel ticks into it. ASC_UP/ASC_DOWN step the speed (bind them to
# a knob for a scroll-speed dial).
POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = custom

# Shared Flask modules (git submodule — see CLAUDE.md "Shared module submodule")
SRC += shared/custom_shift_keys.c
SRC += shared/select_word.c
SRC += shared/sentence_case.c
SRC += shared/os_shortcuts.c
SRC += shared/num_word.c
SRC += shared/autoscroll.c

# Board-local modules
SRC += per_layer_rgb.c
SRC += oled_display.c

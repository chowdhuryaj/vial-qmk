#include "os_shortcuts.h"
#include "os_detection.h"

static bool    os_sc_mac      = OS_SHORTCUTS_MAC_DEFAULT;
static bool    os_sc_follow   = OS_SHORTCUTS_FOLLOW_DEFAULT;
static uint8_t os_sc_detected = OS_UNSURE;

static bool apply_detection(void) {
    if (!os_sc_follow || os_sc_detected == OS_UNSURE) return false;
    bool mac = (os_sc_detected == OS_MACOS || os_sc_detected == OS_IOS);
    if (mac == os_sc_mac) return false;
    os_sc_mac = mac;
    return true;
}

void os_shortcuts_tap(os_shortcut_t action) {
    switch (action) {
        case OS_SHORTCUT_CUT:
            tap_code16(os_sc_mac ? G(KC_X) : C(KC_X));
            break;
        case OS_SHORTCUT_COPY:
            tap_code16(os_sc_mac ? G(KC_C) : C(KC_C));
            break;
        case OS_SHORTCUT_PASTE:
            tap_code16(os_sc_mac ? G(KC_V) : C(KC_V));
            break;
        case OS_SHORTCUT_UNDO:
            tap_code16(os_sc_mac ? G(KC_Z) : C(KC_Z));
            break;
        case OS_SHORTCUT_REDO:
            // Windows/Linux redo is app-splintered (^Y vs ^⇧Z); ^Y is the
            // classic Windows binding and what getreuer's tables use.
            tap_code16(os_sc_mac ? G(S(KC_Z)) : C(KC_Y));
            break;
    }
}

void os_shortcuts_set_mac(bool mac) {
    os_sc_mac = mac;
}

bool os_shortcuts_get_mac(void) {
    return os_sc_mac;
}

void os_shortcuts_set_follow(bool follow) {
    os_sc_follow = follow;
    apply_detection();
}

bool os_shortcuts_get_follow(void) {
    return os_sc_follow;
}

bool os_shortcuts_on_detect(os_variant_t os) {
    os_sc_detected = (uint8_t)os;
    return apply_detection();
}

uint8_t os_shortcuts_detected(void) {
    return os_sc_detected;
}

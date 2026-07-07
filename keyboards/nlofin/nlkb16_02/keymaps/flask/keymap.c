/* SPDX-License-Identifier: GPL-2.0-or-later */
/* NLOFIN NLKB16-02 Flask keymap.
 *
 * Architecture mirrors the Ploopy Adept flask firmware (see
 * keyboards/ploopyco/madromys/keymaps/vial/keymap.c and CLAUDE.md): Vial for
 * keymap/macros/dynamic entries, VialRGB for stock RGB control, and the
 * Flask raw HID protocol (VIA custom-value commands, channels 0x10+) for
 * everything Vial can't express — module tunables, leader sequences,
 * per-layer per-key RGB, and the OLED push channel.
 */

#include QMK_KEYBOARD_H

#include "shared/custom_shift_keys.h"
#include "shared/select_word.h"
#include "shared/sentence_case.h"
#include "shared/os_shortcuts.h"
#include "shared/num_word.h"
#include "shared/autoscroll.h"
#include "per_layer_rgb.h"
#include "oled_display.h"

/* ---------------------------------------------------------------------------
 * Custom keycodes.
 *
 * These map 1:1 to the Vial "customKeycodes" list, indexed from QK_KB_0.
 * The order here MUST match the order in vial.json (THE keycode rule).
 * ------------------------------------------------------------------------- */
enum nlkb16_keycodes {
    SELWORD = QK_KB_0, // 0  select word fwd (Shift: line); repeat extends
    SELWDBK,           // 1  select word backward
    SELLINE,           // 2  select line fwd
    SELLNUP,           // 3  select line upward
    SC_TOG,            // 4  sentence case: toggle (state persists)
    CSK_TOG,           // 5  custom shift keys: toggle
    OS_CUT,            // 6  cut (⌘X / ^X)
    OS_COPY,           // 7  copy (⌘C / ^C)
    OS_PSTE,           // 8  paste (⌘V / ^V)
    OS_UNDO,           // 9  undo (⌘Z / ^Z)
    OS_REDO,           // 10 redo (⇧⌘Z / ^Y)
    OS_APPSW,          // 11 app switch (⌘Tab / AltTab)
    OS_SELALL,         // 12 select all (⌘A / ^A)
    OS_NTAB,           // 13 new tab (⌘T / ^T)
    OS_CLOSE,          // 14 close window/tab (⌘W / ^W)
    NUMWORD,           // 15 num word: layer stays on while typing numbers
    RGBMAP_TOG,        // 16 per-layer RGB map: toggle map mode vs VialRGB effects
    // v3 additions (2026-07-06)
    OS_TABP,           // 17 previous browser tab (^⇧Tab, both modes)
    OS_TABN,           // 18 next browser tab (^Tab, both modes)
    OS_LNCH,           // 19 launcher (⌘Space in mac mode; pc: no-op)
    // v4 additions (2026-07-06): autoscroll, stepped mode only (no ball =
    // no jog). Bind to a knob's CCW/CW for a scroll-speed dial.
    ASC_UP,            // 20 autoscroll: step speed up (through zero stops)
    ASC_DOWN,          // 21 autoscroll: step speed down
    // v7 additions (2026-07-07): layer dial — wraps through all 8 layers.
    LYR_UP,            // 22 go one layer up (7 wraps to 0)
    LYR_DN,            // 23 go one layer down (0 wraps to 7)
};

/* ---------------------------------------------------------------------------
 * Keymap — 8 dynamic layers, seeded stock-parity.
 *
 * LAYOUT() order: (0,0..0,3), (0,4)=knob1 push, (1,0..1,3), (1,4)=knob2 push,
 * (2,0..2,3), (2,4)=BIG knob push (hidden in stock's layout, exposed here),
 * (3,0..3,3).
 *
 * Stock behavior kept: knob2 push cycles layers (TO chain, now through all
 * 8), big knob push = mute, L1 = RGB controls. L7 = settings (boot, RGB map).
 * ------------------------------------------------------------------------- */
// clang-format off
/* FLASK-BAKE-BEGIN — Flask's Build tab "Bake current keymap as default"
 * replaces everything between these two markers with a raw-matrix dump of
 * the device's live keymap (row-major [row][col] hex keycodes, one block per
 * layer). Don't hand-edit a baked block; edit the layout in Flask/Vial and
 * re-bake. The LAYOUT() form below survives only until the first bake.
 * Worth more on this board than the others: the Maple bootloader mass-erases
 * the EEPROM on every flash, so the baked block IS the post-flash layout. */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Hand-seeded 2026-07-07 for the rads workstation (RadKit AHK hotkey
       targets — ~/RadKit/README.md is the decode table for every chord
       below). Raw matrix form, row-major [row][col] hex keycodes; a Flask
       re-bake replaces this whole block.

       Standalone-survival rules (locked-down PC: no Flask, no Vial GUI, and
       the Maple bootloader erases the EEPROM on every flash — this block IS
       the post-flash keymap):
       - knob2 push (1,4) = LYR_UP (0x7E16) on EVERY layer. The old stock
         TO() chain had TO(0) on layer 0 — a dead end that made layers 2-6
         unreachable post-flash without host software.
       - knob1 push (0,4) on work layers = TO(0) (0x5200): one-press escape
         back to base from anywhere.
       - unbound keys on work layers are KC_NO (0x0000), NOT KC_TRNS — a
         fall-through to base would fire F11/F12, which PACS binds to
         scout/localizer. */
    /* ===== 0: BASE ===== */
    [0] = {
        { 0x0015, 0x001C, 0x0044, 0x0045, 0x00AE },
        { 0x0068, 0x0069, 0x006A, 0x006B, 0x7E16 },
        { 0x0029, 0x022B, 0x002B, 0x0028, 0x00A8 },
        { 0x006B, 0x006C, 0x002C, 0x4139, 0x0000 },
    },
    /* ===== 1: RGB / features ===== */
    [1] = {
        { 0x7C00, 0x7829, 0x0001, 0x0001, 0x0001 },
        { 0x7826, 0x7825, 0x0001, 0x0001, 0x7E16 },
        { 0x7822, 0x7821, 0x7823, 0x0001, 0x0001 },
        { 0x7820, 0x7828, 0x7824, 0x0001, 0x0000 },
    },
    /* ===== 2: DICT — PowerScribe dictation/report =====
       Caps=dictate  F13/F14=prev/next field  ^!R=compare
       ^F13/^F14=word L/R  F7/F8=bksp/fwd-del
       F17/⇧F17=undo/redo  ^!N=notepad  ^!S=stream deck
       ^!Z=prelim  F18=changelist  F19=pager  ^!K=radiopaedia */
    [2] = {
        { 0x0039, 0x0068, 0x0069, 0x0515, 0x5200 },
        { 0x0168, 0x0169, 0x0040, 0x0041, 0x7E16 },
        { 0x006C, 0x026C, 0x0511, 0x0516, 0x0001 },
        { 0x051D, 0x006D, 0x006E, 0x050E, 0x0000 },
    },
    /* ===== 3: CALL — call workflow =====
       ^!Z=prelim  F18=changelist  F19=pager  ^!Q=break-the-glass
       ^!K=radiopaedia  ^!L=edge  ^!E=epic  F24=reference menu
       ^!S=deck  ^!N=notepad  Caps=dictate  ^!R=compare
       ⇧F7/⇧F8=cursor to L/R monitor  ^⇧Caps=master toggle  ^!⇧F12=panic */
    [3] = {
        { 0x051D, 0x006D, 0x006E, 0x0514, 0x5200 },
        { 0x050E, 0x050F, 0x0508, 0x0073, 0x7E16 },
        { 0x0516, 0x0511, 0x0039, 0x0515, 0x0001 },
        { 0x0240, 0x0241, 0x0339, 0x0745, 0x0000 },
    },
    /* ===== 4: UTIL — panels & utility =====
       ^!S=deck  ^!N=notepad  F24=reference  ^!K=radiopaedia
       ^!L=edge  ^!E=epic  ^!Q=break-the-glass
       ⇧F7/⇧F8=monitor teleport  ^⇧Caps=master toggle  ^!⇧F12=panic */
    [4] = {
        { 0x0516, 0x0511, 0x0073, 0x050E, 0x5200 },
        { 0x050F, 0x0508, 0x0514, 0x0000, 0x7E16 },
        { 0x0240, 0x0241, 0x0000, 0x0000, 0x0001 },
        { 0x0339, 0x0745, 0x0000, 0x0000, 0x0000 },
    },
    /* ===== 5: PACS — IntelliSpace tools (RadKit ^!x mouse-macro chords) =====
       ^!1=measure  ^!⇧7=ROI  ^!D=scout  ^!==clear
       ^!⇧3=localizer  ^!G=arrow  ^!3=angle  F15=BODY/NEURO WL mode
       F16/^F16=next/prev WL preset  F11/F12=scout/localizer keys
       ==zoom in  -=zoom out  Caps=dictate  ^!⇧F12=panic */
    [5] = {
        { 0x051E, 0x0724, 0x0507, 0x052E, 0x5200 },
        { 0x0720, 0x050A, 0x0520, 0x006A, 0x7E16 },
        { 0x006B, 0x016B, 0x0044, 0x0045, 0x0001 },
        { 0x002E, 0x002D, 0x0039, 0x0745, 0x0000 },
    },
    /* ===== 6: SITE — reserved for iSite 4.7 bindings (ISITE_EXE still
       unmapped in RadKit; fill once Window Spy'd at an iSite workstation) */
    [6] = {
        { 0x0000, 0x0000, 0x0000, 0x0000, 0x5200 },
        { 0x0000, 0x0000, 0x0000, 0x0000, 0x7E16 },
        { 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 },
        { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    },
    /* ===== 7: CONF — settings ===== */
    [7] = {
        { 0x0001, 0x0001, 0x0001, 0x7C00, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x7E16 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x7E10, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
};
/* FLASK-BAKE-END */

#ifdef ENCODER_MAP_ENABLE
// Firmware defaults = the post-flash state (Vial's dynamic encoder overrides
// live in EEPROM, which the Maple bootloader erases on every flash). Work
// layers seed their RadKit-relevant dials; KC_TRNS falls through to layer 0.
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT), ENCODER_CCW_CW(KC_PGDN, KC_PGUP), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(RGB_VAD, RGB_VAI), ENCODER_CCW_CW(RGB_SAD, RGB_SAI), ENCODER_CCW_CW(RGB_HUD, RGB_HUI) },
    // DICT: knob1 = report field prev/next, knob2 = word left/right.
    [2] = { ENCODER_CCW_CW(KC_F13, KC_F14), ENCODER_CCW_CW(C(KC_F13), C(KC_F14)), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [3] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [4] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    // PACS: knob1 = W/L preset prev/next; knob2 = autoscroll speed dial
    // (hands-free stack cine — stop-on-any-key exits it); big knob = zoom
    // out/in (RadKit maps wheel zoom to =/-). Plain PGDN/PGUP paging stays
    // on layer 0's knob2.
    [5] = { ENCODER_CCW_CW(C(KC_F16), KC_F16), ENCODER_CCW_CW(ASC_DOWN, ASC_UP), ENCODER_CCW_CW(KC_MINS, KC_EQL) },
    [6] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [7] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
};
#endif
// clang-format on

/* ---------------------------------------------------------------------------
 * Dynamic leader sequences (Adept pattern). QMK core LEADER_ENABLE collects
 * up to 5 keys and calls leader_end_user(); slots live in RAM, are edited
 * over HID channel 0x19, and persist in the EEPROM datablock. A slot is
 * [key0..key4, output]; unused key positions are KC_NO, output == KC_NO
 * means the slot is empty. Output fires via tap_code16 — basic keycodes +
 * C()/S()/A()/G() combos only, NO macros/layer keys/QK_KB_* customs.
 * ------------------------------------------------------------------------- */
static uint16_t leader_seqs[NLK_LEADER_SEQ_COUNT][NLK_LEADER_SEQ_KEYS + 1];

// Live leader timeout (v5): strong override of the weak getter added to
// quantum/leader.c — the sequence fires this many ms after the last key.
// HID 0x19/0x01, persisted; lower = snappier output, less typing margin.
static uint16_t leader_timeout_ms = LEADER_TIMEOUT;

uint16_t leader_timeout_get(void) {
    return leader_timeout_ms;
}

static void nlk_leader_set_timeout(uint16_t ms) {
    if (ms < NLK_LEADER_TIMEOUT_MIN) ms = NLK_LEADER_TIMEOUT_MIN;
    if (ms > NLK_LEADER_TIMEOUT_MAX) ms = NLK_LEADER_TIMEOUT_MAX;
    leader_timeout_ms = ms;
}

// Globals defined in quantum/leader.c but not declared in leader.h (it only
// exposes fixed-arity prefix matchers, which can't length-check).
extern uint16_t leader_sequence[5];
extern uint8_t  leader_sequence_size;

static bool leader_seq_matches(const uint16_t *seq) {
    uint8_t len = 0;
    while (len < NLK_LEADER_SEQ_KEYS && seq[len] != KC_NO) {
        len++;
    }
    if (len == 0 || leader_sequence_size != len) return false;
    for (uint8_t i = 0; i < len; i++) {
        if (leader_sequence[i] != seq[i]) return false;
    }
    return true;
}

// Weak default in quantum/leader.c:22.
void leader_end_user(void) {
    for (uint8_t s = 0; s < NLK_LEADER_SEQ_COUNT; s++) {
        if (leader_seqs[s][NLK_LEADER_SEQ_KEYS] != KC_NO && leader_seq_matches(leader_seqs[s])) {
            tap_code16(leader_seqs[s][NLK_LEADER_SEQ_KEYS]);
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Per-combo layer gating (Adept parity): u16 mask per combo slot — bit N
 * allows the combo while layer N is the highest active layer. Default 0xFFFF
 * keeps stock behavior. Compiles in via COMBO_SHOULD_TRIGGER (config.h);
 * edited over HID channel 0x20, persisted in nlk_config.
 * ------------------------------------------------------------------------- */
static uint16_t combo_layer_masks[VIAL_COMBO_ENTRIES];

// Weak default in quantum/process_keycode/process_combo.c:98.
bool combo_should_trigger(uint16_t combo_index, combo_t *combo, uint16_t keycode, keyrecord_t *record) {
    if (combo_index >= VIAL_COMBO_ENTRIES) return true;
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    return (combo_layer_masks[combo_index] & ((uint16_t)1 << layer)) != 0;
}

/* ---------------------------------------------------------------------------
 * Persisted settings (EEPROM user datablock).
 *
 * RAM mirror of the EECONFIG_USER_DATA_SIZE datablock; bump
 * EECONFIG_USER_DATA_VERSION (config.h) on any layout change. nlk_config is
 * the LAST-SAVED state; the modules' runtime variables are the live state —
 * they meet at boot (apply) and on an explicit per-channel save.
 * ------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    // getreuer typing modules + leader.
    uint8_t            csk_enabled;
    custom_shift_key_t csk_table[CUSTOM_SHIFT_KEYS_SLOTS];
    uint8_t            selword_mac;
    uint8_t            sentence_case_on;
    uint16_t           leader_seqs[NLK_LEADER_SEQ_COUNT][NLK_LEADER_SEQ_KEYS + 1];
    // OS-aware shortcuts.
    uint8_t os_follow;
    uint8_t os_mac;
    // Num word.
    uint16_t nw_timeout;
    uint8_t  nw_layer;
    // Per-combo layer masks.
    uint16_t combo_layer_masks[VIAL_COMBO_ENTRIES];
    // Per-layer per-key RGB map.
    uint8_t      rgbmap_enabled;
    nlk_rgbmap_t rgbmap;
    // OLED display.
    uint16_t disp_hold_ms;
    // v2: per-line fallback-screen widget assignment + idle sleep (seconds
    // of no input before the panel switches off).
    // v4: widgets shrank to the 4 big lines (display went double-height).
    uint8_t  disp_widgets[NLK_DISPLAY_BIG_LINES];
    uint16_t disp_sleep_s;
    // v3: autoscroll (stepped mode; this board has no ball, so no jog
    // tunables — only invert, speed scale and the stop-on-key switch).
    uint8_t  as_inverted;
    uint16_t as_speed_scale_x100;
    uint8_t  as_stop_on_key;
    // v4: custom widget text per line, overlay duration, leader timeout.
    char     disp_custom[NLK_DISPLAY_BIG_LINES][NLK_DISPLAY_BIG_COLS];
    uint16_t disp_overlay_ms;
    uint16_t leader_timeout_ms;
} nlk_config_t;
_Static_assert(sizeof(nlk_config_t) <= EECONFIG_USER_DATA_SIZE, "nlk_config_t exceeds EECONFIG_USER_DATA_SIZE");

static nlk_config_t nlk_config;

static void nlk_config_write(void) {
    // eeconfig_update_user_datablock also (re)writes the validity word, so a
    // save on a fresh/invalid block marks it valid in the same call.
    eeconfig_update_user_datablock(&nlk_config, 0, sizeof(nlk_config));
}

static void nlk_config_set_defaults(void) {
    nlk_config = (nlk_config_t){
        .csk_enabled      = CUSTOM_SHIFT_KEYS_ENABLED_DEFAULT ? 1 : 0,
        .selword_mac      = SELECT_WORD_MAC_DEFAULT ? 1 : 0,
        .sentence_case_on = SENTENCE_CASE_ON_DEFAULT ? 1 : 0,
        .os_follow        = OS_SHORTCUTS_FOLLOW_DEFAULT ? 1 : 0,
        .os_mac           = OS_SHORTCUTS_MAC_DEFAULT ? 1 : 0,
        .nw_timeout       = NUM_WORD_IDLE_TIMEOUT_DEFAULT,
        .nw_layer         = NUM_WORD_LAYER_DEFAULT,
        .rgbmap_enabled   = NLK_RGBMAP_ENABLED_DEFAULT ? 1 : 0,
        .disp_hold_ms     = NLK_DISPLAY_HOLD_MS_DEFAULT,
        .disp_widgets     = NLK_DISPLAY_WIDGET_DEFAULTS,
        .disp_sleep_s     = NLK_DISPLAY_SLEEP_S_DEFAULT,
        .as_inverted      = AUTOSCROLL_INVERTED_DEFAULT ? 1 : 0,
        .as_speed_scale_x100 = AUTOSCROLL_SPEED_SCALE_X100,
        .as_stop_on_key   = AUTOSCROLL_STOP_ON_KEY_DEFAULT ? 1 : 0,
        .disp_overlay_ms  = NLK_DISPLAY_OVERLAY_MS_DEFAULT,
        .leader_timeout_ms = LEADER_TIMEOUT,
    };
    // csk_table, leader_seqs and rgbmap zero-init = empty slots / all-black
    // map; combo masks default to "all layers allowed"; custom widget text
    // defaults to spaces.
    for (uint8_t i = 0; i < VIAL_COMBO_ENTRIES; i++) {
        nlk_config.combo_layer_masks[i] = 0xFFFF;
    }
    memset(nlk_config.disp_custom, ' ', sizeof(nlk_config.disp_custom));
}

// Push the persisted snapshot into every module's live runtime state (boot).
static void nlk_config_apply(void) {
    custom_shift_keys_set_enabled(nlk_config.csk_enabled != 0);
    memcpy(custom_shift_keys_table(), nlk_config.csk_table, sizeof(nlk_config.csk_table));
    select_word_set_mac(nlk_config.selword_mac != 0);
    if (nlk_config.sentence_case_on) {
        sentence_case_on();
    } else {
        sentence_case_off();
    }
    memcpy(leader_seqs, nlk_config.leader_seqs, sizeof(leader_seqs));
    os_shortcuts_set_follow(nlk_config.os_follow != 0);
    os_shortcuts_set_mac(nlk_config.os_mac != 0);
    num_word_set_timeout(nlk_config.nw_timeout);
    num_word_set_layer(nlk_config.nw_layer);
    memcpy(combo_layer_masks, nlk_config.combo_layer_masks, sizeof(combo_layer_masks));
    per_layer_rgb_set_enabled(nlk_config.rgbmap_enabled != 0);
    memcpy(per_layer_rgb_table(), nlk_config.rgbmap, sizeof(nlk_config.rgbmap));
    oled_display_set_hold_ms(nlk_config.disp_hold_ms);
    oled_display_set_sleep_s(nlk_config.disp_sleep_s);
    oled_display_set_overlay_ms(nlk_config.disp_overlay_ms);
    // Through the setter (not memcpy): it clamps ids a newer config wrote.
    for (uint8_t i = 0; i < NLK_DISPLAY_BIG_LINES; i++) {
        oled_display_set_widget(i, nlk_config.disp_widgets[i]);
        oled_display_set_custom(i, (const uint8_t *)nlk_config.disp_custom[i], NLK_DISPLAY_BIG_COLS);
    }
    set_autoscroll_inverted(nlk_config.as_inverted != 0);
    set_autoscroll_speed_scale(nlk_config.as_speed_scale_x100);
    set_autoscroll_stop_on_key(nlk_config.as_stop_on_key != 0);
    nlk_leader_set_timeout(nlk_config.leader_timeout_ms);
}

/* ---------------------------------------------------------------------------
 * Key handling
 * ------------------------------------------------------------------------- */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Num word watches every key event except its own toggle to decide when
    // the number layer should drop (shared/num_word.h wiring contract).
    if (keycode != NUMWORD) {
        num_word_on_record(keycode, record);
    }

    // Autoscroll auto-exit (Adept convention, tunable via 0x1A/0x06): any
    // key press except its own controls stops it; the press still performs
    // its normal action.
    if (record->event.pressed && autoscroll_is_active() && get_autoscroll_stop_on_key()) {
        switch (keycode) {
            case ASC_UP:
            case ASC_DOWN:
                break; // the controls keep their own behavior
            default:
                autoscroll_stop();
                break;
        }
    }

    // Display overlays (v5, observe-only — the keys still do their jobs):
    // volume and RGB-brightness events flash a big readout on the glass.
    if (record->event.pressed) {
        switch (keycode) {
            case KC_AUDIO_MUTE:
            case KC_KB_MUTE:
                oled_display_overlay(NLK_OVERLAY_VOLUME, 'M');
                break;
            case KC_AUDIO_VOL_UP:
            case KC_KB_VOLUME_UP:
                oled_display_overlay(NLK_OVERLAY_VOLUME, '+');
                break;
            case KC_AUDIO_VOL_DOWN:
            case KC_KB_VOLUME_DOWN:
                oled_display_overlay(NLK_OVERLAY_VOLUME, '-');
                break;
            case RGB_VAI:
            case RGB_VAD:
                // Value renders live from rgb_matrix_get_val() during the
                // overlay window, so it tracks the change process_rgb is
                // about to apply.
                oled_display_overlay(NLK_OVERLAY_RGB_VAL, 0);
                break;
        }
    }

    // Ported getreuer modules see every key event. custom_shift_keys may
    // consume the event (Shift+key replacement); the other two only track.
    if (!process_record_custom_shift_keys(keycode, record)) {
        return false;
    }
    process_record_sentence_case(keycode, record); // state tracking; never consumes
    select_word_on_record(keycode, record);        // state tracking; never consumes

    switch (keycode) {
        case SELWORD: {
            // Shift+SELWORD selects the whole line (upstream behavior).
            if (record->event.pressed) {
                const bool shifted = (get_mods() | get_weak_mods() | get_oneshot_mods()) & MOD_MASK_SHIFT;
                select_word_register(shifted ? 'L' : 'W');
            } else {
                select_word_unregister();
            }
            return false;
        }
        case SELWDBK:
            if (record->event.pressed) select_word_register('B');
            else select_word_unregister();
            return false;
        case SELLINE:
            if (record->event.pressed) select_word_register('L');
            else select_word_unregister();
            return false;
        case SELLNUP:
            if (record->event.pressed) select_word_register('U');
            else select_word_unregister();
            return false;
        case SC_TOG:
            if (record->event.pressed) sentence_case_toggle();
            return false;
        case CSK_TOG:
            if (record->event.pressed) custom_shift_keys_set_enabled(!custom_shift_keys_get_enabled());
            return false;
        case OS_CUT:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_CUT);
            return false;
        case OS_COPY:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_COPY);
            return false;
        case OS_PSTE:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_PASTE);
            return false;
        case OS_UNDO:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_UNDO);
            return false;
        case OS_REDO:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_REDO);
            return false;
        case OS_APPSW:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_APP_SWITCH);
            return false;
        case OS_SELALL:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_SELECT_ALL);
            return false;
        case OS_NTAB:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_NEW_TAB);
            return false;
        case OS_CLOSE:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_CLOSE_WIN);
            return false;
        case NUMWORD:
            if (record->event.pressed) num_word_toggle();
            return false;
        case RGBMAP_TOG:
            if (record->event.pressed) per_layer_rgb_set_enabled(!per_layer_rgb_get_enabled());
            return false;
        case OS_TABP:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_PREV_TAB);
            return false;
        case OS_TABN:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_NEXT_TAB);
            return false;
        case OS_LNCH:
            if (record->event.pressed) os_shortcuts_tap(OS_SHORTCUT_LAUNCH);
            return false;
        case ASC_UP:
            if (record->event.pressed) {
                autoscroll_step(1);
                oled_display_overlay(NLK_OVERLAY_AUTOSCROLL, 0);
            }
            return false;
        case ASC_DOWN:
            if (record->event.pressed) {
                autoscroll_step(-1);
                oled_display_overlay(NLK_OVERLAY_AUTOSCROLL, 0);
            }
            return false;
        case LYR_UP:
        case LYR_DN: {
            // Layer dial: move to the adjacent dynamic layer, wrapping at the
            // ends. layer_move (not layer_on) so stacked TO()/momentary state
            // can't accumulate while spinning an encoder through the ring.
            if (record->event.pressed) {
                uint8_t layer = get_highest_layer(layer_state | default_layer_state);
                uint8_t count = DYNAMIC_KEYMAP_LAYER_COUNT;
                layer_move(keycode == LYR_UP ? (layer + 1) % count
                                             : (layer + count - 1) % count);
            }
            return false;
        }
    }
    return true;
}

// The weak custom-driver init returns FALSE ("init failed"), which parks
// pointing_device_status away from SUCCESS and pointing_device_task bails
// before any task hook runs — autoscroll silently dead (hardware-found
// 2026-07-07). There is no hardware to init; report success.
// (Weak default at quantum/pointing_device.c:95.)
bool pointing_device_driver_init(void) {
    return true;
}

// The custom driver's get_report is a passthrough (weak no-op) — autoscroll
// injects its wheel ticks into the empty report here. Jog never engages on
// this board (nothing feeds report.y).
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    return autoscroll_apply(mouse_report);
}

// OS detection result (weak default at quantum/os_detection.c:124). Feeds the
// os_shortcuts mode and, while follow is on, mirrors the mac/pc answer into
// select word's hotkey style so one detection drives both.
bool process_detected_host_os_kb(os_variant_t detected_os) {
    if (os_shortcuts_on_detect(detected_os)) {
        select_word_set_mac(os_shortcuts_get_mac());
    }
    return true;
}

/* RGB boot/wake holdoff — rationale + hardware bisect history at
 * NLK_RGB_HOLDOFF_MS in config.h. Armed at boot and on USB wake; release
 * restores the EEPROM RGB state (respects a saved rgb-off) and re-inits the
 * panel in case the power transition already wedged it. */
static bool     nlk_rgb_held          = false;
static uint32_t nlk_rgb_holdoff_until = 0;

static void nlk_rgb_holdoff_arm(void) {
    rgb_matrix_disable_noeeprom();
    nlk_rgb_holdoff_until = timer_read32() + NLK_RGB_HOLDOFF_MS;
    nlk_rgb_held          = true;
}

// Weak default in quantum/keyboard.c (no-op).
void suspend_wakeup_init_user(void) {
    nlk_rgb_holdoff_arm();
}

// Idle-timeout ticks for the ported getreuer modules.
void housekeeping_task_user(void) {
    select_word_task();
    sentence_case_task();
    num_word_task();
    if (nlk_rgb_held && timer_expired32(timer_read32(), nlk_rgb_holdoff_until)) {
        nlk_rgb_held = false;
        rgb_matrix_reload_from_eeprom();
        oled_display_request_reinit();
    }
}

/* ---------------------------------------------------------------------------
 * OLED + RGB hooks
 * ------------------------------------------------------------------------- */
#ifdef OLED_ENABLE
// SSD1306 128x64, glass mounted portrait (rotated 90° CW in the case) —
// rotate the canvas to compensate: 64x128 logical = 10 cols x 16 lines.
// The GLASS shows only lines 4-11 x cols 0-4 of that canvas (64x32 window —
// see NLK_DISPLAY_VISIBLE_* and the board config.h panel notes).
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_90;
}

bool oled_task_user(void) {
    return oled_display_task();
}
#endif

/* (The 2026-07-06 display-saga RX/TX debug blinks lived here — magenta and
 * cyan key flashes on every HID frame. Removed 2026-07-07 on user feedback:
 * every app edit flashed the board. git history has them if ever needed.) */
bool rgb_matrix_indicators_user(void) {
    per_layer_rgb_render();
    return false;
}

/* ---------------------------------------------------------------------------
 * Init
 * ------------------------------------------------------------------------- */
// Called on a full eeconfig reset (first boot / version bump). The core
// zeroes the datablock and writes its validity word first; this then seeds
// our defaults over the zeros.
void eeconfig_init_user(void) {
    nlk_config_set_defaults();
    nlk_config_write();
}

void keyboard_post_init_user(void) {
    if (eeconfig_is_user_datablock_valid()) {
        eeconfig_read_user_datablock(&nlk_config, 0, sizeof(nlk_config));
        if (nlk_config.nw_layer >= 8 || nlk_config.disp_hold_ms > NLK_DISPLAY_HOLD_MS_MAX) {
            // Wholesale-garbage canary: version word matched but content is
            // implausible — re-seed everything rather than trust the rest.
            nlk_config_set_defaults();
            nlk_config_write();
        }
    } else {
        nlk_config_set_defaults();
        nlk_config_write();
    }
    nlk_config_apply();
    nlk_rgb_holdoff_arm(); // LEDs dark through the USB power-up window
}

/* ---------------------------------------------------------------------------
 * Flask raw HID tuning protocol.
 *
 * VIA_CUSTOM_LIGHTING_ENABLE (config.h) routes VIA command IDs 0x07/0x08/0x09
 * here. Frame: data[0]=command, data[1]=channel, data[2]=value_id,
 * data[3..]=payload (u16 big-endian unless noted). Unknown anything →
 * data[0]=id_unhandled. via.c echoes the buffer back — do NOT call
 * raw_hid_send() here (weak default at quantum/via.c:188).
 *
 * VIALRGB COEXISTENCE: via.c calls vialrgb_*_value() BEFORE this hook on the
 * same buffer. VialRGB frames carry their value ID in data[1] (0x40..0x44) —
 * this handler must return without touching those or it would overwrite the
 * response vialrgb just wrote. Flask channels (0x00, 0x10+, all < 0x40) are
 * disjoint by design.
 *
 * GET/SET act on live module runtime state (setters clamp); SAVE snapshots
 * live state into nlk_config per-channel and writes the whole datablock.
 * ------------------------------------------------------------------------- */
// v2 (2026-07-06): display channel raw-cmd inject (0x07) + reinit (0x08),
// panel freeze diagnosis.
// v3 (2026-07-06): fallback-screen widgets (display 0x09 count RO, 0x20+line
// get/set, persisted) + idle panel sleep (0x0A seconds, 0 = never) + 3 new
// OS-shortcut keycodes (OS_TABP/OS_TABN/OS_LNCH, indices 17-19).
// v4 (2026-07-06): autoscroll channel 0x1A (stepped mode only — 0x01 invert,
// 0x02 speed scale, 0x05 live level/force-stop, 0x06 stop-on-key; no jog
// ids, this board has no ball) + ASC_UP/ASC_DOWN keycodes (indices 20-21).
// v5 (2026-07-07): display goes double-height — 4 big lines × 5 chars.
// Widgets now 0x20-0x23 (+ CUSTOM widget id 13, per-line text at 0x30+line,
// payload-addressed); push line ids are BIG lines 0-3; transient overlays
// (volume/RGB brightness/autoscroll level) with duration at 0x0B; leader
// timeout live at 0x19/0x01. RX/TX debug key blinks removed.
// v6 (2026-07-07): rendered-line mirror (display 0x0C, payload-addressed) —
// feeds the Flask HUD's live OLED tile.
// v7 (2026-07-07): LYR_UP/LYR_DN keycodes (indices 22-23, wrap through all 8
// layers via layer_move — encoder layer dial) + encoder 0/1 rotation
// direction corrected at the pin level (keyboard.json pin_a/pin_b swapped;
// the two small knobs read backwards on hardware) + RGB_MATRIX_DEFAULT_ON
// false (panel boots dark after the Maple bootloader's EEPROM erase).
// v8 (2026-07-07): LAYERNAME display widget (id 14) + RadKit workstation
// bake (keymap-only otherwise — no new channels/value ids, EEPROM v4 stands).
#define NLK_HID_PROTOCOL_VERSION 8

enum nlk_hid_channel {
    nlk_ch_meta        = 0x00, // 0x01 protocol version RO; 0x02 active layer RO
    nlk_ch_csk         = 0x16, // custom shift keys (Adept channel parity)
    nlk_ch_selword     = 0x17, // select word
    nlk_ch_sentence    = 0x18, // sentence case
    nlk_ch_leader      = 0x19, // leader sequences
    nlk_ch_autoscroll  = 0x1A, // autoscroll (stepped only — no ball, no jog; v4)
    nlk_ch_os          = 0x1D, // OS-aware shortcuts
    nlk_ch_numword     = 0x1E, // num word
    nlk_ch_combolayers = 0x20, // per-combo layer masks
    nlk_ch_rgbmap      = 0x21, // per-layer per-key RGB map (NEW, this board)
    nlk_ch_display     = 0x22, // OLED push channel (NEW, this board)
};

enum nlk_hid_meta_value {
    nlk_meta_protocol_version = 0x01, // RO
    nlk_meta_active_layer     = 0x02, // RO
};

enum nlk_hid_csk_value {
    nlk_csk_enabled    = 0x01,
    nlk_csk_slot_count = 0x02, // RO
    nlk_csk_key_base   = 0x10, // + slot (16 slots)
    nlk_csk_shift_base = 0x30, // + slot
};

enum nlk_hid_selword_value {
    nlk_selword_mac = 0x01, // bool: macOS hotkey style
};

enum nlk_hid_sentence_value {
    nlk_sentence_on = 0x01,
};

enum nlk_hid_leader_value {
    nlk_leader_timeout = 0x01, // v5: ms after the last key before output fires
    // Slots: 0x10 + seq*8 + pos; pos 0..4 = keys, 5 = output keycode.
    nlk_leader_slot_base = 0x10,
};

enum nlk_hid_autoscroll_value {
    // Adept 0x1A value-id parity; 0x03/0x04 (jog deadzone/range) are NOT
    // served — this board has no ball, jog can never engage.
    nlk_as_inverted    = 0x01,
    nlk_as_speed_scale = 0x02, // x100; 100 = Ben White's interval table as-is
    // Live: GET = signed stepped level (-9..9), 0 idle; SET force-stops.
    nlk_as_state       = 0x05,
    nlk_as_stop_on_key = 0x06, // bool: any other key press stops autoscroll
};

enum nlk_hid_os_value {
    nlk_os_follow   = 0x01, // bool: OS detection drives the mac/pc mode
    nlk_os_mac      = 0x02, // bool: live mac/pc mode
    nlk_os_detected = 0x03, // RO: raw os_variant_t
};

enum nlk_hid_numword_value {
    nlk_nw_timeout = 0x01, // idle ms before the layer drops (0 = never)
    nlk_nw_layer   = 0x02, // target layer
    nlk_nw_state   = 0x03, // live: GET = active?, SET any = force off
};

enum nlk_hid_combolayers_value {
    nlk_cl_count_id  = 0x01, // RO: number of combo slots
    nlk_cl_mask_base = 0x10, // + combo index; u16 mask, bit N = layer N allowed
};

enum nlk_hid_rgbmap_value {
    nlk_rgbmap_enabled  = 0x01,
    nlk_rgbmap_layers   = 0x02, // RO
    nlk_rgbmap_leds     = 0x03, // RO
    // Payload-addressed (value IDs stay scalar):
    // GET 0x10: payload in = [layer, led]; out = [layer, led, H, S, V]
    // SET 0x10: payload = [layer, led, H, S, V]
    nlk_rgbmap_led      = 0x10,
    // SET 0x11: payload = [layer, start_led, count(<=8), count*(H,S,V)]
    nlk_rgbmap_bulk     = 0x11,
    // SET 0x12: payload = [layer, H, S, V] — fill a whole layer
    nlk_rgbmap_fill     = 0x12,
};

enum nlk_hid_display_value {
    nlk_display_hold_ms    = 0x01,
    nlk_display_active     = 0x02, // RO: pushed content currently showing?
    nlk_display_push_age   = 0x03, // RO: seconds since last push (0xFFFF = none)
    nlk_display_i2c_fails  = 0x04, // RO: OLED I2C transmit failures
    nlk_display_i2c_recov  = 0x05, // RO: successful transfers (diagnostic)
    nlk_display_i2c_scan   = 0x06, // SET: run bus scan; GET: (count<<8)|first_addr
    // SET: payload = [len(1..8), cmd bytes...] — inject raw panel commands;
    // GET: last inject result (0xFFFF never, 1 ACK, 0 fail). v2, freeze probe.
    nlk_display_raw_cmd    = 0x07,
    nlk_display_reinit     = 0x08, // SET: re-run full oled_init(). v2.
    nlk_display_widget_cnt = 0x09, // RO: NLK_WIDGET_COUNT (v3)
    nlk_display_sleep_s    = 0x0A, // idle seconds before panel off; 0 = never (v3)
    nlk_display_overlay_ms = 0x0B, // transient overlay duration; 0 = off (v5)
    // GET only, payload-addressed (v6): in [line] → out [line, invert mask,
    // 5 rendered chars, panel_on]. Mirrors the renderer's own line cache —
    // pushes, widgets and overlays all read back exactly as shown.
    nlk_display_line       = 0x0C,
    // SET 0x10: payload = [line, ASCII chars] — push a line (v5: BIG line 0-3)
    nlk_display_push       = 0x10,
    // SET 0x11: release (back to fallback screen immediately)
    nlk_display_release    = 0x11,
    // v3: fallback-screen widget per line: 0x20 + line, u16 = nlk_widget_t
    // id. v5: 4 big lines (0x20-0x23). Persisted with the channel save.
    nlk_display_widget_base = 0x20,
    // v5: custom widget text per line, payload-addressed: SET [chars ≤5],
    // GET → payload [5 chars]. 0x30 + line.
    nlk_display_custom_base = 0x30,
};

static void nlk_hid_write_u16(uint8_t *payload, uint16_t value) {
    payload[0] = value >> 8;
    payload[1] = value & 0xFF;
}

static uint16_t nlk_hid_read_u16(const uint8_t *payload) {
    return ((uint16_t)payload[0] << 8) | payload[1];
}

// GET: read straight from the modules' live runtime state (not nlk_config —
// that's only the last-saved snapshot). Returns true if handled.
static bool nlk_hid_get(uint8_t channel, uint8_t value_id, uint8_t *payload) {
    switch (channel) {
        case nlk_ch_meta:
            if (value_id == nlk_meta_protocol_version) {
                nlk_hid_write_u16(payload, NLK_HID_PROTOCOL_VERSION);
                return true;
            }
            if (value_id == nlk_meta_active_layer) {
                nlk_hid_write_u16(payload, get_highest_layer(layer_state | default_layer_state));
                return true;
            }
            return false;

        case nlk_ch_csk:
            if (value_id == nlk_csk_enabled) {
                nlk_hid_write_u16(payload, custom_shift_keys_get_enabled() ? 1 : 0);
                return true;
            }
            if (value_id == nlk_csk_slot_count) {
                nlk_hid_write_u16(payload, CUSTOM_SHIFT_KEYS_SLOTS);
                return true;
            }
            if (value_id >= nlk_csk_key_base && value_id < nlk_csk_key_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                nlk_hid_write_u16(payload, custom_shift_keys_get_slot_keycode(value_id - nlk_csk_key_base));
                return true;
            }
            if (value_id >= nlk_csk_shift_base && value_id < nlk_csk_shift_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                nlk_hid_write_u16(payload, custom_shift_keys_get_slot_shifted(value_id - nlk_csk_shift_base));
                return true;
            }
            return false;

        case nlk_ch_selword:
            if (value_id == nlk_selword_mac) {
                nlk_hid_write_u16(payload, select_word_get_mac() ? 1 : 0);
                return true;
            }
            return false;

        case nlk_ch_sentence:
            if (value_id == nlk_sentence_on) {
                nlk_hid_write_u16(payload, is_sentence_case_on() ? 1 : 0);
                return true;
            }
            return false;

        case nlk_ch_leader:
            if (value_id == nlk_leader_timeout) {
                nlk_hid_write_u16(payload, leader_timeout_ms);
                return true;
            }
            if (value_id >= nlk_leader_slot_base && value_id < nlk_leader_slot_base + NLK_LEADER_SEQ_COUNT * 8) {
                uint8_t rel = value_id - nlk_leader_slot_base;
                uint8_t seq = rel / 8;
                uint8_t pos = rel % 8;
                if (pos > NLK_LEADER_SEQ_KEYS) return false; // 0..4 keys, 5 output
                nlk_hid_write_u16(payload, leader_seqs[seq][pos]);
                return true;
            }
            return false;

        case nlk_ch_autoscroll:
            switch (value_id) {
                case nlk_as_inverted: nlk_hid_write_u16(payload, get_autoscroll_inverted() ? 1 : 0); return true;
                case nlk_as_speed_scale: nlk_hid_write_u16(payload, get_autoscroll_speed_scale()); return true;
                case nlk_as_state: nlk_hid_write_u16(payload, (uint16_t)(int16_t)autoscroll_get_level()); return true;
                case nlk_as_stop_on_key: nlk_hid_write_u16(payload, get_autoscroll_stop_on_key() ? 1 : 0); return true;
                default: return false;
            }

        case nlk_ch_os:
            switch (value_id) {
                case nlk_os_follow: nlk_hid_write_u16(payload, os_shortcuts_get_follow() ? 1 : 0); return true;
                case nlk_os_mac: nlk_hid_write_u16(payload, os_shortcuts_get_mac() ? 1 : 0); return true;
                case nlk_os_detected: nlk_hid_write_u16(payload, os_shortcuts_detected()); return true;
                default: return false;
            }

        case nlk_ch_numword:
            switch (value_id) {
                case nlk_nw_timeout: nlk_hid_write_u16(payload, num_word_get_timeout()); return true;
                case nlk_nw_layer: nlk_hid_write_u16(payload, num_word_get_layer()); return true;
                case nlk_nw_state: nlk_hid_write_u16(payload, num_word_is_active() ? 1 : 0); return true;
                default: return false;
            }

        case nlk_ch_combolayers:
            if (value_id == nlk_cl_count_id) {
                nlk_hid_write_u16(payload, VIAL_COMBO_ENTRIES);
                return true;
            }
            if (value_id >= nlk_cl_mask_base && value_id < nlk_cl_mask_base + VIAL_COMBO_ENTRIES) {
                nlk_hid_write_u16(payload, combo_layer_masks[value_id - nlk_cl_mask_base]);
                return true;
            }
            return false;

        case nlk_ch_rgbmap:
            switch (value_id) {
                case nlk_rgbmap_enabled: nlk_hid_write_u16(payload, per_layer_rgb_get_enabled() ? 1 : 0); return true;
                case nlk_rgbmap_layers: nlk_hid_write_u16(payload, NLK_RGBMAP_LAYERS); return true;
                case nlk_rgbmap_leds: nlk_hid_write_u16(payload, NLK_RGBMAP_LEDS); return true;
                case nlk_rgbmap_led:
                    // Payload in: [layer, led]; out: [layer, led, H, S, V].
                    return per_layer_rgb_get(payload[0], payload[1], &payload[2]);
                default: return false;
            }

        case nlk_ch_display:
            if (value_id == nlk_display_hold_ms) {
                nlk_hid_write_u16(payload, oled_display_get_hold_ms());
                return true;
            }
            if (value_id == nlk_display_active) {
                nlk_hid_write_u16(payload, oled_display_pushed_active() ? 1 : 0);
                return true;
            }
            if (value_id == nlk_display_push_age) {
                nlk_hid_write_u16(payload, oled_display_seconds_since_push());
                return true;
            }
            if (value_id == nlk_display_i2c_fails) {
                nlk_hid_write_u16(payload, oled_display_i2c_fails());
                return true;
            }
            if (value_id == nlk_display_i2c_recov) {
                nlk_hid_write_u16(payload, oled_display_i2c_recovers());
                return true;
            }
            if (value_id == nlk_display_i2c_scan) {
                nlk_hid_write_u16(payload, oled_display_i2c_scan_result());
                return true;
            }
            if (value_id == nlk_display_raw_cmd) {
                nlk_hid_write_u16(payload, oled_display_raw_cmd_result());
                return true;
            }
            if (value_id == nlk_display_widget_cnt) {
                nlk_hid_write_u16(payload, NLK_WIDGET_COUNT);
                return true;
            }
            if (value_id == nlk_display_line) {
                // Payload in: [line]; out: [line, mask, 5 chars, panel_on].
                if (!oled_display_rendered_line(payload[0], &payload[1])) return false;
                payload[1 + 1 + NLK_DISPLAY_BIG_COLS] = oled_display_panel_on() ? 1 : 0;
                return true;
            }
            if (value_id == nlk_display_sleep_s) {
                nlk_hid_write_u16(payload, oled_display_get_sleep_s());
                return true;
            }
            if (value_id == nlk_display_overlay_ms) {
                nlk_hid_write_u16(payload, oled_display_get_overlay_ms());
                return true;
            }
            if (value_id >= nlk_display_widget_base && value_id < nlk_display_widget_base + NLK_DISPLAY_BIG_LINES) {
                nlk_hid_write_u16(payload, oled_display_get_widget(value_id - nlk_display_widget_base));
                return true;
            }
            if (value_id >= nlk_display_custom_base && value_id < nlk_display_custom_base + NLK_DISPLAY_BIG_LINES) {
                memcpy(payload, oled_display_get_custom(value_id - nlk_display_custom_base), NLK_DISPLAY_BIG_COLS);
                return true;
            }
            return false;

        default:
            return false;
    }
}

// SET: write the modules' live runtime state; setters clamp. Values that
// arrive as u16 are clamped in wire-width space BEFORE any narrowing cast
// (hard-won Adept rule — a bare narrow cast wrapped 200 → −56 on hardware).
static bool nlk_hid_set(uint8_t channel, uint8_t value_id, uint8_t *payload) {
    switch (channel) {
        case nlk_ch_csk:
            if (value_id == nlk_csk_enabled) {
                custom_shift_keys_set_enabled(nlk_hid_read_u16(payload) != 0);
                return true;
            }
            if (value_id >= nlk_csk_key_base && value_id < nlk_csk_key_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                custom_shift_keys_set_slot_keycode(value_id - nlk_csk_key_base, nlk_hid_read_u16(payload));
                return true;
            }
            if (value_id >= nlk_csk_shift_base && value_id < nlk_csk_shift_base + CUSTOM_SHIFT_KEYS_SLOTS) {
                custom_shift_keys_set_slot_shifted(value_id - nlk_csk_shift_base, nlk_hid_read_u16(payload));
                return true;
            }
            return false;

        case nlk_ch_selword:
            if (value_id == nlk_selword_mac) {
                select_word_set_mac(nlk_hid_read_u16(payload) != 0);
                return true;
            }
            return false;

        case nlk_ch_sentence:
            if (value_id == nlk_sentence_on) {
                if (nlk_hid_read_u16(payload) != 0) {
                    sentence_case_on();
                } else {
                    sentence_case_off();
                }
                return true;
            }
            return false;

        case nlk_ch_leader:
            if (value_id == nlk_leader_timeout) {
                nlk_leader_set_timeout(nlk_hid_read_u16(payload)); // clamps
                return true;
            }
            if (value_id >= nlk_leader_slot_base && value_id < nlk_leader_slot_base + NLK_LEADER_SEQ_COUNT * 8) {
                uint8_t rel = value_id - nlk_leader_slot_base;
                uint8_t seq = rel / 8;
                uint8_t pos = rel % 8;
                if (pos > NLK_LEADER_SEQ_KEYS) return false;
                leader_seqs[seq][pos] = nlk_hid_read_u16(payload);
                return true;
            }
            return false;

        case nlk_ch_autoscroll:
            if (value_id == nlk_as_inverted) {
                set_autoscroll_inverted(nlk_hid_read_u16(payload) != 0);
                return true;
            }
            if (value_id == nlk_as_speed_scale) {
                set_autoscroll_speed_scale(nlk_hid_read_u16(payload)); // setter clamps
                return true;
            }
            if (value_id == nlk_as_state) {
                autoscroll_stop(); // rescue switch, never persisted
                return true;
            }
            if (value_id == nlk_as_stop_on_key) {
                set_autoscroll_stop_on_key(nlk_hid_read_u16(payload) != 0);
                return true;
            }
            return false;

        case nlk_ch_os:
            if (value_id == nlk_os_follow) {
                os_shortcuts_set_follow(nlk_hid_read_u16(payload) != 0);
                return true;
            }
            if (value_id == nlk_os_mac) {
                bool mac = nlk_hid_read_u16(payload) != 0;
                os_shortcuts_set_mac(mac);
                select_word_set_mac(mac); // keep the two in lockstep, Adept semantics
                return true;
            }
            return false;

        case nlk_ch_numword:
            if (value_id == nlk_nw_timeout) {
                uint16_t ms = nlk_hid_read_u16(payload);
                if (ms > NUM_WORD_IDLE_TIMEOUT_MAX) ms = NUM_WORD_IDLE_TIMEOUT_MAX;
                num_word_set_timeout(ms);
                return true;
            }
            if (value_id == nlk_nw_layer) {
                uint16_t layer = nlk_hid_read_u16(payload);
                if (layer > 7) layer = 7;
                num_word_set_layer((uint8_t)layer);
                return true;
            }
            if (value_id == nlk_nw_state) {
                num_word_off();
                return true;
            }
            return false;

        case nlk_ch_combolayers:
            if (value_id >= nlk_cl_mask_base && value_id < nlk_cl_mask_base + VIAL_COMBO_ENTRIES) {
                combo_layer_masks[value_id - nlk_cl_mask_base] = nlk_hid_read_u16(payload);
                return true;
            }
            return false;

        case nlk_ch_rgbmap:
            switch (value_id) {
                case nlk_rgbmap_enabled:
                    per_layer_rgb_set_enabled(nlk_hid_read_u16(payload) != 0);
                    return true;
                case nlk_rgbmap_led:
                    // [layer, led, H, S, V]
                    per_layer_rgb_set(payload[0], payload[1], payload[2], payload[3], payload[4]);
                    return true;
                case nlk_rgbmap_bulk: {
                    // [layer, start_led, count(<=8), count*(H,S,V)]
                    uint8_t layer = payload[0];
                    uint8_t start = payload[1];
                    uint8_t count = payload[2];
                    if (count > 8) count = 8;
                    for (uint8_t i = 0; i < count; i++) {
                        const uint8_t *hsv = &payload[3 + i * 3];
                        per_layer_rgb_set(layer, start + i, hsv[0], hsv[1], hsv[2]);
                    }
                    return true;
                }
                case nlk_rgbmap_fill:
                    // [layer, H, S, V]
                    per_layer_rgb_fill_layer(payload[0], payload[1], payload[2], payload[3]);
                    return true;
                default:
                    return false;
            }

        case nlk_ch_display:
            if (value_id == nlk_display_hold_ms) {
                oled_display_set_hold_ms(nlk_hid_read_u16(payload)); // setter clamps
                return true;
            }
            if (value_id == nlk_display_push) {
                // [line, chars...] — length capped by the module.
                oled_display_push_line(payload[0], &payload[1], NLK_DISPLAY_COLS);
                return true;
            }
            if (value_id == nlk_display_release) {
                oled_display_release();
                return true;
            }
            if (value_id == nlk_display_i2c_scan) {
                oled_display_i2c_scan();
                return true;
            }
            if (value_id == nlk_display_raw_cmd) {
                // [len(1..8), cmd bytes...] — module validates len.
                oled_display_queue_raw_cmd(&payload[1], payload[0]);
                return true;
            }
            if (value_id == nlk_display_reinit) {
                oled_display_request_reinit();
                return true;
            }
            if (value_id == nlk_display_sleep_s) {
                oled_display_set_sleep_s(nlk_hid_read_u16(payload)); // setter clamps
                return true;
            }
            if (value_id == nlk_display_overlay_ms) {
                oled_display_set_overlay_ms(nlk_hid_read_u16(payload)); // setter clamps
                return true;
            }
            if (value_id >= nlk_display_widget_base && value_id < nlk_display_widget_base + NLK_DISPLAY_BIG_LINES) {
                // Clamp in u16 wire space; the setter clamps the id range.
                uint16_t widget = nlk_hid_read_u16(payload);
                if (widget > 0xFF) widget = 0xFF;
                oled_display_set_widget(value_id - nlk_display_widget_base, (uint8_t)widget);
                return true;
            }
            if (value_id >= nlk_display_custom_base && value_id < nlk_display_custom_base + NLK_DISPLAY_BIG_LINES) {
                // Payload = raw chars (≤5); module space-pads + sanitizes.
                oled_display_set_custom(value_id - nlk_display_custom_base, payload, NLK_DISPLAY_BIG_COLS);
                return true;
            }
            return false;

        default:
            return false;
    }
}

// SAVE: snapshot live state into nlk_config for the given channel, then
// persist the whole datablock.
static bool nlk_hid_save(uint8_t channel) {
    switch (channel) {
        case nlk_ch_csk:
            nlk_config.csk_enabled = custom_shift_keys_get_enabled() ? 1 : 0;
            memcpy(nlk_config.csk_table, custom_shift_keys_table(), sizeof(nlk_config.csk_table));
            break;
        case nlk_ch_selword:
            nlk_config.selword_mac = select_word_get_mac() ? 1 : 0;
            break;
        case nlk_ch_sentence:
            nlk_config.sentence_case_on = is_sentence_case_on() ? 1 : 0;
            break;
        case nlk_ch_leader:
            memcpy(nlk_config.leader_seqs, leader_seqs, sizeof(nlk_config.leader_seqs));
            nlk_config.leader_timeout_ms = leader_timeout_ms;
            break;
        case nlk_ch_autoscroll:
            nlk_config.as_inverted         = get_autoscroll_inverted() ? 1 : 0;
            nlk_config.as_speed_scale_x100 = get_autoscroll_speed_scale();
            nlk_config.as_stop_on_key      = get_autoscroll_stop_on_key() ? 1 : 0;
            break;
        case nlk_ch_os:
            nlk_config.os_follow = os_shortcuts_get_follow() ? 1 : 0;
            nlk_config.os_mac    = os_shortcuts_get_mac() ? 1 : 0;
            break;
        case nlk_ch_numword:
            nlk_config.nw_timeout = num_word_get_timeout();
            nlk_config.nw_layer   = num_word_get_layer();
            break;
        case nlk_ch_combolayers:
            memcpy(nlk_config.combo_layer_masks, combo_layer_masks, sizeof(nlk_config.combo_layer_masks));
            break;
        case nlk_ch_rgbmap:
            nlk_config.rgbmap_enabled = per_layer_rgb_get_enabled() ? 1 : 0;
            memcpy(nlk_config.rgbmap, per_layer_rgb_table(), sizeof(nlk_config.rgbmap));
            break;
        case nlk_ch_display:
            nlk_config.disp_hold_ms    = oled_display_get_hold_ms();
            nlk_config.disp_sleep_s    = oled_display_get_sleep_s();
            nlk_config.disp_overlay_ms = oled_display_get_overlay_ms();
            memcpy(nlk_config.disp_widgets, oled_display_widget_table(), sizeof(nlk_config.disp_widgets));
            for (uint8_t i = 0; i < NLK_DISPLAY_BIG_LINES; i++) {
                memcpy(nlk_config.disp_custom[i], oled_display_get_custom(i), NLK_DISPLAY_BIG_COLS);
            }
            break;
        default:
            return false;
    }
    nlk_config_write();
    return true;
}

// Weak default at quantum/via.c:188. via.c echoes the buffer after this
// returns — no raw_hid_send() here.
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command  = &data[0];
    uint8_t  channel  = data[1];
    uint8_t  value_id = data[2];
    uint8_t *payload  = &data[3];

    // VialRGB frame (via.c already ran vialrgb_*_value on this buffer and
    // wrote the response) — leave it untouched.
    if (channel >= 0x40 && channel <= 0x44) {
        return;
    }

    bool handled = false;
    switch (*command) {
        case id_custom_get_value:
            handled = nlk_hid_get(channel, value_id, payload);
            break;
        case id_custom_set_value:
            handled = nlk_hid_set(channel, value_id, payload);
            break;
        case id_custom_save:
            handled = nlk_hid_save(channel);
            break;
    }
    if (!handled) {
        *command = id_unhandled;
    }
}

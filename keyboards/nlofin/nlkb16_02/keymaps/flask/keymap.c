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
    /* Baked by Flask (2026-07-06) from the live keymap of NLKB16-02.
       Raw matrix form, row-major [row][col] hex keycodes. Re-bake from
       the Build tab instead of hand-editing. */
    /* ===== 0: Layer 0 ===== */
    [0] = {
        { 0x001E, 0x001F, 0x0020, 0x0021, 0x00AE },
        { 0x0022, 0x0023, 0x0024, 0x0025, 0x5201 },
        { 0x0026, 0x0027, 0x0052, 0x0028, 0x00A8 },
        { 0x5227, 0x0050, 0x0051, 0x004F, 0x0000 },
    },
    /* ===== 1: Layer 1 ===== */
    [1] = {
        { 0x782A, 0x7829, 0x0001, 0x0001, 0x0001 },
        { 0x7826, 0x7825, 0x0001, 0x0001, 0x5202 },
        { 0x7822, 0x7821, 0x7823, 0x0001, 0x0001 },
        { 0x7820, 0x7828, 0x7824, 0x7827, 0x0000 },
    },
    /* ===== 2: Layer 2 ===== */
    [2] = {
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x5203 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
    /* ===== 3: Layer 3 ===== */
    [3] = {
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x5204 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
    /* ===== 4: Layer 4 ===== */
    [4] = {
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x5205 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
    /* ===== 5: Layer 5 ===== */
    [5] = {
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x5206 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
    /* ===== 6: Layer 6 ===== */
    [6] = {
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x5207 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
    /* ===== 7: Layer 7 ===== */
    [7] = {
        { 0x0001, 0x0001, 0x0001, 0x7C00, 0x0001 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x5200 },
        { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 },
        { 0x7E10, 0x0001, 0x0001, 0x0001, 0x0000 },
    },
};
/* FLASK-BAKE-END */

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT), ENCODER_CCW_CW(KC_PGDN, KC_PGUP), ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [1] = { ENCODER_CCW_CW(RGB_VAD, RGB_VAI), ENCODER_CCW_CW(RGB_SAD, RGB_SAI), ENCODER_CCW_CW(RGB_HUD, RGB_HUI) },
    [2] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [3] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [4] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
    [5] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS), ENCODER_CCW_CW(KC_TRNS, KC_TRNS) },
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
    // v2: per-line fallback-screen widget assignment (visible lines 0-7)
    // + idle sleep (seconds of no input before the panel switches off).
    uint8_t  disp_widgets[NLK_DISPLAY_VISIBLE_LINES];
    uint16_t disp_sleep_s;
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
    };
    // csk_table, leader_seqs and rgbmap zero-init = empty slots / all-black
    // map; combo masks default to "all layers allowed".
    for (uint8_t i = 0; i < VIAL_COMBO_ENTRIES; i++) {
        nlk_config.combo_layer_masks[i] = 0xFFFF;
    }
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
    // Through the setter (not memcpy): it clamps ids a newer config wrote.
    for (uint8_t i = 0; i < NLK_DISPLAY_VISIBLE_LINES; i++) {
        oled_display_set_widget(i, nlk_config.disp_widgets[i]);
    }
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
    }
    return true;
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

/* Raw-HID path debug blinks (2026-07-06 display saga): key (0,0) magenta =
 * a VIA custom frame reached raw_hid_receive_kb; key (0,3) cyan = a response
 * left via via_raw_hid_send. Visible proof of RX/TX liveness without a
 * working host readback. Harmless to leave in — only lights during traffic. */
static volatile uint32_t nlk_dbg_rx_time = 0;
static volatile uint32_t nlk_dbg_tx_time = 0;

#include "raw_hid.h"
// Weak default at quantum/via.c:68 (fork has a src-tagged send hook).
void via_raw_hid_send(uint8_t src, uint8_t *data, uint8_t length) {
    nlk_dbg_tx_time = timer_read32();
    raw_hid_send(data, length);
}

bool rgb_matrix_indicators_user(void) {
    per_layer_rgb_render();
    if (timer_elapsed32(nlk_dbg_rx_time) < 400) {
        rgb_matrix_set_color(0, 255, 0, 255); // magenta: frame received
    }
    if (timer_elapsed32(nlk_dbg_tx_time) < 400) {
        rgb_matrix_set_color(3, 0, 255, 255); // cyan: response sent
    }
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
#define NLK_HID_PROTOCOL_VERSION 3

enum nlk_hid_channel {
    nlk_ch_meta        = 0x00, // 0x01 protocol version RO; 0x02 active layer RO
    nlk_ch_csk         = 0x16, // custom shift keys (Adept channel parity)
    nlk_ch_selword     = 0x17, // select word
    nlk_ch_sentence    = 0x18, // sentence case
    nlk_ch_leader      = 0x19, // leader sequences
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
    // Slots: 0x10 + seq*8 + pos; pos 0..4 = keys, 5 = output keycode.
    nlk_leader_slot_base = 0x10,
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
    // SET 0x10: payload = [line, ASCII chars] — push a line
    nlk_display_push       = 0x10,
    // SET 0x11: release (back to fallback screen immediately)
    nlk_display_release    = 0x11,
    // v3: fallback-screen widget per visible line (0-7): 0x20 + line,
    // u16 = nlk_widget_t id. Persisted with the display channel save.
    nlk_display_widget_base = 0x20,
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
            if (value_id >= nlk_leader_slot_base && value_id < nlk_leader_slot_base + NLK_LEADER_SEQ_COUNT * 8) {
                uint8_t rel = value_id - nlk_leader_slot_base;
                uint8_t seq = rel / 8;
                uint8_t pos = rel % 8;
                if (pos > NLK_LEADER_SEQ_KEYS) return false; // 0..4 keys, 5 output
                nlk_hid_write_u16(payload, leader_seqs[seq][pos]);
                return true;
            }
            return false;

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
            if (value_id == nlk_display_sleep_s) {
                nlk_hid_write_u16(payload, oled_display_get_sleep_s());
                return true;
            }
            if (value_id >= nlk_display_widget_base && value_id < nlk_display_widget_base + NLK_DISPLAY_VISIBLE_LINES) {
                nlk_hid_write_u16(payload, oled_display_get_widget(value_id - nlk_display_widget_base));
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
            if (value_id >= nlk_leader_slot_base && value_id < nlk_leader_slot_base + NLK_LEADER_SEQ_COUNT * 8) {
                uint8_t rel = value_id - nlk_leader_slot_base;
                uint8_t seq = rel / 8;
                uint8_t pos = rel % 8;
                if (pos > NLK_LEADER_SEQ_KEYS) return false;
                leader_seqs[seq][pos] = nlk_hid_read_u16(payload);
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
            if (value_id >= nlk_display_widget_base && value_id < nlk_display_widget_base + NLK_DISPLAY_VISIBLE_LINES) {
                // Clamp in u16 wire space; the setter clamps the id range.
                uint16_t widget = nlk_hid_read_u16(payload);
                if (widget > 0xFF) widget = 0xFF;
                oled_display_set_widget(value_id - nlk_display_widget_base, (uint8_t)widget);
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
            nlk_config.disp_hold_ms = oled_display_get_hold_ms();
            nlk_config.disp_sleep_s = oled_display_get_sleep_s();
            memcpy(nlk_config.disp_widgets, oled_display_widget_table(), sizeof(nlk_config.disp_widgets));
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
    nlk_dbg_rx_time = timer_read32(); // debug blink: frame reached us

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

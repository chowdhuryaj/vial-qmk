/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "per_layer_rgb.h"
#include "rgb_matrix.h"

static nlk_rgbmap_t rgbmap;
static bool         rgbmap_enabled = NLK_RGBMAP_ENABLED_DEFAULT;

void per_layer_rgb_set_enabled(bool on) {
    rgbmap_enabled = on;
}

bool per_layer_rgb_get_enabled(void) {
    return rgbmap_enabled;
}

void per_layer_rgb_set(uint8_t layer, uint8_t led, uint8_t h, uint8_t s, uint8_t v) {
    if (layer >= NLK_RGBMAP_LAYERS || led >= NLK_RGBMAP_LEDS) return;
    rgbmap[layer][led][0] = h;
    rgbmap[layer][led][1] = s;
    rgbmap[layer][led][2] = v;
}

bool per_layer_rgb_get(uint8_t layer, uint8_t led, uint8_t *hsv_out) {
    if (layer >= NLK_RGBMAP_LAYERS || led >= NLK_RGBMAP_LEDS) return false;
    hsv_out[0] = rgbmap[layer][led][0];
    hsv_out[1] = rgbmap[layer][led][1];
    hsv_out[2] = rgbmap[layer][led][2];
    return true;
}

void per_layer_rgb_fill_layer(uint8_t layer, uint8_t h, uint8_t s, uint8_t v) {
    if (layer >= NLK_RGBMAP_LAYERS) return;
    for (uint8_t i = 0; i < NLK_RGBMAP_LEDS; i++) {
        rgbmap[layer][i][0] = h;
        rgbmap[layer][i][1] = s;
        rgbmap[layer][i][2] = v;
    }
}

nlk_rgbmap_t *per_layer_rgb_table(void) {
    return &rgbmap;
}

void per_layer_rgb_render(void) {
    if (!rgbmap_enabled) return;
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (layer >= NLK_RGBMAP_LAYERS) layer = NLK_RGBMAP_LAYERS - 1;
    for (uint8_t i = 0; i < NLK_RGBMAP_LEDS && i < RGB_MATRIX_LED_COUNT; i++) {
        hsv_t hsv = {rgbmap[layer][i][0], rgbmap[layer][i][1], rgbmap[layer][i][2]};
        if (hsv.v > RGB_MATRIX_MAXIMUM_BRIGHTNESS) hsv.v = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
        rgb_t rgb = hsv_to_rgb(hsv);
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
}

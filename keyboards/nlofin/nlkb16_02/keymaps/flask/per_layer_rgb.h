/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Per-layer per-key RGB map. A persisted HSV color per (layer, LED); when
 * enabled, the active layer's colors are painted every frame from the
 * rgb_matrix indicators hook, overriding whatever VialRGB effect is running.
 * When disabled the board behaves like stock (VialRGB owns the LEDs).
 * Edited live over Flask HID channel 0x21, persisted in nlk_config.
 */

#pragma once

#include "quantum.h"

#ifndef NLK_RGBMAP_LAYERS
#    define NLK_RGBMAP_LAYERS 8
#endif
#ifndef NLK_RGBMAP_LEDS
#    define NLK_RGBMAP_LEDS 23
#endif
#ifndef NLK_RGBMAP_ENABLED_DEFAULT
#    define NLK_RGBMAP_ENABLED_DEFAULT false
#endif

// [layer][led][0]=H [1]=S [2]=V
typedef uint8_t nlk_rgbmap_t[NLK_RGBMAP_LAYERS][NLK_RGBMAP_LEDS][3];

void per_layer_rgb_set_enabled(bool on);
bool per_layer_rgb_get_enabled(void);

void per_layer_rgb_set(uint8_t layer, uint8_t led, uint8_t h, uint8_t s, uint8_t v);
// Writes 3 bytes (H,S,V) into hsv_out; false if layer/led out of range.
bool per_layer_rgb_get(uint8_t layer, uint8_t led, uint8_t *hsv_out);
void per_layer_rgb_fill_layer(uint8_t layer, uint8_t h, uint8_t s, uint8_t v);

// Direct access for EEPROM snapshot/restore.
nlk_rgbmap_t *per_layer_rgb_table(void);

// Call from rgb_matrix_indicators_user; paints when enabled.
void per_layer_rgb_render(void);

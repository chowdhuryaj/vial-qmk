/* SPDX-License-Identifier: GPL-2.0-or-later */
/* NLOFIN NLKB16-02 — DOIO KB16 rev2 clone (STM32F103, Maple/stm32duino
 * bootloader, app at 0x8002000). Board files reconstructed 2026-07-06 from
 * mainline QMK + vial-qmk doio/kb16/rev2 plus values read off the live
 * device over HID (23-LED chain, VID/PID). See keymaps/flask/ for features.
 */

#pragma once

/* Custom font carried over from kb16 (includes the glyphs the OLED code uses) */
#define OLED_FONT_H "lib/glcdfont.c"

#ifdef OLED_ENABLE
/* OLED is on I2C2 (PB10/PB11); QMK's I2C driver alias points at I2CD2 */
#    define I2C1_SCL_PIN B10
#    define I2C1_SDA_PIN B11
#    define I2C_DRIVER I2CD2
/* Conservative bus timing + short timeout: transfers were failing wholesale
 * on hardware (2026-07-06); a failing transfer must cost 10 ms, not 100,
 * or the retry path starves the main loop. */
#    define I2C1_CLOCK_SPEED 100000
#    define I2C1_DUTY_CYCLE STD_DUTY_CYCLE
#    define OLED_I2C_TIMEOUT 10
/* Panel is a plain SSD1306 128x64 (settled 2026-07-06 by decoding the stock
 * firmware dump: its init table at 0xD4EF is QMK's own oled_driver sequence
 * with multiplex 0x3F/64 rows, COM pins 0x12, horizontal addressing, charge
 * pump on — i.e. stock is QMK built with OLED_DISPLAY_128X64). Driving it as
 * the default 128x32 was the entire display saga: 32-row mux on a 64-row
 * alternative-COM panel interleaves rows into what looks like frozen hash.
 *
 * The GLASS, however, is a 64x32 window into that 128x64 RAM (center 64 SEG
 * columns 32..95 x top 32 COM rows — hardware-mapped 2026-07-06 by pushing
 * boundary-probe text): on the portrait ROTATION_90 canvas the visible
 * window is logical text lines 4..11 x columns 0..4. Everything outside
 * lands in RAM the glass never displays. Keymap-side layout constants:
 * NLK_DISPLAY_VISIBLE_* in keymaps/flask/config.h. */
#    define OLED_DISPLAY_128X64
#endif

/* WS2812 stays on the BITBANG backend — the PWM+DMA backend is IMPOSSIBLE on
 * this board (tried + boot-hung 2026-07-06): PA10 is TIM1_CH3, the ws2812_pwm
 * driver triggers DMA off the timer UPDATE event, and on the F103's fixed DMA
 * map TIM1_UP shares DMA1 channel 5 with I2C2_RX — which the OLED's I2C2
 * already holds (ChibiOS I2Cv1 allocates its DMA channels at i2cStart and
 * keeps them). Second alloc = ChibiOS halt before USB enumerates. Don't retry
 * without moving one of the peripherals. */

# NLOFIN NLKB16-02

16-key macro pad + 3 rotary encoders (2 pushable exposed, big knob push wired
at matrix (2,4)) + 128x32 OLED + 23 WS2812 LEDs (16 per-key, 7 bottom strip).

Rebadged DOIO KB16 rev2: STM32F103, Maple (stm32duino) bootloader, app at
`0x8002000`. Board files reconstructed 2026-07-06 from mainline QMK
`doio/kb16/rev2` + values dumped from the live device over raw HID
(VID `0xD020`, PID `0x1603`, 23-LED geometry, stock keymap).

* Bootloader entry: hold key (0,0) (top-left) while plugging in, or Vial
  bootloader-jump after unlock.
* Flash: `dfu-util -d 1eaf:0003 -a 2 -D <firmware>.bin`
* Stock firmware backup: `~/nlkb16-02-stock-firmware-2026-07-06.bin`
  (120 KiB app-region dump; restore with the same dfu-util command).

Make example:

    make nlofin/nlkb16_02:flask

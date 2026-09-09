# Evangelion nice!view animation

This branch adds a custom right/peripheral nice!view animation directly to this ZMK config.

Runtime sequence (22 seconds total, repeating):
- Shinji, Rei, Asuka, Misato, Kaworu, Gendo, Ritsuko, Kaji — about 2 seconds each
- NERV logo — about 1.6 seconds
- WARNING sequence — alternating/blinking for about 2.4 seconds
- EVA Unit-01 — 10 progressive jaw-opening frames over about 2 seconds

The keyboard does **not** decode a GIF. The on-device animation uses LVGL `lv_animimg` and 24 unique 140x68 1-bit image descriptors. The longer timing/holds are implemented by referencing those 24 images 110 times in `widgets/peripheral_status.c`, so the slideshow can be slow while the mouth animation remains smooth without duplicating image data.

Animation timing is configured by the `nice_view_custom` shield Kconfig (22,000 ms total), so the left build never receives a custom-only Kconfig symbol.

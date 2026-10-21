# Validation checklist

No automated or hardware test has been run as part of this refactor.

1. Open `MacroSynth.ino` in Arduino IDE, select ESP32-S3, compile.
2. Check boot messages at 115200 baud; verify I2S initialization succeeds.
3. Verify the ST7789 display uses SPI MODE2, 12 MHz, inversion enabled, rotation 3.
4. With safe listening volume, confirm PCM5102 produces clean sound in both headphone channels.
5. Test Serial keys `1`–`8`, `+`, `-`, `[`, `]`, `v`, `V`, `m`, `p`, `?`.
6. Verify encoder navigation, short press selection, long press parameter menu.
7. Check for clicks/underruns when navigating menus and repainting the display.
8. Verify no pin conflicts with the board's PSRAM.

If the IDE reports missing headers, install Adafruit GFX and Adafruit ST7789; `ESP_I2S.h` requires a compatible ESP32 Arduino core.

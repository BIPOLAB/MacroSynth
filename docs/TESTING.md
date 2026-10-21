# Validation checklist

The audio/UI separation has been committed, but it has **not yet been compiled or tested on the physical ESP32-S3**.

1. Open `MacroSynth.ino` in Arduino IDE, select the correct ESP32-S3 board/core, and compile.
2. Check boot messages at 115200 baud. Expected messages include I2S initialization and `Tarea de audio iniciada en core 0.`
3. With safe listening volume, confirm PCM5102 produces clean sound in both headphone channels.
4. Test sound while turning the encoder slowly and rapidly on the main screen, engine menu, parameter menu, and parameter edit screen.
5. Repeat while the display redraws and while issuing Serial commands.
6. Listen for dropouts, clicks, or distorted transients. If artifacts remain, record whether they occur only during parameter edits or also while browsing the engine menu.
7. Verify short/long encoder presses and Serial keys `1`–`8`, `+`, `-`, `[`, `]`, `v`, `V`, `m`, `p`, `?`.
8. Verify no pin conflicts with the board's PSRAM.

If the IDE reports missing headers, install Adafruit GFX and Adafruit ST7789; `ESP_I2S.h` requires a compatible ESP32 Arduino core. A successful compile is necessary but not a substitute for the hardware audio test.

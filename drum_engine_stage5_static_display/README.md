# Stage 5 — Static ST7789 display

Stage 4 (kick + snare + closed hi-hat) was confirmed clean on the actual hardware.

## Single new subsystem
Initialize the 240x240 ST7789 using the previously reported working pinout: SCLK 12, MOSI 11, DC 13, RST 10, BL 14, CS not connected (-1). SPI mode 2, 12 MHz, inverted display. Uses Adafruit GFX and Adafruit ST7789 Arduino libraries.

The static BIPOLAB / MACROSYNTH / 110 BPM / KICK SNARE CLOSED HAT screen is drawn **once in setup**, before I2S starts. No screen drawing in `loop()` or while audio is playing. No encoder support yet.

## Audio
The Stage 4 kick, snare, hi-hat DSP, pattern, I2S 44.1 kHz stereo 16-bit, 128-frame buffers and I2S pins 16/18/8 are unchanged.

## Hardware validation
Upload `drum_engine_stage5_static_display.ino` with Arduino IDE. Verify both (1) display renders correctly and stays static and (2) all three voices remain clean. If display appears incorrect, check rotation, library version, and established ST7789 settings before changing audio.

Not compiled or physically tested in this session. Keep Stage 4 available for regression testing.

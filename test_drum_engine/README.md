# MacroSynth — isolated drum engine test

This is a diagnostic firmware for the ESP32-S3 + PCM5102. It intentionally excludes the TFT, encoder, menus, serial UI, preset filesystem, and the FreeRTOS audio/UI split.

## What it tests
- The current MacroSynth `Synth.cpp` drum synthesis engine, copied from `main`.
- Direct continuous stereo I2S writes at 44.1 kHz / 16-bit.
- A four-on-the-floor kick pattern at 110 BPM.
- Minimal audio-path instrumentation for short I2S writes.

## Wiring
- BCLK: GPIO 16
- LRCK / WS: GPIO 18
- DOUT: GPIO 8

## Upload
Open `test_drum_engine.ino` from this folder in Arduino IDE and select the same ESP32-S3 board configuration used for the known-good I2S sketch.

## Scope / caveat
This first isolation build intentionally sequences only the kick. The current synthesis engine has shared single-voice drum state, so adding kick/snare/hat triggers before establishing a clean baseline would cause voices to interrupt one another and muddy the diagnosis.

The source has been committed to the test branch, but it has not been compiled or run on physical hardware in this session. Compare its sound with the known-good simple 808 sketch. If it is clean, the existing drum DSP is likely healthy and the issue is in the full firmware's scheduling, mixing, or state integration. If it is distorted, focus on the DSP path itself.

# Drum Engine — Stage 2: minimal voice interface

This stage is based on the user-confirmed clean Stage 1 kick-only test.

## Single intended change
The kick trigger and render functions are routed through a minimal `DrumVoice` structure containing two callbacks:
- `trigger`: starts the voice.
- `render`: returns one audio sample.

Only one voice (kick) is registered. No snare, hat, presets, display, encoder, UI tasks, or filesystem are added.

## Kept unchanged
- Kick DSP formulas and state.
- Pattern, BPM, sample rate, output format and buffer size.
- I2S pin mapping and `I2S.write` block handling.

## Test
Open `drum_engine_stage2_voice_interface.ino` in Arduino IDE and upload to the ESP32-S3. Compare its sound to Stage 1. Do not proceed until the user confirms it remains clean.

This sketch has been committed but has not been compiled or physically tested in this session.

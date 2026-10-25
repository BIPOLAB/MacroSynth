# Stage 4 — Kick + Snare + Closed Hi-hat

Stage 3 was confirmed clean on the user's ESP32-S3 + PCM5102 hardware.

This test adds only the closed hi-hat DSP and eighth-note pattern from the known-good Stage 0 sketch, routed through the existing `DrumVoice` trigger/render interface. Kick, snare, I2S pins (16/18/8), 44.1 kHz, stereo 16-bit, 128-frame blocks, and I2S write strategy are unchanged.

Open `drum_engine_stage4_full_kit.ino` in Arduino IDE and upload it. Verify all three voices sound clean, with no glitches or dropouts. Display, encoder, presets and UI remain intentionally disabled. Do not proceed to display integration before this hardware test passes.

Committed to GitHub; compilation and hardware testing were not performed in this session.

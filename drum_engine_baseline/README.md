# Drum Engine — Stage 0 baseline

This folder is the starting point for rebuilding the drum engine incrementally.

## Stage 0: known-good audio path
- Exact DSP and I2S strategy from the standalone `test_808_sequencer` sketch that was previously reported to sound clean.
- No MacroSynth `Synth.cpp`, preset structs, display, encoder, UI task, or filesystem.
- ESP32-S3 + PCM5102, 44.1 kHz, 16-bit stereo, I2S BCLK/LRCK/DOUT on GPIO 16/18/8.

## Test
Open `drum_engine_baseline.ino` in Arduino IDE and upload it to the ESP32-S3.
First confirm this baseline sounds clean on the same hardware. Do not move to Stage 1 until confirmed.

## Incremental plan
1. Stage 0: known-good baseline (this sketch).
2. Stage 1: isolate kick only, without changing I2S/block handling.
3. Stage 2: add a small drum voice interface.
4. Stage 3: add snare and hat one at a time.
5. Stage 4: add preset/configuration structures.
6. Stage 5: only then consider integrating the engine into MacroSynth.

Every stage should be a separate commit and hardware-tested before proceeding.

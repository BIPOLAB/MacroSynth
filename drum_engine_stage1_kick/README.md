# Drum Engine — Stage 1: kick only

This sketch is a controlled change from the Stage 0 baseline, which the user confirmed sounds clean.

## What changed
- Only the kick is triggered and rendered.
- Snare and hi-hat generation have been removed from this test.

## What did not change
- ESP32-S3 + PCM5102 pin mapping: BCLK 16, LRCK 18, DOUT 8.
- I2S: 44.1 kHz, 16-bit stereo.
- Audio block size: 128 frames.
- Buffer layout, sample conversion, block write strategy, BPM, and kick synthesis remain the same as Stage 0.

## Test
Open `drum_engine_stage1_kick.ino` in Arduino IDE and upload it to the ESP32-S3. Confirm that the kick sounds clean and stable. Do not proceed to the next stage until this is confirmed on the hardware.

This sketch has been committed but has not been compiled or physically tested in this session.

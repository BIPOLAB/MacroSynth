# Stage 7 — Encoder-controlled BPM

Stage 6 was confirmed clean with the beat indicator updating during playback.

## Change
Add polling of the rotary encoder on GPIO35 (A) and GPIO36 (B). Each detected quadrature transition adjusts tempo by 1 BPM, bounded from 60 to 180 BPM. The BPM number is refreshed by clearing/redrawing only its small screen region. Encoder pushbutton GPIO37 is configured but not used yet. Polling happens once per audio block; no interrupt handlers, extra tasks, or dynamic allocation are added.

## Unchanged
Stage 6 drum DSP, pattern and beat indicator; I2S GPIO16/18/8, 44.1 kHz, stereo 16-bit, 128-frame buffer; ST7789 setup and SPI mode/frequency.

## Hardware test
Upload `drum_engine_stage7_encoder_bpm.ino`. Check that rotating the encoder changes the BPM display and tempo, and listen for audio artifacts or dropouts during rotation. If direction is reversed, report it; do not alter DSP. Button behavior is intentionally not implemented in this stage.

Committed, but not compiled or physically tested in this session.

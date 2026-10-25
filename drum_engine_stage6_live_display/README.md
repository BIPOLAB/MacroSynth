# Stage 6 — Live ST7789 beat indicator

Stage 5 was confirmed to have clean audio and a working static display on hardware.

## Change
Add four small beat boxes at the bottom of the existing 240x240 screen. After each 128-frame audio block is generated and written to I2S, update only the active beat box when the beat changes (approximately every 545 ms at 110 BPM). At most two small fill rectangles are drawn on each beat transition. This introduces SPI transactions during audio playback for the first time.

## Unchanged
All Stage 5 DSP, drum patterns, I2S pin mapping (16/18/8), sample rate (44100), stereo 16-bit format, buffer size (128 frames), ST7789 pins and SPI settings are unchanged. No encoder or background tasks.

## Hardware test
Upload `drum_engine_stage6_live_display.ino`. Confirm the indicator advances every beat, and listen closely for clicks, gaps or other audio artifacts *when the indicator changes*. If glitches appear, return to the confirmed Stage 5 and report timing. This is a deliberately limited SPI/audio interaction test, not the final UI architecture.

Not compiled or physically tested in this session.

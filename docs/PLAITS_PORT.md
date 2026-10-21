# Plaits DSP port to ESP32-S3

## Goal

Integrate the original Mutable Instruments Plaits synthesis DSP into MacroSynth while preserving the existing ESP32-S3 hardware layer:

- ESP32-S3, Arduino IDE / Arduino-ESP32.
- PCM5102 DAC over I2S (BCLK GPIO16, LRCK GPIO18, DOUT GPIO8).
- ST7789 240x240 display (DC GPIO13, RST GPIO10, MOSI GPIO11, SCLK GPIO12, backlight GPIO14).
- One rotary encoder with push button (CLK GPIO35, DT GPIO36, SW GPIO37).
- Serial control remains available during bring-up.

## Source and licensing

The DSP source is based on the official `pichenettes/eurorack` repository, directory `plaits/dsp`, authored by Émilie Gillet. The original firmware code is MIT-licensed. Keep each upstream copyright/license header intact and add the upstream MIT license notice to this repository before distributing imported code. The derivative product must not be named "Mutable Instruments Plaits"; use a BipoLab name and credit the original author.

Upstream:
- https://github.com/pichenettes/eurorack/tree/master/plaits
- https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/open_source/

## Porting strategy

This is a DSP port, not a port of the original STM32 firmware or Eurorack hardware. Do not import the original STM32 drivers, ADC/CV input handling, LEDs, bootloader, calibration, or original UI as runtime dependencies.

1. Vendor the required `plaits/dsp` and `stmlib` DSP/utility source with license headers and a recorded upstream revision.
2. Build an Arduino-compatible adapter around `plaits::Voice`; translate MacroSynth's UI values to Plaits controls and triggers.
3. Keep the I2S driver and PCM5102 output owned by the existing audio task. Render audio in fixed blocks and avoid allocation, logging, display work, or locks that can block inside the per-sample path.
4. The encoder and display task only update pending parameters. Transfer a coherent parameter snapshot to the audio task at block boundaries; keep Plaits voice/DSP state exclusively owned by the audio task.
5. Begin at 48 kHz because the original Plaits DSP is designed around that rate. Confirm whether the ESP32-S3 Arduino I2S driver and PCM5102 path can use 48 kHz cleanly, then compare against 44.1 kHz only if necessary.
6. Use the display/encoder for engine, harmonics, timbre, morph, note/frequency, trigger/gate, and output level. Retain Serial diagnostics while bringing up the port.
7. Add engines incrementally and measure CPU time and audio continuity on hardware before enabling all models.

## Bring-up milestones

- [ ] M0: record upstream revision and include MIT license / attribution.
- [ ] M1: compile the minimal Plaits DSP dependency set for ESP32-S3 without the original STM32 drivers.
- [ ] M2: render one engine through the existing I2S audio task.
- [ ] M3: add the encoder and display parameter adapter; ensure display redraws never run in the audio task.
- [ ] M4: exercise all engines and monitor block render time, heap/stack use, and dropouts.
- [ ] M5: add presets/persistence only after audio and control concurrency are stable.

## Validation status

This document describes the intended integration and is not a claim that the port currently compiles or runs. Each milestone must be validated separately. The existing MacroSynth firmware's audio task and display are not yet evidence that Plaits DSP has been ported.

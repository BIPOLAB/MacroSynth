# Firmware architecture

```text
MacroSynth.ino
  ├── UI loop (core 1): Controls → Menu → State
  │                    SerialControl ──→ State
  │                    Display ────────→ State
  └── Audio task (core 0): Audio → Synth ──→ State
```

All modules compile as normal Arduino sketch-local C++ files; no additional build system is required. `Config.h` centralizes board constants, and `State.cpp` owns the single definitions of shared globals declared `extern` in `State.h`.

## Runtime and audio scheduling

The audio path now runs in a dedicated FreeRTOS task pinned to core 0 at priority 3. The Arduino `loop()` remains responsible for polling controls, processing serial commands and drawing the ST7789 display. This prevents synchronous TFT rendering from directly stopping audio-block generation on the same task.

Each audio block contains 128 stereo frames at 44.1 kHz (about 2.90 ms per block). `I2S.write()` is still synchronous inside the audio task and may block while the driver accepts data; it no longer shares the UI task's execution path. If task creation fails, audio is disabled and an error is printed rather than falling back to the glitch-prone single-loop design.

## Concurrency caveat

The UI and audio task still share legacy DSP/control globals, so encoder-driven changes to those fields can still produce artifacts and require a separate parameter snapshot/queue refactor. Sound presets are safer: UI edits publish a complete preset through a short FreeRTOS critical section, and the audio task consumes the pending snapshot at the start of an audio block. The active preset being edited is never read directly by the sample loop. This snapshot reduces preset-field tearing but does not make all legacy globals race-free. Verify encoder parameter changes and engine switching on hardware; if artifacts remain, investigate the remaining shared controls and I2S buffer/driver configuration.

## BIPO DRUM DSP

BIPO DRUM is a BipoLab-original layered percussive voice: three phase accumulators feed a selectable FM body, with per-hit pitch sweep, body hold/decay, independent filtered noise and transient layers, and final one-shot amplitude contour. Model presets provide starting points for kick, snare, closed/open hat, clap, tom, rim and metal. The UI exposes these values in the existing parameter browser. This is a first synthesis pass, not yet a fully profiled/validated production engine. See [BIPO DRUM research and design](BIPO_DRUM_ENGINE.md).

## Engines

Analog, FM, Wavetable, Pluck, Modal, Noise, Chord and Texture are prototype algorithms preserved from the original monolithic sketch. Their oscillator phases, envelope, filter and noise PRNG are currently shared, and switching engines does not fully reset that state.

## Next architectural steps

- Compile and validate on ESP32-S3 with actual display, encoder and PCM5102.
- Measure audio stability during full-screen redraws and rapid encoder movement.
- Encapsulate per-engine DSP state behind an interface and transfer UI parameter changes through a queue or snapshot.
- Profile BIPO DRUM worst-case DSP cost and reduce per-sample transcendental math if needed.
- Add multiple simultaneous drum voices and a dedicated 16-step sequencer only after single-voice sound design is stable.
- Introduce engine-specific parameters, preset serialization and USB MIDI as separate milestones.
- Audit GPIO35–37 against the exact N16R8 board pinout.

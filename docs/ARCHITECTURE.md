# Firmware architecture

```text
MacroSynth.ino
  ├── Controls → Menu → State
  ├── SerialControl ──→ State
  ├── Display ────────→ State
  └── Audio → Synth ──→ State
```

All modules compile as normal Arduino sketch-local C++ files; no additional build system is required. `Config.h` centralizes board constants, and `State.cpp` owns the single definitions of shared globals declared `extern` in `State.h`.

## Runtime
Each iteration generates and writes a 128-frame stereo audio block, polls the encoder/button, consumes available serial input, and redraws the screen only when `displayDirty` is true. The audio write is synchronous in the original implementation. TFT drawing can interrupt block production; this is an **existing real-time limitation**, not a solved scheduling architecture.

## Engines
Analog, FM, Wavetable, Pluck, Modal, Noise, Chord and Texture are prototype algorithms preserved from the original monolithic sketch. Their state (oscillator phases, envelope, filter and noise PRNG) is currently shared, and switching engines does not fully reset that state.

## Next architectural steps
- Compile and validate on ESP32-S3 with actual display, encoder and PCM5102.
- Introduce a bounded audio producer/consumer design before adding expensive UI or MIDI work.
- Encapsulate per-engine DSP state behind an interface; avoid shared mutable globals.
- Introduce engine-specific parameters, preset serialization and USB MIDI as separate milestones.
- Audit GPIO35–37 against the exact N16R8 board pinout.

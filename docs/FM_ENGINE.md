# FM Engine — MacroSynth MK1

## Status

Initial modular, monophonic FM voice. It has no dynamic allocation and does not depend on Arduino, I2S, display, or UI code. This module is intentionally separate from the existing `Synth.cpp` until the audio path is measured and the voice can be integrated safely.

## Included

- Switchable 2-operator and 4-operator modes.
- Two 2-op routings and five 4-op routings.
- Per-operator frequency ratio, level, and ADSR envelope.
- Note-on, note-off, velocity, and bounded feedback.
- Operator frequencies capped at 45% of sample rate to limit extreme aliasing.
- Persistent phase/envelope state and no heap use in the sample loop.

## API

- `begin(sampleRate)`
- `setOperatorCount(2|4)`
- `setAlgorithm(index)`
- `setOperator(index, parameters)`
- `noteOn(frequencyHz, velocity)`
- `noteOff()`
- `renderSample()`

## Known limitations

- Mono voice; no polyphony/voice allocator yet.
- Modulation depths in the topology are initial constants, not yet per-edge parameters.
- Uses `sinf` as the reference implementation. Measure CPU use on ESP32-S3 before optimizing.
- The 4-op routings are an initial set for architecture validation, not a DX-compatible algorithm set.
- This module has not yet been integrated into the audio output path or validated on physical hardware.

## Validation plan

1. Compile the module with the Arduino ESP32 toolchain.
2. Add a controlled note trigger and verify all 2-op/4-op algorithms.
3. Measure worst-case render time on ESP32-S3.
4. Only then connect it to the main mixer/audio task, with parameter updates transferred safely between UI and audio contexts.

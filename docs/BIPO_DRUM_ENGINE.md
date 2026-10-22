# BipoLab Drum Engine — research and design brief

## Reference study: Elektron Digitone II

The reference is specifically the **Digitone II**, not the original Digitone. The original Digitone is a four-operator FM synthesizer with a subtractive-style signal path; it is not a dedicated drum machine. Digitone II adds a separate **FM DRUM** synthesis machine designed for percussive sounds.

Sources consulted:
- Elektron, *Digitone II User Manual*, OS 1.10B, Appendix A.2.2 “FM Drum”: https://www.elektron.se/wp-content/uploads/2025/06/Digitone-2-User-Manual_ENG_OS1.10B_250609.pdf
- Elektron support, “Digitone FM synthesis overview”: https://support.elektron.se/support/solutions/articles/43000566557-digitone-fm-synthesis-overview
- Sound On Sound, “Elektron Digitone II”, March 2025: https://www.soundonsound.com/reviews/elektron-digitone-ii

This document records functional ideas and a BipoLab implementation plan; it does not copy Elektron's firmware, code, exact parameter maps, graphics, or proprietary algorithms.

## What makes the Digitone II FM Drum useful

FM Drum is a hybrid percussive synthesizer, not merely a pitch oscillator with an ADSR. Its voice is divided into cooperating layers:

1. **Body:** a three-operator FM sound source, with selectable operator waveforms and algorithms.
2. **Pitch movement:** tune plus a pitch sweep with independent time and depth. This is critical for kicks, toms, laser-like percussion, and metallic hits.
3. **FM timbre:** operator ratios, modulation envelopes, feedback and wavefolding. These make the body evolve rather than sound like a static sine.
4. **Body envelope:** a hold stage followed by decay; the body can ring or stop abruptly without forcing the noise tail to behave the same way.
5. **Noise/transient layer:** independently controlled noise envelope and level, plus a selectable transient component. This gives snare wires, hat attacks, claps, clicks and impact.
6. **Noise shaping:** a separate filter region (base and width) lets the transient/noise layer sit bright, dark, thin or broad without retuning the body.
7. **Performance workflow:** parameters are grouped into compact pages, every sound can be auditioned while editing, and a preset can be saved independently. Digitone II also separates reusable presets from kits/pattern state.

The crucial lesson is the **independent body/noise/transient architecture**. A kick, snare, clap and hat should not be four unrelated fixed samples or four labels around the same oscillator. They should be starting points over a shared, deeply editable synthesis engine.

## BipoLab's interpretation: BIPO DRUM

BIPO DRUM will borrow these *high-level design principles* while using original DSP, parameter names, ranges, UI, and defaults.

### Sound path

```
FM BODY (operators + pitch sweep + wavefolder) ─┐
                                                ├─> tone shaping -> amp -> stereo mix
NOISE / TRANSIENT (own envelope + filter) ──────┘
```

Initial implementation target is one drum voice at a time on the ESP32-S3 MK0. This keeps the first milestone realistic at 44.1 kHz with 128-frame I2S blocks. Later, voices can be pooled and mixed for the 16-step sequencer.

### Planned drum families
- Kick: pitched body, fast downward sweep, optional click transient.
- Snare: tonal shell + noise burst + crack transient.
- Closed hat: high-passed metallic/noise burst, short decay.
- Open hat: same broad family with longer noise decay.
- Clap: several short noise bursts plus a diffuse tail.
- Tom: tuned resonant body and controllable sweep.
- Rim / clave: short, inharmonic resonant attack.
- Metallic percussion: inharmonic FM ratios, feedback and longer decay.

These are presets/models within the engine, not separate hard-coded instruments. Every family should remain editable beyond its starting point.

### Parameter-page layout

- **BODY:** tune, sweep time/depth, algorithm, operator ratios, modulation, feedback, fold, body level.
- **BODY ENV:** hold and decay.
- **NOISE:** noise level, filter base/width, noise hold/decay, reset and optional ring modulation.
- **TRANSIENT:** transient type and level.
- **AMP:** final level and one-shot envelope.
- **PRESET:** audition, save, Save As, load and rename.

On the current MK0, the single encoder navigates the pages and edits one selected value. The future 16 physical buttons should be used for direct triggering/sequencing, not made prerequisites for sound design. The TFT is the primary interface; Serial remains diagnostic.

### Presets and kits

- A **sound preset** stores the complete parameter state for one synthesized drum sound.
- A **kit** will store independent copies of its assigned sound parameters plus track level/mute/solo and future send amounts. Editing a kit must not silently alter the source preset.
- A **pattern** will store triggers and per-step overrides. This is where Elektron-style parameter locks become a useful later milestone: a step can change pitch, decay, noise, or transient for only that hit.
- Saving, loading, file I/O and screen rendering stay outside the audio render loop.

### Engineering constraints

- Original BipoLab C++ DSP; no direct port of Digitone code or copied proprietary algorithms.
- Arduino IDE / ESP32 Arduino core; no Python runtime.
- 44.1 kHz stereo I2S to PCM5102, 128-frame blocks, ESP32-S3 N16R8.
- No heap allocation, LittleFS access, display drawing, serial printing, or blocking UI work inside per-sample DSP.
- UI-to-audio state must be transferred as a complete snapshot or queue event. The audio task must not read mutable UI globals directly.
- Avoid expensive exponentials/powers per sample where possible; use envelope coefficients, phase increments, and precomputed per-hit values.
- Any schema change must preserve or explicitly migrate existing BPS presets; never silently reinterpret old binary payloads.
- Compile and listen-test on the physical board before claiming the engine is complete. The existing prototype has a known risk of artifacts while turning the encoder because legacy globals remain shared.

## Implementation sequence

1. Define and validate the BIPO DRUM parameter model, including a versioned preset migration.
2. Implement the layered body/noise/transient DSP with bounded parameters and deterministic triggering.
3. Add parameter pages and a drum-family selector to the ST7789/encoder UI.
4. Add factory starting points for kick, snare, closed hat, open hat, clap, tom, rim and metallic percussion.
5. Add A/B audition, reset-page, randomize-page and safe Save As.
6. Stabilize UI/audio handoff and confirm no encoder artifacts.
7. Only after this is stable, add kits, 16-step sequencing, per-step parameter locks and pattern persistence.

## Important scope distinction

The Digitone II is a polyphonic FM synthesizer with a dedicated FM Drum machine and advanced sequencing; it is not identical to Digitakt's sample-oriented workflow. MacroSynth's first goal is to reproduce the **sound-design depth and immediate editability of FM Drum**, adapted to our smaller screen and single encoder. A complete Elektron-style sequencer is a separate milestone.

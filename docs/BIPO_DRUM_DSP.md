# BIPO DRUM — DSP engineering notes

## Objective

BIPO DRUM is an original BipoLab synthesis engine for the ESP32-S3. Open-source instruments are references for DSP principles and product architecture, not source-code bases for this implementation.

## Reference study

- **Mutable Instruments Plaits** — the published firmware includes separate percussion models for bass drum, snare and hi-hat, plus noise/particle and resonator techniques. The key architectural lesson is to use a model-specific signal path rather than make every instrument the same oscillator/noise recipe. Official source: https://github.com/pichenettes/eurorack/tree/master/plaits and documentation: https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/
- **Mutable Instruments Peaks / Braids** — useful published examples of short excitation pulses, envelopes, state-variable filtering, and separating control-rate updates from sample-rate DSP. Reference overview: https://pichenettes.github.io/mutable-instruments-documentation/modules/braids/open_source/
- **Mutable Instruments Peaks** — a compact kick/snare model with explicit punch/tone/snappy/decay controls; useful lessons include exponential envelopes, constrained parameter sets, and testing DSP by rendering WAVs. Source and documentation: https://github.com/pichenettes/eurorack/tree/master/peaks and https://pichenettes.github.io/mutable-instruments-documentation/modules/peaks/open_source/
- **CTAG TBD / dada tbd** — an open-source embedded DSP platform with dedicated drum voices (including FM kick, clap and rimshot) and a plugin architecture. Useful as a reference for keeping each engine self-contained. Its GPL-3.0 licensing means we will not copy or port code into a differently licensed project without a deliberate license decision. Source: https://github.com/ctag-fh-kiel/ctag-tbd and platform docs: https://dadamachines.github.io/ctag-tbd/
- **RedtailWorks OPEN-1 / HYP-1** — OPEN-1 explores a multi-firmware instrument on Daisy Seed; HYP-1 demonstrates musical interaction between synthesis voices, drum sequencing and effects. We take the architecture lessons (separate instrument engines, clocked events, cross-engine modulation and firmware/hardware separation), not code. References: https://rustypawz.github.io/fm1-HYP1/open1/ and https://rustypawz.github.io/fm1-HYP1/ . Check each upstream file's license before any reuse; HYP-1 is GPL-3.0.
- **Hydrogen** — an open-source drum machine and useful reference for the instrument/kit/preset workflow, not a direct model for this embedded DSP. Source: https://github.com/hydrogen-music/hydrogen
- **Digitone II FM DRUM** — a commercial architecture reference only: its manual shows distinct FM/body, noise, transient, filtering and amplitude paths. We use that high-level signal-flow idea, not Elektron code or proprietary implementation. Manual: https://www.elektron.se/wp-content/uploads/2025/10/Digitone-2-User-Manual_ENG_OS1.10D_251022.pdf

## Current implementation improvements

1. The FM operator ratios are capped below Nyquist. Previously, high ratios could produce severe aliasing and many phase-wrap iterations per sample, which also wasted CPU time.
2. The drum synthesis path uses a bounded polynomial sine approximation rather than repeated calls to the general-purpose math-library sine function.
3. The final drum mix uses smooth saturation instead of hard clipping. This reduces harsh discontinuities when the body, noise and transient layers sum above full scale.
4. The existing synthesis remains a first-generation layered FM/noise/transient design. The next DSP pass should separate the sound families into genuinely different models:
   - kick/tom: pitch-swept resonant body with controlled transient;
   - snare/clap: tonal shell plus independently enveloped, band-limited noise and staggered clap bursts;
   - closed/open hat and metal: inharmonic metallic partial bank plus high-passed noise;
   - rim: short, high-Q resonant modes and a short excitation.
5. Before adding polyphony or a sequencer, measure block execution time and I2S continuity on the actual ESP32-S3. At 44.1 kHz, 128 frames represent only 2.90 ms. No software-only claim can substitute for listening and timing tests on hardware.

## Known limitations / validation

- The latest code changes have not yet been verified by a completed CI build or a listening test on the user's hardware.
- The drum voice is still monophonic.
- The polynomial sine, soft limiter, FM ratio cap, and overall tonal balance must be checked by listening, especially for low kicks and bright metallic hats.
- Do not solve a common artifact by lowering each preset independently. First confirm CPU budget, I2S continuity, peak levels and each layer in isolation.
- Any future reuse of third-party source must be reviewed file-by-file for license compatibility and attribution. This implementation is intended to remain original BipoLab DSP.

## OPEN-1 / HYP-1 adoption plan (ESP32-S3)

### Stage 0 — clean audio first

- Verify DMA/I2S continuity and render-time budget with worst-case drum settings; log underruns or over-budget blocks without printing from the audio callback.
- Isolate body, noise, transient and final limiter in reproducible tests. Record WAVs for spectral and peak analysis.
- Keep the 44.1 kHz / 128-frame real-time deadline (~2.90 ms) explicit. No allocations, serial logging or UI operations inside the audio callback.

### Stage 1 — instrument event architecture

- Define timestamped `NoteOn`, `NoteOff`, `ParameterChange` and `ClockTick` events, with bounded queues and deterministic event ordering.
- Split drum model state from UI/preset serialization. Each drum family owns its signal path and envelope state, while presets remain versioned data.
- Keep audio-rate and control-rate updates separate; smooth parameter changes to avoid clicks when turning the encoder.

### Stage 2 — clocked sequencing and modulation

- Introduce a shared transport (tempo, step, swing and pattern boundary), initially internal; MIDI clock can be added without coupling to synthesis.
- Allow an instrument to emit a bounded modulation event (e.g., kick trigger -> envelope ducking on another voice) through an explicit routing matrix. Avoid direct voice-to-voice calls in the audio callback.
- Preserve independent kits, patterns and sound presets; a pattern references sound IDs rather than embedding the DSP implementation.

### Stage 3 — offline test harness

- Compile the DSP core on a host without Arduino/UI dependencies; render reference WAVs from deterministic trigger sequences.
- Automated checks: finite samples, peak and RMS, discontinuities, silence at rest, decay tails, render-time budget and regression against saved audio metrics.
- A browser emulator is a possible future tool, not a prerequisite for clean audio on hardware.

### Design boundaries

- **Target remains ESP32-S3 + PCM5102**. Daisy Seed is a design reference, not a migration decision.
- **Original BipoLab code only** for this implementation; do not import GPL-3.0 or other third-party code without explicit license review.
- **Prioritize sonic quality and stability over adding features**. Polyphony and sequencer come after measured audio reliability.

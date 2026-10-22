# BipoLab sound presets — schema v1

## Design goals
- A sound is an independently named asset, not a pointer to live UI globals.
- Loading a sound restores the stored engine and parameter values.
- Save As means saving the current values under a new name; it does not modify the original unless the user deliberately saves with the same name.
- A preset includes its engine ID and schema version so later engine-specific parameters can be added through explicit schema migrations.
- Presets are independent from future kits. A kit should eventually embed a copy of each assigned sound's parameter payload, plus per-track level, mute/solo, routing and effect sends. Editing a kit must not mutate the source sound bank.

## Current schema
`SoundPreset` schema v1 stores:
- Name (1–23 ASCII characters: letters, digits, underscore and hyphen).
- Engine ID, frequency, brightness, level and modulation flag.
- Amplitude envelope: attack, decay, sustain and release.
- Pitch envelope: semitone amount, decay time and enable flag.

BIPO Kick currently consumes frequency, level, amplitude contour and pitch envelope. Its amplitude envelope is one-shot: attack, decay toward sustain, then release to silence. There is no held gate for a kick voice. Other engines currently retain the shared legacy controls; their own complete engine-specific parameter sets are future work.

## Persistence format
- LittleFS directory: `/sounds`
- Extension: `.bps`
- Header: magic `BPS1`, schema version, payload byte count and FNV-1a checksum.
- The payload is the fixed-width `SoundPreset` structure for the ESP32 firmware build.
- A temporary file is written and flushed before it is renamed into place.
- Loads reject unknown schema versions, unexpected payload sizes, trailing bytes, invalid ranges and checksum mismatches.
- `LittleFS.begin(false)` is used: a mount failure is reported; the code must not format the filesystem automatically.

This is a firmware-local binary format, not yet a portable desktop interchange format. If fields/layout change, bump the schema version and implement an explicit migration or reject the old file cleanly.

## Serial workflow
Set Serial Monitor to 115200 baud with newline enabled:
```text
:kick
:env 1 180 0 30
:pitch 30 45
:freq 55
:level 0.8
:save BIPO_Kick_01
:load BIPO_Kick_01
:list
```

Create a variant by changing parameters and saving with another name, for example `:save BIPO_Kick_02`. Parameters changed in RAM are not persistent until saved.

## Next milestones
1. Separate DSP runtime state from UI state and publish parameter changes safely to the audio task to eliminate races and reduce encoder artifacts.
2. Add dedicated engine parameter structs and serialization for FM, wavetable, analog, modal and texture engines.
3. Add named sound banks and kits that embed sound snapshots rather than mutable references.
4. Add kit/track levels, per-track envelopes, effects and eventual 16-step sequencer integration.
5. Add automated schema/validation tests and compile checks, then verify save/reboot/load and audio behavior on the physical board.

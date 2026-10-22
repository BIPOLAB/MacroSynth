#pragma once

#include <Arduino.h>

// Versioned, engine-independent preset container. Engine-specific fields can be
// added in later schema versions without changing the on-disk contract silently.
constexpr uint16_t BIPO_SOUND_SCHEMA_VERSION = 1;
constexpr size_t BIPO_SOUND_NAME_LENGTH = 24;

enum SoundEngineId : uint8_t {
  SOUND_ENGINE_ANALOG = 0,
  SOUND_ENGINE_FM,
  SOUND_ENGINE_WAVETABLE,
  SOUND_ENGINE_PLUCK,
  SOUND_ENGINE_MODAL,
  SOUND_ENGINE_NOISE,
  SOUND_ENGINE_CHORD,
  SOUND_ENGINE_TEXTURE,
  SOUND_ENGINE_KICK
};

struct EnvelopeParameters {
  float attackMs;
  float decayMs;
  float sustain;
  float releaseMs;
};

struct PitchEnvelopeParameters {
  float amountSemitones;
  float decayMs;
  bool enabled;
};

struct SoundPreset {
  uint16_t schemaVersion;
  char name[BIPO_SOUND_NAME_LENGTH];
  SoundEngineId engine;
  float frequencyHz;
  float brightness;
  float level;
  bool modulationEnabled;
  EnvelopeParameters amplitudeEnvelope;
  PitchEnvelopeParameters pitchEnvelope;
};

// Initializes a useful kick preset with bounded, musical defaults.
void makeDefaultKickPreset(SoundPreset &preset, const char *name = "BIPO_Kick_01");
bool validateSoundPreset(const SoundPreset &preset);

bool saveSoundPreset(const SoundPreset &preset);
bool loadSoundPreset(const char *name, SoundPreset &preset);
bool deleteSoundPreset(const char *name);
void listSoundPresets(Stream &output);

// Publish UI edits as an atomic snapshot; audio consumes snapshots at block boundaries.
void publishSoundPreset(const SoundPreset &preset);
bool consumePendingSoundPreset(SoundPreset &preset);

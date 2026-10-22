#pragma once

#include <Arduino.h>

// BPS schema v2 adds the original BipoLab layered drum synthesis parameters.
// SoundPreset v1 files are migrated on load by SoundPreset.cpp.
constexpr uint16_t BIPO_SOUND_SCHEMA_VERSION = 2;
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

enum DrumModel : uint8_t {
  DRUM_MODEL_KICK = 0,
  DRUM_MODEL_SNARE,
  DRUM_MODEL_CLOSED_HAT,
  DRUM_MODEL_OPEN_HAT,
  DRUM_MODEL_CLAP,
  DRUM_MODEL_TOM,
  DRUM_MODEL_RIM,
  DRUM_MODEL_METAL,
  DRUM_MODEL_COUNT
};

enum DrumTransient : uint8_t {
  DRUM_TRANSIENT_NONE = 0,
  DRUM_TRANSIENT_CLICK,
  DRUM_TRANSIENT_NOISE,
  DRUM_TRANSIENT_CRACK,
  DRUM_TRANSIENT_COUNT
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

struct DrumParameters {
  DrumModel model;
  uint8_t algorithm;
  uint8_t bodyWave;
  uint8_t transientType;
  float sweepTimeMs;
  float sweepDepthSemitones;
  float ratioA;
  float ratioB;
  float modulationA;
  float modulationB;
  float feedback;
  float wavefold;
  float bodyHoldMs;
  float bodyDecayMs;
  float bodyLevel;
  float noiseHoldMs;
  float noiseDecayMs;
  float noiseLevel;
  float transientLevel;
  float noiseBaseHz;
  float noiseWidthHz;
  bool noiseReset;
  bool noiseRingMod;
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
  DrumParameters drum;
};

// Initializes factory defaults for the original BipoLab layered drum engine.
void makeDefaultKickPreset(SoundPreset &preset, const char *name = "BIPO_Kick_01");
void makeDefaultDrumPreset(SoundPreset &preset, DrumModel model, const char *name);
bool validateSoundPreset(const SoundPreset &preset);

bool saveSoundPreset(const SoundPreset &preset);
bool loadSoundPreset(const char *name, SoundPreset &preset);
bool deleteSoundPreset(const char *name);
void listSoundPresets(Stream &output);

// Publish UI edits as an atomic snapshot; audio consumes snapshots at block boundaries.
void publishSoundPreset(const SoundPreset &preset);
bool consumePendingSoundPreset(SoundPreset &preset);
int getSoundPresetNames(char names[][BIPO_SOUND_NAME_LENGTH], int capacity);

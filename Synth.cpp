#include "Synth.h"
#include <math.h>
float clampFloat(float value, float minimum, float maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}
float wrapPhase(float phase) {
  while (phase >= TWO_PI) phase -= TWO_PI;
  while (phase < 0.0f) phase += TWO_PI;
  return phase;
}
float fastSine(float phase) { return sinf(phase); }
float sawWave(float phase) { return (phase / PI) - 1.0f; }
float squareWave(float phase, float pulseWidth = 0.5f) { return phase < TWO_PI * pulseWidth ? 1.0f : -1.0f; }
float triangleWave(float phase) {
  if (phase < PI) return -1.0f + (2.0f * phase / PI);
  return 3.0f - (2.0f * phase / PI);
}
float nextNoise() {
  noiseState ^= noiseState << 13; noiseState ^= noiseState >> 17; noiseState ^= noiseState << 5;
  return ((float)(noiseState & 0xFFFF) / 32767.5f) - 1.0f;
}
float normalizeSample(float sample) { return clampFloat(sample, -1.0f, 1.0f); }

static float generateKickSample() {
  if (kickTriggerRequested) {
    kickTriggerRequested = false;
    kickAgeSamples = 0;
    kickAmplitude = 1.0f;
    kickPhase = 0.0f;
  }
  const float sr = (float)SAMPLE_RATE;
  const float ageMs = (1000.0f * (float)kickAgeSamples) / sr;
  const float decayMs = fmaxf(1.0f, audioSoundPreset.pitchEnvelope.decayMs);
  const float pitchAmount = audioSoundPreset.pitchEnvelope.enabled
      ? audioSoundPreset.pitchEnvelope.amountSemitones * expf(-ageMs / decayMs) : 0.0f;
  const float frequency = clampFloat(audioSoundPreset.frequencyHz * powf(2.0f, pitchAmount / 12.0f), 20.0f, 1200.0f);
  kickPhase = wrapPhase(kickPhase + TWO_PI * frequency / sr);

  const float attackMs = audioSoundPreset.amplitudeEnvelope.attackMs;
  const float ampDecayMs = fmaxf(1.0f, audioSoundPreset.amplitudeEnvelope.decayMs);
  const float sustain = clampFloat(audioSoundPreset.amplitudeEnvelope.sustain, 0.0f, 1.0f);
  const float decayEndMs = attackMs + ampDecayMs;
  const float releaseMs = audioSoundPreset.amplitudeEnvelope.releaseMs;
  if (attackMs > 0.0f && ageMs < attackMs) {
    kickAmplitude = ageMs / attackMs;
  } else if (ageMs < decayEndMs) {
    const float decayProgress = (ageMs - attackMs) / ampDecayMs;
    kickAmplitude = 1.0f + (sustain - 1.0f) * decayProgress;
  } else if (releaseMs > 0.0f && ageMs < decayEndMs + releaseMs) {
    const float releaseProgress = (ageMs - decayEndMs) / releaseMs;
    kickAmplitude = sustain * (1.0f - releaseProgress);
  } else {
    kickAmplitude = 0.0f;
  }
  if (kickAmplitude < 0.0001f) kickAmplitude = 0.0f;
  ++kickAgeSamples;
  return sinf(kickPhase) * kickAmplitude * audioSoundPreset.level;
}

float generateSample() {
  const float sampleRate = (float)SAMPLE_RATE;
  if (currentEngine == ENGINE_KICK) return normalizeSample(generateKickSample());
  float frequency = baseFrequency;
  if (modulationEnabled) frequency *= 1.0f + sinf(phase3) * 0.015f;
  frequency = clampFloat(frequency, 20.0f, 8000.0f);
  phase1 = wrapPhase(phase1 + TWO_PI * frequency / sampleRate);
  phase2 = wrapPhase(phase2 + TWO_PI * (frequency * 2.0f) / sampleRate);
  phase3 = wrapPhase(phase3 + TWO_PI * 0.25f / sampleRate);
  float sample = 0.0f;
  switch (currentEngine) {
    case ENGINE_ANALOG:
      sample = sawWave(phase1) * (0.35f + brightness * 0.35f) +
               squareWave(phase1) * (0.25f - brightness * 0.10f) + fastSine(phase1) * 0.25f;
      break;
    case ENGINE_FM: {
      const float index = 0.5f + brightness * 8.0f;
      sample = sinf(phase1 + sinf(phase2) * index);
      break;
    }
    case ENGINE_WAVETABLE:
      sample = sinf(phase1) * 0.50f + sinf(phase1 * 2.0f) * brightness * 0.30f +
               sinf(phase1 * 3.0f) * brightness * 0.15f + sinf(phase1 * 4.0f) * brightness * 0.05f;
      break;
    case ENGINE_PLUCK: {
      const float decay = 0.995f - brightness * 0.025f;
      envelope = envelope * decay + 0.0005f;
      if (envelope > 1.0f) envelope = 1.0f;
      sample = (sinf(phase1) * 0.70f + sinf(phase2 * 1.503f) * 0.30f) * envelope;
      if (envelope < 0.01f) envelope = 0.15f;
      break;
    }
    case ENGINE_MODAL:
      sample = sinf(phase1) * 0.50f + sinf(phase1 * 2.756f) * 0.30f + sinf(phase1 * 5.404f) * 0.20f;
      sample *= 0.75f + brightness * 0.25f;
      break;
    case ENGINE_NOISE: {
      const float noise = nextNoise();
      filterState += (noise - filterState) * (0.02f + brightness * 0.45f);
      sample = filterState;
      break;
    }
    case ENGINE_CHORD:
      sample = sinf(phase1) * 0.34f + sinf(phase1 * 1.259921f) * 0.33f + sinf(phase1 * 1.498307f) * 0.33f;
      break;
    case ENGINE_TEXTURE:
      sample = sinf(phase1) * 0.45f + sinf(phase2 * 1.37f) * 0.25f + nextNoise() * brightness * 0.30f;
      break;
    default: break;
  }
  return normalizeSample(sample);
}

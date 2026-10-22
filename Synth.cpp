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

static float bodyPhaseC = 0.0f;
static float bodyPhaseA = 0.0f;
static float bodyPhaseB = 0.0f;
static float noiseLowState = 0.0f;
static float noiseBaseState = 0.0f;
static uint32_t drumNoiseState = 0xA341316Cu;

static float drumNoise() {
  drumNoiseState ^= drumNoiseState << 13;
  drumNoiseState ^= drumNoiseState >> 17;
  drumNoiseState ^= drumNoiseState << 5;
  return ((float)(drumNoiseState & 0xFFFFu) / 32767.5f) - 1.0f;
}

static float oneShotEnvelope(float ageMs, float holdMs, float decayMs) {
  if (ageMs < holdMs) return 1.0f;
  return expf(-(ageMs - holdMs) / fmaxf(1.0f, decayMs));
}

static float drumTransient(float ageMs, DrumTransient type) {
  if (type == DRUM_TRANSIENT_NONE) return 0.0f;
  if (type == DRUM_TRANSIENT_CLICK) {
    if (ageMs > 3.0f) return 0.0f;
    return drumNoise() * expf(-ageMs * 2.2f);
  }
  if (type == DRUM_TRANSIENT_CRACK) {
    if (ageMs > 18.0f) return 0.0f;
    const float burst = (ageMs < 2.0f || (ageMs > 5.0f && ageMs < 7.0f) ||
                         (ageMs > 10.0f && ageMs < 11.5f)) ? 1.0f : 0.0f;
    return drumNoise() * burst * expf(-ageMs * 0.08f);
  }
  if (ageMs > 35.0f) return 0.0f;
  return drumNoise() * expf(-ageMs * 0.13f);
}

static float generateKickSample() {
  if (kickTriggerRequested) {
    kickTriggerRequested = false;
    kickAgeSamples = 0;
    kickAmplitude = 1.0f;
    kickPhase = 0.0f;
    bodyPhaseC = bodyPhaseA = bodyPhaseB = 0.0f;
    noiseLowState = noiseBaseState = 0.0f;
    drumNoiseState = audioSoundPreset.drum.noiseReset ? 0xA341316Cu : (drumNoiseState + 0x9E3779B9u);
  }

  const DrumParameters &d = audioSoundPreset.drum;
  const float sr = (float)SAMPLE_RATE;
  const float ageMs = (1000.0f * (float)kickAgeSamples) / sr;
  const float sweep = d.sweepTimeMs > 0.1f
      ? expf(-ageMs / d.sweepTimeMs) : 0.0f;
  const float pitchSemitones = d.sweepDepthSemitones * sweep;
  const float frequency = clampFloat(audioSoundPreset.frequencyHz * powf(2.0f, pitchSemitones / 12.0f), 20.0f, 15000.0f);

  bodyPhaseC = wrapPhase(bodyPhaseC + TWO_PI * frequency / sr);
  bodyPhaseA = wrapPhase(bodyPhaseA + TWO_PI * frequency * d.ratioA / sr);
  bodyPhaseB = wrapPhase(bodyPhaseB + TWO_PI * frequency * d.ratioB / sr);

  const float bodyEnv = oneShotEnvelope(ageMs, d.bodyHoldMs, d.bodyDecayMs);
  const float modAEnv = expf(-ageMs / fmaxf(1.0f, d.bodyDecayMs * 0.45f));
  const float modBEnv = expf(-ageMs / fmaxf(1.0f, d.bodyDecayMs * 0.72f));
  const float opA = sinf(bodyPhaseA + sinf(bodyPhaseB) * d.modulationB * modBEnv);
  const float opB = sinf(bodyPhaseB) * d.modulationB * modBEnv;
  float bodyPhaseMod = 0.0f;
  switch (d.algorithm) {
    case 0: bodyPhaseMod = sinf(bodyPhaseA) * d.modulationA * modAEnv + opB; break;
    case 1: bodyPhaseMod = opA * d.modulationA * modAEnv; break;
    case 2: bodyPhaseMod = sinf(bodyPhaseB) * d.modulationB * modBEnv + sinf(bodyPhaseA) * d.modulationA * modAEnv; break;
    case 3: bodyPhaseMod = sinf(bodyPhaseA) * sinf(bodyPhaseB) * d.modulationA * modAEnv * 3.0f; break;
    case 4: bodyPhaseMod = sinf(bodyPhaseA) * d.modulationA * modAEnv + sinf(bodyPhaseC * 2.0f) * d.feedback * 4.0f; break;
    case 5: bodyPhaseMod = sinf(bodyPhaseA + opB) * d.modulationA * modAEnv; break;
    case 6: bodyPhaseMod = sinf(bodyPhaseA) * d.modulationA * modAEnv - opB; break;
    default: bodyPhaseMod = opA * d.modulationA * modAEnv + opB; break;
  }
  float body = sinf(bodyPhaseC + bodyPhaseMod + sinf(bodyPhaseC * 2.0f) * d.feedback);
  if (d.bodyWave == 1) body = (body * 0.7f) + (sinf(bodyPhaseC * 2.0f) * 0.3f);
  else if (d.bodyWave == 2) body = (body > 0.0f) ? body : body * 0.25f;
  else if (d.bodyWave == 3) body = sinf(bodyPhaseC) * (0.65f + 0.35f * sinf(bodyPhaseA));
  if (d.wavefold > 0.001f) {
    const float foldDrive = 1.0f + d.wavefold * 7.0f;
    body = sinf(body * foldDrive);
  }
  body *= bodyEnv * d.bodyLevel;

  const float noise = drumNoise();
  const float baseCutoff = clampFloat(d.noiseBaseHz, 20.0f, 18000.0f);
  const float widthCutoff = clampFloat(d.noiseWidthHz, 20.0f, 18000.0f);
  const float baseCoeff = 1.0f - expf(-TWO_PI * baseCutoff / sr);
  const float widthCoeff = 1.0f - expf(-TWO_PI * widthCutoff / sr);
  noiseBaseState += (noise - noiseBaseState) * baseCoeff;
  const float highPassed = noise - noiseBaseState;
  noiseLowState += (highPassed - noiseLowState) * widthCoeff;
  float noiseLayer = noiseLowState * oneShotEnvelope(ageMs, d.noiseHoldMs, d.noiseDecayMs) * d.noiseLevel;
  if (d.noiseRingMod) noiseLayer *= (0.25f + 0.75f * fabsf(sinf(bodyPhaseC)));

  float transient = drumTransient(ageMs, (DrumTransient)d.transientType) * d.transientLevel;
  if (d.model == DRUM_MODEL_CLAP && ageMs < 28.0f) {
    const bool clapBurst = (ageMs < 3.0f) || (ageMs > 7.0f && ageMs < 10.0f) ||
                           (ageMs > 14.0f && ageMs < 17.0f) || (ageMs > 21.0f && ageMs < 24.0f);
    transient += clapBurst ? drumNoise() * d.transientLevel * 0.75f : 0.0f;
  }

  const float ampAttack = audioSoundPreset.amplitudeEnvelope.attackMs;
  const float ampDecay = fmaxf(1.0f, audioSoundPreset.amplitudeEnvelope.decayMs);
  const float sustain = clampFloat(audioSoundPreset.amplitudeEnvelope.sustain, 0.0f, 1.0f);
  const float decayEnd = ampAttack + ampDecay;
  if (ampAttack > 0.0f && ageMs < ampAttack) kickAmplitude = ageMs / ampAttack;
  else if (ageMs < decayEnd) kickAmplitude = 1.0f + (sustain - 1.0f) * ((ageMs - ampAttack) / ampDecay);
  else if (audioSoundPreset.amplitudeEnvelope.releaseMs > 0.0f &&
           ageMs < decayEnd + audioSoundPreset.amplitudeEnvelope.releaseMs) {
    kickAmplitude = sustain * (1.0f - (ageMs - decayEnd) / audioSoundPreset.amplitudeEnvelope.releaseMs);
  } else kickAmplitude = 0.0f;

  ++kickAgeSamples;
  return (body + noiseLayer + transient) * kickAmplitude * audioSoundPreset.level;
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

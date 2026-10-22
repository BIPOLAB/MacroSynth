#include "Synth.h"
#include <math.h>
float clampFloat(float value, float minimum, float maximum) {
  if (value < minimum) return minimum;
  if (value > maximum) return maximum;
  return value;
}
float wrapPhase(float phase) {
  if (phase >= TWO_PI) phase -= TWO_PI;
  else if (phase < 0.0f) phase += TWO_PI;
  return phase;
}
// Fast, bounded sine approximation for the per-sample drum path. Avoids the
// high cost of libm sinf() at every operator on the ESP32-S3.
static inline float fastSin(float phase) {
  const float turns = (float)((int)(phase / TWO_PI));
  phase -= turns * TWO_PI;
  if (phase > PI) phase -= TWO_PI;
  else if (phase < -PI) phase += TWO_PI;
  const float x2 = phase * phase;
  return phase * (1.0f + x2 * (-0.1666666716f + x2 * (0.0083333477f +
         x2 * (-0.0001984090f + x2 * 0.0000027526f))));
}
float fastSine(float phase) { return fastSin(phase); }
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
static float bodyEnvelopeState = 1.0f;
static float noiseEnvelopeState = 1.0f;
static float modAEnvelopeState = 1.0f;
static float modBEnvelopeState = 1.0f;
static float sweepEnvelopeState = 1.0f;
static float bodyDecayCoeff = 1.0f;
static float noiseDecayCoeff = 1.0f;
static float modADecayCoeff = 1.0f;
static float modBDecayCoeff = 1.0f;
static float sweepDecayCoeff = 1.0f;
static float noiseBaseCoeff = 0.0f;
static float noiseWidthCoeff = 0.0f;
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
    const float attack = fminf(1.0f, ageMs / 0.20f);
    return drumNoise() * attack * expf(-ageMs * 2.2f);
  }
  if (type == DRUM_TRANSIENT_CRACK) {
    if (ageMs > 18.0f) return 0.0f;
    const float burst = (ageMs < 2.0f || (ageMs > 5.0f && ageMs < 7.0f) ||
                         (ageMs > 10.0f && ageMs < 11.5f)) ? 1.0f : 0.0f;
    const float attack = fminf(1.0f, ageMs / 0.20f);
    return drumNoise() * burst * attack * expf(-ageMs * 0.08f);
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
    bodyEnvelopeState = noiseEnvelopeState = 1.0f;
    modAEnvelopeState = modBEnvelopeState = sweepEnvelopeState = 1.0f;
    const DrumParameters &triggerParams = audioSoundPreset.drum;
    const float triggerSr = (float)SAMPLE_RATE;
    bodyDecayCoeff = expf(-1000.0f / (triggerSr * fmaxf(1.0f, triggerParams.bodyDecayMs)));
    noiseDecayCoeff = expf(-1000.0f / (triggerSr * fmaxf(1.0f, triggerParams.noiseDecayMs)));
    modADecayCoeff = expf(-1000.0f / (triggerSr * fmaxf(1.0f, triggerParams.bodyDecayMs * 0.45f)));
    modBDecayCoeff = expf(-1000.0f / (triggerSr * fmaxf(1.0f, triggerParams.bodyDecayMs * 0.72f)));
    sweepDecayCoeff = triggerParams.sweepTimeMs > 0.1f
        ? expf(-1000.0f / (triggerSr * triggerParams.sweepTimeMs)) : 0.0f;
    noiseBaseCoeff = 1.0f - expf(-TWO_PI * clampFloat(triggerParams.noiseBaseHz, 20.0f, 18000.0f) / triggerSr);
    noiseWidthCoeff = 1.0f - expf(-TWO_PI * clampFloat(triggerParams.noiseWidthHz, 20.0f, 18000.0f) / triggerSr);
    drumNoiseState = triggerParams.noiseReset ? 0xA341316Cu : (drumNoiseState + 0x9E3779B9u);
  }

  const DrumParameters &d = audioSoundPreset.drum;
  const float sr = (float)SAMPLE_RATE;
  const float ageMs = (1000.0f * (float)kickAgeSamples) / sr;
  const float sweep = sweepEnvelopeState;
  sweepEnvelopeState *= sweepDecayCoeff;
  const float pitchSemitones = d.sweepDepthSemitones * sweep;
  const float frequency = clampFloat(audioSoundPreset.frequencyHz * powf(2.0f, pitchSemitones / 12.0f), 20.0f, 15000.0f);

  bodyPhaseC = wrapPhase(bodyPhaseC + TWO_PI * frequency / sr);
  // Keep the FM operators below Nyquist to reduce harsh aliasing and CPU spikes.
  const float freqA = fminf(frequency * d.ratioA, sr * 0.45f);
  const float freqB = fminf(frequency * d.ratioB, sr * 0.45f);
  bodyPhaseA = wrapPhase(bodyPhaseA + TWO_PI * freqA / sr);
  bodyPhaseB = wrapPhase(bodyPhaseB + TWO_PI * freqB / sr);

  if (ageMs >= d.bodyHoldMs) bodyEnvelopeState *= bodyDecayCoeff;
  if (ageMs >= 0.0f) {
    modAEnvelopeState *= modADecayCoeff;
    modBEnvelopeState *= modBDecayCoeff;
  }
  const float bodyEnv = ageMs < d.bodyHoldMs ? 1.0f : bodyEnvelopeState;
  const float modAEnv = modAEnvelopeState;
  const float modBEnv = modBEnvelopeState;
  const float opA = fastSin(bodyPhaseA + fastSin(bodyPhaseB) * d.modulationB * modBEnv);
  const float opB = fastSin(bodyPhaseB) * d.modulationB * modBEnv;
  float bodyPhaseMod = 0.0f;
  switch (d.algorithm) {
    case 0: bodyPhaseMod = fastSin(bodyPhaseA) * d.modulationA * modAEnv + opB; break;
    case 1: bodyPhaseMod = opA * d.modulationA * modAEnv; break;
    case 2: bodyPhaseMod = fastSin(bodyPhaseB) * d.modulationB * modBEnv + fastSin(bodyPhaseA) * d.modulationA * modAEnv; break;
    case 3: bodyPhaseMod = fastSin(bodyPhaseA) * fastSin(bodyPhaseB) * d.modulationA * modAEnv * 3.0f; break;
    case 4: bodyPhaseMod = fastSin(bodyPhaseA) * d.modulationA * modAEnv + fastSin(bodyPhaseC * 2.0f) * d.feedback * 4.0f; break;
    case 5: bodyPhaseMod = fastSin(bodyPhaseA + opB) * d.modulationA * modAEnv; break;
    case 6: bodyPhaseMod = fastSin(bodyPhaseA) * d.modulationA * modAEnv - opB; break;
    default: bodyPhaseMod = opA * d.modulationA * modAEnv + opB; break;
  }
  float body = fastSin(bodyPhaseC + bodyPhaseMod + fastSin(bodyPhaseC * 2.0f) * d.feedback);
  if (d.bodyWave == 1) body = (body * 0.7f) + (fastSin(bodyPhaseC * 2.0f) * 0.3f);
  else if (d.bodyWave == 2) body = (body > 0.0f) ? body : body * 0.25f;
  else if (d.bodyWave == 3) body = fastSin(bodyPhaseC) * (0.65f + 0.35f * fastSin(bodyPhaseA));
  if (d.wavefold > 0.001f) {
    const float foldDrive = 1.0f + d.wavefold * 7.0f;
    body = fastSin(body * foldDrive);
  }
  body *= bodyEnv * d.bodyLevel;

  // Family-specific excitation/partials: these are deliberately different
  // synthesis paths, not just the same FM patch with different names.
  switch (d.model) {
    case DRUM_MODEL_SNARE:
      // A short shell mode sits under the wire/noise layer.
      body = body * 0.68f +
             (fastSin(bodyPhaseA) * 0.20f + fastSin(bodyPhaseB) * 0.12f) *
             bodyEnv * d.bodyLevel;
      break;
    case DRUM_MODEL_CLOSED_HAT:
    case DRUM_MODEL_OPEN_HAT:
    case DRUM_MODEL_METAL:
      // Inharmonic operator blend creates a metallic bank; ratio caps above
      // keep its upper partials from wrapping repeatedly above Nyquist.
      body = (fastSin(bodyPhaseC) * 0.18f +
              fastSin(bodyPhaseA) * 0.41f +
              fastSin(bodyPhaseB) * 0.41f) * bodyEnv * d.bodyLevel;
      break;
    case DRUM_MODEL_RIM:
      body = (fastSin(bodyPhaseC) * 0.25f +
              fastSin(bodyPhaseA) * 0.45f +
              fastSin(bodyPhaseB) * 0.30f) * bodyEnv * d.bodyLevel;
      break;
    case DRUM_MODEL_CLAP:
      // Clap identity is mostly its staggered noise bursts, with only a small
      // tonal shell to prevent the sound becoming an undifferentiated noise hit.
      body *= 0.45f;
      break;
    case DRUM_MODEL_KICK:
    case DRUM_MODEL_TOM:
    default:
      break;
  }

  const float noise = drumNoise();
  const float baseCutoff = clampFloat(d.noiseBaseHz, 20.0f, 18000.0f);
  const float widthCutoff = clampFloat(d.noiseWidthHz, 20.0f, 18000.0f);
  noiseBaseState += (noise - noiseBaseState) * noiseBaseCoeff;
  const float highPassed = noise - noiseBaseState;
  noiseLowState += (highPassed - noiseLowState) * noiseWidthCoeff;
  if (ageMs >= d.noiseHoldMs) noiseEnvelopeState *= noiseDecayCoeff;
  const float noiseEnv = ageMs < d.noiseHoldMs ? 1.0f : noiseEnvelopeState;
  float noiseLayer = noiseLowState * noiseEnv * d.noiseLevel;
  if (d.noiseRingMod) noiseLayer *= (0.25f + 0.75f * fabsf(fastSin(bodyPhaseC)));

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
  if (modulationEnabled) frequency *= 1.0f + fastSin(phase3) * 0.015f;
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
      sample = fastSin(phase1 + fastSin(phase2) * index);
      break;
    }
    case ENGINE_WAVETABLE:
      sample = fastSin(phase1) * 0.50f + fastSin(phase1 * 2.0f) * brightness * 0.30f +
               fastSin(phase1 * 3.0f) * brightness * 0.15f + fastSin(phase1 * 4.0f) * brightness * 0.05f;
      break;
    case ENGINE_PLUCK: {
      const float decay = 0.995f - brightness * 0.025f;
      envelope = envelope * decay + 0.0005f;
      if (envelope > 1.0f) envelope = 1.0f;
      sample = (fastSin(phase1) * 0.70f + fastSin(phase2 * 1.503f) * 0.30f) * envelope;
      if (envelope < 0.01f) envelope = 0.15f;
      break;
    }
    case ENGINE_MODAL:
      sample = fastSin(phase1) * 0.50f + fastSin(phase1 * 2.756f) * 0.30f + fastSin(phase1 * 5.404f) * 0.20f;
      sample *= 0.75f + brightness * 0.25f;
      break;
    case ENGINE_NOISE: {
      const float noise = nextNoise();
      filterState += (noise - filterState) * (0.02f + brightness * 0.45f);
      sample = filterState;
      break;
    }
    case ENGINE_CHORD:
      sample = fastSin(phase1) * 0.34f + fastSin(phase1 * 1.259921f) * 0.33f + fastSin(phase1 * 1.498307f) * 0.33f;
      break;
    case ENGINE_TEXTURE:
      sample = fastSin(phase1) * 0.45f + fastSin(phase2 * 1.37f) * 0.25f + nextNoise() * brightness * 0.30f;
      break;
    default: break;
  }
  return normalizeSample(sample);
}

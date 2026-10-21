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

float fastSine(float phase) {
  return sinf(phase);
}

float sawWave(float phase) {
  return (phase / PI) - 1.0f;
}

float squareWave(float phase, float pulseWidth = 0.5f) {
  return phase < TWO_PI * pulseWidth ? 1.0f : -1.0f;
}

float triangleWave(float phase) {
  if (phase < PI) {
    return -1.0f + (2.0f * phase / PI);
  }

  return 3.0f - (2.0f * phase / PI);
}

float nextNoise() {
  noiseState ^= noiseState << 13;
  noiseState ^= noiseState >> 17;
  noiseState ^= noiseState << 5;

  return ((float)(noiseState & 0xFFFF) / 32767.5f) - 1.0f;
}

float normalizeSample(float sample) {
  return clampFloat(sample, -1.0f, 1.0f);
}


float generateSample() {
  const float sampleRate = (float)SAMPLE_RATE;

  float frequency = baseFrequency;

  if (modulationEnabled) {
    float mod = sinf(phase3);
    frequency *= 1.0f + mod * 0.015f;
  }

  frequency = clampFloat(frequency, 20.0f, 8000.0f);

  float increment1 = TWO_PI * frequency / sampleRate;
  float increment2 = TWO_PI * (frequency * 2.0f) / sampleRate;
  float increment3 = TWO_PI * 0.25f / sampleRate;

  phase1 = wrapPhase(phase1 + increment1);
  phase2 = wrapPhase(phase2 + increment2);
  phase3 = wrapPhase(phase3 + increment3);

  float sample = 0.0f;

  switch (currentEngine) {

    // --------------------------------------------------------
    // ANALOG
    // --------------------------------------------------------
    case ENGINE_ANALOG: {
      float saw = sawWave(phase1);
      float square = squareWave(phase1);
      float sine = fastSine(phase1);

      sample =
        saw * (0.35f + brightness * 0.35f) +
        square * (0.25f - brightness * 0.10f) +
        sine * 0.25f;

      break;
    }

    // --------------------------------------------------------
    // FM
    // --------------------------------------------------------
    case ENGINE_FM: {
      float modulationIndex = 0.5f + brightness * 8.0f;
      float modulator = sinf(phase2) * modulationIndex;

      sample = sinf(phase1 + modulator);

      break;
    }

    // --------------------------------------------------------
    // WAVETABLE: forma armonica sintetica
    // --------------------------------------------------------
    case ENGINE_WAVETABLE: {
      sample =
        sinf(phase1) * 0.50f +
        sinf(phase1 * 2.0f) * brightness * 0.30f +
        sinf(phase1 * 3.0f) * brightness * 0.15f +
        sinf(phase1 * 4.0f) * brightness * 0.05f;

      break;
    }

    // --------------------------------------------------------
    // PLUCK: cuerda sintetica sencilla
    // --------------------------------------------------------
    case ENGINE_PLUCK: {
      float decay = 0.995f - brightness * 0.025f;

      envelope = envelope * decay + 0.0005f;
      if (envelope > 1.0f) envelope = 1.0f;

      sample =
        sinf(phase1) * 0.70f +
        sinf(phase2 * 1.503f) * 0.30f;

      sample *= envelope;

      if (envelope < 0.01f) {
        envelope = 0.15f;
      }

      break;
    }

    // --------------------------------------------------------
    // MODAL: suma de resonancias
    // --------------------------------------------------------
    case ENGINE_MODAL: {
      sample =
        sinf(phase1) * 0.50f +
        sinf(phase1 * 2.756f) * 0.30f +
        sinf(phase1 * 5.404f) * 0.20f;

      sample *= 0.75f + brightness * 0.25f;

      break;
    }

    // --------------------------------------------------------
    // NOISE
    // --------------------------------------------------------
    case ENGINE_NOISE: {
      float noise = nextNoise();

      filterState += (noise - filterState) *
                     (0.02f + brightness * 0.45f);

      sample = filterState;

      break;
    }

    // --------------------------------------------------------
    // CHORD: acorde sencillo
    // --------------------------------------------------------
    case ENGINE_CHORD: {
      float f2 = phase1 * 1.259921f;
      float f3 = phase1 * 1.498307f;

      sample =
        sinf(phase1) * 0.34f +
        sinf(f2) * 0.33f +
        sinf(f3) * 0.33f;

      break;
    }

    // --------------------------------------------------------
    // TEXTURE
    // --------------------------------------------------------
    case ENGINE_TEXTURE: {
      float noise = nextNoise();

      sample =
        sinf(phase1) * 0.45f +
        sinf(phase2 * 1.37f) * 0.25f +
        noise * brightness * 0.30f;

      break;
    }
  }

  return normalizeSample(sample);
}



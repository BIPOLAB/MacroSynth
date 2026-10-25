/*
  MacroSynth Drum Engine — Stage 2: minimal voice interface, kick only
  Stage 1 kick synthesis and I2S path are retained. The only structural
  change is that trigger/render are routed through a tiny DrumVoice interface.

  ESP32-S3 + PCM5102
  I2S: BCLK GPIO16, LRCK GPIO18, DOUT GPIO8
  Sample rate: 44100 Hz, stereo 16-bit; block size: 128 frames
*/

#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

static constexpr int PIN_I2S_BCLK = 16;
static constexpr int PIN_I2S_LRCK = 18;
static constexpr int PIN_I2S_DOUT = 8;
static constexpr uint32_t SAMPLE_RATE = 44100;
static constexpr uint16_t BLOCK_FRAMES = 128;
static constexpr float BPM = 110.0f;
static constexpr float TWO_PI_F = 6.28318530717958647692f;

I2SClass I2S;
int16_t audioBuffer[BLOCK_FRAMES * 2];

static uint64_t sampleCounter = 0;
static uint32_t rngState = 0xA341316Cu;
static uint32_t stepIndex = 0;

static bool kickActive = false;
static float kickAge = 0.0f;
static float kickPhase = 0.0f;

static const uint8_t kickPattern[16] = {
  1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0
};

static inline float clampAudio(float x) {
  if (x > 1.0f) return 1.0f;
  if (x < -1.0f) return -1.0f;
  return x;
}

static inline float noiseSample() {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return (float)(rngState & 0xFFFFu) / 32767.5f - 1.0f;
}

// Voice callbacks: deliberately minimal for this first architecture test.
using TriggerVoiceFn = void (*)();
using RenderVoiceFn = float (*)();

struct DrumVoice {
  TriggerVoiceFn trigger;
  RenderVoiceFn render;
};

static void triggerKick() {
  kickActive = true;
  kickAge = 0.0f;
  kickPhase = 0.0f;
}

static float renderKick() {
  if (!kickActive) return 0.0f;
  const float ageMs = kickAge * 1000.0f / SAMPLE_RATE;
  const float pitchHz = 48.0f + 145.0f * expf(-ageMs / 14.0f);
  kickPhase += TWO_PI_F * pitchHz / SAMPLE_RATE;
  if (kickPhase >= TWO_PI_F) kickPhase -= TWO_PI_F;
  const float amp = expf(-ageMs / 185.0f);
  const float click = (ageMs < 2.0f)
      ? noiseSample() * (1.0f - ageMs / 2.0f) * 0.07f : 0.0f;
  const float out = sinf(kickPhase) * amp * 0.82f + click;
  kickAge += 1.0f;
  if (ageMs > 850.0f) kickActive = false;
  return out;
}

static DrumVoice kickVoice = { triggerKick, renderKick };

static void triggerStep(uint32_t step) {
  if (kickPattern[step]) kickVoice.trigger();
}

static void renderAudioBlock() {
  const uint64_t samplesPerStep =
      (uint64_t)((60.0f * SAMPLE_RATE) / (BPM * 4.0f));

  for (uint16_t i = 0; i < BLOCK_FRAMES; ++i) {
    if ((sampleCounter % samplesPerStep) == 0) {
      stepIndex = (uint32_t)((sampleCounter / samplesPerStep) % 16u);
      triggerStep(stepIndex);
    }

    float mix = kickVoice.render();
    mix = mix / (1.0f + 0.18f * fabsf(mix));
    const int16_t out = (int16_t)(clampAudio(mix) * 32767.0f);
    audioBuffer[i * 2] = out;
    audioBuffer[i * 2 + 1] = out;
    ++sampleCounter;
  }

  const size_t bytes = sizeof(audioBuffer);
  const size_t written = I2S.write((uint8_t *)audioBuffer, bytes);
  if (written != bytes) {
    static bool warned = false;
    if (!warned) {
      Serial.printf("WARNING: I2S short write (%u/%u bytes)\n",
                    (unsigned)written, (unsigned)bytes);
      warned = true;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("BIPOLAB — Drum Engine Stage 2 (voice interface, kick only)");
  Serial.println("ESP32-S3 / PCM5102 | 44.1 kHz | 110 BPM");
  Serial.println("Kick only: four-on-the-floor pattern.");

  I2S.setPins(PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);
  const bool ok = I2S.begin(
      I2S_MODE_STD, SAMPLE_RATE,
      I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

  if (!ok) {
    Serial.println("ERROR: I2S.begin failed. Check wiring and board package.");
    while (true) delay(1000);
  }
  Serial.println("I2S initialized. Kick sequencer running.");
}

void loop() {
  renderAudioBlock();
}

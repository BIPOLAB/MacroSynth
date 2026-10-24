// MacroSynth drum-engine isolation firmware
// ESP32-S3 + PCM5102. No display, encoder, menus, filesystem, or UI task.
// This sketch uses the current MacroSynth Synth.cpp drum engine for an isolated test.
#include <Arduino.h>
#include <ESP_I2S.h>
#include "Config.h"
#include "State.h"
#include "Synth.h"

I2SClass I2S;

// Minimal definitions required by the existing synthesis engine.
int currentEngine = ENGINE_KICK;
float baseFrequency = 55.0f, brightness = 0.35f, masterVolume = 0.20f;
bool modulationEnabled = false;
float phase1 = 0.0f, phase2 = 0.0f, phase3 = 0.0f;
float envelope = 0.0f, filterState = 0.0f;
uint32_t noiseState = 0x12345678;
int16_t audioBuffer[AUDIO_FRAMES * 2] = {};
SoundPreset activeSoundPreset = {};
SoundPreset audioSoundPreset = {};
volatile bool kickTriggerRequested = false;
float kickPhase = 0.0f;
uint32_t kickAgeSamples = 0;
float kickAmplitude = 0.0f;

static constexpr uint32_t BPM = 110;
static constexpr uint32_t STEP_COUNT = 16;
static constexpr uint32_t STEP_SAMPLES = (SAMPLE_RATE * 60UL) / (BPM * 4UL);
static uint64_t sampleClock = 0;
static uint32_t stepIndex = 0;
static uint32_t blocksSinceReport = 0;
static bool i2sReady = false;
static uint32_t shortWrites = 0;

static void setKickPreset() {
  memset(&audioSoundPreset, 0, sizeof(audioSoundPreset));
  audioSoundPreset.schemaVersion = BIPO_SOUND_SCHEMA_VERSION;
  strlcpy(audioSoundPreset.name, "ENGINE_KICK_TEST", sizeof(audioSoundPreset.name));
  audioSoundPreset.engine = SOUND_ENGINE_KICK;
  audioSoundPreset.frequencyHz = 48.0f;
  audioSoundPreset.brightness = 0.0f;
  audioSoundPreset.level = 0.9f;
  audioSoundPreset.amplitudeEnvelope = {0.0f, 240.0f, 0.0f, 0.0f};
  audioSoundPreset.pitchEnvelope = {0.0f, 45.0f, true};
  audioSoundPreset.drum = {
    DRUM_MODEL_KICK, 0, 0, DRUM_TRANSIENT_CLICK,
    28.0f, 24.0f, 1.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 260.0f, 1.0f, 0.0f, 80.0f, 0.04f, 0.35f,
    1800.0f, 4200.0f, true, false
  };
  activeSoundPreset = audioSoundPreset;
}

static void startStepIfNeeded() {
  if ((sampleClock % STEP_SAMPLES) != 0) return;
  stepIndex = (sampleClock / STEP_SAMPLES) % STEP_COUNT;
  // A deliberately sparse four-on-the-floor kick pattern: isolate the actual
  // MacroSynth drum DSP before introducing overlapping voices or UI scheduling.
  if (stepIndex == 0 || stepIndex == 4 || stepIndex == 8 || stepIndex == 12) {
    kickTriggerRequested = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println();
  Serial.println("MacroSynth: DRUM ENGINE ONLY");
  Serial.println("No TFT / controls / menus / presets / UI task");
  setKickPreset();

  I2S.setPins(I2S_BCLK, I2S_LRCK, I2S_DOUT);
  i2sReady = I2S.begin(I2S_MODE_STD, SAMPLE_RATE,
                       I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  if (!i2sReady) {
    Serial.println("ERROR: I2S.begin failed");
    return;
  }
  Serial.println("I2S ready: 44100 Hz, 16-bit stereo; kick sequence at 110 BPM");
}

void loop() {
  if (!i2sReady) {
    delay(1000);
    return;
  }

  for (int i = 0; i < AUDIO_FRAMES; ++i) {
    startStepIfNeeded();
    float sample = generateSample();
    if (!isfinite(sample)) sample = 0.0f;
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    const int16_t out = (int16_t)(sample * 32767.0f);
    audioBuffer[i * 2] = out;
    audioBuffer[i * 2 + 1] = out;
    ++sampleClock;
  }

  const size_t expected = sizeof(audioBuffer);
  const size_t written = I2S.write((uint8_t*)audioBuffer, expected);
  if (written != expected) ++shortWrites;

  if (++blocksSinceReport >= 172) {
    blocksSinceReport = 0;
    Serial.printf("Audio running | step %lu | short writes: %lu\n",
                  (unsigned long)stepIndex, (unsigned long)shortWrites);
  }
}

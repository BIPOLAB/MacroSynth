/*
  MacroSynth Drum Engine — Stage 9: 16-step pattern editor prototype
  Temporary prototype controls: rotate encoder to select one of 16 kick steps,
  short press toggles that step, long press toggles transport. Final hardware
  will use the physical sequencer switches directly, not the encoder.

  ESP32-S3 + PCM5102
  I2S: BCLK GPIO16, LRCK GPIO18, DOUT GPIO8
  Sample rate: 44100 Hz, stereo 16-bit; block size: 128 frames
*/

#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

static constexpr int PIN_I2S_BCLK = 16;
static constexpr int PIN_I2S_LRCK = 18;
static constexpr int PIN_I2S_DOUT = 8;
static constexpr uint32_t SAMPLE_RATE = 44100;
static constexpr uint16_t BLOCK_FRAMES = 128;
static float bpm = 110.0f;
static constexpr float BPM_MIN = 60.0f;
static constexpr float BPM_MAX = 180.0f;
static bool playing = true;
static bool lastDrawnPlaying = true;
static constexpr float TWO_PI_F = 6.28318530717958647692f;

static constexpr int PIN_TFT_SCLK = 12;
static constexpr int PIN_TFT_MOSI = 11;
static constexpr int PIN_TFT_DC = 13;
static constexpr int PIN_TFT_RST = 10;
static constexpr int PIN_TFT_BL = 14;
static constexpr int PIN_ENCODER_A = 35;
static constexpr int PIN_ENCODER_B = 36;
static constexpr int PIN_ENCODER_SW = 37;

Adafruit_ST7789 tft(&SPI, -1, PIN_TFT_DC, PIN_TFT_RST);

static void drawStartupScreen() {
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
  SPI.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, -1);
  tft.init(240, 240, SPI_MODE2);
  tft.setSPISpeed(12000000);
  tft.invertDisplay(true);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(24, 24);
  tft.print("BIPOLAB");
  tft.drawFastHLine(24, 58, 192, ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(24, 78);
  tft.print("MACROSYNTH");
  tft.setTextSize(3);
  tft.setCursor(24, 119);
  tft.print("110 BPM");
  tft.setTextSize(1);
  tft.setCursor(24, 151);
  tft.print("PLAYING");
  tft.setTextSize(2);
  tft.setCursor(24, 170);
  tft.print("KICK  SNARE");
  tft.setCursor(24, 198);
  tft.print("CLOSED HAT");
  // 16-step kick editor: temporary encoder-based prototype UI.
  tft.setTextSize(1);
  tft.setCursor(24, 215);
  tft.print("KICK STEPS (encoder prototype)");
  for (uint8_t i = 0; i < 16; ++i) {
    const int x = 16 + i * 13;
    tft.drawRect(x, 228, 10, 10, ST77XX_WHITE);
  }
}

I2SClass I2S;
int16_t audioBuffer[BLOCK_FRAMES * 2];

static uint64_t sampleCounter = 0;
static uint32_t rngState = 0xA341316Cu;
static uint32_t stepIndex = 0;
static uint8_t lastDrawnBeat = 255;
static int lastEncoderA = HIGH;
static int lastButtonReading = HIGH;
static int stableButtonState = HIGH;
static uint32_t lastButtonChangeMs = 0;
static uint32_t buttonPressedAtMs = 0;
static bool longPressHandled = false;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
static constexpr uint32_t LONG_PRESS_MS = 650;
static uint8_t selectedStep = 0;
static uint8_t lastDrawnSelectedStep = 255;
static uint8_t lastDrawnKickPattern[16] = {0};
static float lastDrawnBpm = -1.0f;

static bool kickActive = false;
static float kickAge = 0.0f;
static float kickPhase = 0.0f;
static bool snareActive = false;
static float snareAge = 0.0f;
static float snarePhase = 0.0f;
static float hatEnvelope = 0.0f;
static float hatHighpassState = 0.0f;

static uint8_t kickPattern[16] = {
  1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0
};

static const uint8_t snarePattern[16] = {
  0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0
};

static const uint8_t hatPattern[16] = {
  1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0
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

static void triggerSnare() {
  snareActive = true;
  snareAge = 0.0f;
  snarePhase = 0.0f;
}

static float renderSnare() {
  if (!snareActive) return 0.0f;
  const float ageMs = snareAge * 1000.0f / SAMPLE_RATE;
  const float tonal = sinf(snarePhase) * expf(-ageMs / 65.0f) * 0.22f;
  snarePhase += TWO_PI_F * 185.0f / SAMPLE_RATE;
  if (snarePhase >= TWO_PI_F) snarePhase -= TWO_PI_F;
  const float noise = noiseSample() * expf(-ageMs / 115.0f) * 0.42f;
  snareAge += 1.0f;
  if (ageMs > 500.0f) snareActive = false;
  return tonal + noise;
}

static void triggerHat() {
  hatEnvelope = 1.0f;
}

static float renderHat() {
  if (hatEnvelope <= 0.0005f) {
    hatEnvelope = 0.0f;
    return 0.0f;
  }
  const float white = noiseSample();
  const float high = white - hatHighpassState;
  hatHighpassState += (white - hatHighpassState) * 0.22f;
  const float out = high * hatEnvelope * 0.13f;
  hatEnvelope *= 0.9945f;
  return out;
}

static DrumVoice hatVoice = { triggerHat, renderHat };
static DrumVoice snareVoice = { triggerSnare, renderSnare };
static DrumVoice kickVoice = { triggerKick, renderKick };

static void triggerStep(uint32_t step) {
  if (kickPattern[step]) kickVoice.trigger();
  if (snarePattern[step]) snareVoice.trigger();
  if (hatPattern[step]) hatVoice.trigger();
}

static void renderAudioBlock() {
  const uint64_t samplesPerStep =
      (uint64_t)((60.0f * SAMPLE_RATE) / (bpm * 4.0f));

  for (uint16_t i = 0; i < BLOCK_FRAMES; ++i) {
    if (playing && (sampleCounter % samplesPerStep) == 0) {
      stepIndex = (uint32_t)((sampleCounter / samplesPerStep) % 16u);
      triggerStep(stepIndex);
    }

    float mix = kickVoice.render() + snareVoice.render() + hatVoice.render();
    mix = mix / (1.0f + 0.18f * fabsf(mix));
    const int16_t out = (int16_t)(clampAudio(mix) * 32767.0f);
    audioBuffer[i * 2] = out;
    audioBuffer[i * 2 + 1] = out;
    if (playing) ++sampleCounter;
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

static void pollEncoder() {
  // Prototype only: the encoder emulates step selection until the physical
  // 16 sequencer switches are connected. Final hardware will use one switch
  // per step, not this encoder workflow.
  const int a = digitalRead(PIN_ENCODER_A);
  if (a != lastEncoderA && a == LOW) {
    const int b = digitalRead(PIN_ENCODER_B);
    if (b != a) selectedStep = (selectedStep + 1) % 16;
    else selectedStep = (selectedStep + 15) % 16;
  }
  lastEncoderA = a;
}

static void pollEncoderButton() {
  const int reading = digitalRead(PIN_ENCODER_SW);
  const uint32_t now = millis();
  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChangeMs = now;
  }

  if ((uint32_t)(now - lastButtonChangeMs) < BUTTON_DEBOUNCE_MS) return;
  if (reading != stableButtonState) {
    stableButtonState = reading;
    if (stableButtonState == LOW) {
      buttonPressedAtMs = now;
      longPressHandled = false;
    } else if (!longPressHandled) {
      // Short press toggles the selected kick step.
      kickPattern[selectedStep] = !kickPattern[selectedStep];
      Serial.printf("KICK step %u: %s\\n", selectedStep + 1,
                    kickPattern[selectedStep] ? "ON" : "OFF");
    }
  }

  if (stableButtonState == LOW && !longPressHandled &&
      (uint32_t)(now - buttonPressedAtMs) >= LONG_PRESS_MS) {
    // Long press keeps transport available while short press edits the pattern.
    playing = !playing;
    if (playing) {
      sampleCounter = 0;
      stepIndex = 0;
      lastDrawnBeat = 255;
      Serial.println("Transport: PLAY");
    } else {
      Serial.println("Transport: STOP (active voices finish naturally)");
    }
    longPressHandled = true;
  }
}

static void updateStepEditorDisplay() {
  for (uint8_t i = 0; i < 16; ++i) {
    const int x = 16 + i * 13;
    const bool selected = (i == selectedStep);
    const bool active = kickPattern[i] != 0;
    if (selectedStep != lastDrawnSelectedStep &&
        (selected || i == lastDrawnSelectedStep)) {
      tft.drawRect(x, 228, 10, 10, selected ? ST77XX_YELLOW : ST77XX_WHITE);
    }
    if (active != (lastDrawnKickPattern[i] != 0)) {
      tft.fillRect(x + 2, 230, 6, 6, active ? ST77XX_GREEN : ST77XX_BLACK);
      lastDrawnKickPattern[i] = active ? 1 : 0;
    }
  }
  lastDrawnSelectedStep = selectedStep;
}

static void updateBpmDisplay() {
  if (bpm == lastDrawnBpm) return;
  tft.fillRect(20, 115, 200, 42, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(24, 119);
  tft.printf("%3.0f BPM", bpm);
  lastDrawnBpm = bpm;
}

static void updateBeatIndicator() {
  const uint8_t beat = (uint8_t)(stepIndex / 4u);
  if (beat == lastDrawnBeat) return;
  if (lastDrawnBeat < 4) {
    tft.fillRect(40 + lastDrawnBeat * 44, 227, 20, 6, ST77XX_BLACK);
  }
  tft.fillRect(40 + beat * 44, 227, 20, 6, ST77XX_WHITE);
  lastDrawnBeat = beat;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("BIPOLAB — Drum Engine Stage 9 (16-step kick editor prototype)");
  Serial.println("ESP32-S3 / PCM5102 | 44.1 kHz | encoder selects step; short press toggles kick; long press transport");
  Serial.println("Kick on every beat, snare on 2/4, closed hat on eighth notes.");

  // Encoder inputs: GPIO35/36 quadrature, GPIO37 pushbutton.
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  lastEncoderA = digitalRead(PIN_ENCODER_A);

  // Initial full-screen drawing happens before audio starts.
  drawStartupScreen();

  I2S.setPins(PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);
  const bool ok = I2S.begin(
      I2S_MODE_STD, SAMPLE_RATE,
      I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);

  if (!ok) {
    Serial.println("ERROR: I2S.begin failed. Check wiring and board package.");
    while (true) delay(1000);
  }
  Serial.println("I2S initialized. Transport starts in PLAY mode.");
}

static void updateTransportDisplay() {
  if (playing == lastDrawnPlaying) return;
  tft.fillRect(20, 148, 120, 18, ST77XX_BLACK);
  tft.setTextColor(playing ? ST77XX_GREEN : ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(24, 151);
  tft.print(playing ? "PLAYING" : "STOPPED");
  lastDrawnPlaying = playing;
}

void loop() {
  pollEncoder();
  pollEncoderButton();
  renderAudioBlock();
  updateBpmDisplay();
  updateTransportDisplay();
  // Only small display regions are redrawn during playback.
  updateStepEditorDisplay();
}

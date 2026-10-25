/*
  MacroSynth Drum Engine — Stage 11: high-contrast compact 240x240 UI
  Temporary prototype controls: rotate encoder to select a step, short press
  toggles the selected instrument at that step, hold to cycle instruments,
  and very long hold toggles transport. Final hardware will use sequencer
  switches directly, not the encoder.

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

static constexpr uint16_t UI_BG = ST77XX_BLACK;
static constexpr uint16_t UI_TEXT = ST77XX_WHITE;
static constexpr uint16_t UI_ACCENT = ST77XX_CYAN;
static constexpr uint16_t UI_SELECTED = ST77XX_YELLOW;
static constexpr uint16_t UI_ON = ST77XX_GREEN;
static constexpr uint16_t UI_DIM = 0x4208;
static constexpr int GRID_X = 55;
static constexpr int GRID_PITCH = 11;
static constexpr int GRID_W = 9;
static constexpr int GRID_H = 17;
static constexpr int GRID_Y[3] = {103, 135, 167};

static void drawStartupScreen() {
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
  SPI.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, -1);
  tft.init(240, 240, SPI_MODE2);
  tft.setSPISpeed(12000000);
  tft.invertDisplay(true);
  tft.setRotation(0);
  tft.fillScreen(UI_BG);
  tft.setTextWrap(false);

  tft.setTextColor(UI_ACCENT);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.print("MACROSYNTH");
  tft.drawFastHLine(10, 31, 220, UI_DIM);

  tft.setTextSize(3);
  tft.setTextColor(UI_TEXT);
  tft.setCursor(12, 41);
  tft.print("110");
  tft.setTextSize(2);
  tft.setCursor(74, 48);
  tft.print("BPM");
  tft.setTextSize(2);
  tft.setTextColor(UI_ON);
  tft.setCursor(148, 48);
  tft.print("PLAY");

  tft.drawFastHLine(10, 79, 220, UI_DIM);
  tft.setTextSize(1);
  tft.setTextColor(UI_DIM);
  for (uint8_t beat = 0; beat < 4; ++beat) {
    tft.setCursor(GRID_X + beat * 44 + 12, 87);
    tft.print(beat + 1);
  }
  for (uint8_t row = 0; row < 3; ++row) {
    tft.setTextSize(2);
    tft.setTextColor(UI_TEXT);
    tft.setCursor(5, GRID_Y[row] + 1);
    tft.print(row == 0 ? "BD" : (row == 1 ? "SD" : "HH"));
    for (uint8_t step = 0; step < 16; ++step) {
      const int x = GRID_X + step * GRID_PITCH;
      tft.drawRect(x, GRID_Y[row], GRID_W, GRID_H, UI_DIM);
    }
  }
  tft.drawFastHLine(10, 196, 220, UI_DIM);
  tft.setTextColor(UI_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 204);
  tft.print("TURN step   CLICK toggle");
  tft.setCursor(10, 216);
  tft.print("HOLD voice  LONG HOLD play");
  tft.setTextColor(UI_DIM);
  tft.setCursor(10, 230);
  tft.print("Prototype: switches later");
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
static constexpr uint32_t TRANSPORT_HOLD_MS = 1500;
static uint8_t selectedStep = 0;
static uint8_t lastDrawnSelectedStep = 255;
static uint8_t selectedInstrument = 0; // 0=Kick, 1=Snare, 2=Closed Hat
static uint8_t lastDrawnInstrument = 255;
static uint8_t lastDrawnPatterns[3][16] = {{0}};
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

static uint8_t snarePattern[16] = {
  0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0
};

static uint8_t hatPattern[16] = {
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

static uint8_t *patternForInstrument(uint8_t instrument) {
  if (instrument == 1) return snarePattern;
  if (instrument == 2) return hatPattern;
  return kickPattern;
}

static const char *instrumentName(uint8_t instrument) {
  if (instrument == 1) return "SNARE";
  if (instrument == 2) return "CLOSED HAT";
  return "KICK";
}

static void pollEncoderButton() {
  const int reading = digitalRead(PIN_ENCODER_SW);
  const uint32_t now = millis();
  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    lastButtonChangeMs = now;
  }

  if ((uint32_t)(now - lastButtonChangeMs) >= BUTTON_DEBOUNCE_MS &&
      reading != stableButtonState) {
    stableButtonState = reading;
    if (stableButtonState == LOW) {
      buttonPressedAtMs = now;
      longPressHandled = false;
    } else {
      const uint32_t heldMs = now - buttonPressedAtMs;
      if (!longPressHandled && heldMs >= LONG_PRESS_MS) {
        selectedInstrument = (selectedInstrument + 1) % 3;
        Serial.printf("Selected instrument: %s\\n", instrumentName(selectedInstrument));
      } else if (!longPressHandled) {
        uint8_t *pattern = patternForInstrument(selectedInstrument);
        pattern[selectedStep] = !pattern[selectedStep];
        Serial.printf("%s step %u: %s\\n", instrumentName(selectedInstrument),
                      selectedStep + 1, pattern[selectedStep] ? "ON" : "OFF");
      }
    }
  }

  // A very long hold stops/starts transport; mark it handled so release
  // does not also change the selected instrument or a pattern step.
  if (stableButtonState == LOW && !longPressHandled &&
      (uint32_t)(now - buttonPressedAtMs) >= TRANSPORT_HOLD_MS) {
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
  uint8_t *patterns[3] = { kickPattern, snarePattern, hatPattern };
  const bool selectionChanged = selectedStep != lastDrawnSelectedStep ||
                                selectedInstrument != lastDrawnInstrument;
  for (uint8_t row = 0; row < 3; ++row) {
    if (selectedInstrument != lastDrawnInstrument) {
      tft.fillRect(4, GRID_Y[row], 45, GRID_H + 1, UI_BG);
      tft.setTextSize(2);
      tft.setTextColor(row == selectedInstrument ? UI_SELECTED : UI_TEXT);
      tft.setCursor(5, GRID_Y[row] + 1);
      tft.print(row == 0 ? "BD" : (row == 1 ? "SD" : "HH"));
    }
    for (uint8_t step = 0; step < 16; ++step) {
      const bool active = patterns[row][step] != 0;
      const bool changed = active != (lastDrawnPatterns[row][step] != 0);
      const bool affectedSelection = selectionChanged &&
        (step == selectedStep || step == lastDrawnSelectedStep);
      if (!changed && !affectedSelection && lastDrawnInstrument != 255) continue;
      const int x = GRID_X + step * GRID_PITCH;
      const uint16_t border = (row == selectedInstrument && step == selectedStep)
                              ? UI_SELECTED : UI_DIM;
      tft.drawRect(x, GRID_Y[row], GRID_W, GRID_H, border);
      tft.fillRect(x + 2, GRID_Y[row] + 2, GRID_W - 4, GRID_H - 4,
                   active ? UI_ON : UI_BG);
      lastDrawnPatterns[row][step] = active;
    }
  }
  lastDrawnSelectedStep = selectedStep;
  lastDrawnInstrument = selectedInstrument;
}

static void updateBpmDisplay() {
  if (bpm == lastDrawnBpm) return;
  tft.fillRect(10, 40, 63, 32, UI_BG);
  tft.setTextColor(UI_TEXT);
  tft.setTextSize(3);
  tft.setCursor(12, 41);
  tft.printf("%3.0f", bpm);
  lastDrawnBpm = bpm;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("BIPOLAB — Drum Engine Stage 11 (high contrast 240x240 sequencer UI)");
  Serial.println("ESP32-S3 / PCM5102 | 44.1 kHz | encoder selects step; click toggles instrument; hold cycles instrument; long hold transport");
  Serial.println("Initial pattern: kick on beats, snare on 2/4, closed hat on eighth notes.");

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
  tft.fillRect(145, 43, 90, 27, UI_BG);
  tft.setTextColor(playing ? UI_ON : ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(148, 48);
  tft.print(playing ? "PLAY" : "STOP");
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

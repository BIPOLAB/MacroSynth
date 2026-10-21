#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <ESP_I2S.h>
#include <math.h>

// ============================================================
// BIPOLAB SYNTH
// ESP32-S3 + PCM5102 + ST7789 + encoder
// ============================================================

// -------------------- CONFIGURACION TFT ---------------------

#define TFT_CS    -1
#define TFT_DC    13
#define TFT_RST   10
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BLK   14

#define ENCODER_CLK 35
#define ENCODER_DT  36
#define ENCODER_SW  37

// -------------------- CONFIGURACION I2S ---------------------

#define I2S_BCLK 16
#define I2S_LRCK 18
#define I2S_DOUT 8

#define SAMPLE_RATE 44100
#define AUDIO_FRAMES 128

// -------------------- COLORES RGB565 ------------------------

#define UI_BG       0x0000
#define UI_WHITE    0xFFFF
#define UI_GRAY     0x8410
#define UI_DARKGRAY 0x4208
#define UI_ACCENT   0xFD20

// -------------------- OBJETOS -------------------------------

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);
I2SClass I2S;

// -------------------- MOTORES -------------------------------

enum EngineType {
  ENGINE_ANALOG = 0,
  ENGINE_FM,
  ENGINE_WAVETABLE,
  ENGINE_PLUCK,
  ENGINE_MODAL,
  ENGINE_NOISE,
  ENGINE_CHORD,
  ENGINE_TEXTURE,
  ENGINE_COUNT
};

const char *engineNames[ENGINE_COUNT] = {
  "ANALOG",
  "FM",
  "WAVETABLE",
  "PLUCK",
  "MODAL",
  "NOISE",
  "CHORD",
  "TEXTURE"
};

const char *engineDescriptions[ENGINE_COUNT] = {
  "Classic oscillator",
  "Frequency modulation",
  "Harmonic waveform",
  "Plucked string",
  "Resonant modes",
  "Noise generator",
  "Chord oscillator",
  "Noise texture"
};

// -------------------- PANTALLAS -----------------------------

enum ScreenState {
  SCREEN_MAIN,
  SCREEN_ENGINE_MENU,
  SCREEN_PARAMETER_MENU,
  SCREEN_EDIT_PARAMETER
};

ScreenState screenState = SCREEN_MAIN;

enum ParameterType {
  PARAM_FREQUENCY = 0,
  PARAM_BRIGHTNESS,
  PARAM_VOLUME,
  PARAMETER_COUNT
};

const char *parameterNames[PARAMETER_COUNT] = {
  "FREQUENCY",
  "BRIGHTNESS",
  "VOLUME"
};

// -------------------- EVENTOS DEL BOTON ---------------------

enum ButtonEvent {
  BUTTON_NONE,
  BUTTON_SHORT,
  BUTTON_LONG
};

// -------------------- ESTADO DEL SINTETIZADOR ----------------

int currentEngine = ENGINE_ANALOG;

float baseFrequency = 110.0f;
float brightness = 0.45f;
float masterVolume = 0.20f;

bool modulationEnabled = true;

int menuSelection = 0;
int parameterSelection = 0;
ParameterType editingParameter = PARAM_FREQUENCY;

bool displayDirty = true;
bool audioReady = false;

// -------------------- ESTADO DEL AUDIO ----------------------

float phase1 = 0.0f;
float phase2 = 0.0f;
float phase3 = 0.0f;

float envelope = 0.0f;
float filterState = 0.0f;

uint32_t noiseState = 0x12345678;

int16_t audioBuffer[AUDIO_FRAMES * 2];

// -------------------- ESTADO DEL ENCODER --------------------

int previousEncoderState = 0;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

uint32_t lastButtonChange = 0;
uint32_t buttonPressTime = 0;

bool longPressSent = false;

const uint32_t BUTTON_DEBOUNCE_MS = 25;
const uint32_t LONG_PRESS_MS = 650;

// ============================================================
// UTILIDADES
// ============================================================

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

// ============================================================
// MOTOR DE SINTESIS
// ============================================================

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

// ============================================================
// GENERACION Y SALIDA DE AUDIO
// ============================================================

void processAudioBlock() {
  for (int i = 0; i < AUDIO_FRAMES; i++) {
    float sample = generateSample();

    sample *= masterVolume;
    sample = clampFloat(sample, -1.0f, 1.0f);

    int16_t output = (int16_t)(sample * 32767.0f);

    audioBuffer[i * 2] = output;
    audioBuffer[i * 2 + 1] = output;
  }

  if (audioReady) {
    I2S.write((uint8_t *)audioBuffer, sizeof(audioBuffer));
  }
}

// ============================================================
// ENCODER
// ============================================================

int readEncoderDelta() {
  int clk = digitalRead(ENCODER_CLK);
  int dt = digitalRead(ENCODER_DT);

  int currentState = (clk << 1) | dt;

  int delta = 0;

  static const int8_t transitionTable[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  static int encoderAccumulation = 0;

  int index = (previousEncoderState << 2) | currentState;

  encoderAccumulation += transitionTable[index];

  if (encoderAccumulation >= 4) {
    delta = 1;
    encoderAccumulation = 0;
  } else if (encoderAccumulation <= -4) {
    delta = -1;
    encoderAccumulation = 0;
  }

  previousEncoderState = currentState;

  return delta;
}

ButtonEvent readButtonEvent() {
  ButtonEvent event = BUTTON_NONE;

  bool reading = digitalRead(ENCODER_SW);
  uint32_t now = millis();

  if (reading != lastButtonReading) {
    lastButtonChange = now;
    lastButtonReading = reading;
  }

  if ((now - lastButtonChange) > BUTTON_DEBOUNCE_MS) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        buttonPressTime = now;
        longPressSent = false;
      } else {
        if (!longPressSent) {
          event = BUTTON_SHORT;
        }
      }
    }

    if (stableButtonState == LOW &&
        !longPressSent &&
        (now - buttonPressTime) >= LONG_PRESS_MS) {
      longPressSent = true;
      event = BUTTON_LONG;
    }
  }

  return event;
}

// ============================================================
// CONTROL DE PARAMETROS
// ============================================================

void changeParameter(int direction) {
  switch (editingParameter) {

    case PARAM_FREQUENCY:
      baseFrequency += direction * 5.0f;
      baseFrequency = clampFloat(baseFrequency, 20.0f, 2000.0f);
      break;

    case PARAM_BRIGHTNESS:
      brightness += direction * 0.03f;
      brightness = clampFloat(brightness, 0.0f, 1.0f);
      break;

    case PARAM_VOLUME:
      masterVolume += direction * 0.02f;
      masterVolume = clampFloat(masterVolume, 0.0f, 0.80f);
      break;

    default:
      break;
  }

  displayDirty = true;
}

void selectCurrentMenuItem() {
  switch (screenState) {

    case SCREEN_MAIN:
      screenState = SCREEN_ENGINE_MENU;
      menuSelection = currentEngine;
      break;

    case SCREEN_ENGINE_MENU:
      currentEngine = menuSelection;
      screenState = SCREEN_MAIN;
      envelope = 0.15f;
      break;

    case SCREEN_PARAMETER_MENU:
      editingParameter = (ParameterType)parameterSelection;
      screenState = SCREEN_EDIT_PARAMETER;
      break;

    case SCREEN_EDIT_PARAMETER:
      screenState = SCREEN_PARAMETER_MENU;
      break;
  }

  displayDirty = true;
}

void handleButton(ButtonEvent event) {
  if (event == BUTTON_SHORT) {
    selectCurrentMenuItem();
  }

  if (event == BUTTON_LONG) {
    if (screenState == SCREEN_MAIN) {
      screenState = SCREEN_PARAMETER_MENU;
      parameterSelection = 0;
    } else {
      screenState = SCREEN_MAIN;
    }

    displayDirty = true;
  }
}

void handleEncoder() {
  int delta = readEncoderDelta();

  if (delta == 0) return;

  switch (screenState) {

    case SCREEN_MAIN:
      currentEngine += delta;

      if (currentEngine < 0) currentEngine = ENGINE_COUNT - 1;
      if (currentEngine >= ENGINE_COUNT) currentEngine = 0;

      envelope = 0.15f;
      break;

    case SCREEN_ENGINE_MENU:
      menuSelection += delta;

      if (menuSelection < 0) menuSelection = ENGINE_COUNT - 1;
      if (menuSelection >= ENGINE_COUNT) menuSelection = 0;
      break;

    case SCREEN_PARAMETER_MENU:
      parameterSelection += delta;

      if (parameterSelection < 0) parameterSelection = PARAMETER_COUNT - 1;
      if (parameterSelection >= PARAMETER_COUNT) parameterSelection = 0;
      break;

    case SCREEN_EDIT_PARAMETER:
      changeParameter(delta);
      break;
  }

  displayDirty = true;
}

// ============================================================
// INTERFAZ: UTILIDADES
// ============================================================

void drawHeader(const char *title) {
  tft.fillRect(0, 0, 240, 34, UI_BG);

  tft.setTextSize(2);
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(10, 8);
  tft.print(title);

  tft.fillRect(0, 33, 240, 2, UI_ACCENT);
}

void drawFooter(const char *text) {
  tft.fillRect(0, 216, 240, 24, UI_BG);

  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(10, 224);
  tft.print(text);
}

void drawMenuRow(int y, const char *label, bool selected) {
  uint16_t bg = selected ? UI_ACCENT : UI_BG;
  uint16_t fg = selected ? UI_BG : UI_WHITE;

  tft.fillRoundRect(8, y, 224, 34, 4, bg);

  tft.setTextSize(2);
  tft.setTextColor(fg, bg);
  tft.setCursor(18, y + 9);
  tft.print(label);

  if (selected) {
    tft.fillRect(8, y, 4, 34, UI_WHITE);
  }
}

// ============================================================
// INTERFAZ: PANTALLA PRINCIPAL
// ============================================================

void drawMainScreen() {
  tft.fillScreen(UI_BG);

  drawHeader("BIPOLAB SYNTH");

  // Motor activo
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(12, 47);
  tft.print("ENGINE");

  tft.setTextSize(3);
  tft.setTextColor(UI_ACCENT, UI_BG);
  tft.setCursor(10, 61);
  tft.print(engineNames[currentEngine]);

  // Separador
  tft.drawFastHLine(10, 101, 220, UI_DARKGRAY);

  // Frecuencia
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(12, 112);
  tft.print("FREQUENCY");

  tft.setTextSize(2);
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(12, 127);
  tft.print((int)baseFrequency);
  tft.print(" Hz");

  // Brillo
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(132, 112);
  tft.print("BRIGHTNESS");

  tft.setTextSize(2);
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(132, 127);
  tft.print((int)(brightness * 100.0f));
  tft.print("%");

  // Barra de brillo
  tft.drawRect(132, 153, 96, 7, UI_GRAY);
  tft.fillRect(134, 155, (int)(92.0f * brightness), 3, UI_ACCENT);

  // Modulación
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(12, 171);
  tft.print("MOD");

  tft.setTextSize(2);
  tft.setTextColor(modulationEnabled ? UI_ACCENT : UI_GRAY, UI_BG);
  tft.setCursor(45, 167);
  tft.print(modulationEnabled ? "ON" : "OFF");

  // Volumen
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(132, 171);
  tft.print("VOLUME");

  tft.setTextSize(2);
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(132, 185);
  tft.print((int)(masterVolume * 100.0f));
  tft.print("%");

  drawFooter("CLICK: ENGINES   HOLD: PARAMETERS");
}

// ============================================================
// INTERFAZ: MENU DE MOTORES
// ============================================================

void drawEngineMenu() {
  tft.fillScreen(UI_BG);
  drawHeader("SELECT ENGINE");

  const int visibleRows = 4;
  int firstItem = 0;

  if (menuSelection >= visibleRows) {
    firstItem = menuSelection - visibleRows + 1;
  }

  for (int row = 0; row < visibleRows; row++) {
    int item = firstItem + row;

    if (item >= ENGINE_COUNT) break;

    drawMenuRow(
      43 + row * 40,
      engineNames[item],
      item == menuSelection
    );
  }

  drawFooter("TURN: SELECT   CLICK: LOAD");
}

// ============================================================
// INTERFAZ: MENU DE PARAMETROS
// ============================================================

void drawParameterMenu() {
  tft.fillScreen(UI_BG);
  drawHeader("PARAMETERS");

  for (int i = 0; i < PARAMETER_COUNT; i++) {
    drawMenuRow(
      47 + i * 48,
      parameterNames[i],
      i == parameterSelection
    );
  }

  drawFooter("CLICK: EDIT   HOLD: BACK");
}

// ============================================================
// INTERFAZ: EDICION DE PARAMETRO
// ============================================================

void drawParameterEdit() {
  tft.fillScreen(UI_BG);
  drawHeader(parameterNames[editingParameter]);

  char valueText[24];

  switch (editingParameter) {
    case PARAM_FREQUENCY:
      snprintf(valueText, sizeof(valueText), "%d Hz", (int)baseFrequency);
      break;

    case PARAM_BRIGHTNESS:
      snprintf(valueText, sizeof(valueText), "%d%%", (int)(brightness * 100.0f));
      break;

    case PARAM_VOLUME:
      snprintf(valueText, sizeof(valueText), "%d%%", (int)(masterVolume * 100.0f));
      break;

    default:
      snprintf(valueText, sizeof(valueText), "--");
      break;
  }

  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setTextSize(3);

  int16_t x1, y1;
  uint16_t w, h;

  tft.getTextBounds(valueText, 0, 0, &x1, &y1, &w, &h);
  int x = (240 - (int)w) / 2;

  tft.setCursor(x, 88);
  tft.print(valueText);

  tft.drawRoundRect(15, 142, 210, 12, 4, UI_GRAY);

  float normalized = 0.0f;

  switch (editingParameter) {
    case PARAM_FREQUENCY:
      normalized = (baseFrequency - 20.0f) / (2000.0f - 20.0f);
      break;

    case PARAM_BRIGHTNESS:
      normalized = brightness;
      break;

    case PARAM_VOLUME:
      normalized = masterVolume / 0.80f;
      break;

    default:
      break;
  }

  normalized = clampFloat(normalized, 0.0f, 1.0f);

  int barWidth = (int)(204.0f * normalized);

  if (barWidth > 0) {
    tft.fillRoundRect(18, 145, barWidth, 6, 2, UI_ACCENT);
  }

  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(22, 174);
  tft.print("TURN TO CHANGE");

  tft.setCursor(22, 190);
  tft.print("CLICK TO RETURN");

  drawFooter("BIPOLAB / SYNTH ENGINE");
}

// ============================================================
// DIBUJADO DE PANTALLAS
// ============================================================

void drawCurrentScreen() {
  switch (screenState) {
    case SCREEN_MAIN:
      drawMainScreen();
      break;

    case SCREEN_ENGINE_MENU:
      drawEngineMenu();
      break;

    case SCREEN_PARAMETER_MENU:
      drawParameterMenu();
      break;

    case SCREEN_EDIT_PARAMETER:
      drawParameterEdit();
      break;
  }

  displayDirty = false;
}

// ============================================================
// SERIAL
// ============================================================

void printHelp() {
  Serial.println();
  Serial.println("===== BIPOLAB SYNTH =====");
  Serial.println("1-8 : seleccionar motor");
  Serial.println("+/- : brillo");
  Serial.println("[/] : frecuencia");
  Serial.println("v/V : volumen -/+");
  Serial.println("m   : activar/desactivar modulacion");
  Serial.println("p   : menu de parametros");
  Serial.println("?   : mostrar ayuda");
  Serial.println("=========================");
  Serial.println();
}

void handleSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c >= '1' && c <= '8') {
      currentEngine = c - '1';
      envelope = 0.15f;
      displayDirty = true;
    }

    switch (c) {
      case '+':
        brightness = clampFloat(brightness + 0.03f, 0.0f, 1.0f);
        displayDirty = true;
        break;

      case '-':
        brightness = clampFloat(brightness - 0.03f, 0.0f, 1.0f);
        displayDirty = true;
        break;

      case '[':
        baseFrequency = clampFloat(baseFrequency - 5.0f, 20.0f, 2000.0f);
        displayDirty = true;
        break;

      case ']':
        baseFrequency = clampFloat(baseFrequency + 5.0f, 20.0f, 2000.0f);
        displayDirty = true;
        break;

      case 'v':
        masterVolume = clampFloat(masterVolume - 0.02f, 0.0f, 0.80f);
        displayDirty = true;
        break;

      case 'V':
        masterVolume = clampFloat(masterVolume + 0.02f, 0.0f, 0.80f);
        displayDirty = true;
        break;

      case 'm':
      case 'M':
        modulationEnabled = !modulationEnabled;
        displayDirty = true;
        break;

      case 'p':
      case 'P':
        screenState = SCREEN_PARAMETER_MENU;
        parameterSelection = 0;
        displayDirty = true;
        break;

      case '?':
        printHelp();
        break;
    }
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  previousEncoderState =
    (digitalRead(ENCODER_CLK) << 1) |
     digitalRead(ENCODER_DT);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, -1);

  tft.init(240, 240, SPI_MODE2);
  tft.setSPISpeed(12000000);
  tft.invertDisplay(true);
  tft.setRotation(3);

  tft.fillScreen(UI_BG);

  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setTextSize(2);
  tft.setCursor(18, 90);
  tft.print("BIPOLAB");

  tft.setTextColor(UI_ACCENT, UI_BG);
  tft.setCursor(18, 118);
  tft.print("SYNTH");

  I2S.setPins(I2S_BCLK, I2S_LRCK, I2S_DOUT);

  audioReady = I2S.begin(
    I2S_MODE_STD,
    SAMPLE_RATE,
    I2S_DATA_BIT_WIDTH_16BIT,
    I2S_SLOT_MODE_STEREO
  );

  if (!audioReady) {
    Serial.println("ERROR: no se pudo iniciar I2S.");
  } else {
    Serial.println("I2S iniciado correctamente.");
  }

  printHelp();

  displayDirty = true;
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  // Mantiene activo el flujo de audio.
  processAudioBlock();

  // Controles físicos.
  handleEncoder();

  ButtonEvent buttonEvent = readButtonEvent();

  if (buttonEvent != BUTTON_NONE) {
    handleButton(buttonEvent);
  }

  // Controles alternativos por Serial.
  handleSerial();

  // Actualizar la pantalla solamente cuando cambia algo.
  if (displayDirty) {
    drawCurrentScreen();
  }
}
#include "Display.h"
#include "Synth.h"
Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);
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


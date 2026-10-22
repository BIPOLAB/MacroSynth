#include "Display.h"
#include "Synth.h"
#include "Menu.h"
#include <stdio.h>
#include <string.h>

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

namespace {
const char *drumModelNames[DRUM_MODEL_COUNT] = {"KICK", "SNARE", "CLOSED HAT", "OPEN HAT", "CLAP", "TOM", "RIM", "METAL"};
const char *transientNames[DRUM_TRANSIENT_COUNT] = {"NONE", "CLICK", "NOISE", "CRACK"};
const char *bodyWaveNames[4] = {"SINE", "SOFT", "ASYM", "FM"};
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
  tft.setCursor(8, 224);
  tft.print(text);
}

void drawMenuRow(int y, const char *label, bool selected, const char *right = nullptr) {
  const uint16_t bg = selected ? UI_ACCENT : UI_BG;
  const uint16_t fg = selected ? UI_BG : UI_WHITE;
  tft.fillRoundRect(8, y, 224, 34, 4, bg);
  tft.setTextSize(2);
  tft.setTextColor(fg, bg);
  tft.setCursor(17, y + 9);
  tft.print(label);
  if (right) {
    int16_t x1, y1; uint16_t w, h;
    tft.getTextBounds(right, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(222 - (int)w, y + 10);
    tft.print(right);
  }
}

void drawRows(const char *title, const char **items, int count, int selected, const char *footer) {
  tft.fillScreen(UI_BG);
  drawHeader(title);
  const int visibleRows = 4;
  int first = selected >= visibleRows ? selected - visibleRows + 1 : 0;
  for (int row = 0; row < visibleRows && first + row < count; ++row) {
    const int item = first + row;
    drawMenuRow(42 + row * 42, items[item], item == selected);
  }
  drawFooter(footer);
}

void drawMainScreen() {
  const char *items[] = {"PLAY", "SOUND", "PRESETS", "SYSTEM"};
  drawRows(soundDirty ? "MACROSYNTH *" : "MACROSYNTH", items, 4, menuSelection, "TURN: SELECT   CLICK: OPEN");
}

void drawSoundMenu() {
  const char *items[] = {"ENGINE", "PARAMETERS", "MODULATION", "TEST KICK", "BACK"};
  drawRows("SOUND", items, 5, menuSelection, "CLICK: SELECT   HOLD: BACK");
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(12, 204);
  tft.print("ACTIVE: ");
  tft.setTextColor(UI_ACCENT, UI_BG);
  tft.print(engineNames[currentEngine]);
}

void drawEngineMenu() {
  tft.fillScreen(UI_BG);
  drawHeader("ENGINE");
  const int visibleRows = 4;
  const int first = menuSelection >= visibleRows ? menuSelection - visibleRows + 1 : 0;
  for (int row = 0; row < visibleRows && first + row < ENGINE_COUNT; ++row) {
    const int item = first + row;
    drawMenuRow(42 + row * 42, engineNames[item], item == menuSelection);
  }
  drawFooter("CLICK: CHOOSE   HOLD: BACK");
}

void drawParameterMenu() {
  tft.fillScreen(UI_BG);
  drawHeader("SOUND PARAMETERS");
  const int visibleRows = 4;
  const int first = parameterSelection >= visibleRows ? parameterSelection - visibleRows + 1 : 0;
  for (int row = 0; row < visibleRows && first + row < PARAMETER_COUNT; ++row) {
    const int item = first + row;
    drawMenuRow(42 + row * 42, parameterNames[item], item == parameterSelection);
  }
  drawFooter("CLICK: EDIT   HOLD: BACK");
}

void drawParameterEdit() {
  tft.fillScreen(UI_BG);
  drawHeader(parameterNames[editingParameter]);
  char value[32] = {};
  switch (editingParameter) {
    case PARAM_FREQUENCY: snprintf(value, sizeof(value), "%.1f Hz", baseFrequency); break;
    case PARAM_BRIGHTNESS: snprintf(value, sizeof(value), "%d%%", (int)(brightness * 100.0f)); break;
    case PARAM_VOLUME: snprintf(value, sizeof(value), "%d%%", (int)((currentEngine == ENGINE_KICK ? activeSoundPreset.level : masterVolume) * 100.0f)); break;
    case PARAM_AMP_ATTACK: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.amplitudeEnvelope.attackMs); break;
    case PARAM_AMP_DECAY: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.amplitudeEnvelope.decayMs); break;
    case PARAM_AMP_SUSTAIN: snprintf(value, sizeof(value), "%d%%", (int)(activeSoundPreset.amplitudeEnvelope.sustain * 100.0f)); break;
    case PARAM_AMP_RELEASE: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.amplitudeEnvelope.releaseMs); break;
    case PARAM_PITCH_AMOUNT: snprintf(value, sizeof(value), "%.1f st", activeSoundPreset.pitchEnvelope.amountSemitones); break;
    case PARAM_PITCH_DECAY: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.pitchEnvelope.decayMs); break;
    case PARAM_DRUM_MODEL: snprintf(value, sizeof(value), "%s", drumModelNames[activeSoundPreset.drum.model]); break;
    case PARAM_SWEEP_TIME: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.drum.sweepTimeMs); break;
    case PARAM_SWEEP_DEPTH: snprintf(value, sizeof(value), "%.1f st", activeSoundPreset.drum.sweepDepthSemitones); break;
    case PARAM_ALGORITHM: snprintf(value, sizeof(value), "%u", activeSoundPreset.drum.algorithm); break;
    case PARAM_BODY_WAVE: snprintf(value, sizeof(value), "%s", bodyWaveNames[activeSoundPreset.drum.bodyWave]); break;
    case PARAM_RATIO_A: snprintf(value, sizeof(value), "%.3f", activeSoundPreset.drum.ratioA); break;
    case PARAM_RATIO_B: snprintf(value, sizeof(value), "%.3f", activeSoundPreset.drum.ratioB); break;
    case PARAM_MOD_A: snprintf(value, sizeof(value), "%.2f", activeSoundPreset.drum.modulationA); break;
    case PARAM_MOD_B: snprintf(value, sizeof(value), "%.2f", activeSoundPreset.drum.modulationB); break;
    case PARAM_FEEDBACK: snprintf(value, sizeof(value), "%d%%", (int)(activeSoundPreset.drum.feedback * 100)); break;
    case PARAM_WAVEFOLD: snprintf(value, sizeof(value), "%d%%", (int)(activeSoundPreset.drum.wavefold * 100)); break;
    case PARAM_BODY_HOLD: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.drum.bodyHoldMs); break;
    case PARAM_BODY_DECAY: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.drum.bodyDecayMs); break;
    case PARAM_BODY_LEVEL: snprintf(value, sizeof(value), "%d%%", (int)(activeSoundPreset.drum.bodyLevel * 100)); break;
    case PARAM_NOISE_HOLD: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.drum.noiseHoldMs); break;
    case PARAM_NOISE_DECAY: snprintf(value, sizeof(value), "%.1f ms", activeSoundPreset.drum.noiseDecayMs); break;
    case PARAM_NOISE_LEVEL: snprintf(value, sizeof(value), "%d%%", (int)(activeSoundPreset.drum.noiseLevel * 100)); break;
    case PARAM_TRANSIENT_TYPE: snprintf(value, sizeof(value), "%s", transientNames[activeSoundPreset.drum.transientType]); break;
    case PARAM_TRANSIENT_LEVEL: snprintf(value, sizeof(value), "%d%%", (int)(activeSoundPreset.drum.transientLevel * 100)); break;
    case PARAM_NOISE_BASE: snprintf(value, sizeof(value), "%.0f Hz", activeSoundPreset.drum.noiseBaseHz); break;
    case PARAM_NOISE_WIDTH: snprintf(value, sizeof(value), "%.0f Hz", activeSoundPreset.drum.noiseWidthHz); break;
    case PARAM_NOISE_RESET: snprintf(value, sizeof(value), "%s", activeSoundPreset.drum.noiseReset ? "ON" : "OFF"); break;
    case PARAM_NOISE_RINGMOD: snprintf(value, sizeof(value), "%s", activeSoundPreset.drum.noiseRingMod ? "ON" : "OFF"); break;
    default: snprintf(value, sizeof(value), "--"); break;
  }
  tft.setTextSize(3);
  tft.setTextColor(UI_ACCENT, UI_BG);
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(value, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - (int)w) / 2, 93);
  tft.print(value);
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(22, 150);
  tft.print("TURN ENCODER TO CHANGE");
  tft.setCursor(22, 170);
  tft.print("CLICK TO FINISH");
  tft.setCursor(22, 190);
  tft.setTextColor(soundDirty ? UI_ACCENT : UI_GRAY, UI_BG);
  tft.print(soundDirty ? "UNSAVED CHANGES *" : "PRESET UNCHANGED");
  drawFooter("HOLD: BACK TO PARAMETERS");
}

void drawPresetBrowser() {
  tft.fillScreen(UI_BG);
  drawHeader("SOUND PRESETS");
  if (presetCount == 0) {
    tft.setTextSize(2);
    tft.setTextColor(UI_GRAY, UI_BG);
    tft.setCursor(20, 86);
    tft.print("NO PRESETS");
    tft.setTextSize(1);
    tft.setCursor(20, 116);
    tft.print("Choose Save As from SOUND.");
  } else {
    const int visibleRows = 4;
    const int first = presetSelection >= visibleRows ? presetSelection - visibleRows + 1 : 0;
    for (int row = 0; row < visibleRows && first + row < presetCount; ++row) {
      const int item = first + row;
      drawMenuRow(42 + row * 42, presetNames[item], item == presetSelection);
    }
  }
  drawFooter("CLICK: PRESET ACTIONS   HOLD: BACK");
}

void drawPresetActions() {
  const char *items[PRESET_ACTION_COUNT];
  for (int i = 0; i < PRESET_ACTION_COUNT; ++i) items[i] = presetActionNames[i];
  drawRows("PRESET ACTION", items, PRESET_ACTION_COUNT, presetActionSelection, "CLICK: CONFIRM   HOLD: BACK");
  if (presetCount > 0) {
    tft.setTextSize(1); tft.setTextColor(UI_GRAY, UI_BG);
    tft.setCursor(12, 204); tft.print(presetNames[presetSelection]);
  }
}

void drawNameEditor() {
  tft.fillScreen(UI_BG);
  drawHeader("SAVE AS");
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(12, 55);
  tft.print("TURN: CHANGE CHARACTER");
  tft.setCursor(12, 72);
  tft.print("CLICK: ACCEPT CHARACTER");
  const int nameStart = editNamePosition >= 18 ? editNamePosition - 17 : 0;
  tft.setTextSize(2);
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(12, 112);
  tft.print(editName + nameStart);
  tft.setTextColor(UI_ACCENT, UI_BG);
  const char current = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"[editCharIndex];
  tft.setCursor(12 + ((editNamePosition - nameStart) * 12), 145);
  tft.print(current == ' ' ? '_' : current);
  tft.setTextSize(1);
  tft.setTextColor(UI_GRAY, UI_BG);
  tft.setCursor(12, 184);
  tft.print("SELECT SPACE TO FINISH");
  drawFooter("HOLD: CANCEL");
}

void drawSystem() {
  tft.fillScreen(UI_BG);
  drawHeader("SYSTEM");
  tft.setTextSize(1);
  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setCursor(12, 54); tft.print("BIPOLAB MACROSYNTH MK0");
  tft.setCursor(12, 78); tft.print("SAMPLE RATE  44100 Hz");
  tft.setCursor(12, 96); tft.print("OUTPUT       PCM5102 / I2S");
  tft.setCursor(12, 114); tft.print("DISPLAY      ST7789 240x240");
  tft.setCursor(12, 132); tft.print("STORAGE      LITTLEFS");
  tft.setCursor(12, 150); tft.print("SERIAL       DEBUG ONLY");
  tft.setCursor(12, 178); tft.setTextColor(UI_ACCENT, UI_BG);
  tft.print(audioReady ? "AUDIO READY" : "AUDIO NOT READY");
  drawFooter("CLICK OR HOLD: BACK");
}
} // namespace

void drawCurrentScreen() {
  switch (screenState) {
    case SCREEN_MAIN: drawMainScreen(); break;
    case SCREEN_SOUND_MENU: drawSoundMenu(); break;
    case SCREEN_ENGINE_MENU: drawEngineMenu(); break;
    case SCREEN_PARAMETER_MENU: drawParameterMenu(); break;
    case SCREEN_EDIT_PARAMETER: drawParameterEdit(); break;
    case SCREEN_PRESET_BROWSER: drawPresetBrowser(); break;
    case SCREEN_PRESET_ACTIONS: drawPresetActions(); break;
    case SCREEN_NAME_EDITOR: drawNameEditor(); break;
    case SCREEN_SYSTEM: drawSystem(); break;
  }
  displayDirty = false;
}

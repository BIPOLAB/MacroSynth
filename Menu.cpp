#include "Menu.h"
#include "Controls.h"
#include "Synth.h"
#include "SoundPreset.h"
#include <string.h>
#include <stdio.h>

namespace {
const char NAME_CHARSET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
const int MAIN_MENU_COUNT = 4;
const int SOUND_MENU_COUNT = 5;

int wrapIndex(int value, int count) {
  if (count <= 0) return 0;
  while (value < 0) value += count;
  while (value >= count) value -= count;
  return value;
}

void syncPresetFromControls() {
  activeSoundPreset.schemaVersion = BIPO_SOUND_SCHEMA_VERSION;
  activeSoundPreset.engine = currentEngine == ENGINE_KICK ? SOUND_ENGINE_KICK : (SoundEngineId)currentEngine;
  activeSoundPreset.frequencyHz = baseFrequency;
  activeSoundPreset.brightness = brightness;
  activeSoundPreset.level = currentEngine == ENGINE_KICK ? activeSoundPreset.level : masterVolume;
  activeSoundPreset.modulationEnabled = modulationEnabled;
}

void loadSelectedPreset() {
  if (presetCount <= 0 || presetSelection < 0 || presetSelection >= presetCount) return;
  SoundPreset loaded;
  if (!loadSoundPreset(presetNames[presetSelection], loaded)) return;
  activeSoundPreset = loaded;
  currentEngine = loaded.engine == SOUND_ENGINE_KICK ? ENGINE_KICK : (int)loaded.engine;
  baseFrequency = loaded.frequencyHz;
  brightness = loaded.brightness;
  if (currentEngine != ENGINE_KICK) masterVolume = loaded.level;
  modulationEnabled = loaded.modulationEnabled;
  publishSoundPreset(activeSoundPreset);
  soundDirty = false;
  screenState = SCREEN_MAIN;
  menuSelection = 0;
}

bool saveCurrentPreset() {
  syncPresetFromControls();
  if (!validateSoundPreset(activeSoundPreset)) return false;
  if (!saveSoundPreset(activeSoundPreset)) return false;
  soundDirty = false;
  return true;
}

void beginSaveAs() {
  syncPresetFromControls();
  strncpy(editName, activeSoundPreset.name, sizeof(editName) - 1);
  editName[sizeof(editName) - 1] = '\\0';
  if (editName[0] == '\\0') strncpy(editName, "BIPO_SOUND", sizeof(editName) - 1);
  editNamePosition = 0;
  const char *found = strchr(NAME_CHARSET, editName[0]);
  editCharIndex = found ? (int)(found - NAME_CHARSET) : 1;
  screenState = SCREEN_NAME_EDITOR;
}

void acceptNameCharacter() {
  if (editNamePosition >= BIPO_SOUND_NAME_LENGTH - 1) {
    editName[BIPO_SOUND_NAME_LENGTH - 1] = '\\0';
    if (editName[0] != '\\0') {
      strncpy(activeSoundPreset.name, editName, sizeof(activeSoundPreset.name) - 1);
      activeSoundPreset.name[sizeof(activeSoundPreset.name) - 1] = '\\0';
      if (saveCurrentPreset()) screenState = SCREEN_PRESET_BROWSER;
      else screenState = SCREEN_PRESET_ACTIONS;
    }
    return;
  }
  const char selected = NAME_CHARSET[editCharIndex];
  if (selected == ' ') {
    editName[editNamePosition] = '\\0';
    if (editName[0] != '\\0') {
      strncpy(activeSoundPreset.name, editName, sizeof(activeSoundPreset.name) - 1);
      activeSoundPreset.name[sizeof(activeSoundPreset.name) - 1] = '\\0';
      if (saveCurrentPreset()) screenState = SCREEN_PRESET_BROWSER;
      else screenState = SCREEN_PRESET_ACTIONS;
    }
    return;
  }
  editName[editNamePosition] = selected;
  if (editNamePosition + 1 < BIPO_SOUND_NAME_LENGTH) editName[editNamePosition + 1] = '\\0';
  ++editNamePosition;
  const char *found = strchr(NAME_CHARSET, editName[editNamePosition]);
  editCharIndex = found ? (int)(found - NAME_CHARSET) : 1;
}

void openPresetActions() {
  presetActionSelection = 0;
  screenState = SCREEN_PRESET_ACTIONS;
}
}

void markSoundEdited() {
  syncPresetFromControls();
  soundDirty = true;
  publishSoundPreset(activeSoundPreset);
  displayDirty = true;
}

void refreshPresetNames() {
  presetCount = getSoundPresetNames(presetNames, 32);
  if (presetSelection >= presetCount) presetSelection = presetCount > 0 ? presetCount - 1 : 0;
  if (presetSelection < 0) presetSelection = 0;
}

void changeParameter(int direction) {
  switch (editingParameter) {
    case PARAM_FREQUENCY:
      baseFrequency = clampFloat(baseFrequency + direction * (currentEngine == ENGINE_KICK ? 1.0f : 5.0f), 20.0f, currentEngine == ENGINE_KICK ? 1200.0f : 8000.0f);
      break;
    case PARAM_BRIGHTNESS:
      brightness = clampFloat(brightness + direction * 0.03f, 0.0f, 1.0f);
      break;
    case PARAM_VOLUME:
      if (currentEngine == ENGINE_KICK) activeSoundPreset.level = clampFloat(activeSoundPreset.level + direction * 0.02f, 0.0f, 1.0f);
      else masterVolume = clampFloat(masterVolume + direction * 0.02f, 0.0f, 0.80f);
      break;
    case PARAM_AMP_ATTACK:
      activeSoundPreset.amplitudeEnvelope.attackMs = clampFloat(activeSoundPreset.amplitudeEnvelope.attackMs + direction * 0.5f, 0.0f, 10000.0f);
      break;
    case PARAM_AMP_DECAY:
      activeSoundPreset.amplitudeEnvelope.decayMs = clampFloat(activeSoundPreset.amplitudeEnvelope.decayMs + direction * 5.0f, 1.0f, 30000.0f);
      break;
    case PARAM_AMP_SUSTAIN:
      activeSoundPreset.amplitudeEnvelope.sustain = clampFloat(activeSoundPreset.amplitudeEnvelope.sustain + direction * 0.02f, 0.0f, 1.0f);
      break;
    case PARAM_AMP_RELEASE:
      activeSoundPreset.amplitudeEnvelope.releaseMs = clampFloat(activeSoundPreset.amplitudeEnvelope.releaseMs + direction * 5.0f, 0.0f, 30000.0f);
      break;
    case PARAM_PITCH_AMOUNT:
      activeSoundPreset.pitchEnvelope.amountSemitones = clampFloat(activeSoundPreset.pitchEnvelope.amountSemitones + direction, -48.0f, 48.0f);
      activeSoundPreset.pitchEnvelope.enabled = true;
      break;
    case PARAM_PITCH_DECAY:
      activeSoundPreset.pitchEnvelope.decayMs = clampFloat(activeSoundPreset.pitchEnvelope.decayMs + direction, 0.0f, 10000.0f);
      activeSoundPreset.pitchEnvelope.enabled = true;
      break;
    default: break;
  }
  markSoundEdited();
}

void selectCurrentMenuItem() {
  switch (screenState) {
    case SCREEN_MAIN:
      switch (menuSelection) {
        case 0:
          if (currentEngine == ENGINE_KICK) kickTriggerRequested = true;
          else screenState = SCREEN_SOUND_MENU;
          break;
        case 1: screenState = SCREEN_SOUND_MENU; menuSelection = 0; break;
        case 2: refreshPresetNames(); screenState = SCREEN_PRESET_BROWSER; presetSelection = 0; break;
        case 3: screenState = SCREEN_SYSTEM; break;
      }
      break;
    case SCREEN_SOUND_MENU:
      if (menuSelection == 0) { screenState = SCREEN_ENGINE_MENU; menuSelection = currentEngine; }
      else if (menuSelection == 1) { screenState = SCREEN_PARAMETER_MENU; parameterSelection = 0; }
      else if (menuSelection == 2) { modulationEnabled = !modulationEnabled; markSoundEdited(); }
      else if (menuSelection == 3) { kickTriggerRequested = true; }
      else { screenState = SCREEN_MAIN; menuSelection = 0; }
      break;
    case SCREEN_ENGINE_MENU:
      currentEngine = menuSelection;
      activeSoundPreset.engine = currentEngine == ENGINE_KICK ? SOUND_ENGINE_KICK : (SoundEngineId)currentEngine;
      if (currentEngine == ENGINE_KICK) {
        baseFrequency = activeSoundPreset.frequencyHz;
      }
      markSoundEdited();
      screenState = SCREEN_SOUND_MENU;
      menuSelection = 0;
      break;
    case SCREEN_PARAMETER_MENU:
      editingParameter = (ParameterType)parameterSelection;
      screenState = SCREEN_EDIT_PARAMETER;
      break;
    case SCREEN_EDIT_PARAMETER:
      screenState = SCREEN_PARAMETER_MENU;
      break;
    case SCREEN_PRESET_BROWSER:
      openPresetActions();
      break;
    case SCREEN_PRESET_ACTIONS:
      if (presetActionSelection == PRESET_LOAD) loadSelectedPreset();
      else if (presetActionSelection == PRESET_SAVE) {
        if (saveCurrentPreset()) screenState = SCREEN_MAIN;
      } else if (presetActionSelection == PRESET_SAVE_AS) beginSaveAs();
      else if (presetActionSelection == PRESET_DELETE) {
        if (presetCount > 0 && deleteSoundPreset(presetNames[presetSelection])) {
          refreshPresetNames();
          screenState = SCREEN_PRESET_BROWSER;
        }
      } else screenState = SCREEN_PRESET_BROWSER;
      break;
    case SCREEN_NAME_EDITOR:
      acceptNameCharacter();
      break;
    case SCREEN_SYSTEM:
      screenState = SCREEN_MAIN;
      menuSelection = 0;
      break;
  }
  displayDirty = true;
}

void handleButton(ButtonEvent event) {
  if (event == BUTTON_SHORT) {
    selectCurrentMenuItem();
    return;
  }
  if (event != BUTTON_LONG) return;
  if (screenState == SCREEN_MAIN) {
    screenState = SCREEN_SYSTEM;
  } else if (screenState == SCREEN_NAME_EDITOR) {
    screenState = SCREEN_PRESET_ACTIONS;
  } else if (screenState == SCREEN_EDIT_PARAMETER) {
    screenState = SCREEN_PARAMETER_MENU;
  } else if (screenState == SCREEN_PRESET_ACTIONS) {
    screenState = SCREEN_PRESET_BROWSER;
  } else if (screenState == SCREEN_PRESET_BROWSER || screenState == SCREEN_SYSTEM) {
    screenState = SCREEN_MAIN;
    menuSelection = 0;
  } else if (screenState == SCREEN_ENGINE_MENU || screenState == SCREEN_PARAMETER_MENU) {
    screenState = SCREEN_SOUND_MENU;
    menuSelection = 0;
  } else {
    screenState = SCREEN_MAIN;
    menuSelection = 0;
  }
  displayDirty = true;
}

void handleEncoder() {
  const int delta = readEncoderDelta();
  if (delta == 0) return;
  switch (screenState) {
    case SCREEN_MAIN: menuSelection = wrapIndex(menuSelection + delta, MAIN_MENU_COUNT); break;
    case SCREEN_SOUND_MENU: menuSelection = wrapIndex(menuSelection + delta, SOUND_MENU_COUNT); break;
    case SCREEN_ENGINE_MENU: menuSelection = wrapIndex(menuSelection + delta, ENGINE_COUNT); break;
    case SCREEN_PARAMETER_MENU: parameterSelection = wrapIndex(parameterSelection + delta, PARAMETER_COUNT); break;
    case SCREEN_EDIT_PARAMETER: changeParameter(delta); return;
    case SCREEN_PRESET_BROWSER:
      refreshPresetNames();
      if (presetCount > 0) presetSelection = wrapIndex(presetSelection + delta, presetCount);
      break;
    case SCREEN_PRESET_ACTIONS: presetActionSelection = wrapIndex(presetActionSelection + delta, PRESET_ACTION_COUNT); break;
    case SCREEN_NAME_EDITOR:
      editCharIndex = wrapIndex(editCharIndex + delta, (int)strlen(NAME_CHARSET));
      break;
    case SCREEN_SYSTEM: break;
  }
  displayDirty = true;
}

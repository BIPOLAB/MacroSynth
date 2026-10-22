#include "SerialControl.h"
#include "State.h"
#include "Synth.h"
#include "SoundPreset.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bool commandMode = false;
static char commandBuffer[96];
static size_t commandLength = 0;

static SoundEngineId selectedSoundEngine() {
  if (currentEngine == ENGINE_KICK) return SOUND_ENGINE_KICK;
  return (SoundEngineId)currentEngine;
}

static void runTextCommand(char *line) {
  char name[BIPO_SOUND_NAME_LENGTH] = {};
  if (strncmp(line, "save ", 5) == 0 && sscanf(line + 5, "%23s", name) == 1) {
    strncpy(activeSoundPreset.name, name, sizeof(activeSoundPreset.name) - 1);
    activeSoundPreset.name[sizeof(activeSoundPreset.name) - 1] = '\0';
    activeSoundPreset.engine = selectedSoundEngine();
    activeSoundPreset.frequencyHz = currentEngine == ENGINE_KICK
        ? activeSoundPreset.frequencyHz : baseFrequency;
    activeSoundPreset.brightness = brightness;
    activeSoundPreset.level = currentEngine == ENGINE_KICK
        ? activeSoundPreset.level : masterVolume;
    activeSoundPreset.modulationEnabled = modulationEnabled;
    if (saveSoundPreset(activeSoundPreset)) Serial.printf("Preset guardado: %s\n", name);
    else Serial.println("ERROR: no se pudo guardar. Usa nombre alfanumerico, _ o - (max. 23).");
  } else if (strncmp(line, "load ", 5) == 0 && sscanf(line + 5, "%23s", name) == 1) {
    SoundPreset loaded;
    if (loadSoundPreset(name, loaded)) {
      activeSoundPreset = loaded;
      currentEngine = loaded.engine == SOUND_ENGINE_KICK ? ENGINE_KICK : (int)loaded.engine;
      baseFrequency = loaded.frequencyHz;
      brightness = loaded.brightness;
      masterVolume = loaded.level;
      modulationEnabled = loaded.modulationEnabled;
      Serial.printf("Preset cargado: %s\n", loaded.name);
      displayDirty = true;
    } else Serial.println("ERROR: preset no encontrado, corrupto o incompatible.");
  } else if (strcmp(line, "list") == 0) {
    Serial.println("Presets de sonido:");
    listSoundPresets(Serial);
  } else if (strcmp(line, "kick") == 0) {
    currentEngine = ENGINE_KICK;
    kickTriggerRequested = true;
    displayDirty = true;
    Serial.println("BIPO Kick disparado.");
  } else if (strncmp(line, "env ", 4) == 0) {
    float a, d, s, r;
    if (sscanf(line + 4, "%f %f %f %f", &a, &d, &s, &r) == 4) {
      activeSoundPreset.amplitudeEnvelope = {a, d, s, r};
      if (validateSoundPreset(activeSoundPreset)) Serial.println("ADSR actualizado (usa :save NOMBRE para persistir).");
      else Serial.println("ADSR fuera de rango; valores permitidos: A 0-10000ms, D 0-30000ms, S 0-1, R 0-30000ms.");
    } else Serial.println("Uso: :env ATTACK_MS DECAY_MS SUSTAIN RELEASE_MS");
  } else if (strncmp(line, "pitch ", 6) == 0) {
    float amount, decay;
    if (sscanf(line + 6, "%f %f", &amount, &decay) == 2) {
      activeSoundPreset.pitchEnvelope.amountSemitones = amount;
      activeSoundPreset.pitchEnvelope.decayMs = decay;
      activeSoundPreset.pitchEnvelope.enabled = true;
      if (validateSoundPreset(activeSoundPreset)) Serial.println("Envolvente de tono actualizada (usa :save NOMBRE para persistir).");
      else Serial.println("Envolvente de tono fuera de rango: cantidad -48..48 semitonos; decay 0..10000ms.");
    } else Serial.println("Uso: :pitch SEMITONOS DECAY_MS");
  } else if (strncmp(line, "freq ", 5) == 0) {
    float value;
    if (sscanf(line + 5, "%f", &value) == 1 && value >= 20.0f && value <= 8000.0f) {
      activeSoundPreset.frequencyHz = value;
      baseFrequency = value;
      Serial.println("Frecuencia base actualizada.");
    } else Serial.println("Uso: :freq HZ (20..8000)");
  } else if (strncmp(line, "level ", 6) == 0) {
    float value;
    if (sscanf(line + 6, "%f", &value) == 1 && value >= 0.0f && value <= 1.0f) {
      activeSoundPreset.level = value;
      masterVolume = value;
      Serial.println("Nivel actualizado.");
    } else Serial.println("Uso: :level 0..1");
  } else if (strcmp(line, "help") == 0) {
    printHelp();
  } else if (line[0] != '\0') {
    Serial.println("Comando desconocido. Escribe :help");
  }
}

void printHelp() {
  Serial.println();
  Serial.println("===== BIPOLAB MACROSYNTH =====");
  Serial.println("1-8 motores base; 9 BIPO KICK");
  Serial.println("+/- brillo | [/] frecuencia | v/V volumen | m modulacion | p parametros");
  Serial.println("Comandos de linea (prefijo : y Enter):");
  Serial.println(":kick | :save NOMBRE | :load NOMBRE | :list");
  Serial.println(":env ATTACK_MS DECAY_MS SUSTAIN RELEASE_MS");
  Serial.println(":pitch SEMITONOS DECAY_MS | :freq HZ | :level 0..1 | :help");
  Serial.println("Los presets se guardan en LittleFS; no se formatea la memoria automaticamente.");
  Serial.println("==============================");
}

void handleSerial() {
  while (Serial.available() > 0) {
    const char c = (char)Serial.read();
    if (commandMode) {
      if (c == '\r') continue;
      if (c == '\n') {
        commandBuffer[commandLength] = '\0';
        runTextCommand(commandBuffer);
        commandLength = 0;
        commandMode = false;
      } else if (commandLength < sizeof(commandBuffer) - 1) {
        commandBuffer[commandLength++] = c;
      } else {
        commandLength = 0;
        commandMode = false;
        Serial.println("ERROR: comando demasiado largo.");
      }
      continue;
    }
    if (c == ':') {
      commandMode = true;
      commandLength = 0;
      continue;
    }
    if (c >= '1' && c <= '9') {
      currentEngine = c - '1';
      if (currentEngine >= ENGINE_COUNT) currentEngine = ENGINE_KICK;
      envelope = 0.15f;
      displayDirty = true;
    }
    switch (c) {
      case '+': brightness = clampFloat(brightness + 0.03f, 0.0f, 1.0f); displayDirty = true; break;
      case '-': brightness = clampFloat(brightness - 0.03f, 0.0f, 1.0f); displayDirty = true; break;
      case '[': baseFrequency = clampFloat(baseFrequency - 5.0f, 20.0f, 2000.0f); displayDirty = true; break;
      case ']': baseFrequency = clampFloat(baseFrequency + 5.0f, 20.0f, 2000.0f); displayDirty = true; break;
      case 'v': masterVolume = clampFloat(masterVolume - 0.02f, 0.0f, 0.80f); displayDirty = true; break;
      case 'V': masterVolume = clampFloat(masterVolume + 0.02f, 0.0f, 0.80f); displayDirty = true; break;
      case 'm': case 'M': modulationEnabled = !modulationEnabled; displayDirty = true; break;
      case 'p': case 'P': screenState = SCREEN_PARAMETER_MENU; parameterSelection = 0; displayDirty = true; break;
      case '?': printHelp(); break;
    }
  }
}

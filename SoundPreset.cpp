#include "SoundPreset.h"
#include <LittleFS.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace {
constexpr char SOUND_DIRECTORY[] = "/sounds";
constexpr char FILE_MAGIC[4] = {'B', 'P', 'S', '1'};

struct DiskHeader {
  char magic[4];
  uint16_t schemaVersion;
  uint16_t payloadSize;
  uint32_t checksum;
};

bool storageReady = false;

uint32_t checksumBytes(const uint8_t *data, size_t length) {
  // FNV-1a: corruption detection, not a cryptographic signature.
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < length; ++i) {
    hash ^= data[i];
    hash *= 16777619UL;
  }
  return hash;
}

bool validName(const char *name) {
  if (name == nullptr || name[0] == '\0') return false;
  size_t length = 0;
  for (; name[length] != '\0'; ++length) {
    if (length >= BIPO_SOUND_NAME_LENGTH - 1) return false;
    const char c = name[length];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  }
  return length > 0;
}

bool ensureStorage() {
  if (!storageReady) {
    storageReady = LittleFS.begin(false);
    if (!storageReady) return false;
  }
  if (!LittleFS.exists(SOUND_DIRECTORY)) {
    if (!LittleFS.mkdir(SOUND_DIRECTORY)) return false;
  }
  return true;
}

bool finiteRange(float value, float low, float high) {
  return isfinite(value) && value >= low && value <= high;
}

void makePath(char *path, size_t capacity, const char *name) {
  snprintf(path, capacity, "%s/%s.bps", SOUND_DIRECTORY, name);
}
}  // namespace

void makeDefaultKickPreset(SoundPreset &preset, const char *name) {
  memset(&preset, 0, sizeof(preset));
  preset.schemaVersion = BIPO_SOUND_SCHEMA_VERSION;
  strncpy(preset.name, name ? name : "BIPO_Kick_01", sizeof(preset.name) - 1);
  preset.engine = SOUND_ENGINE_KICK;
  preset.frequencyHz = 55.0f;
  preset.brightness = 0.35f;
  preset.level = 0.8f;
  preset.modulationEnabled = false;
  preset.amplitudeEnvelope = {0.5f, 180.0f, 0.0f, 30.0f};
  preset.pitchEnvelope = {30.0f, 45.0f, true};
}

bool validateSoundPreset(const SoundPreset &preset) {
  if (preset.schemaVersion != BIPO_SOUND_SCHEMA_VERSION) return false;
  if (!validName(preset.name)) return false;
  if (preset.engine > SOUND_ENGINE_KICK) return false;
  if (!finiteRange(preset.frequencyHz, 20.0f, 8000.0f)) return false;
  if (!finiteRange(preset.brightness, 0.0f, 1.0f)) return false;
  if (!finiteRange(preset.level, 0.0f, 1.0f)) return false;
  if (!finiteRange(preset.amplitudeEnvelope.attackMs, 0.0f, 10000.0f)) return false;
  if (!finiteRange(preset.amplitudeEnvelope.decayMs, 0.0f, 30000.0f)) return false;
  if (!finiteRange(preset.amplitudeEnvelope.sustain, 0.0f, 1.0f)) return false;
  if (!finiteRange(preset.amplitudeEnvelope.releaseMs, 0.0f, 30000.0f)) return false;
  if (!finiteRange(preset.pitchEnvelope.amountSemitones, -48.0f, 48.0f)) return false;
  if (!finiteRange(preset.pitchEnvelope.decayMs, 0.0f, 10000.0f)) return false;
  return true;
}

bool saveSoundPreset(const SoundPreset &preset) {
  if (!validateSoundPreset(preset) || !ensureStorage()) return false;

  char path[48];
  makePath(path, sizeof(path), preset.name);
  const String temporaryPath = String(path) + ".tmp";
  const String backupPath = String(path) + ".bak";

  DiskHeader header;
  memcpy(header.magic, FILE_MAGIC, sizeof(FILE_MAGIC));
  header.schemaVersion = BIPO_SOUND_SCHEMA_VERSION;
  header.payloadSize = sizeof(SoundPreset);
  header.checksum = checksumBytes(reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));

  // Write a temporary file first; only replace the named preset after the full
  // header and payload have been written successfully.
  File file = LittleFS.open(temporaryPath, "w");
  if (!file) return false;
  const size_t headerWritten = file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header));
  const size_t payloadWritten = file.write(reinterpret_cast<const uint8_t *>(&preset), sizeof(preset));
  file.flush();
  file.close();
  if (headerWritten != sizeof(header) || payloadWritten != sizeof(preset)) {
    LittleFS.remove(temporaryPath);
    return false;
  }

  // Keep the previous version recoverable if replacing the target fails.
  LittleFS.remove(backupPath);
  const bool hadPrevious = LittleFS.exists(path);
  if (hadPrevious && !LittleFS.rename(path, backupPath)) {
    LittleFS.remove(temporaryPath);
    return false;
  }
  if (!LittleFS.rename(temporaryPath, path)) {
    if (hadPrevious) LittleFS.rename(backupPath, path);
    LittleFS.remove(temporaryPath);
    return false;
  }
  if (hadPrevious) LittleFS.remove(backupPath);
  return true;
}

bool loadSoundPreset(const char *name, SoundPreset &preset) {
  if (!validName(name) || !ensureStorage()) return false;
  char path[48];
  makePath(path, sizeof(path), name);
  File file = LittleFS.open(path, "r");
  if (!file) return false;

  DiskHeader header = {};
  SoundPreset candidate = {};
  const bool headerRead = file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header)) == sizeof(header);
  const bool payloadRead = file.read(reinterpret_cast<uint8_t *>(&candidate), sizeof(candidate)) == sizeof(candidate);
  const bool noExtraData = file.available() == 0;
  file.close();

  if (!headerRead || !payloadRead || !noExtraData) return false;
  if (memcmp(header.magic, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0) return false;
  if (header.schemaVersion != BIPO_SOUND_SCHEMA_VERSION ||
      header.payloadSize != sizeof(SoundPreset)) return false;
  if (header.checksum != checksumBytes(reinterpret_cast<const uint8_t *>(&candidate), sizeof(candidate))) return false;
  if (!validateSoundPreset(candidate) || strncmp(candidate.name, name, sizeof(candidate.name)) != 0) return false;

  preset = candidate;
  return true;
}

bool deleteSoundPreset(const char *name) {
  if (!validName(name) || !ensureStorage()) return false;
  char path[48];
  makePath(path, sizeof(path), name);
  return LittleFS.exists(path) && LittleFS.remove(path);
}

void listSoundPresets(Stream &output) {
  if (!ensureStorage()) {
    output.println("ERROR: no se pudo montar LittleFS; no se formateo la memoria.");
    return;
  }
  File directory = LittleFS.open(SOUND_DIRECTORY);
  if (!directory || !directory.isDirectory()) {
    output.println("(sin presets)");
    return;
  }
  bool found = false;
  File entry = directory.openNextFile();
  while (entry) {
    const char *fileName = entry.name();
    const size_t length = strlen(fileName);
    if (!entry.isDirectory() && length > 4 &&
        strcmp(fileName + length - 4, ".bps") == 0) {
      output.println(fileName);
      found = true;
    }
    entry.close();
    entry = directory.openNextFile();
  }
  directory.close();
  if (!found) output.println("(sin presets)");
}

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

namespace {
portMUX_TYPE presetSnapshotMux = portMUX_INITIALIZER_UNLOCKED;
SoundPreset pendingAudioPreset = {};
bool pendingAudioPresetDirty = false;
}

void publishSoundPreset(const SoundPreset &preset) {
  portENTER_CRITICAL(&presetSnapshotMux);
  pendingAudioPreset = preset;
  pendingAudioPresetDirty = true;
  portEXIT_CRITICAL(&presetSnapshotMux);
}

bool consumePendingSoundPreset(SoundPreset &preset) {
  bool updated = false;
  portENTER_CRITICAL(&presetSnapshotMux);
  if (pendingAudioPresetDirty) {
    preset = pendingAudioPreset;
    pendingAudioPresetDirty = false;
    updated = true;
  }
  portEXIT_CRITICAL(&presetSnapshotMux);
  return updated;
}

int getSoundPresetNames(char names[][BIPO_SOUND_NAME_LENGTH], int capacity) {
  if (names == nullptr || capacity <= 0 || !ensureStorage()) return 0;
  File directory = LittleFS.open(SOUND_DIRECTORY);
  if (!directory || !directory.isDirectory()) return 0;
  int count = 0;
  File entry = directory.openNextFile();
  while (entry && count < capacity) {
    const char *fileName = entry.name();
    const char *baseName = strrchr(fileName, '/');
    baseName = baseName ? baseName + 1 : fileName;
    const size_t length = strlen(baseName);
    if (!entry.isDirectory() && length > 4 &&
        strcmp(baseName + length - 4, ".bps") == 0) {
      const size_t nameLength = length - 4;
      if (nameLength > 0 && nameLength < BIPO_SOUND_NAME_LENGTH) {
        memcpy(names[count], baseName, nameLength);
        names[count][nameLength] = '\0';
        ++count;
      }
    }
    entry.close();
    entry = directory.openNextFile();
  }
  directory.close();
  return count;
}

#include "State.h"
const char *engineNames[ENGINE_COUNT] = {
  "ANALOG", "FM", "WAVETABLE", "PLUCK", "MODAL", "NOISE", "CHORD", "TEXTURE", "BIPO DRUM"
};
const char *engineDescriptions[ENGINE_COUNT] = {
  "Classic oscillator", "Frequency modulation", "Harmonic waveform", "Plucked string",
  "Resonant modes", "Noise generator", "Chord oscillator", "Noise texture", "Layered FM body, noise and transient drum synth"
};
const char *mainMenuNames[4] = { "PLAY", "SOUND", "PRESETS", "SYSTEM" };
const char *presetActionNames[PRESET_ACTION_COUNT] = { "LOAD", "SAVE CURRENT", "SAVE AS", "DELETE", "CANCEL" };
const char *parameterNames[PARAMETER_COUNT] = {
  "FREQUENCY", "BRIGHTNESS", "LEVEL", "AMP ATTACK", "AMP DECAY",
  "AMP SUSTAIN", "AMP RELEASE", "PITCH AMOUNT", "PITCH DECAY",
  "DRUM MODEL", "SWEEP TIME", "SWEEP DEPTH", "ALGORITHM", "BODY WAVE",
  "RATIO A", "RATIO B", "MOD A", "MOD B", "FEEDBACK", "WAVEFOLD",
  "BODY HOLD", "BODY DECAY", "BODY LEVEL", "NOISE HOLD", "NOISE DECAY",
  "NOISE LEVEL", "TRANSIENT", "TRANSIENT LEVEL", "NOISE BASE", "NOISE WIDTH",
  "NOISE RESET", "NOISE RINGMOD"
};
ScreenState screenState = SCREEN_MAIN;
int currentEngine = ENGINE_KICK;
float baseFrequency = 55.0f;
float brightness = 0.35f;
float masterVolume = 0.20f;
bool modulationEnabled = false;
int menuSelection = 0, parameterSelection = 0, presetSelection = 0, presetActionSelection = 0;
ParameterType editingParameter = PARAM_FREQUENCY;
bool displayDirty = true, audioReady = false, soundDirty = false;
float phase1 = 0.0f, phase2 = 0.0f, phase3 = 0.0f;
float envelope = 0.0f, filterState = 0.0f;
uint32_t noiseState = 0x12345678;
int16_t audioBuffer[AUDIO_FRAMES * 2];
int previousEncoderState = 0;
bool lastButtonReading = HIGH, stableButtonState = HIGH;
uint32_t lastButtonChange = 0, buttonPressTime = 0;
bool longPressSent = false;
SoundPreset activeSoundPreset;
SoundPreset audioSoundPreset;
volatile bool kickTriggerRequested = false;
float kickPhase = 0.0f;
uint32_t kickAgeSamples = 0;
float kickAmplitude = 0.0f;
char presetNames[32][BIPO_SOUND_NAME_LENGTH] = {};
int presetCount = 0;
char editName[BIPO_SOUND_NAME_LENGTH] = {};
int editNamePosition = 0, editCharIndex = 0;
bool nameEditorSaveAs = false;

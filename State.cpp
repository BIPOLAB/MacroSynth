#include "State.h"
const char *engineNames[ENGINE_COUNT] = {
  "ANALOG", "FM", "WAVETABLE", "PLUCK", "MODAL", "NOISE", "CHORD", "TEXTURE", "BIPO KICK"
};
const char *engineDescriptions[ENGINE_COUNT] = {
  "Classic oscillator", "Frequency modulation", "Harmonic waveform", "Plucked string",
  "Resonant modes", "Noise generator", "Chord oscillator", "Noise texture", "Kick with amp/pitch envelopes"
};
ScreenState screenState = SCREEN_MAIN;
const char *parameterNames[PARAMETER_COUNT] = { "FREQUENCY", "BRIGHTNESS", "VOLUME" };
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
float phase1 = 0.0f, phase2 = 0.0f, phase3 = 0.0f;
float envelope = 0.0f, filterState = 0.0f;
uint32_t noiseState = 0x12345678;
int16_t audioBuffer[AUDIO_FRAMES * 2];
int previousEncoderState = 0;
bool lastButtonReading = HIGH, stableButtonState = HIGH;
uint32_t lastButtonChange = 0, buttonPressTime = 0;
bool longPressSent = false;
SoundPreset activeSoundPreset;
volatile bool kickTriggerRequested = false;
float kickPhase = 0.0f;
uint32_t kickAgeSamples = 0;
float kickAmplitude = 0.0f;

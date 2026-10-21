#pragma once
#include "Config.h"
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

enum ScreenState {
  SCREEN_MAIN,
  SCREEN_ENGINE_MENU,
  SCREEN_PARAMETER_MENU,
  SCREEN_EDIT_PARAMETER
};

enum ParameterType {
  PARAM_FREQUENCY = 0,
  PARAM_BRIGHTNESS,
  PARAM_VOLUME,
  PARAMETER_COUNT
};

enum ButtonEvent {
  BUTTON_NONE,
  BUTTON_SHORT,
  BUTTON_LONG
};

extern const char *engineNames[ENGINE_COUNT];
extern const char *engineDescriptions[ENGINE_COUNT];
extern const char *parameterNames[PARAMETER_COUNT];
extern ScreenState screenState;
extern int currentEngine;
extern float baseFrequency, brightness, masterVolume;
extern bool modulationEnabled;
extern int menuSelection, parameterSelection;
extern ParameterType editingParameter;
extern bool displayDirty, audioReady;
extern float phase1, phase2, phase3, envelope, filterState;
extern uint32_t noiseState;
extern int16_t audioBuffer[AUDIO_FRAMES * 2];
extern int previousEncoderState;
extern bool lastButtonReading, stableButtonState;
extern uint32_t lastButtonChange, buttonPressTime;
extern bool longPressSent;

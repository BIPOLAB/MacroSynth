#pragma once
#include "Config.h"
#include "SoundPreset.h"

enum EngineType {
  ENGINE_ANALOG = 0, ENGINE_FM, ENGINE_WAVETABLE, ENGINE_PLUCK,
  ENGINE_MODAL, ENGINE_NOISE, ENGINE_CHORD, ENGINE_TEXTURE, ENGINE_KICK,
  ENGINE_COUNT
};
enum ScreenState {
  SCREEN_MAIN, SCREEN_SOUND_MENU, SCREEN_PARAMETER_MENU, SCREEN_EDIT_PARAMETER,
  SCREEN_PRESET_BROWSER, SCREEN_PRESET_ACTIONS, SCREEN_NAME_EDITOR, SCREEN_SYSTEM
};
enum ParameterType {
  PARAM_FREQUENCY = 0, PARAM_BRIGHTNESS, PARAM_VOLUME,
  PARAM_AMP_ATTACK, PARAM_AMP_DECAY, PARAM_AMP_SUSTAIN, PARAM_AMP_RELEASE,
  PARAM_PITCH_AMOUNT, PARAM_PITCH_DECAY, PARAMETER_COUNT
};
enum ButtonEvent { BUTTON_NONE, BUTTON_SHORT, BUTTON_LONG };
enum PresetAction { PRESET_LOAD = 0, PRESET_SAVE, PRESET_SAVE_AS, PRESET_DELETE, PRESET_CANCEL, PRESET_ACTION_COUNT };

extern const char *engineNames[ENGINE_COUNT];
extern const char *engineDescriptions[ENGINE_COUNT];
extern const char *parameterNames[PARAMETER_COUNT];
extern const char *mainMenuNames[4];
extern const char *presetActionNames[PRESET_ACTION_COUNT];
extern ScreenState screenState;
extern int currentEngine;
extern float baseFrequency, brightness, masterVolume;
extern bool modulationEnabled;
extern int menuSelection, parameterSelection, presetSelection, presetActionSelection;
extern ParameterType editingParameter;
extern bool displayDirty, audioReady, soundDirty;
extern float phase1, phase2, phase3, envelope, filterState;
extern uint32_t noiseState;
extern int16_t audioBuffer[AUDIO_FRAMES * 2];
extern int previousEncoderState;
extern bool lastButtonReading, stableButtonState;
extern uint32_t lastButtonChange, buttonPressTime;
extern bool longPressSent;
extern SoundPreset activeSoundPreset;
extern SoundPreset audioSoundPreset;
extern volatile bool kickTriggerRequested;
extern float kickPhase;
extern uint32_t kickAgeSamples;
extern float kickAmplitude;
extern char presetNames[32][BIPO_SOUND_NAME_LENGTH];
extern int presetCount;
extern char editName[BIPO_SOUND_NAME_LENGTH];
extern int editNamePosition, editCharIndex;
extern bool nameEditorSaveAs;

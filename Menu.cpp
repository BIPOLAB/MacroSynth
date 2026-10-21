#include "Menu.h"
#include "Controls.h"
#include "Synth.h"
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



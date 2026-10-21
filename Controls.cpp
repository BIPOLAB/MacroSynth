#include "Controls.h"
int readEncoderDelta() {
  int clk = digitalRead(ENCODER_CLK);
  int dt = digitalRead(ENCODER_DT);

  int currentState = (clk << 1) | dt;

  int delta = 0;

  static const int8_t transitionTable[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  static int encoderAccumulation = 0;

  int index = (previousEncoderState << 2) | currentState;

  encoderAccumulation += transitionTable[index];

  if (encoderAccumulation >= 4) {
    delta = 1;
    encoderAccumulation = 0;
  } else if (encoderAccumulation <= -4) {
    delta = -1;
    encoderAccumulation = 0;
  }

  previousEncoderState = currentState;

  return delta;
}

ButtonEvent readButtonEvent() {
  ButtonEvent event = BUTTON_NONE;

  bool reading = digitalRead(ENCODER_SW);
  uint32_t now = millis();

  if (reading != lastButtonReading) {
    lastButtonChange = now;
    lastButtonReading = reading;
  }

  if ((now - lastButtonChange) > BUTTON_DEBOUNCE_MS) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        buttonPressTime = now;
        longPressSent = false;
      } else {
        if (!longPressSent) {
          event = BUTTON_SHORT;
        }
      }
    }

    if (stableButtonState == LOW &&
        !longPressSent &&
        (now - buttonPressTime) >= LONG_PRESS_MS) {
      longPressSent = true;
      event = BUTTON_LONG;
    }
  }

  return event;
}



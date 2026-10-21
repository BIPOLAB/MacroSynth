#include "SerialControl.h"
#include "State.h"
#include "Synth.h"
void printHelp() {
  Serial.println();
  Serial.println("===== BIPOLAB SYNTH =====");
  Serial.println("1-8 : seleccionar motor");
  Serial.println("+/- : brillo");
  Serial.println("[/] : frecuencia");
  Serial.println("v/V : volumen -/+");
  Serial.println("m   : activar/desactivar modulacion");
  Serial.println("p   : menu de parametros");
  Serial.println("?   : mostrar ayuda");
  Serial.println("=========================");
  Serial.println();
}

void handleSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c >= '1' && c <= '8') {
      currentEngine = c - '1';
      envelope = 0.15f;
      displayDirty = true;
    }

    switch (c) {
      case '+':
        brightness = clampFloat(brightness + 0.03f, 0.0f, 1.0f);
        displayDirty = true;
        break;

      case '-':
        brightness = clampFloat(brightness - 0.03f, 0.0f, 1.0f);
        displayDirty = true;
        break;

      case '[':
        baseFrequency = clampFloat(baseFrequency - 5.0f, 20.0f, 2000.0f);
        displayDirty = true;
        break;

      case ']':
        baseFrequency = clampFloat(baseFrequency + 5.0f, 20.0f, 2000.0f);
        displayDirty = true;
        break;

      case 'v':
        masterVolume = clampFloat(masterVolume - 0.02f, 0.0f, 0.80f);
        displayDirty = true;
        break;

      case 'V':
        masterVolume = clampFloat(masterVolume + 0.02f, 0.0f, 0.80f);
        displayDirty = true;
        break;

      case 'm':
      case 'M':
        modulationEnabled = !modulationEnabled;
        displayDirty = true;
        break;

      case 'p':
      case 'P':
        screenState = SCREEN_PARAMETER_MENU;
        parameterSelection = 0;
        displayDirty = true;
        break;

      case '?':
        printHelp();
        break;
    }
  }
}



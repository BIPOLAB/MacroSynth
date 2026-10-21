// MacroSynth: sketch entry point. Implementation lives in separate modules.
#include "Config.h"
#include "State.h"
#include "Synth.h"
#include "Audio.h"
#include "Controls.h"
#include "Menu.h"
#include "Display.h"
#include "SerialControl.h"

void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  previousEncoderState =
    (digitalRead(ENCODER_CLK) << 1) |
     digitalRead(ENCODER_DT);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, -1);

  tft.init(240, 240, SPI_MODE2);
  tft.setSPISpeed(12000000);
  tft.invertDisplay(true);
  tft.setRotation(3);

  tft.fillScreen(UI_BG);

  tft.setTextColor(UI_WHITE, UI_BG);
  tft.setTextSize(2);
  tft.setCursor(18, 90);
  tft.print("BIPOLAB");

  tft.setTextColor(UI_ACCENT, UI_BG);
  tft.setCursor(18, 118);
  tft.print("SYNTH");

  I2S.setPins(I2S_BCLK, I2S_LRCK, I2S_DOUT);

  audioReady = I2S.begin(
    I2S_MODE_STD,
    SAMPLE_RATE,
    I2S_DATA_BIT_WIDTH_16BIT,
    I2S_SLOT_MODE_STEREO
  );

  if (!audioReady) {
    Serial.println("ERROR: no se pudo iniciar I2S.");
  } else {
    Serial.println("I2S iniciado correctamente.");
  }

  printHelp();

  displayDirty = true;
}


void loop() {
  // Mantiene activo el flujo de audio.
  processAudioBlock();

  // Controles físicos.
  handleEncoder();

  ButtonEvent buttonEvent = readButtonEvent();

  if (buttonEvent != BUTTON_NONE) {
    handleButton(buttonEvent);
  }

  // Controles alternativos por Serial.
  handleSerial();

  // Actualizar la pantalla solamente cuando cambia algo.
  if (displayDirty) {
    drawCurrentScreen();
  }
}


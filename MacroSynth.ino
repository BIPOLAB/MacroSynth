// MacroSynth: sketch entry point. Audio runs independently from UI.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Config.h"
#include "State.h"
#include "Synth.h"
#include "Audio.h"
#include "Controls.h"
#include "Menu.h"
#include "Display.h"
#include "SerialControl.h"

// I2S.write() is blocking, so keep it off the Arduino loop that draws the TFT.
// Core 0 runs audio; Arduino loop/UI remains on core 1 on the ESP32-S3.
static TaskHandle_t audioTaskHandle = nullptr;

static void audioTask(void *parameter) {
  (void)parameter;
  for (;;) {
    if (audioReady) {
      processAudioBlock();
    } else {
      // Do not spin at full speed if I2S initialization failed.
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

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
    BaseType_t taskCreated = xTaskCreatePinnedToCore(
      audioTask,
      "MacroSynthAudio",
      6144,
      nullptr,
      3,
      &audioTaskHandle,
      0
    );
    if (taskCreated != pdPASS) {
      audioReady = false;
      Serial.println("ERROR: no se pudo crear la tarea de audio.");
    } else {
      Serial.println("Tarea de audio iniciada en core 0.");
    }
  }

  printHelp();
  displayDirty = true;
}

void loop() {
  // Audio is produced by audioTask; UI activity cannot block I2S block generation.
  handleEncoder();

  ButtonEvent buttonEvent = readButtonEvent();
  if (buttonEvent != BUTTON_NONE) {
    handleButton(buttonEvent);
  }

  handleSerial();

  // Screen drawing remains on the Arduino/UI task, separate from audio generation.
  if (displayDirty) {
    drawCurrentScreen();
  }

  // Yield to other tasks and avoid a needless full-speed UI polling loop.
  delay(1);
}

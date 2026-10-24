#pragma once
#include <Arduino.h>
// ESP32-S3 / PCM5102 / ST7789 pin mapping. Keep hardware changes here.
#define TFT_CS -1
#define TFT_DC 13
#define TFT_RST 10
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_BLK 14
#define ENCODER_CLK 35
#define ENCODER_DT 36
#define ENCODER_SW 37
#define I2S_BCLK 16
#define I2S_LRCK 18
#define I2S_DOUT 8
#define SAMPLE_RATE 44100
#define AUDIO_FRAMES 128
#define UI_BG 0x0000
#define UI_WHITE 0xFFFF
#define UI_GRAY 0x8410
#define UI_DARKGRAY 0x4208
#define UI_ACCENT 0xFD20
constexpr uint32_t BUTTON_DEBOUNCE_MS = 25;
constexpr uint32_t LONG_PRESS_MS = 650;

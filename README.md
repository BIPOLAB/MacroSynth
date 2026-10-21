# MacroSynth — BipoLab

Modular experimental synthesizer firmware for **ESP32-S3**, **PCM5102 I2S DAC**, **ST7789 240×240 display**, and a rotary encoder.

## Quick start
1. Open `MacroSynth.ino` with Arduino IDE (keep all `.h` and `.cpp` files in the same `MacroSynth` sketch folder).
2. Install ESP32 Arduino board support and the **Adafruit GFX** and **Adafruit ST7789** libraries.
3. Select your ESP32-S3 board, verify the pin mapping in `Config.h`, and compile/upload.
4. Open Serial Monitor at **115200 baud** for the current serial interface.

The initial modularization deliberately preserves the original eight synthesis engines, UI behavior and audio path. It is **not yet hardware-verified after refactoring**.

## Modules
| File | Responsibility |
| --- | --- |
| `MacroSynth.ino` | Setup and main loop |
| `Config.h` | Hardware pinout, sample rate and UI colors |
| `State.h/.cpp` | Shared runtime state and enumerations |
| `Synth.h/.cpp` | Oscillators, utilities and eight sound engines |
| `Audio.h/.cpp` | PCM sample blocks and ESP_I2S output |
| `Controls.h/.cpp` | Encoder and button debounce |
| `Menu.h/.cpp` | Parameter editing and menu navigation |
| `Display.h/.cpp` | ST7789 screen rendering |
| `SerialControl.h/.cpp` | Serial commands and help |

See [hardware pinout](docs/PINOUT.md), [architecture](docs/ARCHITECTURE.md), and [validation checklist](docs/TESTING.md).

## Serial commands
`1`–`8` select engines; `+`/`-` brightness; `[`/`]` frequency; `v`/`V` volume; `m` modulation; `p` parameters; `?` help.

## Status
Early prototype / MK0. No presets, MIDI input, polyphony, or independent engine parameter models yet. Changes should be tested on hardware before merging into `main`.

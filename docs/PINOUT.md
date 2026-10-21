# Hardware pinout

| Function | ESP32-S3 GPIO | Peripheral |
| --- | ---: | --- |
| I2S BCLK | 16 | PCM5102 BCK |
| I2S LRCK / WS | 18 | PCM5102 LCK |
| I2S DATA OUT | 8 | PCM5102 DIN |
| TFT MOSI | 11 | ST7789 SDA |
| TFT SCLK | 12 | ST7789 SCL |
| TFT DC | 13 | ST7789 DC |
| TFT RESET | 10 | ST7789 RST |
| TFT backlight | 14 | ST7789 BLK |
| TFT CS | -1 | Not used |
| Encoder CLK | 35 | Rotary encoder A |
| Encoder DT | 36 | Rotary encoder B |
| Encoder SW | 37 | Encoder button |

**Important:** GPIO35–37 may be reserved for OPI PSRAM on certain ESP32-S3 modules. Verify the exact module/board schematic before connecting an encoder. These values mirror the original sketch and have not been independently hardware-validated.

PCM5102 SCK is not connected in the tested setup. Confirm VIN/GND and module power requirements before powering up. The firmware uses stereo 16-bit 44.1 kHz audio, duplicated mono on L/R.

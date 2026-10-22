# Validation checklist

The audio/UI separation has been committed, but it has **not yet been compiled or tested on the physical ESP32-S3**.

1. Open `MacroSynth.ino` in Arduino IDE, select the correct ESP32-S3 board/core, and compile.
2. Check boot messages at 115200 baud. Expected messages include I2S initialization and `Tarea de audio iniciada en core 0.`
3. With safe listening volume, confirm PCM5102 produces clean sound in both headphone channels.
4. Test sound while turning the encoder slowly and rapidly on the main screen, engine menu, parameter menu, and parameter edit screen.
5. Repeat while the display redraws and while issuing Serial commands.
6. Listen for dropouts, clicks, or distorted transients. If artifacts remain, record whether they occur only during parameter edits or also while browsing the engine menu.
7. Verify short/long encoder presses and Serial keys `1`–`8`, `+`, `-`, `[`, `]`, `v`, `V`, `m`, `p`, `?`.
8. Verify no pin conflicts with the board's PSRAM.

If the IDE reports missing headers, install Adafruit GFX and Adafruit ST7789; `ESP_I2S.h` requires a compatible ESP32 Arduino core. A successful compile is necessary but not a substitute for the hardware audio test.


## Sound preset / BIPO Kick checks
9. Send `:help` and confirm the preset commands are listed.
10. Send `:kick` and confirm a short kick transient is audible.
11. Send `:env 1 180 0 30`, `:pitch 30 45`, `:freq 55`, then `:save BIPO_Kick_01`.
12. Change the envelope and pitch, save as `:save BIPO_Kick_02`, and verify the first file is still listed separately.
13. Load each preset and listen for the expected difference. Confirm invalid values are rejected without replacing the active valid envelope.
14. Power-cycle the board, send `:load BIPO_Kick_01`, then `:kick`; confirm the saved sound is restored.
15. Test `:list` with no presets, with multiple presets, and after a failed LittleFS mount if that can be simulated safely. Confirm the firmware never formats LittleFS automatically.
16. Repeat encoder and display tests while audio is playing. Preset snapshots reduce races for preset data only; shared legacy controls are still a known potential source of artifacts.


## Standalone TFT interface checks
17. Boot without opening Serial Monitor and confirm the main menu appears on the ST7789.
18. Navigate PLAY / SOUND / PRESETS / SYSTEM using only the encoder; short press selects and long press returns.
19. Open SOUND → PARAMETERS and edit frequency, level, amplitude ADSR and pitch envelope. Confirm values update on screen and the BIPO Kick sound responds.
20. Open PRESETS with an empty filesystem, choose SAVE AS, enter a name with the encoder, and save. Confirm the preset appears in the browser.
21. Change envelope values, use SAVE AS again, and confirm a distinct name is created without overwriting the original.
22. Load both presets and verify their parameters return. Reboot, load them again and verify persistence.
23. Test save-current overwrite deliberately, delete a selected preset, and cancel out of every menu using long press.
24. Repeat audio tests while editing parameters. The UI and serial debug paths are not proof of glitch-free DSP; listen for artifacts and report exact screen/action if one occurs.

# MacroSynth — BipoLab

Firmware experimental de síntesis para **ESP32-S3**, DAC I2S **PCM5102**, pantalla **ST7789 240×240** y encoder rotativo.

## Inicio rápido
1. Abre `MacroSynth.ino` en Arduino IDE y conserva todos los archivos `.h` y `.cpp` en la misma carpeta del sketch.
2. Instala el soporte de placas ESP32 y las librerías **Adafruit GFX** y **Adafruit ST7789**.
3. Selecciona la placa ESP32-S3 correcta, revisa `Config.h` y compila/sube el firmware.
4. Abre Serial Monitor a **115200 baudios**. Para comandos de texto selecciona envío con salto de línea (Newline).

## Motores
| Motor | Estado |
| --- | --- |
| Analog, FM, Wavetable, Pluck, Modal, Noise, Chord, Texture | Motores experimentales originales del prototipo |
| BIPO Kick | Primer motor percusivo con envolvente de amplitud de un disparo y envolvente de tono |

## Presets de sonido
El modelo `SoundPreset` guarda nombre, versión de esquema, motor, frecuencia, brillo, nivel, modulación, envolvente de amplitud y envolvente de tono. Los presets se guardan en LittleFS como archivos `.bps`, con cabecera/versionado y checksum para detectar corrupción. El montaje de LittleFS no formatea la memoria automáticamente si falla.

Comandos de texto (escribe los dos puntos y pulsa Enter):
- `:kick` — selecciona y dispara BIPO Kick.
- `:save BIPO_Kick_01` — guarda el sonido actual con ese nombre.
- `:load BIPO_Kick_01` — carga un preset existente.
- `:list` — lista los presets guardados.
- `:env 1 180 0 30` — define ataque (ms), decay (ms), sustain (0–1) y release (ms).
- `:pitch 30 45` — activa una caída de tono de 30 semitonos con decay de 45 ms.
- `:freq 55` — define la frecuencia base en Hz.
- `:level 0.8` — define el nivel del sonido de 0 a 1.
- `:help` — muestra la ayuda.

Los nombres admiten letras, números, guion bajo y guion medio, hasta 23 caracteres. Para crear una variante, usa otro nombre con `:save`; así el preset original no se sobrescribe. La edición de parámetros se realiza en RAM hasta que se guarda explícitamente.

## Módulos principales
| Archivo | Responsabilidad |
| --- | --- |
| `MacroSynth.ino` | Inicialización y loop |
| `Config.h` | Pines, frecuencia de muestreo y configuración UI |
| `State.h/.cpp` | Estado global y enumeraciones |
| `Synth.h/.cpp` | Generación de audio y motores |
| `SoundPreset.h/.cpp` | Modelo de preset y persistencia LittleFS |
| `Audio.h/.cpp` | Bloques PCM estéreo e I2S |
| `Controls.h/.cpp` | Encoder y antirrebote |
| `Menu.h/.cpp` | Navegación por encoder |
| `Display.h/.cpp` | Pantalla ST7789 |
| `SerialControl.h/.cpp` | Comandos seriales |

Consulta [pinout](docs/PINOUT.md), [arquitectura](docs/ARCHITECTURE.md), [modelo de presets](docs/SOUND_PRESETS.md) y [validación](docs/TESTING.md).

## Estado y limitaciones
Prototipo MK0. El motor BIPO Kick utiliza una envolvente one-shot de amplitud (attack/decay/sustain/release secuenciales) y una envolvente de tono. La estructura ya reserva parámetros para otros motores, pero sus modelos específicos todavía deben desarrollarse. Aún no hay secuenciador de 16 pasos, MIDI, polifonía ni kits. Los cambios de audio y persistencia requieren compilación Arduino y pruebas físicas con ESP32-S3/PCM5102 antes de considerarse verificados.

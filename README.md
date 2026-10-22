# MacroSynth — BipoLab

Firmware de síntesis para **ESP32-S3**, DAC I2S **PCM5102**, pantalla **ST7789 240×240** y encoder rotativo.

## Control del instrumento
La pantalla y el encoder son la interfaz principal; el puerto serie queda como herramienta auxiliar de diagnóstico.

- Girar el encoder: navegar por menús o cambiar el parámetro seleccionado.
- Pulsación corta: entrar, confirmar o aceptar.
- Pulsación larga: volver atrás.
- En el editor de nombres, girar cambia el carácter, pulsar acepta el carácter y seleccionar el espacio termina el nombre.

### Menú principal
- **PLAY:** dispara el sonido actual; BIPO Kick tiene disparo manual.
- **SOUND:** selecciona el motor, edita parámetros y controla modulación.
- **PRESETS:** explora sonidos guardados y permite cargar, guardar, crear variantes y eliminar presets.
- **SYSTEM:** muestra información del instrumento.

### Editar y guardar sonidos
En `SOUND → PARAMETERS` se pueden editar frecuencia, brillo, nivel, ataque, decay, sustain, release y parámetros de la envolvente de tono. Los cambios se aplican al motor de audio y quedan marcados como no guardados.

En `PRESETS` se puede cargar un sonido o abrir sus acciones. `SAVE CURRENT` guarda el sonido activo con su nombre; `SAVE AS` permite crear una variante. Si el nombre de Save As ya existe, el firmware crea automáticamente un nombre libre con sufijo, para no sobrescribir el preset existente.

Los presets se almacenan en LittleFS como archivos `.bps`, con versión de esquema y checksum. Si no se puede montar LittleFS, el firmware no formatea la memoria automáticamente.

## Motores
| Motor | Estado |
| --- | --- |
| Analog, FM, Wavetable, Pluck, Modal, Noise, Chord, Texture | Motores experimentales del prototipo |
| BIPO Kick | Motor percusivo con envolventes de amplitud y tono |

## Puerto serie (diagnóstico)
Monitor serie a 115200 baudios. Los comandos existentes se mantienen para pruebas, pero no son necesarios para la operación habitual del instrumento. Escribe `:help` para verlos.

## Módulos principales
| Archivo | Responsabilidad |
| --- | --- |
| `MacroSynth.ino` | Inicialización y loop |
| `Config.h` | Pines y configuración |
| `State.h/.cpp` | Estado de interfaz y DSP |
| `Synth.h/.cpp` | Generación de audio |
| `SoundPreset.h/.cpp` | Modelo, listado y persistencia de presets |
| `Audio.h/.cpp` | Bloques PCM estéreo e I2S |
| `Controls.h/.cpp` | Encoder y antirrebote |
| `Menu.h/.cpp` | Navegación y edición |
| `Display.h/.cpp` | Interfaz ST7789 |
| `SerialControl.h/.cpp` | Herramientas de diagnóstico |

Consulta [pinout](docs/PINOUT.md), [arquitectura](docs/ARCHITECTURE.md), [modelo de presets](docs/SOUND_PRESETS.md) y [validación](docs/TESTING.md).

## Estado
Prototipo MK0. La interfaz autónoma de navegación y edición está implementada en firmware, pero necesita compilación y pruebas en la placa real. Aún no hay secuenciador de 16 pasos, kits completos, polifonía ni MIDI. La edición de envolventes afecta al motor BIPO Kick; los parámetros específicos de los demás motores requieren desarrollo posterior.

# MacroSynth — BipoLab

Firmware de síntesis para **ESP32-S3**, DAC I2S **PCM5102**, pantalla **ST7789 240×240** y encoder rotativo.

## Control del instrumento
La pantalla y el encoder son la interfaz principal; el puerto serie queda como herramienta auxiliar de diagnóstico.

- Girar el encoder: navegar por menús o cambiar el parámetro seleccionado.
- Pulsación corta: entrar, confirmar o aceptar.
- Pulsación larga: volver atrás.
- En el editor de nombres, girar cambia el carácter, pulsar acepta el carácter y seleccionar el espacio termina el nombre.

### Menú principal
- **PLAY:** dispara el sonido actual; BIPO DRUM tiene disparo manual.
- **SOUND:** selecciona el motor, edita parámetros y controla modulación.
- **PRESETS:** explora sonidos guardados y permite cargar, guardar, crear variantes y eliminar presets.
- **SYSTEM:** muestra información del instrumento.

### Editar y guardar sonidos
En `SOUND → PARAMETERS` se pueden editar frecuencia, brillo, nivel, ataque, decay, sustain, release y parámetros de la envolvente de tono. Los cambios se aplican al motor de audio y quedan marcados como no guardados.

En `PRESETS` se puede cargar un sonido o abrir sus acciones. `SAVE CURRENT` guarda el sonido activo con su nombre; `SAVE AS` permite crear una variante. Si el nombre de Save As ya existe, el firmware crea automáticamente un nombre libre con sufijo, para no sobrescribir el preset existente.

Los presets se almacenan en LittleFS como archivos `.bps`, con versión de esquema y checksum. Si no se puede montar LittleFS, el firmware no formatea la memoria automáticamente.

## BIPO DRUM: síntesis percusiva\nBIPO DRUM usa una voz híbrida original: cuerpo FM de tres osciladores, barrido de tono, varios ruteos de modulación, wavefolding, ruido filtrado y una capa de transitorio independiente. Cada capa tiene controles de nivel y/o envolvente propios. Desde `SOUND → PARAMETERS` se puede elegir una familia inicial (kick, snare, closed hat, open hat, clap, tom, rim o metal) y editar el sonido. Al cambiar de familia se cargan valores de partida; después se pueden retocar y guardar como preset. Dentro de la edición de parámetros, una pulsación corta dispara una escucha y una pulsación larga vuelve.\n\nLa estructura está inspirada en los principios generales del FM Drum de Digitone II, pero el DSP, los ruteos, los valores y la interfaz de BipoLab son propios. No es una copia ni un port del código de Elektron.\n\n## Motores
| Motor | Estado |
| --- | --- |
| Analog, FM, Wavetable, Pluck, Modal, Noise, Chord, Texture | Motores experimentales del prototipo |
| BIPO DRUM | Cuerpo FM + ruido + transitorio, con ocho familias de percusión |

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
Prototipo MK0. La interfaz autónoma de navegación y edición está implementada en firmware, pero necesita compilación y pruebas en la placa real. Aún no hay secuenciador de 16 pasos, kits completos, polifonía ni MIDI. BIPO DRUM ahora tiene parámetros propios para cuerpo FM, barrido de tono, ruido y transitorio; los parámetros específicos de los otros motores requieren desarrollo posterior. La síntesis percusiva necesita escucha y validación en la placa real.

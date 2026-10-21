# bipoDSP: arquitectura y hoja de ruta inicial

## Visión

bipoDSP será la plataforma DSP original de BipoLab: una colección coherente de motores expresivos, diseñados para el presupuesto de cómputo del ESP32-S3 y controlables desde una interfaz física pequeña. La ambición no es sumar algoritmos sin límite, sino crear un instrumento con personalidad, estabilidad y una experiencia inmediata.

## Capas

1. **Audio I/O** — inicialización y escritura I2S. En la implementación actual, PCM5102, 44.1 kHz y bloques de 128 muestras son el punto de partida; no son requisitos inmutables.
2. **Audio Engine** — ciclo de renderizado, mezcla, límites de salida, estado de voz y entrega de bloques. Sin dependencias de pantalla o controles.
3. **DSP Core** — tipos, conversión de unidades, rampas de parámetros, osciladores/utilidades comunes y convenciones numéricas.
4. **Synthesis Engines** — módulos reemplazables que reciben parámetros y producen audio. Primeros: BIPO VCO y BIPO FM.
5. **Modulation** — envolventes, LFO y rutas de modulación con destinos definidos.
6. **Effects** — filtro, saturación, chorus, delay y reverb, añadidos de forma gradual y medidos por coste.
7. **Control/UI** — encoder, comandos seriales y pantalla ST7789. Publica cambios; no modifica directamente el estado interno del DSP.
8. **Preset/State** — configuración persistente y formato versionado; la escritura en flash nunca debe interrumpir el renderizado.

## Contrato de procesamiento

- Procesamiento por bloques de tamaño fijo.
- Sin asignación dinámica, impresión serial, acceso a pantalla, I/O de archivos ni esperas bloqueantes en la ruta de audio.
- La tarea de audio es propietaria del estado mutable de todos los módulos DSP.
- Los cambios desde UI llegan como eventos o snapshots completos y coherentes; los parámetros continuos se suavizan en audio para evitar clicks.
- El sistema debe tolerar un bloque perdido de UI sin afectar el reloj de audio.
- Clipping y saturación de salida se controlan explícitamente; no se confía en overflow accidental.
- Registrar presupuesto de CPU, duración máxima de bloque y margen antes del siguiente deadline.

A 44.1 kHz, un bloque de 128 muestras representa aproximadamente 2.90 ms. La renderización y la escritura de ese bloque deben terminar con margen respecto de ese intervalo. Este valor es un objetivo de diseño, no una medición del firmware actual.

## Primer motor: BIPO VCO

Objetivo funcional:
- Oscilación continua con fase preservada.
- Seno, triángulo, diente de sierra y pulso con ancho variable.
- Frecuencia musicalmente útil, afinación estable y rampas para cambios de tono.
- Modulación de frecuencia y PWM como destinos futuros.
- Estrategia anti-aliasing elegida y medida por forma de onda; no prometer aliasing cero en todos los registros sin validación.
- API independiente de Arduino y del PCM5102.

La primera entrega prioriza estabilidad de fase, cambios de parámetro sin clicks y calidad espectral sobre cantidad de opciones.

## Segundo motor: BIPO FM

Objetivo funcional:
- Dos operadores inicialmente, con relación de frecuencia configurable.
- Índice de modulación con rango útil y controlado.
- Envolventes con respuesta estable.
- Prevención de valores extremos y DC no deseada.
- Evolución posterior hacia más operadores o feedback solo si el presupuesto de CPU lo permite.

Los controles macroscópicos deben ser musicales: por ejemplo, **Character**, **Motion** y **Body**, con mapeos específicos por motor documentados y normalizados. Los nombres son provisionales y se validarán escuchando.

## Principios de experiencia de instrumento

Inspiración general en Teenage Engineering, sin replicar sus diseños: acceso inmediato, pocos controles relevantes, feedback visual claro, consistencia entre panel y pantalla y descubrimiento por exploración. La pantalla no debe intentar mostrar todos los parámetros a la vez. Un usuario debería poder seleccionar un motor, entender sus tres controles principales y llegar a un sonido interesante en pocos segundos.

## Hoja de ruta

### Fase 0 — estabilidad de audio
- [ ] Revisar y corregir la comunicación entre UI y tarea de audio.
- [ ] Asegurar que el DSP interno solo se modifica desde la tarea de audio.
- [ ] Añadir rampas/smoothing de parámetros.
- [ ] Probar encoder lento, rápido y cambios repetidos mientras suena audio.
- [ ] Registrar observaciones de escucha y pruebas.

### Fase 1 — núcleo DSP portable
- [ ] Definir tipos y límites numéricos comunes.
- [ ] Separar motor de síntesis del transporte I2S.
- [ ] Preparar un renderizador de escritorio o pruebas unitarias de DSP puro.
- [ ] Generar señales de referencia y medir DC, clipping y continuidad.

### Fase 2 — BIPO VCO
- [ ] Implementar oscilador original y selección de formas de onda.
- [ ] Evaluar espectro y aliasing a varias frecuencias.
- [ ] Comprobar cambios de frecuencia y timbre durante audio.
- [ ] Integrar con el menú existente sin rediseñarlo todavía.

### Fase 3 — BIPO FM
- [ ] Implementar dos operadores.
- [ ] Diseñar mapeos macroscópicos por oído.
- [ ] Comparar CPU y continuidad frente a BIPO VCO.

### Fase 4 — carácter y expresión
- [ ] Modulación y envolventes.
- [ ] Filtro y saturación.
- [ ] Presets versionados y recuperación fiable.
- [ ] Selección de los próximos motores basada en pruebas auditivas y presupuesto real.

## Criterios de aceptación antes de ampliar

No declarar una fase completa por el mero hecho de que compile. Registrar por separado:
- Compilación (herramienta/versión y resultado).
- Prueba en placa (hardware y configuración).
- Continuidad de audio bajo interacción.
- Carga de CPU y margen temporal.
- Pruebas auditivas (qué se escuchó y con qué configuración).
- Licencias/procedencia si se usa código de terceros.

## Referencias de estudio

La selección y su aplicación a decisiones de diseño están documentadas en [DSP_RESEARCH.md](DSP_RESEARCH.md). Esa investigación distingue explícitamente entre código abierto para estudio técnico y referencias de diseño de producto como Teenage Engineering.

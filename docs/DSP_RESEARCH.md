# Investigación comparativa: DSP abierto e instrumentos inspiradores

## Objetivo

Diseñar **bipoDSP**, un motor de síntesis original de BipoLab para ESP32-S3. Los proyectos externos se estudian para aprender de sus decisiones de arquitectura, familias de síntesis, controles y validación. La intención del producto es desarrollar algoritmos propios: no copiar código, tablas, presets, interfaz ni comportamientos distintivos de un instrumento concreto.

Esta investigación es una primera selección técnica, no una auditoría completa de cada repositorio ni de todas sus dependencias. Antes de reutilizar cualquier implementación habrá que revisar la licencia del archivo exacto, sus avisos de copyright y las licencias transitivas.

## Referentes de código abierto

### 1. Mutable Instruments — Plaits y Braids

- Repositorio principal: https://github.com/pichenettes/eurorack
- Documentación de código abierto de Braids: https://pichenettes.github.io/mutable-instruments-documentation/modules/braids/open_source/
- Aporta como referencia conceptual: diversidad de modelos de oscilador, controles macroscópicos que cambian de significado según el motor, modelos físicos y digitales en una misma plataforma, y un instrumento que se puede explorar sin entrar en una matriz enorme de parámetros.
- Lección para bipoDSP: cada motor debería tener una identidad sonora y tres controles expresivos principales que produzcan cambios útiles a lo largo de todo su recorrido.
- Precaución: no se importará el código de Plaits/Braids en la línea original de bipoDSP. Aunque existen componentes con licencia MIT, el producto seguirá una implementación propia y mantendrá registro de procedencia para cualquier componente externo futuro.

### 2. Electro-Smith — DaisySP

- Repositorio: https://github.com/electro-smith/DaisySP
- Licencia declarada por el proyecto: MIT.
- Es especialmente relevante para hardware embebido: C++ ligero, osciladores, envolventes, filtros, distorsión, síntesis FM, modelado físico, granular, percusión y utilidades DSP.
- Lección para bipoDSP: estudiar la separación entre bloques DSP pequeños y reutilizables; medir el coste por muestra y por bloque; crear pruebas de regresión que se puedan ejecutar fuera de la placa.
- Decisión: usarlo como referencia de organización y catálogo. No añadirlo como dependencia automática ni copiar implementaciones al núcleo propietario sin una decisión explícita de licencia.

### 3. monome — norns

- Repositorio: https://github.com/monome/norns
- Es un entorno de instrumentos de sonido programables, con scripts, DSP y una arquitectura centrada en instrumentos y experimentación.
- Lección para bipoDSP: el motor no debería ser una colección de algoritmos aislados; debe permitir construir instrumentos completos con rutas claras de señal, modulación, parámetros y estados.
- Adaptación: bipoDSP separará motor de síntesis, modulación, efectos, gestión de parámetros y hardware. No replicará la arquitectura Linux/Lua de norns en el ESP32-S3.

### 4. Ornament & Crime / Hemisphere Suite

- Repositorios de referencia: https://github.com/mxmxmx/O_C y https://github.com/Chysn/O_C-HemisphereSuite
- Son referencias de firmware abierto para instrumentos modulares, con múltiples aplicaciones y funciones seleccionables.
- Lección para bipoDSP: definir módulos con responsabilidades acotadas, límites claros de recursos y modos fáciles de explorar desde una interfaz pequeña.
- Adaptación: evitar que el firmware se convierta en un menú interminable; cada motor debe ser accesible y entendible desde la pantalla pequeña y el encoder.

### 5. Teensy Audio Library

- Repositorio: https://github.com/PaulStoffregen/Audio
- Es una referencia útil para el diseño de grafos de audio, objetos conectables y procesamiento por bloques en microcontroladores.
- Lección para bipoDSP: mantener un flujo de señal explícito y bloques con contratos claros de entrada, salida, latencia y consumo de CPU.
- Adaptación: el núcleo no copiará la API ni asumirá el hardware Teensy; se diseñará para el ESP32-S3 y la salida I2S PCM5102 actual.

### 6. Faust y su ecosistema DSP

- Proyecto: https://github.com/grame-cncm/faust
- Faust ofrece un lenguaje y herramientas para describir DSP y generar implementaciones para diferentes plataformas.
- Lección para bipoDSP: pensar los algoritmos como bloques definidos matemáticamente y validar la misma lógica con pruebas reproducibles.
- Adaptación: no incorporaremos un compilador ni runtime Faust en el firmware inicial. Puede servir para prototipar ideas y contrastar respuestas fuera del ESP32.

## Teenage Engineering: referencia de producto, no de código abierto

Fuentes de producto:
- OP-1 original: https://teenage.engineering/products/op-1/original/overview
- Arquitectura modular del OP-1: https://teenage.engineering/products/op-1/original/modules
- Sistema Field: https://teenage.engineering/products/field-system

Teenage Engineering no se toma aquí como fuente de DSP abierto ni como especificación técnica interna. Es una referencia de **diseño de instrumento y experiencia de usuario**.

Principios que sí queremos estudiar y traducir a decisiones propias:

1. **Complejidad sonora con una interfaz contenida.** Muchos motores, pero controles principales comprensibles.
2. **Controles con propósito musical.** Un control debe invitar a tocar y escuchar, no obligar a editar números sin contexto.
3. **Coherencia entre hardware y software.** El menú, la respuesta de los controles, la serigrafía y el sonido deben sentirse parte del mismo instrumento.
4. **Exploración rápida.** Elegir un motor, cambiar el carácter y recuperar una idea debe requerir pocos pasos.
5. **Identidad visual propia.** Minimalismo, claridad y economía de elementos son referencias generales; no se copiarán iconos, layouts, tipografías, gráficos ni detalles distintivos de Teenage Engineering.
6. **Instrumento primero, lista de funciones después.** Las decisiones de arquitectura deben mejorar la experiencia musical real, no solo aumentar el número de características.

## Principios de arquitectura derivados

- **DSP desacoplado del hardware:** el núcleo no incluirá Arduino, TFT, encoder, Wi-Fi ni llamadas a I2S.
- **Estado de audio de un solo propietario:** solo la tarea de audio modifica el estado interno de osciladores, filtros, envolventes y efectos. La UI publica parámetros coherentes mediante una cola o snapshots seguros.
- **Tiempo real predecible:** nada de delay(), asignaciones dinámicas, impresión serial, renderizado de pantalla o esperas de bloqueo en la ruta de audio.
- **Bloques deterministas:** frecuencia de muestreo y tamaño de bloque definidos; registrar presupuesto temporal y medir picos, no solo promedios.
- **Pruebas antes de más motores:** pruebas de señal, clipping, discontinuidades, aliasing, cambios de parámetros y carga de CPU.
- **Portabilidad para validar:** cuando sea posible, compilar el DSP puro en escritorio para generar WAV y medir resultados sin flashear la placa en cada iteración.
- **Licencias trazables:** mantener un registro de procedencia, versión, licencia y modificaciones de cada dependencia o fragmento externo si en el futuro se decide reutilizar código.

## Decisiones para el primer prototipo

1. Mantener el actual camino de audio I2S y aislar la lógica DSP del hardware.
2. Crear un núcleo de procesamiento pequeño y medible.
3. Implementar un **BIPO VCO original** como primer motor, con formas de onda y control de timbre diseñados para sonar bien y evitar aliasing en la medida que permita el presupuesto del ESP32-S3.
4. Desarrollar después **BIPO FM**, con relaciones de frecuencia, índice de modulación y envolventes diseñados para una interfaz de pocos controles.
5. Añadir pruebas de renderizado y una ruta de comparación A/B de audio.
6. Solo después de estabilizar estos fundamentos, priorizar armónicos, resonadores, granular, percusión y efectos.

## Fuentes iniciales consultadas

- Mutable Instruments / Plaits y Braids: código y documentación pública del proyecto.
- Electro-Smith DaisySP: repositorio y descripción de sus familias DSP.
- monome norns: repositorio y documentación del entorno de instrumentos.
- Ornament & Crime / Hemisphere Suite: repositorios públicos de firmware.
- Teensy Audio Library: repositorio público de objetos de audio.
- Faust: repositorio público y documentación de generación de DSP.
- Teenage Engineering: páginas oficiales de OP-1 y Field System para principios de producto e interacción.

Última actualización: 2026-10-21.

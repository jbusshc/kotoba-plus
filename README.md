# Kotoba Plus

**Kotoba Plus** es un proyecto personal de software para el **estudio avanzado del japonés**, centrado en vocabulario y kanji, construido sobre **JMDict** y diseñado desde cero con foco en **eficiencia, control de datos y extensibilidad**.

El objetivo es ser una herramienta **especializada en japonés**, con estructuras de datos explícitas, un **SRS propio**, y una arquitectura pensada para crecer (sync, media, análisis, etc.).

---

## Objetivos del proyecto

- Proveer un **diccionario japonés estructurado**, basado en JMDict
- Implementar un **sistema de repetición espaciada (SRS)** controlado por el usuario
- Mantener **búsquedas rápidas y eficientes**
- Separar claramente:
  - datos base (diccionario)
  - progreso del usuario
  - lógica de estudio
- Diseñar el sistema para permitir futuras extensiones:
  - servidor local
  - sincronización
  - media (audio / imágenes)
  - análisis de estudio

---

## Principios de diseño

- **JMDict es la fuente de verdad**
  - No se modifica
  - No se “contamina” con datos de usuario
- **Datos explícitos > abstracciones mágicas**
  - Estructuras claras en C
  - Campos opcionales bien definidos
- **Separación estricta de responsabilidades**
  - Diccionario ≠ SRS ≠ UI ≠ Sync
- **Eficiencia primero**
  - Pensado para grandes volúmenes de entradas
  - Sin dependencias innecesarias

---

## Arquitectura general

El proyecto está dividido conceptualmente en los siguientes módulos:

### 1. Diccionario (JMDict)

Representación estructurada de las entradas de JMDict, incluyendo:

- **k_ele**: elementos kanji
- **r_ele**: lecturas
- **sense**: significados, glosas, etiquetas
- **example**: ejemplos asociados (cuando existan)

Estas estructuras:
- usan arreglos fijos donde es posible
- manejan campos opcionales explícitamente
- están pensadas para acceso rápido y serialización

📌 **Importante:**  
El diccionario es **solo lectura**.

---

### 2. SRS (Spaced Repetition System)

Sistema propio de repetición espaciada, independiente del diccionario.

- Maneja:
  - estado de aprendizaje
  - intervalos
  - historial de respuestas
- Las entradas del SRS **referencian** elementos del diccionario, no los duplican
- Permite ajustar reglas sin tocar los datos base

---

### 3. CLI

Interfaz de línea de comandos para:

- búsquedas en el diccionario
- sesiones de estudio
- revisión SRS
- pruebas y debugging

El CLI es intencionalmente simple y scriptable.

---

### 4. Componentes futuros (en diseño)

Estas partes **no están completamente implementadas**, pero la arquitectura las considera:

- **Servidor local**
  - para sincronización
  - manejo de media
- **Cliente externo**
  - posible UI gráfica
  - apps móviles
- **Exportación / importación**
  - estadísticas
  - backups

No todo tiene que estar escrito en C; algunos componentes pueden implementarse en otros lenguajes si tiene sentido (ej. .NET para el servidor).

---

## Estado actual

- ✔️ Estructuras de datos del diccionario definidas
- ✔️ Parsing y carga de JMDict
- ✔️ Base del SRS
- ✔️ CLI funcional en desarrollo
- 🚧 Diseño del servidor local
- 🚧 Sync y media

El proyecto está en **fase experimental / exploratoria**.

---

## Motivación

Este proyecto nace de dos intereses principales:

1. **Estudio serio del japonés**, con control total sobre los datos
2. **Diseño de software de bajo nivel**, donde las estructuras importan

Kotoba Plus prioriza **claridad, control y aprendizaje técnico** por sobre rapidez de desarrollo.

---

## Licencia

Por definir.

JMDict se utiliza respetando su licencia correspondiente.

# INFORME TÉCNICO DE ARQUITECTURA - FASE I
## Simulador de Kernel y Gestor de Procesos en Consola (Shell & Scheduling Engine)

**Universidad Católica Andrés Bello (UCAB)**  
**Facultad de Ingeniería - Escuela de Ingeniería Informática**  
**Cátedra:** Sistemas Operativos  
**Profesor:** Marcel J. Castro G.  
**Integrantes:**  
- Andrés Querecuto (`aaquerecuto.25@est.ucab.edu.ve`)
- Ángel Bolívar (`asbolivar.24@est.ucab.edu.ve`)

---

### 1. Resumen Ejecutivo
El proyecto consiste en el diseño e implementación en lenguaje C (estándar C11) de un núcleo ejecutor simulado y una Shell interactiva de gestión para un sistema operativo de bajo consumo orientado a robótica industrial. La solución aborda la coexistencia concurrente de una interfaz de línea de comandos (CLI) no bloqueante con un motor de planificación de CPU basado en reloj simulado discreto (*clock ticks*), garantizando integridad y sincronización mediante cerrojos POSIX (`pthread_mutex_t`).

---

### 2. Estructura Modular del Sistema

El sistema está organizado bajo una arquitectura modular limpia con separación estricta de interfaces (`include/`) e implementaciones (`src/`):

- **Módulo PCB (`include/pcb.h`, `src/pcb.c`):**
  Modela el Process Control Block con identificación única autoincremental (PID), prioridades (1 a 5), ráfaga de CPU y seguimiento exhaustivo del ciclo de vida y métricas temporales ($WT$, $TAT$, $RT$).
- **Módulo de Sincronización y Colas (`include/process_queue.h`, `src/process_queue.c`):**
  Estructura de cola genérica y protegida por exclusión mutua para la manipulación concurrente de procesos sin condiciones de carrera.
- **Módulo de Algoritmos de Planificación (`include/scheduler_algorithms.h`, `src/scheduler_algorithms.c`):**
  Implementa las políticas **Round Robin (RR)** con quantum dinámico y **Shortest Remaining Time First (SRTF)** con desapropiación inmediata (*preemption*).
- **Módulo Motor de CPU (`include/cpu_scheduler.h`, `src/cpu_scheduler.c`):**
  Hilo independiente de simulación de CPU, reloj discreto y control de velocidad en milisegundos (`set_speed`).
- **Módulo Batch Loader (`include/batch_loader.h`, `src/batch_loader.c`):**
  Parser de archivos de trazas de trabajo por lotes (`workload_test.txt`).
- **Módulo Diagrama de Gantt y Métricas (`include/metrics_gantt.h`, `src/metrics_gantt.c`):**
  Registro cronológico de los intervalos de uso de CPU y cálculo estadístico global ($WT$, $TAT$, $RT$ y % de uso de CPU).
- **Módulo CLI (`include/cli.h`, `src/cli.c`):**
  Shell interactiva para el operador con validación rigurosa de sintaxis, tipos y manejo tolerante a fallos.

---

### 3. Diagrama de Arquitectura Concurrente y Sincronización

```
  +--------------------------------------------------------------+
  |                     HILO CLI INTERACTIVO                     |
  |  (Lectura de comandos, validaciones, creación, ps, pcb, etc) |
  +--------------------------------------------------------------+
                                  |
              pthread_mutex_lock  |  pthread_mutex_unlock
                                  v
     +-------------------------------------------------------+
     |                 ESTRUCTURAS COMPARTIDAS               |
     |  - Ready Queue (Cola de LISTOS)                       |
     |  - All Processes Queue (Registro global de procesos)  |
     |  - Gantt Tracker (Historial de segmentos de CPU)      |
     |  - Variables de control (running, paused, speed, etc) |
     +-------------------------------------------------------+
                                  ^
              pthread_mutex_lock  |  pthread_mutex_unlock
                                  |
  +--------------------------------------------------------------+
  |                   HILO MOTOR DE SCHEDULING                   |
  |     (Reloj discreto, retardo Sleep, políticas RR y SRTF)     |
  +--------------------------------------------------------------+
```

---

### 4. Referencias Bibliográficas
1. Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). *Operating System Concepts* (10th ed.). Wiley.
2. Tanenbaum, A. S., & Bos, H. (2014). *Modern Operating Systems* (4th ed.). Pearson.
3. Kerrisk, M. (2010). *The Linux Programming Interface: A Linux and UNIX System Programming Handbook*. No Starch Press.
4. IEEE and The Open Group. *POSIX.1-2017 Specification: Threads (pthread)*.

---

### 5. Anexo Obligatorio: Declaración del Uso de Inteligencia Artificial
En cumplimiento con lo estipulado en la Sección 8 y 9 de las especificaciones del Proyecto - Fase I:

- **Herramienta Utilizada:** Antigravity (Google DeepMind / Gemini 3.8 Flash).
- **Rol Desempeñado:** Asistente técnico de programación en pares (*pair programmer*), estructuración modular del código en C11 y gestor de automatización con GitHub CLI y GitHub Projects.
- **Prompts Principales Empleados:**
  1. *"Inicializa el repositorio Git en la rama `main`, define la estructura de directorios del proyecto en C11 (`include/`, `src/`, `build/`, `bin/`) con un Makefile que soporte compilación cruzada en Windows y Linux, incluyendo los flags `-std=c11 -Wall -Wextra -pthread` y un target `clean` correctamente implementado. Commitea el estado inicial y vincúlalo al remoto de GitHub."* → Configuración del entorno de desarrollo, sistema de compilación y flujo de trabajo Git con ramas `main` y `develop`.
  2. *"Instala y configura la CLI oficial de GitHub (`gh`) en el sistema Windows y autentica la sesión mediante el flujo de dispositivo OAuth para habilitar la gestión programática de repositorios, Issues y GitHub Projects desde la línea de comandos."* → Integración del entorno CI/CD con GitHub CLI y automatización del tablero Kanban.
  3. *"Descompón el sistema en módulos cohesivos siguiendo el principio de separación de responsabilidades. Para cada módulo define un Issue en GitHub con título, descripción funcional y criterios de aceptación. Luego organízalos como tarjetas en un tablero Kanban (GitHub Projects) con columnas Todo / In Progress / Done, de forma que el flujo de desarrollo sea trazable."* → Desglose del sistema en 10 módulos funcionales con Issues estructurados y tablero Kanban de seguimiento.
  4. *"Implementa el Módulo 6 (Shell CLI concurrente) como un hilo POSIX independiente que lea comandos por `stdin` sin bloquear el hilo del motor de CPU. Incluye validación estricta de tipos y aridad para cada comando (`create`, `ps`, `pcb`, `set_sched`, `set_speed`, `start`, `pause`, `gantt`, `stats`, `help`, `exit`), con mensajes de error descriptivos diferenciados por categoría (`[ERROR DE SINTAXIS]`, `[ERROR DE TIPO]`, `[ERROR DE PARÁMETRO]`). Implementa también el Módulo 7 con un parser de archivos de trazas por lotes en formato CSV comentado (`# NOMBRE, BURST, PRIORIDAD, ARRIVAL_TIME`), con manejo robusto de líneas malformadas y retroalimentación de las inserciones exitosas."* → Implementación de la interfaz interactiva y del sistema de carga por lotes con manejo de errores tolerante a fallos.
  5. *"Implementa el Módulo 8 con un `GanttTracker` protegido por `pthread_mutex_t` que registre por cada tick de CPU el intervalo `[inicio, fin)` y el PID del proceso en ejecución, comprimiendo segmentos consecutivos del mismo proceso para optimizar la representación. Implementa `gantt_print()` que produzca una salida en formato `[t0-t1: Nombre] -> [t1-t2: Nombre]` y `stats_print()` que calcule en tiempo real $\bar{WT}$, $\bar{TAT}$, $\bar{RT}$ y el porcentaje de utilización de CPU sobre los procesos en estado `TERMINATED`. Valida los escenarios obligatorios del enunciado: (1) modo interactivo con cambio en caliente de política de scheduling y (2) carga por lotes con desapropiación SRTF."* → Implementación del diagrama de Gantt, estadísticas del sistema y validación de los escenarios de prueba requeridos.
- **Validación y Pensamiento Crítico:**
  Todo el código generado fue inspeccionado, compilado bajo `-std=c11 -Wall -Wextra -pthread` con GCC/MinGW y verificado contra los formatos exactos de salida exigidos en la pauta (comandos `ps`, `pcb <PID>`, `gantt` y `stats`), garantizando la ausencia de condiciones de carrera mediante mutex POSIX.

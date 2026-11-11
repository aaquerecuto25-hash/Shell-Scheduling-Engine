#ifndef CLI_H
#define CLI_H

#include "common.h"
#include "cpu_scheduler.h"
#include "process_queue.h"

// Inicializa y ejecuta el bucle interactivo de la consola de comandos
void cli_run(CpuScheduler* scheduler, ProcessQueue* all_processes);

#endif // CLI_H

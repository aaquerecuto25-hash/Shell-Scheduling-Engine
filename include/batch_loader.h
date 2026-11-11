#ifndef BATCH_LOADER_H
#define BATCH_LOADER_H

#include "common.h"
#include "pcb.h"
#include "process_queue.h"

// Carga procesos desde un archivo con formato:
// # NOMBRE, BURST_TIME, PRIORIDAD, ARRIVAL_TIME
// Job_A, 4, 2, 0
// Retorna la cantidad de procesos insertados con éxito, o un valor negativo en caso de error.
int batch_load_file(const char* filepath, ProcessQueue* destination_queue, ProcessQueue* all_processes_queue);

#endif // BATCH_LOADER_H

#ifndef PCB_H
#define PCB_H

#include "common.h"

#define MAX_NAME_LEN 64
#define DEFAULT_PRIORITY 3
#define MIN_PRIORITY 1
#define MAX_PRIORITY 5

typedef enum {
    PROCESS_STATE_NEW,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} ProcessState;

typedef struct Process {
    int pid;
    char name[MAX_NAME_LEN];
    ProcessState state;
    int priority;           // Rango 1 - 5 (defecto 3)
    
    // Tiempos (medidos en clock ticks)
    int arrival_time;       // Tiempo de llegada al sistema
    int burst_time;         // Ráfaga inicial requerida
    int remaining_time;     // Ráfaga restante por ejecutar
    int waiting_time;       // Tiempo acumulado en espera (WT)
    int turnaround_time;    // Tiempo total de retorno (TAT)
    int response_time;      // Tiempo hasta primera asignación de CPU (RT)
    int first_run_time;     // Tick de primera ejecución (-1 si no ha ejecutado)
    int completion_time;    // Tick en que finalizó (-1 si aún no termina)
    
    struct Process* next;   // Enlace para colas y listas enlazadas
} Process;

// Funciones de ciclo de vida del PCB
Process* pcb_create(const char* name, int burst_time, int priority, int arrival_time);
void pcb_destroy(Process* process);
const char* pcb_state_to_string(ProcessState state);
void pcb_print_details(const Process* process);

#endif // PCB_H

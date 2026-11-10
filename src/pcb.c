#include "pcb.h"

static int g_next_pid = 1;

Process* pcb_create(const char* name, int burst_time, int priority, int arrival_time) {
    if (!name || burst_time <= 0) {
        return NULL;
    }

    if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
        priority = DEFAULT_PRIORITY;
    }

    Process* proc = (Process*)malloc(sizeof(Process));
    if (!proc) {
        perror("Error al asignar memoria para PCB");
        return NULL;
    }

    proc->pid = g_next_pid++;
    strncpy(proc->name, name, MAX_NAME_LEN - 1);
    proc->name[MAX_NAME_LEN - 1] = '\0';

    proc->state = PROCESS_STATE_NEW;
    proc->priority = priority;

    proc->arrival_time = arrival_time;
    proc->burst_time = burst_time;
    proc->remaining_time = burst_time;

    proc->waiting_time = 0;
    proc->turnaround_time = 0;
    proc->response_time = -1;
    proc->first_run_time = -1;
    proc->completion_time = -1;

    proc->next = NULL;

    return proc;
}

void pcb_destroy(Process* process) {
    if (process) {
        free(process);
    }
}

const char* pcb_state_to_string(ProcessState state) {
    switch (state) {
        case PROCESS_STATE_NEW:
            return "NEW";
        case PROCESS_STATE_READY:
            return "READY";
        case PROCESS_STATE_RUNNING:
            return "RUNNING";
        case PROCESS_STATE_BLOCKED:
            return "BLOCKED";
        case PROCESS_STATE_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

void pcb_print_details(const Process* process) {
    if (!process) {
        printf("[ERROR]: PCB no valido o nulo.\n");
        return;
    }

    printf("==================================================\n");
    printf("CONTROL BLOCK DE PROCESO (PCB) - PID: %d\n", process->pid);
    printf("==================================================\n");
    printf("Nombre del Proceso : %s\n", process->name);
    printf("Estado Actual      : %s\n", pcb_state_to_string(process->state));
    printf("Prioridad          : %d\n", process->priority);
    printf("Tiempo de Llegada  : %d ticks\n", process->arrival_time);
    printf("Ráfaga Inicial     : %d ticks\n", process->burst_time);
    printf("Ráfaga Restante    : %d ticks\n", process->remaining_time);
    printf("Tiempo de Espera   : %d ticks\n", process->waiting_time);
    
    if (process->state == PROCESS_STATE_TERMINATED) {
        printf("Tiempo de Retorno  : %d ticks\n", process->turnaround_time);
    } else {
        printf("Tiempo de Retorno  : N/A (En ejecución)\n");
    }

    if (process->response_time >= 0) {
        printf("Tiempo de Respuesta: %d ticks\n", process->response_time);
    } else {
        printf("Tiempo de Respuesta: N/A (No iniciado)\n");
    }
    printf("==================================================\n");
}

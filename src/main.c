#include "common.h"
#include "pcb.h"
#include "process_queue.h"
#include "cpu_scheduler.h"
#include "cli.h"

static void on_process_complete(Process* proc, int finish_tick) {
    (void)finish_tick;
    // El proceso se mantiene en all_processes con estado TERMINATED para consultas ps, pcb y stats.
    proc->state = PROCESS_STATE_TERMINATED;
}

int main(void) {
    ProcessQueue ready_queue;
    ProcessQueue all_processes;
    queue_init(&ready_queue);
    queue_init(&all_processes);

    CpuScheduler scheduler;
    scheduler_init(&scheduler, &ready_queue, &all_processes, on_process_complete);

    // Ejecutar Shell Interactiva en el hilo principal
    cli_run(&scheduler, &all_processes);

    // Limpieza al salir
    scheduler_stop(&scheduler);
    scheduler_join(&scheduler);
    scheduler_destroy(&scheduler);

    queue_destroy(&ready_queue);
    queue_destroy(&all_processes);

    printf("[Main]: Sistema finalizado correctamente.\n");
    return 0;
}

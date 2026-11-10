#include "common.h"
#include "pcb.h"
#include "process_queue.h"
#include "cpu_scheduler.h"

static void on_process_complete(Process* proc, int finish_tick) {
    printf("\n[COMPLETADO] PID %d (%s) | Fin tick: %d | TAT: %d | WT: %d | RT: %d\n",
           proc->pid, proc->name,
           finish_tick,
           proc->turnaround_time,
           proc->waiting_time,
           proc->response_time);
    pcb_destroy(proc);
}

int main(void) {
    printf("============================================================\n");
    printf("  Simulador de Kernel - Modulo 4: Motor de Scheduling\n");
    printf("============================================================\n\n");

    ProcessQueue ready_queue;
    queue_init(&ready_queue);

    Process* p1 = pcb_create("ProcesoA", 4, 3, 0);
    Process* p2 = pcb_create("ProcesoB", 2, 2, 0);
    Process* p3 = pcb_create("ProcesoC", 6, 4, 0);

    p1->state = PROCESS_STATE_READY;
    p2->state = PROCESS_STATE_READY;
    p3->state = PROCESS_STATE_READY;

    queue_enqueue(&ready_queue, p1);
    queue_enqueue(&ready_queue, p2);
    queue_enqueue(&ready_queue, p3);

    printf("[Main] Cola de listos inicializada con %d procesos.\n\n",
           queue_size(&ready_queue));

    CpuScheduler scheduler;
    scheduler_init(&scheduler, &ready_queue, on_process_complete);

    scheduler_set_speed(&scheduler, 300);
    scheduler_start(&scheduler);

    struct timespec ts1 = { .tv_sec = 1, .tv_nsec = 350000000L };
    nanosleep(&ts1, NULL);

    scheduler_print_status(&scheduler);

    printf("\n[Main] Pausando motor por 1 segundo...\n");
    scheduler_pause(&scheduler);

    struct timespec ts2 = { .tv_sec = 1, .tv_nsec = 0 };
    nanosleep(&ts2, NULL);

    scheduler_print_status(&scheduler);

    printf("\n[Main] Reanudando motor...\n");
    scheduler_resume(&scheduler);

    struct timespec ts3 = { .tv_sec = 3, .tv_nsec = 0 };
    nanosleep(&ts3, NULL);

    scheduler_stop(&scheduler);
    scheduler_join(&scheduler);

    printf("\n");
    scheduler_print_status(&scheduler);

    scheduler_destroy(&scheduler);
    queue_destroy(&ready_queue);

    printf("\n[Main] Simulacion finalizada correctamente.\n");
    return 0;
}

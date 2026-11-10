#include "common.h"
#include "pcb.h"
#include "process_queue.h"
#include "cpu_scheduler.h"
#include "scheduler_algorithms.h"

static void on_process_complete(Process* proc, int finish_tick) {
    printf("[COMPLETADO] PID %d (%s) | Fin tick: %d | TAT: %d | WT: %d | RT: %d\n",
           proc->pid, proc->name,
           finish_tick,
           proc->turnaround_time,
           proc->waiting_time,
           proc->response_time);
    pcb_destroy(proc);
}

static void run_simulation(SchedulerPolicy policy, int quantum) {
    printf("\n------------------------------------------------------------\n");
    printf("  Simulacion: %s", policy_to_string(policy));
    if (policy == POLICY_RR) printf(" (quantum = %d)", quantum);
    printf("\n------------------------------------------------------------\n");

    ProcessQueue ready_queue;
    queue_init(&ready_queue);

    Process* p1 = pcb_create("ProcA", 4, 3, 0);
    Process* p2 = pcb_create("ProcB", 2, 2, 0);
    Process* p3 = pcb_create("ProcC", 6, 4, 0);

    p1->state = PROCESS_STATE_READY;
    p2->state = PROCESS_STATE_READY;
    p3->state = PROCESS_STATE_READY;

    queue_enqueue(&ready_queue, p1);
    queue_enqueue(&ready_queue, p2);
    queue_enqueue(&ready_queue, p3);

    CpuScheduler scheduler;
    scheduler_init(&scheduler, &ready_queue, on_process_complete);
    scheduler_set_policy(&scheduler, policy, quantum);
    scheduler_set_speed(&scheduler, 250);
    scheduler_start(&scheduler);

    struct timespec ts = { .tv_sec = 4, .tv_nsec = 0 };
    nanosleep(&ts, NULL);

    scheduler_stop(&scheduler);
    scheduler_join(&scheduler);

    printf("\n");
    scheduler_print_status(&scheduler);

    scheduler_destroy(&scheduler);
    queue_destroy(&ready_queue);
}

int main(void) {
    printf("============================================================\n");
    printf("  Simulador de Kernel - Modulo 5: Algoritmos de Scheduling\n");
    printf("============================================================\n");

    run_simulation(POLICY_RR, 2);

    run_simulation(POLICY_SRTF, 1);

    printf("\n[Main] Simulacion finalizada correctamente.\n");
    return 0;
}

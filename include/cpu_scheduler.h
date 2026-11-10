#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

#include "common.h"
#include "pcb.h"
#include "process_queue.h"
#include "scheduler_algorithms.h"

#define DEFAULT_MS_PER_TICK 500
#define MIN_MS_PER_TICK      50
#define MAX_MS_PER_TICK    5000
#define DEFAULT_QUANTUM      1

typedef struct {
    volatile int    clock_ticks;

    int             ms_per_tick;
    pthread_mutex_t speed_mutex;

    volatile bool   running;
    volatile bool   paused;
    volatile bool   stop_requested;

    pthread_mutex_t ctrl_mutex;
    pthread_cond_t  ctrl_cond;

    pthread_t thread;
    bool      thread_started;

    ProcessQueue* ready_queue;

    Process*        current_process;
    pthread_mutex_t current_mutex;

    SchedulerPolicy policy;
    int             quantum;
    pthread_mutex_t policy_mutex;

    void (*on_process_complete)(Process* proc, int finish_tick);

} CpuScheduler;

void scheduler_init(CpuScheduler* scheduler,
                    ProcessQueue* ready_queue,
                    void (*on_complete)(Process*, int));

void scheduler_destroy(CpuScheduler* scheduler);

int  scheduler_start(CpuScheduler* scheduler);

void scheduler_pause(CpuScheduler* scheduler);

void scheduler_resume(CpuScheduler* scheduler);

void scheduler_stop(CpuScheduler* scheduler);

void scheduler_join(CpuScheduler* scheduler);

void scheduler_set_speed(CpuScheduler* scheduler, int ms);

void scheduler_set_policy(CpuScheduler* scheduler, SchedulerPolicy policy, int quantum);

SchedulerPolicy scheduler_get_policy(CpuScheduler* scheduler);

int  scheduler_get_tick(const CpuScheduler* scheduler);

Process* scheduler_get_current(CpuScheduler* scheduler);

void scheduler_print_status(CpuScheduler* scheduler);

#endif

#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

#include "common.h"
#include "pcb.h"
#include "process_queue.h"

#define DEFAULT_MS_PER_TICK 500
#define MIN_MS_PER_TICK      50
#define MAX_MS_PER_TICK    5000

typedef struct {
    volatile int   clock_ticks;

    int            ms_per_tick;
    pthread_mutex_t speed_mutex;

    volatile bool  running;
    volatile bool  paused;
    volatile bool  stop_requested;

    pthread_mutex_t  ctrl_mutex;
    pthread_cond_t   ctrl_cond;

    pthread_t thread;
    bool      thread_started;

    ProcessQueue* ready_queue;

    Process* current_process;
    pthread_mutex_t current_mutex;

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

int  scheduler_get_tick(const CpuScheduler* scheduler);

Process* scheduler_get_current(CpuScheduler* scheduler);

void scheduler_print_status(CpuScheduler* scheduler);

#endif

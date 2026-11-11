#include "cpu_scheduler.h"

#ifdef _WIN32
#include <windows.h>
#endif

static void sleep_ms(int ms) {
    if (ms <= 0) return;
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

static void* scheduler_thread_func(void* arg) {
    CpuScheduler* s = (CpuScheduler*)arg;
    Process* running = NULL;
    int ticks_on_cpu = 0;

    while (1) {
        pthread_mutex_lock(&s->ctrl_mutex);
        if (s->stop_requested) {
            pthread_mutex_unlock(&s->ctrl_mutex);
            break;
        }
        while (s->paused && !s->stop_requested) {
            pthread_cond_wait(&s->ctrl_cond, &s->ctrl_mutex);
        }
        if (s->stop_requested) {
            pthread_mutex_unlock(&s->ctrl_mutex);
            break;
        }
        pthread_mutex_unlock(&s->ctrl_mutex);

        pthread_mutex_lock(&s->speed_mutex);
        int ms = s->ms_per_tick;
        pthread_mutex_unlock(&s->speed_mutex);

        pthread_mutex_lock(&s->policy_mutex);
        SchedulerPolicy policy = s->policy;
        int quantum = s->quantum;
        pthread_mutex_unlock(&s->policy_mutex);

        int tick = s->clock_ticks;

        Process* next;
        if (policy == POLICY_RR) {
            next = rr_select(s->ready_queue, running, &ticks_on_cpu, quantum);
        } else {
            next = srtf_select(s->ready_queue, running);
            if (next != running) ticks_on_cpu = 0;
        }
        running = next;

        pthread_mutex_lock(&s->current_mutex);
        s->current_process = running;
        pthread_mutex_unlock(&s->current_mutex);

        // Registrar tick en el diagrama de Gantt
        gantt_record_tick(&s->gantt_tracker, tick, running);

        if (running != NULL) {
            if (running->first_run_time < 0) {
                running->first_run_time = tick;
                running->response_time  = tick - running->arrival_time;
                running->state          = PROCESS_STATE_RUNNING;
            }

            running->remaining_time--;
            ticks_on_cpu++;

            if (running->remaining_time <= 0) {
                running->state           = PROCESS_STATE_TERMINATED;
                running->completion_time = tick + 1;
                running->turnaround_time = running->completion_time - running->arrival_time;
                running->waiting_time    = running->turnaround_time - running->burst_time;
                if (running->waiting_time < 0) running->waiting_time = 0;

                if (s->on_process_complete) {
                    s->on_process_complete(running, running->completion_time);
                }
                running = NULL;
                ticks_on_cpu = 0;
            }
        }

        pthread_mutex_lock(&s->current_mutex);
        s->current_process = running;
        pthread_mutex_unlock(&s->current_mutex);

        s->clock_ticks = tick + 1;
        sleep_ms(ms);
    }

    if (running != NULL) {
        running->state = PROCESS_STATE_READY;
        queue_enqueue(s->ready_queue, running);
    }

    pthread_mutex_lock(&s->current_mutex);
    s->current_process = NULL;
    pthread_mutex_unlock(&s->current_mutex);

    pthread_mutex_lock(&s->ctrl_mutex);
    s->running = false;
    pthread_mutex_unlock(&s->ctrl_mutex);

    return NULL;
}

void scheduler_init(CpuScheduler* scheduler,
                    ProcessQueue* ready_queue,
                    ProcessQueue* all_processes,
                    void (*on_complete)(Process*, int)) {
    if (!scheduler) return;

    scheduler->clock_ticks     = 0;
    scheduler->ms_per_tick     = DEFAULT_MS_PER_TICK;
    scheduler->running         = false;
    scheduler->paused          = false;
    scheduler->stop_requested  = false;
    scheduler->thread_started  = false;
    scheduler->ready_queue     = ready_queue;
    scheduler->all_processes   = all_processes;
    scheduler->current_process = NULL;
    scheduler->policy          = POLICY_RR;
    scheduler->quantum         = DEFAULT_QUANTUM;
    scheduler->on_process_complete = on_complete;

    gantt_init(&scheduler->gantt_tracker);

    pthread_mutex_init(&scheduler->speed_mutex,   NULL);
    pthread_mutex_init(&scheduler->ctrl_mutex,    NULL);
    pthread_cond_init (&scheduler->ctrl_cond,     NULL);
    pthread_mutex_init(&scheduler->current_mutex, NULL);
    pthread_mutex_init(&scheduler->policy_mutex,  NULL);
}

void scheduler_destroy(CpuScheduler* scheduler) {
    if (!scheduler) return;

    gantt_destroy(&scheduler->gantt_tracker);

    pthread_mutex_destroy(&scheduler->speed_mutex);
    pthread_mutex_destroy(&scheduler->ctrl_mutex);
    pthread_cond_destroy (&scheduler->ctrl_cond);
    pthread_mutex_destroy(&scheduler->current_mutex);
    pthread_mutex_destroy(&scheduler->policy_mutex);
}

int scheduler_start(CpuScheduler* scheduler) {
    if (!scheduler) return -1;

    pthread_mutex_lock(&scheduler->ctrl_mutex);

    if (scheduler->thread_started) {
        scheduler->paused = false;
        pthread_cond_signal(&scheduler->ctrl_cond);
        pthread_mutex_unlock(&scheduler->ctrl_mutex);
        return 0;
    }

    scheduler->running        = true;
    scheduler->paused         = false;
    scheduler->stop_requested = false;
    scheduler->clock_ticks    = 0;

    int rc = pthread_create(&scheduler->thread, NULL,
                            scheduler_thread_func, scheduler);
    if (rc != 0) {
        scheduler->running = false;
        pthread_mutex_unlock(&scheduler->ctrl_mutex);
        perror("[Scheduler] Error al crear hilo");
        return -1;
    }

    scheduler->thread_started = true;
    pthread_mutex_unlock(&scheduler->ctrl_mutex);

    printf("[Scheduler] Motor iniciado. Politica: %s | Quantum: %d | Velocidad: %d ms/tick.\n",
           policy_to_string(scheduler->policy),
           scheduler->quantum,
           scheduler->ms_per_tick);
    return 0;
}

void scheduler_pause(CpuScheduler* scheduler) {
    if (!scheduler) return;

    pthread_mutex_lock(&scheduler->ctrl_mutex);
    if (scheduler->running && !scheduler->paused) {
        scheduler->paused = true;
        printf("[Scheduler] Motor pausado en tick %d.\n", scheduler->clock_ticks);
    }
    pthread_mutex_unlock(&scheduler->ctrl_mutex);
}

void scheduler_resume(CpuScheduler* scheduler) {
    if (!scheduler) return;

    pthread_mutex_lock(&scheduler->ctrl_mutex);
    if (scheduler->paused) {
        scheduler->paused = false;
        pthread_cond_signal(&scheduler->ctrl_cond);
        printf("[Scheduler] Motor reanudado en tick %d.\n", scheduler->clock_ticks);
    }
    pthread_mutex_unlock(&scheduler->ctrl_mutex);
}

void scheduler_stop(CpuScheduler* scheduler) {
    if (!scheduler || !scheduler->thread_started) return;

    pthread_mutex_lock(&scheduler->ctrl_mutex);
    scheduler->stop_requested = true;
    scheduler->paused         = false;
    pthread_cond_signal(&scheduler->ctrl_cond);
    pthread_mutex_unlock(&scheduler->ctrl_mutex);

    printf("[Scheduler] Solicitud de detencion enviada.\n");
}

void scheduler_join(CpuScheduler* scheduler) {
    if (!scheduler || !scheduler->thread_started) return;

    pthread_join(scheduler->thread, NULL);
    scheduler->thread_started = false;
    printf("[Scheduler] Hilo del motor finalizado. Ticks totales: %d.\n",
           scheduler->clock_ticks);
}

void scheduler_set_speed(CpuScheduler* scheduler, int ms) {
    if (!scheduler) return;

    if (ms < MIN_MS_PER_TICK) ms = MIN_MS_PER_TICK;
    if (ms > MAX_MS_PER_TICK) ms = MAX_MS_PER_TICK;

    pthread_mutex_lock(&scheduler->speed_mutex);
    scheduler->ms_per_tick = ms;
    pthread_mutex_unlock(&scheduler->speed_mutex);

    printf("[Scheduler] Velocidad ajustada a %d ms/tick.\n", ms);
}

void scheduler_set_policy(CpuScheduler* scheduler, SchedulerPolicy policy, int quantum) {
    if (!scheduler) return;
    if (quantum < 1) quantum = 1;

    pthread_mutex_lock(&scheduler->policy_mutex);
    scheduler->policy  = policy;
    scheduler->quantum = quantum;
    pthread_mutex_unlock(&scheduler->policy_mutex);

    printf("[Scheduler] Politica: %s | Quantum: %d tick(s).\n",
           policy_to_string(policy), quantum);
}

SchedulerPolicy scheduler_get_policy(CpuScheduler* scheduler) {
    if (!scheduler) return POLICY_RR;

    pthread_mutex_lock(&scheduler->policy_mutex);
    SchedulerPolicy p = scheduler->policy;
    pthread_mutex_unlock(&scheduler->policy_mutex);
    return p;
}

int scheduler_get_tick(const CpuScheduler* scheduler) {
    if (!scheduler) return -1;
    return scheduler->clock_ticks;
}

Process* scheduler_get_current(CpuScheduler* scheduler) {
    if (!scheduler) return NULL;

    pthread_mutex_lock(&scheduler->current_mutex);
    Process* p = scheduler->current_process;
    pthread_mutex_unlock(&scheduler->current_mutex);
    return p;
}

void scheduler_print_status(CpuScheduler* scheduler) {
    if (!scheduler) return;

    pthread_mutex_lock(&scheduler->ctrl_mutex);
    bool running = scheduler->running;
    bool paused  = scheduler->paused;
    int  ticks   = scheduler->clock_ticks;
    pthread_mutex_unlock(&scheduler->ctrl_mutex);

    pthread_mutex_lock(&scheduler->speed_mutex);
    int ms = scheduler->ms_per_tick;
    pthread_mutex_unlock(&scheduler->speed_mutex);

    pthread_mutex_lock(&scheduler->policy_mutex);
    SchedulerPolicy policy  = scheduler->policy;
    int             quantum = scheduler->quantum;
    pthread_mutex_unlock(&scheduler->policy_mutex);

    pthread_mutex_lock(&scheduler->current_mutex);
    Process* cur = scheduler->current_process;
    pthread_mutex_unlock(&scheduler->current_mutex);

    printf("==================================================\n");
    printf("MOTOR DE SCHEDULING - ESTADO\n");
    printf("==================================================\n");
    printf("Estado         : %s\n",
           !running ? "DETENIDO" : (paused ? "PAUSADO" : "ACTIVO"));
    printf("Politica       : %s\n", policy_to_string(policy));
    printf("Quantum        : %d tick(s)\n", quantum);
    printf("Tick actual    : %d\n", ticks);
    printf("ms por tick    : %d ms\n", ms);
    printf("Cola de listos : %d proceso(s)\n", queue_size(scheduler->ready_queue));
    if (cur) {
        printf("En CPU         : PID %d (%s) - restante: %d tick(s)\n",
               cur->pid, cur->name, cur->remaining_time);
    } else {
        printf("En CPU         : [IDLE]\n");
    }
    printf("==================================================\n");
}

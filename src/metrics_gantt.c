#include "metrics_gantt.h"

void gantt_init(GanttTracker* tracker) {
    if (!tracker) return;
    tracker->head = NULL;
    tracker->tail = NULL;
    tracker->total_cpu_ticks = 0;
    tracker->total_idle_ticks = 0;
    pthread_mutex_init(&tracker->mutex, NULL);
}

void gantt_destroy(GanttTracker* tracker) {
    if (!tracker) return;
    pthread_mutex_lock(&tracker->mutex);
    GanttRecord* curr = tracker->head;
    while (curr) {
        GanttRecord* next = curr->next;
        free(curr);
        curr = next;
    }
    tracker->head = NULL;
    tracker->tail = NULL;
    pthread_mutex_unlock(&tracker->mutex);
    pthread_mutex_destroy(&tracker->mutex);
}

void gantt_record_tick(GanttTracker* tracker, int tick, Process* running_proc) {
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);
    int pid = running_proc ? running_proc->pid : -1;
    const char* name = running_proc ? running_proc->name : "IDLE";

    if (running_proc) {
        tracker->total_cpu_ticks++;
    } else {
        tracker->total_idle_ticks++;
    }

    if (tracker->tail != NULL && tracker->tail->pid == pid && tracker->tail->end_tick == tick) {
        // Continuar el segmento actual
        tracker->tail->end_tick = tick + 1;
    } else {
        // Iniciar un nuevo segmento
        GanttRecord* rec = (GanttRecord*)malloc(sizeof(GanttRecord));
        if (rec) {
            rec->start_tick = tick;
            rec->end_tick = tick + 1;
            rec->pid = pid;
            strncpy(rec->name, name, MAX_NAME_LEN - 1);
            rec->name[MAX_NAME_LEN - 1] = '\0';
            rec->next = NULL;

            if (!tracker->head) {
                tracker->head = rec;
                tracker->tail = rec;
            } else {
                tracker->tail->next = rec;
                tracker->tail = rec;
            }
        }
    }
    pthread_mutex_unlock(&tracker->mutex);
}

void gantt_print(GanttTracker* tracker, int current_tick) {
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);
    printf("\nDIAGRAMA DE GANTT (0 - %d ticks):\n", current_tick);
    if (!tracker->head) {
        printf("[Sin actividad registrada aun]\n");
        pthread_mutex_unlock(&tracker->mutex);
        return;
    }

    GanttRecord* curr = tracker->head;
    bool first = true;
    while (curr) {
        if (!first) {
            printf(" -> ");
        }
        first = false;

        if (curr->pid == -1) {
            printf("[%d-%ds: IDLE]", curr->start_tick, curr->end_tick);
        } else {
            printf("[%d-%ds: %s]", curr->start_tick, curr->end_tick, curr->name);
        }
        curr = curr->next;
    }
    printf("\n");
    pthread_mutex_unlock(&tracker->mutex);
}

void stats_print(ProcessQueue* all_procs, int total_clock_ticks, int idle_ticks) {
    if (!all_procs) return;

    queue_lock(all_procs);
    int total_processes = all_procs->size;
    int terminated_count = 0;
    double sum_wt = 0.0;
    double sum_tat = 0.0;
    double sum_rt = 0.0;

    Process* curr = all_procs->head;
    while (curr) {
        if (curr->state == PROCESS_STATE_TERMINATED) {
            terminated_count++;
            sum_wt += curr->waiting_time;
            sum_tat += curr->turnaround_time;
            sum_rt += (curr->response_time >= 0 ? curr->response_time : 0);
        }
        curr = curr->next;
    }
    queue_unlock(all_procs);

    double avg_wt = terminated_count > 0 ? (sum_wt / terminated_count) : 0.0;
    double avg_tat = terminated_count > 0 ? (sum_tat / terminated_count) : 0.0;
    double avg_rt = terminated_count > 0 ? (sum_rt / terminated_count) : 0.0;

    int busy_ticks = total_clock_ticks - idle_ticks;
    if (busy_ticks < 0) busy_ticks = 0;
    double cpu_usage = total_clock_ticks > 0 ? ((double)busy_ticks / total_clock_ticks) * 100.0 : 0.0;

    printf("\nESTADÍSTICAS DEL SISTEMA:\n");
    printf("- Procesos Finalizados      : %d / %d\n", terminated_count, total_processes);
    printf("- Tiempo Promedio de Espera : %.2f ticks\n", avg_wt);
    printf("- Tiempo Promedio de Retorno: %.2f ticks\n", avg_tat);
    printf("- Tiempo Promedio Respuesta : %.2f ticks\n", avg_rt);
    printf("- Uso de CPU                : %.1f%%\n", cpu_usage);
}

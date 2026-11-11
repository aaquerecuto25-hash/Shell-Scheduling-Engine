#ifndef METRICS_GANTT_H
#define METRICS_GANTT_H

#include "common.h"
#include "pcb.h"
#include "process_queue.h"

// Entrada en el registro de Gantt
typedef struct GanttRecord {
    int start_tick;
    int end_tick;
    char name[MAX_NAME_LEN];
    int pid; // -1 si IDLE
    struct GanttRecord* next;
} GanttRecord;

typedef struct {
    GanttRecord* head;
    GanttRecord* tail;
    int total_cpu_ticks;
    int total_idle_ticks;
    pthread_mutex_t mutex;
} GanttTracker;

void gantt_init(GanttTracker* tracker);
void gantt_destroy(GanttTracker* tracker);
void gantt_record_tick(GanttTracker* tracker, int tick, Process* running_proc);
void gantt_print(GanttTracker* tracker, int current_tick);
void stats_print(ProcessQueue* all_procs, int total_clock_ticks, int idle_ticks);

#endif // METRICS_GANTT_H

#ifndef SCHEDULER_ALGORITHMS_H
#define SCHEDULER_ALGORITHMS_H

#include "process_queue.h"

typedef enum {
    POLICY_RR,
    POLICY_SRTF
} SchedulerPolicy;

const char* policy_to_string(SchedulerPolicy policy);

Process* rr_select(ProcessQueue* q, Process* current, int* ticks_on_cpu, int quantum);

Process* srtf_select(ProcessQueue* q, Process* current);

#endif


#include "scheduler_algorithms.h"

const char* policy_to_string(SchedulerPolicy policy) {
    switch (policy) {
        case POLICY_RR:   return "Round Robin (RR)";
        case POLICY_SRTF: return "SRTF";
        default:          return "UNKNOWN";
    }
}

Process* rr_select(ProcessQueue* q, Process* current, int* ticks_on_cpu, int quantum) {
    if (current == NULL) {
        *ticks_on_cpu = 0;
        return queue_dequeue(q);
    }

    if (*ticks_on_cpu >= quantum) {
        *ticks_on_cpu = 0;
        current->state = PROCESS_STATE_READY;
        queue_enqueue(q, current);
        return queue_dequeue(q);
    }

    return current;
}

Process* srtf_select(ProcessQueue* q, Process* current) {
    if (current == NULL) {
        return queue_extract_min_remaining(q);
    }

    Process* candidate = queue_peek_min_remaining(q);
    if (candidate != NULL && candidate->remaining_time < current->remaining_time) {
        current->state = PROCESS_STATE_READY;
        queue_enqueue(q, current);
        return queue_extract_min_remaining(q);
    }

    return current;
}


#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include "common.h"
#include "pcb.h"

typedef struct ProcessQueue {
    Process* head;
    Process* tail;
    int size;
    pthread_mutex_t mutex;
} ProcessQueue;

// Inicialización y destrucción
void queue_init(ProcessQueue* q);
void queue_destroy(ProcessQueue* q);

// Control de exclusión mutua manual (cuando se requieran operaciones compuestas)
void queue_lock(ProcessQueue* q);
void queue_unlock(ProcessQueue* q);

// Operaciones hilo-seguras básicas
void queue_enqueue(ProcessQueue* q, Process* proc);
Process* queue_dequeue(ProcessQueue* q);
Process* queue_peek(ProcessQueue* q);
bool queue_is_empty(ProcessQueue* q);
int queue_size(ProcessQueue* q);

Process* queue_extract_min_remaining(ProcessQueue* q);
Process* queue_peek_min_remaining(ProcessQueue* q);
Process* queue_find_by_pid(ProcessQueue* q, int pid);
Process* queue_remove_by_pid(ProcessQueue* q, int pid);

#endif

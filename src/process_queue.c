#include "process_queue.h"

void queue_init(ProcessQueue* q) {
    if (!q) return;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
}

void queue_destroy(ProcessQueue* q) {
    if (!q) return;
    pthread_mutex_lock(&q->mutex);
    Process* curr = q->head;
    while (curr) {
        Process* next = curr->next;
        pcb_destroy(curr);
        curr = next;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
}

void queue_lock(ProcessQueue* q) {
    if (q) {
        pthread_mutex_lock(&q->mutex);
    }
}

void queue_unlock(ProcessQueue* q) {
    if (q) {
        pthread_mutex_unlock(&q->mutex);
    }
}

void queue_enqueue(ProcessQueue* q, Process* proc) {
    if (!q || !proc) return;

    pthread_mutex_lock(&q->mutex);
    proc->next = NULL;
    if (q->tail == NULL) {
        q->head = proc;
        q->tail = proc;
    } else {
        q->tail->next = proc;
        q->tail = proc;
    }
    q->size++;
    pthread_mutex_unlock(&q->mutex);
}

Process* queue_dequeue(ProcessQueue* q) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->mutex);
    if (q->head == NULL) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    Process* proc = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    proc->next = NULL;
    q->size--;
    pthread_mutex_unlock(&q->mutex);

    return proc;
}

Process* queue_peek(ProcessQueue* q) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->mutex);
    Process* proc = q->head;
    pthread_mutex_unlock(&q->mutex);

    return proc;
}

bool queue_is_empty(ProcessQueue* q) {
    if (!q) return true;

    pthread_mutex_lock(&q->mutex);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->mutex);

    return empty;
}

int queue_size(ProcessQueue* q) {
    if (!q) return 0;

    pthread_mutex_lock(&q->mutex);
    int s = q->size;
    pthread_mutex_unlock(&q->mutex);

    return s;
}

// Extrae el proceso con menor remaining_time (desempate por arrival_time)
Process* queue_extract_min_remaining(ProcessQueue* q) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->mutex);
    if (!q->head) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    Process* prev_best = NULL;
    Process* best = q->head;

    Process* prev = q->head;
    Process* curr = q->head->next;

    while (curr) {
        if (curr->remaining_time < best->remaining_time ||
            (curr->remaining_time == best->remaining_time && curr->arrival_time < best->arrival_time)) {
            best = curr;
            prev_best = prev;
        }
        prev = curr;
        curr = curr->next;
    }

    // Desconectar best de la lista
    if (best == q->head) {
        q->head = q->head->next;
        if (q->head == NULL) {
            q->tail = NULL;
        }
    } else {
        prev_best->next = best->next;
        if (best == q->tail) {
            q->tail = prev_best;
        }
    }

    best->next = NULL;
    q->size--;
    pthread_mutex_unlock(&q->mutex);

    return best;
}

Process* queue_find_by_pid(ProcessQueue* q, int pid) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->mutex);
    Process* curr = q->head;
    while (curr) {
        if (curr->pid == pid) {
            pthread_mutex_unlock(&q->mutex);
            return curr;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&q->mutex);

    return NULL;
}

Process* queue_remove_by_pid(ProcessQueue* q, int pid) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->mutex);
    Process* prev = NULL;
    Process* curr = q->head;

    while (curr) {
        if (curr->pid == pid) {
            if (prev == NULL) {
                q->head = curr->next;
                if (q->head == NULL) {
                    q->tail = NULL;
                }
            } else {
                prev->next = curr->next;
                if (curr == q->tail) {
                    q->tail = prev;
                }
            }
            curr->next = NULL;
            q->size--;
            pthread_mutex_unlock(&q->mutex);
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&q->mutex);

    return NULL;
}

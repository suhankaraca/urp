#include "queue.h"
#include <stdlib.h>
#include <string.h>

queue_t* queue_create() {
    queue_t* q = (queue_t*)malloc(sizeof(queue_t));
    if (!q) return NULL;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    return q;
}

void queue_destroy(queue_t* q) {
    if (!q) return;
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
    free(q);
}

bool enqueue_segment(queue_t* q, segment_t* seg) {
    pthread_mutex_lock(&q->mutex);
    if (q->count == QUEUE_SIZE) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }
    
    // We need to copy the segment because the buffer in server.c might be reused
    segment_t* copy = (segment_t*)malloc(sizeof(segment_t));
    if (!copy) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }
    memcpy(copy, seg, sizeof(segment_t));

    q->segments[q->tail] = copy;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

segment_t* dequeue_segment(queue_t* q) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    segment_t* seg = q->segments[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    pthread_mutex_unlock(&q->mutex);
    return seg;
}

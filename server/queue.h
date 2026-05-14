#pragma once

#include "../segment/segment.h"
#include <pthread.h>
#include <stdbool.h>

#define QUEUE_SIZE 1024

typedef struct {
    segment_t* segments[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} queue_t;

queue_t* queue_create();
void queue_destroy(queue_t* q);
bool enqueue_segment(queue_t* q, segment_t* seg);
segment_t* dequeue_segment(queue_t* q);

#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <assert.h>
#include "order.h"

#define QUEUE_SIZE (10)

typedef struct queue {
    int front;
    int back;
    size_t value_count;
    int values[QUEUE_SIZE];
} queue_t;

void enqueue(queue_t* queue, int value);
int dequeue(queue_t* queue);

#endif /* QUEUE_H */
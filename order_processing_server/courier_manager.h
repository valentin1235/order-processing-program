#ifndef COURIER_MANAGER_H
#define COURIER_MANAGER_H

#include "order_manager.h"

#define COURIER_QUEUE_SIZE (1000)

typedef struct courier {
    time_t took_order_at;
    time_t arrived_at;
    char target[ORDER_NAME_SIZE];
    order_t* order;
} courier_t;

void* process_courier_thread(void* p);

void* listen_targeted_delivery_event_thread(void* p);

void* listen_random_delivery_event_thread(void* p);


void init_courier_queue(void);
void enqueue_random_courier(courier_t* value);
courier_t* dequeue_courier(void);

#endif /* COURIER_MANAGER_H */

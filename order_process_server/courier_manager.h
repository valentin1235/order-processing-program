#ifndef COURIER_MANAGER_H
#define COURIER_MANAGER_H

#include "order_manager.h"

#define COURIER_QUEUE_SIZE (1000)

extern pthread_mutex_t g_random_courier_mutex;

typedef struct courier {
    time_t took_order_at;
    time_t arrived_at;
    char target[ORDER_NAME_SIZE];
    order_t* order;
} courier_t;

void* process_courier_thread(void* p);

void* listen_target_delivery_event_thread(void* p);

void deliver_order(courier_t* courier);

void init_random_courier_queue(void);

courier_t* dequeue_random_courier(void);

#endif /* COURIER_MANAGER_H */

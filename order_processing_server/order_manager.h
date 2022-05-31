#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <pthread.h>

#define ORDER_NAME_SIZE (100)
#define ORDER_ID_SIZE (100)
#define ORDER_LIST_SIZE (1000)

extern pthread_mutex_t g_order_mutex;

/* types */
typedef struct order {
    char order_id[ORDER_ID_SIZE];
    char name[ORDER_NAME_SIZE];
    unsigned int prep_time;
    time_t ready_at; /* null if an order is not processed */
    time_t taken_at;
} order_t;




/* methods */
void process_orders();

order_t* pop_random_order_or_null(void);

order_t* pop_order_or_null(const char* name);

void add_order(order_t* order); /* tmp */

void* cook(void* p);

#endif /* ORDER_MANAGER_H */

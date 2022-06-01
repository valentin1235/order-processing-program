#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <pthread.h>

#define ORDER_NAME_SIZE (100)
#define ORDER_ID_SIZE (100)
#define ORDER_LIST_SIZE (1000)

/* types */
typedef struct order {
    char order_id[ORDER_ID_SIZE];
    char name[ORDER_NAME_SIZE];
    unsigned int prep_time;
    time_t ready_at; /* null if an order is not processed */
    time_t taken_at;
} order_t;

extern pthread_t g_thread_cook;
extern pthread_mutex_t g_order_mutex;
extern order_t* g_ready_orders[ORDER_LIST_SIZE];
extern int g_ready_order_count;

/* methods */
void process_orders(const char* file_name);

order_t* pop_random_order_or_null(void);

void remove_order(size_t i);

#endif /* ORDER_MANAGER_H */

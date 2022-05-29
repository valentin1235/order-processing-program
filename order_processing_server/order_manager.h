#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <pthread.h>

#define ORDER_MESSAGE_SIZE (200)
#define ORDER_NAME_SIZE (50)
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




/* methods */
void* process_order_thread(void* p);

order_t* pop_random_order_or_null();

order_t* pop_order_or_null(const char* name);

#endif /* ORDER_MANAGER_H */

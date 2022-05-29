#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "courier_manager.h"
#include "server.h"
#include "utils/message.h"




typedef struct courier_queue {
    int front;
    int back;
    size_t value_count;
    courier_t* values[COURIER_QUEUE_SIZE];
} courier_queue_t;

static courier_queue_t s_courier_queue;



static void init_queue()
{
    pthread_mutex_lock(&g_mutex);
    {
        s_courier_queue.value_count = 0;
        s_courier_queue.front = 0;
        s_courier_queue.back = 0;
        memset(s_courier_queue.values, 0, sizeof(courier_queue_t) * COURIER_QUEUE_SIZE);
    }
    pthread_mutex_unlock(&g_mutex);
}

static void enqueue_courier(courier_t* value)
{
    assert(s_courier_queue.value_count < COURIER_QUEUE_SIZE);

    pthread_mutex_lock(&g_mutex);
    {
        s_courier_queue.values[s_courier_queue.back] = value;
        s_courier_queue.back = (s_courier_queue.back + 1) % COURIER_QUEUE_SIZE;
        ++s_courier_queue.value_count;
    }
    pthread_mutex_unlock(&g_mutex);

    printf("[enqueue] front:%d, back:%d, count:%lu\n", s_courier_queue.front, s_courier_queue.back, s_courier_queue.value_count);
}

static order_t* dequeue_courier()
{
    order_t* ret;
    if (s_courier_queue.value_count < 1) {
        return NULL;
    }

    pthread_mutex_lock(&g_mutex);
    {
        ret = s_courier_queue.values[s_courier_queue.front];
        s_courier_queue.front = (s_courier_queue.front + 1) % COURIER_QUEUE_SIZE;
        --s_courier_queue.value_count;
    }
    pthread_mutex_unlock(&g_mutex);

    printf("[dequeue] front:%d, back:%d, count:%lu, ret:%s\n", s_courier_queue.front, s_courier_queue.back, s_courier_queue.value_count, ret->name);

    return ret;
}

void* process_courier_thread(void* p)
{
    courier_t* courier;
    char* pa_message = (char*)p;
    int sec_taken_to_arrive;

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    if (strcmp(strtok(pa_message, "#"), "COURIER") != 0) {
        printf(MESSAGE_INVALID_SERVICE);
        goto end;
    }

    courier = malloc(sizeof(courier_t));
    
    
    strcmp(courier->target, strtok(pa_message, "#"));
    sec_taken_to_arrive = atoi(strtok(pa_message, "#"));

    /* sleep thread till courier arrive */
    sleep(sec_taken_to_arrive);

    courier->arrived_at = time(NULL);
    courier->order = NULL;

    /* add order */
    init_queue();
    enqueue_courier(courier);

    printf("배달원이 도착했습니다 : %d\n", (int)courier->arrived_at);

end:
    pthread_exit((void*)0);
}

static void deliver_order(courier_t* courier)
{
    free(courier->order);
    free(courier);
    printf("주문이 배송되었습니다 : %d\n", (int)courier->took_order_at);
    print_time_records();
    
}


void* listen_targeted_delivery_event_thread(void* p)
{
    courier_t* courier = NULL;

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    do {
        order_t* order = NULL;
        courier = dequeue_courier();

        if (courier != NULL && strcmp(courier->target, "none") != 0) {
            while(order = pop_order_or_null(courier->target) != NULL) {
                courier->order = order;
                break;
            }
            deliver_order(courier);
        }
    } while(1);
}

void* listen_random_delivery_event_thread(void* p)
{
    courier_t* courier = NULL;

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    do {
        order_t* order = NULL;
        courier = dequeue_courier();

        if (courier != NULL && strcmp(courier->target, "none") == 0) {
            while(order = pop_random_order_or_null() != NULL) {
                courier->order = order;
                break;
            }
            deliver_order(courier);
        }
    } while(1);
}
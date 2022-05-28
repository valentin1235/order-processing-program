#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "courier_manager.h"
#include "server.h"




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
    request_t* pa_request = (char*)p;
    int sec_taken_to_arrive;

    if (strcmp(strtok(pa_request->message, "#"), "COURIER") != 0) {
        goto end;
    }

    courier = malloc(sizeof(courier_t));
    
    
    strcmp(courier->target, strtok(pa_request->message, "#"));
    sec_taken_to_arrive = atoi(strtok(pa_request->message, "#"));

    /* sleep thread till courier arrive */
    sleep(sec_taken_to_arrive);

    courier->arrived_at = time(NULL);
    courier->order = NULL;

    /* add order */
    init_queue();
    enqueue_courier(courier);

    printf("%order processed : %d\n", (int)courier->took_order_at);

end:
    pthread_exit((void*)0);
}

static void deliver_order(courier_t* courier)
{
    free(courier->order);
    free(courier);
}

void* listen_delivery_event_thread(void* p)
{
    courier_t* courier = NULL;
    while(courier = dequeue_courier() != NULL) {
        order_t* order = NULL;

        switch (strcmp(courier->target, "none")) {
        case 0:
            while(order = pop_random_order_or_null() != NULL) {
                courier->order = order;
            }
            break;
        
        default:
            while(order = pop_order_or_null(courier->target) != NULL) {
                courier->order = order;
            }
            break;
        }

        deliver_order(courier);
    }
}
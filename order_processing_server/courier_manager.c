#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "courier_manager.h"
#include "server.h"
#include "time_manager.h"
#include "utils/message.h"
#include "utils/bool.h"

pthread_mutex_t g_random_courier_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct random_courier_queue {
    int front;
    int back;
    int value_count;
    courier_t* values[COURIER_QUEUE_SIZE];
} random_courier_queue_t;

static random_courier_queue_t* s_random_courier_queue;

courier_t s_target_couriers[COURIER_QUEUE_SIZE];
size_t s_target_courier_count = 0;



void init_courier_queue(void)
{
    s_random_courier_queue = malloc(sizeof(random_courier_queue_t));
    
    pthread_mutex_lock(&g_random_courier_mutex);
    {
        s_random_courier_queue->value_count = 0;
        s_random_courier_queue->front = 0;
        s_random_courier_queue->back = 0;
    }
    pthread_mutex_unlock(&g_random_courier_mutex);
}

void enqueue_random_courier(courier_t* value)
{
    printf("0 !!!");
    pthread_mutex_lock(&g_random_courier_mutex);
    {
        if (s_random_courier_queue->value_count >= COURIER_QUEUE_SIZE) {
            printf("1 !!!");
            goto exit;
        }
        s_random_courier_queue->values[s_random_courier_queue->back] = value;
        s_random_courier_queue->back = (s_random_courier_queue->back + 1) % COURIER_QUEUE_SIZE;
        ++s_random_courier_queue->value_count;

        printf("[enqueue_random_courier] front:%d, back:%d, count:%lu\n", s_random_courier_queue->front, s_random_courier_queue->back, s_random_courier_queue->value_count);
    }
exit:
    pthread_mutex_unlock(&g_random_courier_mutex);
    printf("2 !!!");
}


courier_t* dequeue_random_courier(void)
{
    courier_t* ret = NULL;

    pthread_mutex_lock(&g_random_courier_mutex);
    {
        if (s_random_courier_queue->value_count <= 0) {
            goto exit;
        }

        ret = s_random_courier_queue->values[s_random_courier_queue->front];
        s_random_courier_queue->front = (s_random_courier_queue->front + 1) % COURIER_QUEUE_SIZE;
        --s_random_courier_queue->value_count;
        printf("[dequeue_random_courier] front:%d, back:%d, count:%lu, ret:%s\n", s_random_courier_queue->front, s_random_courier_queue->back, s_random_courier_queue->value_count, ret->target);
    }
exit:
    pthread_mutex_unlock(&g_random_courier_mutex);

    return ret;
}


void* process_courier_thread(void* p)
{
    courier_t* courier;
    char* pa_message = (char*)p;
    int sec_taken_to_arrive;

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    printf("%s\n", pa_message);

    if (strcmp(strtok(pa_message, "#"), "COURIER") != 0) {
        printf(MESSAGE_INVALID_SERVICE);
        goto end;
    }

    courier = malloc(sizeof(courier_t));
    
    
    strncpy(courier->target, strtok(NULL, "#"), ORDER_NAME_SIZE);
    sec_taken_to_arrive = atoi(strtok(NULL, "#"));

    /* sleep thread till courier arrive */
    printf("배달원이 (%d)초 후 도착합니다\n", sec_taken_to_arrive);
    sleep(sec_taken_to_arrive);

    courier->arrived_at = time(NULL);
    courier->order = NULL;

    /* add order */
    if (strcmp(courier->target, "none") == 0) {
        printf("1@@@\n");
        enqueue_random_courier(courier);
    }
    

    printf("[courier] 배달원이 도착했습니다(주문이름:%s). 총 (%d)명의 배달원이 대기중입니다\n", courier->target, s_random_courier_queue->value_count);

end:
    pthread_exit((void*)0);
}

static void deliver_order(courier_t* courier)
{
    free(courier->order);
    free(courier);
    printf("[courier] 주문이 배송되었습니다 : %d\n", (int)courier->took_order_at);
    print_time_records();
    
}

#if 0
void* listen_targeted_delivery_event_thread(void* p)
{
    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    printf("[courier] 고정 주문 배달원을 기다리고 있습니다...\n");
    while (TRUE) {
        courier_t* courier = NULL;
        courier = dequeue_target_courier();

        if (courier != NULL) {
            order_t* order = NULL;
            printf("[courier] 고정 주문 배달원 (%s)가 음식을 기다리고 있습니다...\n", courier->target);
            while ((order = pop_order_or_null(courier->target)) != NULL) {
                order->taken_at = time(NULL);
                courier->order = order;
                deliver_order(courier);

                break;
            }
        }
    };
}
#endif

void* listen_random_delivery_event_thread(void* p)
{
    order_t* order = NULL;

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    printf("[random_courier] 주문이 포장되길 기다리고 있습니다...\n");
    while ((order = pop_random_order_or_null()) != NULL) {
        courier_t* courier;

        printf("[random_courier] 배달 기사님이 오기를 기다리고 있습니다...\n");
        while ((courier = dequeue_random_courier()) != NULL) {
            time_t now = time(NULL);

            courier->order = order;
            courier->took_order_at = now;
            order->taken_at = now;

            deliver_order(courier);

            break;
        }
    }
    
}

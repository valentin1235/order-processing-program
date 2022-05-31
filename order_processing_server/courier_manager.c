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

pthread_mutex_t g_random_courier_mutex;


typedef struct random_courier_queue {
    int front;
    int back;
    int value_count;
    courier_t* values[COURIER_QUEUE_SIZE];
} random_courier_queue_t;

static random_courier_queue_t* s_random_courier_queue;

courier_t s_target_couriers[COURIER_QUEUE_SIZE];
size_t s_target_courier_count = 0;


static int validate_request(char* msg) {
    return msg[0] == 'C' 
            && msg[1] == 'O' 
            && msg[2] == 'U' 
            && msg[3] == 'R' 
            && msg[4] == 'I'
            && msg[5] == 'E'
            && msg[6] == 'R'
            && msg[7] == '#';
}

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
    pthread_mutex_lock(&g_random_courier_mutex);
    {
        if (s_random_courier_queue->value_count >= COURIER_QUEUE_SIZE) {
            goto exit;
        }
        s_random_courier_queue->values[s_random_courier_queue->back] = value;
        s_random_courier_queue->back = (s_random_courier_queue->back + 1) % COURIER_QUEUE_SIZE;
        ++s_random_courier_queue->value_count;

        printf("[enqueue_random_courier] front:%d, back:%d, count:%d\n", s_random_courier_queue->front, s_random_courier_queue->back, s_random_courier_queue->value_count);
    }
exit:
    pthread_mutex_unlock(&g_random_courier_mutex);
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
        printf("[dequeue_random_courier] front:%d, back:%d, count:%d, ret:%s\n", s_random_courier_queue->front, s_random_courier_queue->back, s_random_courier_queue->value_count, ret->target);
    }
exit:
    pthread_mutex_unlock(&g_random_courier_mutex);

    return ret;
}


void* process_courier_thread(void* p)
{
    courier_t* courier = NULL;
    order_t* order = NULL;
    request_t* pa_request = *(request_t**)p;
    int sec_taken_to_arrive;

    pthread_mutex_init(&g_random_courier_mutex, NULL);

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    while (TRUE) {
        while (validate_request(pa_request->message) == FALSE) {
            int read_len;
            memset(pa_request->message, 0, MESSAGE_SIZE);
            read_len = read(pa_request->client_socket, pa_request->message, MESSAGE_SIZE);
            
            if (read_len == -1 || read_len == 0) {
                printf(MESSAGE_CONNECTION_CLOSED);
                goto end;
            }
        }

        courier = malloc(sizeof(courier_t));
        strtok(pa_request->message, "#"); /* remove service indicator from the source */
        strncpy(courier->target, strtok(NULL, "#"), ORDER_NAME_SIZE);
        sec_taken_to_arrive = atoi(strtok(NULL, "#"));


        /* sleep thread till courier arrive */
        printf("[process_courier_thread] 배달원이 (%d)초 후 도착합니다\n", sec_taken_to_arrive);
        sleep(sec_taken_to_arrive);


        courier->arrived_at = time(NULL);
        courier->order = NULL;


        /* add courier */
        if (strcmp(courier->target, "none") == 0) {
            enqueue_random_courier(courier);
        }

        printf("[courier] 배달원이 도착했습니다(주문이름:%s). 총 (%d)명의 배달원이 대기중입니다\n", courier->target, s_random_courier_queue->value_count);

        /* deliver order */
        if ((order = pop_random_order_or_null()) != NULL) {
            courier->order = order;
            deliver_order(dequeue_random_courier());
        }
    }

end:
    free(pa_request);
    pthread_mutex_destroy(&g_random_courier_mutex);
    pthread_exit((void*)0);
}

void deliver_order(courier_t* courier)
{
    int now = time(NULL);
    double courier_time_wait = difftime(now, courier->arrived_at);
    double order_time_wait = difftime(now, courier->order->ready_at);

    printf("[courier] 주문이 배송되었습니다\n");
    printf("[courier] (%s)음식이 배달되기까지 (%.2f)초 걸렸습니다\n", courier->order->name, order_time_wait);
    printf("[courier] 배달원은 (%.2f)초 기다렸습니다\n", courier_time_wait);

    add_time_records(order_time_wait, courier_time_wait);

    free(courier->order);
    free(courier);



    print_time_records();
    
}

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

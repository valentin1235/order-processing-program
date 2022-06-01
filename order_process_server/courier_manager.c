#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "courier_manager.h"
#include "server.h"
#include "time_manager.h"
#include "utils/bool.h"
#include "utils/message.h"

pthread_mutex_t g_random_courier_mutex;
pthread_mutex_t g_target_courier_mutex;

typedef struct random_courier_queue {
    int front;
    int back;
    int value_count;
    courier_t* values[COURIER_QUEUE_SIZE];
} random_courier_queue_t;

static random_courier_queue_t* s_random_courier_queue;
static courier_t* s_target_couriers[COURIER_QUEUE_SIZE] = { 0 };
static int s_target_courier_count = 0;

/* static functions */
static int validate_request(char* msg)
{
    return msg[0] == 'C'
        && msg[1] == 'O'
        && msg[2] == 'U'
        && msg[3] == 'R'
        && msg[4] == 'I'
        && msg[5] == 'E'
        && msg[6] == 'R'
        && msg[7] == '#';
}

static void enqueue_random_courier(courier_t* value)
{
    pthread_mutex_lock(&g_random_courier_mutex);
    {
        if (s_random_courier_queue->value_count >= COURIER_QUEUE_SIZE) {
            goto exit;
        }
        s_random_courier_queue->values[s_random_courier_queue->back] = value;
        s_random_courier_queue->back = (s_random_courier_queue->back + 1) % COURIER_QUEUE_SIZE;
        ++s_random_courier_queue->value_count;
    }
exit:
    pthread_mutex_unlock(&g_random_courier_mutex);
}

static void remove_target_courier(size_t i)
{
    pthread_mutex_lock(&g_target_courier_mutex);
    {
        if (s_target_courier_count == 0) {
            goto end;
        }
        s_target_couriers[i] = s_target_couriers[--s_target_courier_count];
    }
end:
    pthread_mutex_unlock(&g_target_courier_mutex);
}

static void add_target_courier(courier_t* courier)
{
    pthread_mutex_lock(&g_target_courier_mutex);
    {
        if (s_target_courier_count == COURIER_QUEUE_SIZE) {
            goto end;
        }

        s_target_couriers[s_target_courier_count++] = courier;
        printf("* 배달원 추가 (총원 : %d) -> %s\n", s_target_courier_count, __func__);
    }

end:
    pthread_mutex_unlock(&g_target_courier_mutex);
}

/* global functions */
void init_random_courier_queue(void)
{
    s_random_courier_queue = malloc(sizeof(random_courier_queue_t));

    s_random_courier_queue->value_count = 0;
    s_random_courier_queue->front = 0;
    s_random_courier_queue->back = 0;
}

courier_t* dequeue_random_courier(void)
{
    courier_t* ret = NULL;

    pthread_mutex_lock(&g_random_courier_mutex);
    {
        if (s_random_courier_queue->value_count == 0) {
            goto exit;
        }

        ret = s_random_courier_queue->values[s_random_courier_queue->front];
        s_random_courier_queue->front = (s_random_courier_queue->front + 1) % COURIER_QUEUE_SIZE;
        --s_random_courier_queue->value_count;
    }
exit:
    pthread_mutex_unlock(&g_random_courier_mutex);

    return ret;
}

void* process_courier_thread(void* p)
{
    courier_t* courier = NULL;
    request_t* pa_request = *(request_t**)p;
    int sec_taken_to_arrive;

    while (TRUE) {
        while (validate_request(pa_request->message) == FALSE) {
            int read_len;
            memset(pa_request->message, 0, MESSAGE_SIZE);
            read_len = read(pa_request->client_socket, pa_request->message, MESSAGE_SIZE);

            if (read_len == -1 || read_len == 0) {
                printf(MESSAGE_CONNECTION_CLOSED);
                if (g_sock_client_count > 0) {
                    --g_sock_client_count;
                }

                goto end;
            }
        }

        courier = malloc(sizeof(courier_t));
        strtok(pa_request->message, "#"); /* remove service indicator from the source */
        strncpy(courier->target, strtok(NULL, "#"), ORDER_NAME_SIZE);
        sec_taken_to_arrive = atoi(strtok(NULL, "#"));

        /* sleep thread till courier arrive */
        printf("[%s] 배달원 (%d)초 후 도착예정\n", COURIER_DISPATCHED, sec_taken_to_arrive);
        sleep(sec_taken_to_arrive);

        courier->arrived_at = time(NULL);
        courier->order = NULL;

        /* add courier */
        if (strcmp(courier->target, "none") == 0) {
            enqueue_random_courier(courier);
            printf("[%s] 배달원 도착(주문이름:%s)\n", COURIER_ARRIVED, courier->target);
        } else {
            add_target_courier(courier);
            printf("[%s] 배달원 도착(주문이름:%s)\n", COURIER_ARRIVED, courier->target);
        }

        /* deliver order */
        {
            order_t* order = pop_random_order_or_null();
            courier_t* courier = dequeue_random_courier();
            if (order != NULL && courier != NULL) {
                courier->order = order;
                deliver_order(courier);
                printf("[%s] (%s)를 배달원이 가져갑니다\n", ORDER_PICKED_UP, order->name);
            }
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

    printf("* (%s)음식이 배달되기까지 (%.2f)초 걸렸습니다 -> %s\n", courier->order->name, order_time_wait, __func__);
    printf("* 배달원은 (%.2f)초 기다렸습니다 -> %s\n", courier_time_wait, __func__);

    add_time_records(order_time_wait, courier_time_wait);

    free(courier->order);
    free(courier);

    print_time_records();
}

void* listen_target_delivery_event_thread(void* p)
{
    int i;
    int j;

    do {
        for (i = 0; i < s_target_courier_count; ++i) {
            for (j = 0; j < g_ready_order_count; ++j) {
                if (strcmp(s_target_couriers[i]->target, g_ready_orders[j]->name) == 0) {
                    courier_t* courier = s_target_couriers[i];
                    order_t* order = g_ready_orders[j];

                    courier->order = order;
                    deliver_order(courier);

                    remove_target_courier(i);
                    remove_order(j);

                    break;
                }
            }
        }

        sleep(1);
    } while (TRUE);

    pthread_exit((void*)0);
}

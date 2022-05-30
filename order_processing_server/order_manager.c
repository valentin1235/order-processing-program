#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "order_manager.h"
#include "server.h"
#include "time_manager.h"
#include "utils/message.h"

pthread_mutex_t g_order_mutex = PTHREAD_MUTEX_INITIALIZER;

static order_t* s_ready_orders[ORDER_LIST_SIZE];
static size_t s_ready_order_count;

/* static methods */
static void add_order(order_t* order)
{
    // pthread_mutex_lock(&g_order_mutex);
    {
        s_ready_orders[s_ready_order_count++] = order;
    }
    // pthread_mutex_unlock(&g_order_mutex);
}

/* methods */
void* process_order_thread(void* p)
{
    order_t* order;
    char* message = (char*)p;

    /* register sig int handler */
    signal(SIGINT, SIG_INT_handler);

    printf("[order] 메세지 전체 : %s\n", message);

    if (strcmp(strtok(message, "#"), "ORDER") != 0) {
        printf(MESSAGE_INVALID_SERVICE);
        goto end;
    }

    order = malloc(sizeof(order_t));
    strncpy(order->order_id, strtok(NULL, "#"), ORDER_ID_SIZE);
    strncpy(order->name, strtok(NULL, "#"), ORDER_NAME_SIZE);
    order->prep_time = atoi(strtok(NULL, "#"));

    /* sleep thread till preperation time end */
    printf("[order] 음식이 만들어지는 시간 (%d)초 간 대기, 주문이름 : %s, 주문아이디 : %s\n", order->prep_time, order->name, order->order_id);
    sleep(order->prep_time); 
    order->ready_at = time(NULL);
    order->taken_at = time(0);

    /* add order */
    add_order(order);

    printf("[order] 음식준비완료. 총 (%lu)개의 음식이 배달원을 기다리고 있습니다 : [ %d, %s ]\n", s_ready_order_count, (int)order->ready_at, order->name);

end:
    pthread_exit((void*)0);
}


order_t* pop_order_or_null(const char* name)
{
    size_t i;

    pthread_mutex_lock(&g_order_mutex);
    for (i = 0; i < s_ready_order_count; ++i) {
        if (strncmp(s_ready_orders[i]->name, name, ORDER_NAME_SIZE) == 0) {
            order_t* order;

            order = s_ready_orders[i];
            s_ready_orders[i] = s_ready_orders[s_ready_order_count - 1];
            --s_ready_order_count;
            
            return order;
        }
    }
    pthread_mutex_unlock(&g_order_mutex);

    return NULL;
}

order_t* pop_random_order_or_null()
{
    size_t i;
    order_t* order;

    pthread_mutex_lock(&g_order_mutex);
    {
        if (s_ready_order_count < 1) {
            return NULL;
        }
        i = rand() % s_ready_order_count;

        order = s_ready_orders[i];
        s_ready_orders[i] = s_ready_orders[s_ready_order_count - 1];
        --s_ready_order_count;
    }
    pthread_mutex_unlock(&g_order_mutex);

    return order;
}

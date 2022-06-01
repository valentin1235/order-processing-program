#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "order_manager.h"
#include "parson.h"
#include "server.h"
#include "time_manager.h"
#include "utils/bool.h"
#include "utils/message.h"

pthread_t g_thread_cook;
pthread_mutex_t g_order_mutex;
order_t* g_ready_orders[ORDER_LIST_SIZE] = { 0 };
int g_ready_order_count = 0;

/* static methods */
static void add_order(order_t* order)
{
    pthread_mutex_lock(&g_order_mutex);
    {
        g_ready_orders[g_ready_order_count++] = order;
    }
    pthread_mutex_unlock(&g_order_mutex);
}

static void* cook(void* p)
{
    courier_t* courier = NULL;
    order_t* order = *(order_t**)p;

    /* sleep thread till preperation time end */
    printf("[%s] (%s)를 요리합니다.(예상시간 %d)\n", __func__, order->name, order->prep_time);
    sleep(order->prep_time);
    order->ready_at = time(NULL);
    order->taken_at = time(0);

    if ((courier = dequeue_random_courier()) != NULL) {
        /* deliver order */
        courier->order = order;
        deliver_order(courier);
        printf("[%s] (%s)를 배달원이 가져갑니다\n", __func__, order->name);
    } else {
        /* add order if courier is not there */
        add_order(order);
        printf("[%s] (%s)준비완료목록에 추가(총 %d개)\n", __func__, order->name, g_ready_order_count);
    }

    pthread_exit((void*)0);
}

/* global methods */
void process_orders(const char* file_name)
{
    JSON_Value* json_file;
    JSON_Array* json_arr;
    size_t json_arr_count;
    size_t i;
    

    pthread_mutex_init(&g_random_courier_mutex, NULL);
    pthread_mutex_init(&g_order_mutex, NULL);

    json_file = json_parse_file(file_name);
    json_arr = json_value_get_array(json_file);
    json_arr_count = json_array_get_count(json_arr);

    for (i = 0; i < json_arr_count; ++i) {
        order_t* order = malloc(sizeof(order_t));
        JSON_Object* obj = json_array_get_object(json_arr, i);

        strncpy(order->order_id, json_object_get_string(obj, "id"), ORDER_ID_SIZE);
        strncpy(order->name, json_object_get_string(obj, "name"), ORDER_NAME_SIZE);
        order->prep_time = (int)json_object_get_number(obj, "prepTime");

        pthread_create(&g_thread_cook, NULL, cook, &order);
    }
}

void remove_order(size_t i)
{

    pthread_mutex_lock(&g_order_mutex);
    {
        if (g_ready_order_count == 0) {
            goto end;
        }
        g_ready_orders[i] = g_ready_orders[--g_ready_order_count];
    }
end:
    pthread_mutex_unlock(&g_order_mutex);
}

order_t* pop_random_order_or_null(void)
{
    size_t i;
    order_t* order = NULL;

    pthread_mutex_lock(&g_order_mutex);
    {
        if (g_ready_order_count < 1) {
            goto end;
        }
        i = rand() % g_ready_order_count;

        order = g_ready_orders[i];
        g_ready_orders[i] = g_ready_orders[g_ready_order_count - 1];
        --g_ready_order_count;
    }

end:
    pthread_mutex_unlock(&g_order_mutex);

    return order;
}

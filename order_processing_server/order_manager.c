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
#include "utils/bool.h"
#include "parson.h"

#define FILE_NAME_SIZE (100)

pthread_mutex_t g_order_mutex;

order_t* s_ready_orders[ORDER_LIST_SIZE];
size_t s_ready_order_count;

/* static methods */
void add_order(order_t* order)
{
    pthread_mutex_lock(&g_order_mutex);
    {
        s_ready_orders[s_ready_order_count++] = order;
    }
    pthread_mutex_unlock(&g_order_mutex);
}


void* cook(void* p)
{
    courier_t* courier = NULL;
    order_t* order = *(order_t**)p;

    /* sleep thread till preperation time end */
    printf("[order] (%s)이 만들어지는 시간 (%d)초 간 대기\n", order->name, order->prep_time);
    sleep(order->prep_time); 
    order->ready_at = time(NULL);
    order->taken_at = time(0);

    // printf("@@@ %d\n", dequeue_random_courier());

    if ((courier = dequeue_random_courier()) != NULL) {
        /* deliver order */
        courier->order = order;
        deliver_order(courier);
        printf("[order] (%s)준비완료. 음식을 배달원이 가져갑니다\n", order->name, s_ready_order_count);
    } else {
        /* add order if courier is not there */
        add_order(order);
        printf("[order] (%s)주문완료목록에 추가. 총 (%lu)개의 음식이 배달원을 기다리고 있습니다\n", order->name, s_ready_order_count);
    }

    pthread_exit((void*)0);
}

void read_filename(char* name, size_t size)
{
    char* p_name = name;

    while (TRUE) {
        printf("\"파일이름.json\" 을 입력하세요\n");
        if (fgets(name, size, stdin) != NULL) {
            printf("%s\n", p_name);
            while (*p_name != '.' && *p_name != '\0' && *p_name != '\n') {
                ++p_name;
            }

            if (*p_name != '.' 
                    || *(p_name + 1) != 'j' 
                    || *(p_name + 2) != 's' 
                    || *(p_name + 3) != 'o' 
                    || *(p_name + 4) != 'n') {
                printf("[오류] 파일형식이 잘못되었습니다\n", *p_name);
                p_name = name;
                continue;
            }

            while (*p_name != '\0' && *p_name != '\n') {
                ++p_name;
            }
            *p_name = '\0';
            p_name = name;

            if (access(name, F_OK) == -1) {
                printf("[오류] 파일이 존재하지 않습니다\n");
                p_name = name;
                continue;
            }

            break;
        }
    }
}

void process_orders()
{
    JSON_Value* json_file;
    JSON_Array* json_arr;
    size_t json_arr_count;
    size_t i;
    pthread_t thread_cook;
    char filename[FILE_NAME_SIZE];

    read_filename(filename, FILE_NAME_SIZE);

    pthread_mutex_init(&g_random_courier_mutex, NULL);
    pthread_mutex_init(&g_order_mutex, NULL);
    
    json_file = json_parse_file(filename);
    json_arr = json_value_get_array(json_file);
    json_arr_count = json_array_get_count(json_arr);

    for (i = 0; i < json_arr_count; ++i) {
        order_t* order = malloc(sizeof(order_t));
        JSON_Object* obj = json_array_get_object(json_arr, i);
        
        strncpy(order->order_id, json_object_get_string(obj, "id"), ORDER_ID_SIZE);
        strncpy(order->name, json_object_get_string(obj, "name"), ORDER_NAME_SIZE);
        order->prep_time = (int)json_object_get_number(obj, "prepTime");

        pthread_create(&thread_cook, NULL, cook, &order);
    }
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

order_t* pop_random_order_or_null(void)
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

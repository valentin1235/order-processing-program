#include <stdio.h>
#include <time.h>
#include <string.h>

#include "time_manager.h"

static double s_courier_waiting_times[COURIER_QUEUE_SIZE] = { 0, };
static size_t s_courier_waiting_times_count = 0;

static double s_order_waiting_times[ORDER_LIST_SIZE] = { 0, };
static size_t s_order_waiting_times_count = 0;


void add_time_records(courier_t* courier)
{
    s_courier_waiting_times[s_courier_waiting_times_count++] = difftime(courier->order->taken_at, courier->arrived_at);
    s_order_waiting_times[s_order_waiting_times_count++] = difftime(courier->order->taken_at, courier->order->ready_at);
}

void print_time_records(void)
{
    double sum_courier_waiting_time = 0.0f;
    double sum_order_waiting_time = 0.0f;
    size_t i;

    for (i = 0; i < s_courier_waiting_times_count; ++i) {
        sum_courier_waiting_time += s_courier_waiting_times[i];
    }

    for (i = 0; i < s_order_waiting_times_count; ++i) {
        sum_order_waiting_time += s_order_waiting_times[i];
    }

    printf("* 배달원 평균 대기 시간 : %f s\n", sum_courier_waiting_time / s_courier_waiting_times_count);
    printf("* 음식 픽업 평균 대기 시간 : %f s\n", sum_order_waiting_time / s_order_waiting_times_count);
}

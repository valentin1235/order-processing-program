#include <stdio.h>
#include <time.h>
#include <string.h>

#include "time_manager.h"

static double s_courier_waiting_times[COURIER_QUEUE_SIZE] = { 0, };
static size_t s_courier_waiting_times_count = 0;

static double s_order_waiting_times[ORDER_LIST_SIZE] = { 0, };
static size_t s_order_waiting_times_count = 0;


void add_time_records(double order_wait_time, double courier_wait_time)
{
    s_courier_waiting_times[s_courier_waiting_times_count++] = courier_wait_time;
    s_order_waiting_times[s_order_waiting_times_count++] = order_wait_time;
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

    printf("* 배달원 평균 대기 시간 : (%.2f)s\n", sum_courier_waiting_time / s_courier_waiting_times_count);
    printf("* 음식 픽업 평균 대기 시간 : (%.2f)s\n", sum_order_waiting_time / s_order_waiting_times_count);
}

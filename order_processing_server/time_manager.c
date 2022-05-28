#include <stdio.h>
#include <time.h>
#include <string.h>

#include "time_manager.h"
#include "order_manager.h"
#include "courier_manager.h"

static double s_courier_waiting_times[COURIER_QUEUE_SIZE] = { 0, };
static size_t s_courier_waiting_times_count = 0;

static double s_order_waiting_times[ORDER_LIST_SIZE] = { 0, };
static size_t s_order_waiting_times_count = 0;


void add_time_records(courier_t* courier)
{
    s_courier_waiting_times[s_courier_waiting_times_count++] = difftime(courier->order->taken_at, courier->arrived_at);
    s_order_waiting_times[s_order_waiting_times_count++] = difftime(courier->order->taken_at, courier->order->ready_at);
}

void print_time_records()
{
    double sum_courier_waiting_time;
    double sum_order_waiting_time;
    size_t i;

    for (i = 0; i < s_courier_waiting_times_count; ++i) {
        sum_courier_waiting_time += s_courier_waiting_times[i];
    }

    for (i = 0; i < s_order_waiting_times_count; ++i) {
        sum_order_waiting_time += s_order_waiting_times[i];
    }

    printf("average of courier wating time : %f sec\n", sum_courier_waiting_time / s_courier_waiting_times_count);
    printf("average of order wating time : %f sec\n", sum_order_waiting_time / s_order_waiting_times_count);
}

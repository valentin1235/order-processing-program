#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "server.h"
#include "order_manager.h"
#include "courier_manager.h"
#include "time_manager.h"
#include "./utils/message.h"


typedef enum service {
    SERVICE_ORDER,
    SERVICE_COURIER,
    SERVICE_INVALID
} service_t;



static service_t get_service_type(char* message)
{
    char* p_message = message;
    char path_str[8];
    size_t i;

    while (*p_message != '#' && *p_message != '\0') {
        path_str[i++] = *p_message;
    }
    path_str[i] = '\0';

    if (strcmp(path_str, "ORDER") == 0) {
        return SERVICE_ORDER;
    } else if (strcmp(path_str, "ORDER") == 0) {
        return SERVICE_COURIER;
    }

    return SERVICE_INVALID;
}

void SIG_INT_handler(int sig)
{
    print_time_records();

    printf("스레드를 종료합니다 : %d\n", sig);
    pthread_exit(NULL);
}


static void server_on(void)
{
    int server_socket;
    struct sockaddr_in server_addr;

    pthread_t random_delivery_event_listening_thread;
    pthread_t targeted_delivery_event_listening_thread;
    pthread_t order_processing_thread;
    pthread_t courier_processing_thread;

    signal(SIGPIPE, SIG_IGN); /* ignore EPIPE(broken pipe) signal */

    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("* 서버 소켓을 생성할수 없습니다\n");
        return ERROR_SOCKET;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(DEFAULT_PORT));

    /* bind socket with socket descriptor */
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("* 서버 소켓 정보 바인딩에 실패했습니다\n");
        return ERROR_BIND;
    }
    
    /* listen via binded socket */
    if (listen(server_socket, 5) == -1) {
        printf("* 서버 listen에 실패했습니다\n");
        return ERROR_LISTEN;
    }

    printf("* 서버가 시작되었습니다. port:3000\n");

    /* start delivery event listener */
    pthread_create(&random_delivery_event_listening_thread, NULL, listen_random_delivery_event_thread, NULL);
    pthread_create(&targeted_delivery_event_listening_thread, NULL, listen_targeted_delivery_event_thread, NULL);

    do {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size;
        int client_socket;

        client_addr_size = sizeof(client_addr);
        printf("* 클라이언트를 받고있습니다...\n");
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("* 클라이언트를 받았습니다 : %d\n", client_socket);

        do {
            int read_len;
            char* pa_message = malloc(sizeof(char) * MESSAGE_SIZE);
            
            printf("클라이언트로부터 메세지를 읽고있습니다...\n");
            read_len = read(client_socket, pa_message, MESSAGE_SIZE); 
            if (read_len == -1 || read_len == 0) {
                printf(MESSAGE_CONNECTION_CLOSED);
                free(pa_message);

                break;
            }

            switch (get_service_type(pa_message)) {
            case SERVICE_ORDER:
                pthread_create(&order_processing_thread, NULL, process_order_thread, pa_message);
                printf("* 주문 처리 스레드가 생성되었습니다.\n");
                break;
            case SERVICE_COURIER:
                pthread_create(&courier_processing_thread, NULL, process_courier_thread, pa_message);
                printf("* 배달원 처리 스레드가 생성되었습니다.\n");
                break;
            case SERVICE_INVALID:
                /* intentional fallthrough */
            default:
                write(client_socket, MESSAGE_INVALID_SERVICE, strlen(MESSAGE_INVALID_SERVICE) + 1);
                printf("* 잘못된 서비스 요청메세지를 보냈습니다.\n");
                continue;
            }
        } while(1);
    } while (1);
}

int main(void)
{
    server_on();
}
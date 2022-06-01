#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "courier_manager.h"
#include "order_manager.h"
#include "server.h"
#include "time_manager.h"
#include "utils/message.h"

#define SERVICE_NAME_LEN (8)

typedef enum service {
    SERVICE_COURIER,
    SERVICE_INVALID,
    SERVICE_IGNORED
} service_t;

static service_t get_service_type(char* message, size_t message_len)
{
    char* p_message = message;
    char path_str[SERVICE_NAME_LEN];
    size_t i = 0;

    printf("서비스타입 헨들러. 도착메세지 : %s\n", message);
    while (*p_message != '\0') {
        path_str[i++] = *p_message;
        ++p_message;

        if (i >= message_len || *p_message == '#' || i == SERVICE_NAME_LEN - 1) {
            break;
        }
    }

    if (*p_message != '#') {
        return SERVICE_IGNORED;
    }

    path_str[i] = '\0';

    if (strcmp(path_str, "COURIER") == 0) {
        return SERVICE_COURIER;
    }

    return SERVICE_INVALID;
}

void SIG_INT_handler(int sig)
{
    print_time_records();

    printf("[sigint handler] 스레드를 종료합니다. (시그널 번호 : %d)\n", sig);
    pthread_exit(NULL);

    exit(1);
}

static error_t server_on(void)
{
    int server_socket;
    struct sockaddr_in server_addr;

    pthread_t thread_target_delivery;
    pthread_t thread_order;
    pthread_t thread_courier;

    signal(SIGPIPE, SIG_IGN); /* ignore EPIPE(broken pipe) signal */

    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("[main] 서버 소켓을 생성할수 없습니다\n");
        return ERROR_SOCKET;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(DEFAULT_PORT));

    /* bind socket with socket descriptor */
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("[main] 서버 소켓 정보 바인딩에 실패했습니다\n");
        return ERROR_BIND;
    }

    /* listen via binded socket */
    if (listen(server_socket, 5) == -1) {
        printf("[main] 서버 listen에 실패했습니다\n");
        return ERROR_LISTEN;
    }

    printf("[main] 서버가 시작되었습니다. port:3000\n");

    /* initialize courier queue */
    init_random_courier_queue();

    /* start delivery event listener */
    pthread_create(&thread_target_delivery, NULL, listen_target_delivery_event_thread, NULL);

    do {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size;
        int client_socket;
        request_t* request;
        int read_len;

        client_addr_size = sizeof(client_addr);
        printf("[main] 클라이언트를 받고있습니다...\n");
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
        printf("[main] 클라이언트를 받았습니다 : %d\n", client_socket);

        request = malloc(sizeof(request_t));
        request->client_socket = client_socket;
        read_len = read(client_socket, request, MESSAGE_SIZE);
        if (read_len == -1 || read_len == 0) {
            printf(MESSAGE_CONNECTION_CLOSED);
            free(request);

            break;
        }

        switch (get_service_type(request->message, read_len)) {
        case SERVICE_COURIER:
            pthread_create(&thread_courier, NULL, process_courier_thread, &request);
            printf("[main] 배달원 처리 스레드가 생성되었습니다.\n");
            break;
        case SERVICE_INVALID:
            write(client_socket, MESSAGE_INVALID_SERVICE, strlen(MESSAGE_INVALID_SERVICE) + 1);
            printf("[main] 잘못된 서비스요청 메세지를 보냈습니다.\n");
            break;
        case SERVICE_IGNORED:
            /* intentional fall through */
        default:
            break;
        }
    } while (1);

    return SUCCESS;
}

int main(int argc, char** argv)
{
    const char* filename = argv[1];

    process_orders(filename);
    server_on();
}

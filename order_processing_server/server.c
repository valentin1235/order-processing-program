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


static void server_on(void)
{
    int server_socket;

    struct sockaddr_in server_addr;
    pthread_t thread;

    signal(SIGPIPE, SIG_IGN); /* ignore EPIPE(broken pipe) signal */

    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("* unable to create socket\n");
        return ERROR_SOCKET;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(DEFAULT_PORT));

    /* bind socket with socket descriptor */
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("* bind error\n");
        return ERROR_BIND;
    }
    printf("* server run on 3000\n");

    /* listen via binded socket */
    if (listen(server_socket, 5) == -1) {
        printf("* listen error\n");
        return ERROR_LISTEN;
    }

    do {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size;
        int client_socket;
        request_t* pa_request;

        int read_len;
        

        client_addr_size = sizeof(client_addr);
        printf("* start accepting client...\n");
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        printf("* client accepted : %d\n", client_socket);

        
        printf("read message..\n");
        pa_request = malloc(sizeof(request_t));
        pa_request->client_socket = client_socket;
        read_len = read(client_socket, pa_request->message, MESSAGE_SIZE); 
        if (read_len == -1 || read_len == 0) {
            printf(MESSAGE_CONNECTION_CLOSED);
            free(pa_request);

            continue;
        }

        switch (get_service_type(pa_request->message)) {
        case SERVICE_ORDER:
            pthread_create(&thread, NULL, process_order_thread, pa_request);
            break;
        case SERVICE_COURIER:
            pthread_create(&thread, NULL, process_courier_thread, pa_request);
            break;
        case SERVICE_INVALID:
            /* intentional fallthrough */
        default:
            write(client_socket, MESSAGE_INVALID_SERVICE, strlen(MESSAGE_INVALID_SERVICE) + 1);
            continue;
        }

        printf("* new thread for (%d) created\n", client_socket);
    } while (1);
}

int main(void)
{
    server_on();
}
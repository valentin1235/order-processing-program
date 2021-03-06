#ifndef SERVER_H
#define SERVER_H

#define DEFAULT_PORT ("3000")
#define MESSAGE_SIZE (1000)

#define MAX_CLIENT (8)

extern int g_sock_clients[MAX_CLIENT];
extern size_t g_sock_client_count;

typedef struct request {
    char message[MESSAGE_SIZE];
    int client_socket;
} request_t;

typedef enum ERROR {
    SUCCESS = 1,
    ERROR_SOCKET = -1,
    ERROR_BIND = -2,
    ERROR_LISTEN = -3,
    ERROR_MESSAGE_FULL = -4
} error_t;

#endif /* SERVER_H */

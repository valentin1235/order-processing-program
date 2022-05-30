
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "parson.h"

#define DEFAULT_PORT ("3000")
#define READ_SIZE (100)
#define WRITE_SIZE (4000)
#define SERVICE_NAME ("ORDER")

static int s_exit = 0;


typedef enum ERROR {
    SUCCESS = 1,
    ERROR_SOCKET = -1,
    ERROR_CONNECT = -2,
    ERROR_READ = -3
} error_t;

char* ID = "id";
char* NAME = "name";
char* PREP_TIME = "prepTime";

void* send_message(void* p)
{
    int client_socket = *(int*)p;
    char buffer[READ_SIZE];

    do {        
        if (fgets(buffer, READ_SIZE, stdin) == NULL) {
            printf("* failed to read from stdin\n");
            break;
        }

        if (strcmp(buffer, "exit\n") == 0) {
            s_exit = 1;
            shutdown(client_socket, SHUT_RDWR);
            printf("* socket shutdown...\n");
            break;
        } else if (s_exit == 1) {
            printf("* unexpected thread exit\n");
            break;
        }

        write(client_socket, buffer, READ_SIZE);
    } while (1);

    pthread_exit((void*)0);

    return NULL;
}

void* receive_message(void* p)
{
    int client_socket = *(int*)p;
    char buffer[READ_SIZE];

    printf("메세지를 받고있습니다..\n");
    do {
        int read_len;

        if (s_exit == 1) {
            printf("* unexpected thread exit\n");
            break;
        }

        read_len = read(client_socket, buffer, sizeof(buffer) - 1);
        if (read_len == 0 || read_len == -1) {
            s_exit = 1;
            shutdown(client_socket, SHUT_RDWR);
            printf("* ECONNRESET : connection closed\n");
            break;
        }

        buffer[READ_SIZE - 1] = '\0';
        printf("* received message : %s\n", buffer);
    } while (1);

    pthread_exit((void*)0);

    return NULL;
    
}

/* TODO : 메세지 build 잘 되는지 검증 필요 */
static void build_message(char* buffer, char* id, char* name, int prep_time)
{
    char* p_buffer = buffer;
    char prep_time_str[11];
    size_t size_service_name = strlen(SERVICE_NAME);
    size_t size_id = strlen(id);
    size_t size_name = strlen(name);
    size_t size_prep_time_str;
    

    snprintf(prep_time_str, 11, "%d", prep_time);
    size_prep_time_str = strlen(prep_time_str);

    p_buffer[0] = '\0';

    strncat(p_buffer, SERVICE_NAME, size_service_name);
    p_buffer += size_service_name;
    *p_buffer = '#';
    ++p_buffer;
    *p_buffer = '\0';

    strncat(p_buffer, id, size_id);
    p_buffer += size_id;
    *p_buffer = '#';
    ++p_buffer;
    *p_buffer = '\0';

    strncat(p_buffer, name, size_name);
    p_buffer += size_name;
    *p_buffer = '#';
    ++p_buffer;
    *p_buffer = '\0';

    strncat(p_buffer, prep_time_str, size_prep_time_str);
    p_buffer += size_prep_time_str;
    *p_buffer = '\0';
}

static void send_message_by_json(const int client_socket, const char* file_name)
{
    JSON_Value* json_file;
    JSON_Array* json_arr;
    size_t json_arr_count;
    size_t i;
    char buffer[WRITE_SIZE];
    
    buffer[0] = '\0';

    json_file = json_parse_file(file_name);
    json_arr = json_value_get_array(json_file);
    json_arr_count = json_array_get_count(json_arr);

    for (i = 0; i < json_arr_count; ++i) {
        JSON_Object* obj = json_array_get_object(json_arr, i);

        build_message( /* TODO : 메세지 build 잘 되는지 검증 필요 */
            buffer, 
            json_object_get_string(obj, ID), 
            json_object_get_string(obj, NAME), 
            (int) json_object_get_number(obj, PREP_TIME)
        );
        printf("[처리된 주문 메세지] %s\n", buffer);

        write(client_socket, buffer, WRITE_SIZE);
    }
}

error_t create_client(const char* server_host, const char* server_port, const char* file_name)
{
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t thread_send;
    pthread_t thread_receive;

    signal(SIGPIPE, SIG_IGN); /* ignore EPIPE(broken pipe) signal */
    
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("* socket error\n");
        return ERROR_SOCKET;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_host);
    server_addr.sin_port = htons(atoi(server_port));

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("* connect error\n");
        return ERROR_CONNECT;
    }

    // pthread_create(&thread_send, NULL, send_message, &client_socket);
    pthread_create(&thread_receive, NULL, receive_message, &client_socket);

    if (file_name != NULL) {
        send_message_by_json(client_socket, file_name);
    }
    
    // pthread_join(thread_send, NULL);
    pthread_join(thread_receive, NULL);

    if (s_exit != 1) {
        close(client_socket);
    }
    
    return SUCCESS;
}




int main(int argc, char** argv)
{
    create_client(argv[1], argv[2], argv[3]);
}

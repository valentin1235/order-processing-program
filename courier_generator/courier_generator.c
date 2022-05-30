#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "courier_generator.h"

#define DEFAULT_PORT ("3000")
#define INPUT_SIZE (1000)
#define READ_SIZE (1000)
#define WRITE_SIZE (4000)
#define SERVICE_NAME ("COURIER")
#define ORDER_NAME_SIZE (50)
#define TIME_STR_SIZE (11)

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


void* receive_message(void* p)
{
    int client_socket = *(int*)p;
    char buffer[READ_SIZE];

    printf("[receive thread] 주문처리 서버로부터 메세지를 받고있습니다..\n");
    do {
        int read_len;

        if (s_exit == 1) {
            printf("[receive thread] 의도하지 않은 종료\n");
            break;
        }

        read_len = read(client_socket, buffer, sizeof(buffer) - 1);
        if (read_len == 0 || read_len == -1) {
            s_exit = 1;
            shutdown(client_socket, SHUT_RDWR);
            printf("[receive thread] ECONNRESET : connection closed\n");
            break;
        }

        buffer[READ_SIZE - 1] = '\0';
        printf("[receive thread] 받은 메세지 : \"%s\"\n", buffer);
    } while (1);

    pthread_exit((void*)0);

    return NULL;
    
}

static void build_message(char* write_buff, char* input)
{
    char* p_write_buff = write_buff;
    size_t size_service_name = strlen(SERVICE_NAME);
    size_t size_input = strlen(input);

    p_write_buff[0] = '\0';

    strncat(p_write_buff, SERVICE_NAME, size_service_name);
    p_write_buff += size_service_name;
    *p_write_buff = '#';
    ++p_write_buff;
    *p_write_buff = '\0';

    strncat(p_write_buff, input, size_input);
    p_write_buff += size_input;
    *p_write_buff = '\0';
}

static int validate_input(char* message, size_t message_len)
{
    char* p_message = message;
    char message_cpy[INPUT_SIZE];
    char* time_take_to_arrive_str = NULL;

    strncpy(message_cpy, message, message_len);
    
    strtok(message_cpy, "#");
    time_take_to_arrive_str = strtok(NULL, "#");

    if (time_take_to_arrive_str == NULL) {
        return FALSE;
    } else if (strtok(NULL, "#") != NULL) {
        return FALSE;
    }
    if (atoi(time_take_to_arrive_str) == 0) {
        return FALSE;
    }

    message[message_len - 1] = '\0'; /* this is for safe string read */ 
    while (*p_message != '\n' && *p_message != '\0') { /* exclude "line break" from the string */
        ++p_message;
    }
    *p_message = '\0';

    return TRUE;
}

error_t create_client(const char* server_host, const char* server_port)
{
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t thread_receive;
    char input_buff[INPUT_SIZE] = { 0, };
    char write_buff[WRITE_SIZE] = { 0, };

    signal(SIGPIPE, SIG_IGN); /* ignore EPIPE(broken pipe) signal */
    
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("[main] 소켓을 가져올수 없습니다\n");
        return ERROR_SOCKET;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_host);
    server_addr.sin_port = htons(atoi(server_port));

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("*[main] 서버 연결에 실패했습니다\n");
        return ERROR_CONNECT;
    }

    pthread_create(&thread_receive, NULL, receive_message, &client_socket);

    printf("[main] 메세지를 입력해주세요. \"주문이름#도착하는데_걸리는_시간\"\n");
    do {
        fgets(input_buff, INPUT_SIZE, stdin);
        if (input_buff[0] == '\0' || validate_input(input_buff, INPUT_SIZE) == FALSE) {
            printf("[main] 메세지 형식이 잘못되었습니다. 메세지를 다시 입력해주세요. \"주문이름#도착하는데_걸리는_시간\"\n");
            continue;
        }

        build_message(write_buff, input_buff);
        write(client_socket, write_buff, WRITE_SIZE);
        printf("[main] 메세지를 보냈습니다 : \"%s\"\n", write_buff);
        
    } while (1);
    
    pthread_join(thread_receive, NULL);

    if (s_exit != 1) {
        close(client_socket);
    }
    
    return SUCCESS;
}




int main(int argc, char** argv)
{
    create_client(argv[1], argv[2]);
}

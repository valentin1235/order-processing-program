#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "courier_generator.h"

#define DEFAULT_PORT ("3000")
#define INPUT_SIZE (1000)
#define READ_SIZE (1000)
#define WRITE_SIZE (4000)
#define SERVICE_NAME ("COURIER")
#define ORDER_NAME_SIZE (50)
#define TIME_STR_SIZE (11)

static pthread_t s_thread_receive;
static int s_sock_client = -1;
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

static void SIGINT_handler(int sig)
{
    pthread_kill(s_thread_receive, sig);

    if (s_sock_client != -1) {
        close(s_sock_client);
    }

    printf("* 프로그램을 종료합니다. (시그널 번호 : %d) -> %s\n", sig, __func__);

    exit(1);
}

static void* receive_message(void* p)
{
    int client_socket = *(int*)p;
    char buffer[READ_SIZE];

    printf("* 주문처리 서버로부터 메세지를 받고있습니다 -> %s\n", __func__);
    do {
        int read_len;

        if (s_exit == 1) {
            printf("* 의도하지 않은 종료 -> %s\n", __func__);
            break;
        }

        read_len = read(client_socket, buffer, sizeof(buffer) - 1);
        if (read_len == 0 || read_len == -1) {
            s_exit = 1;
            shutdown(client_socket, SHUT_RDWR);
            printf("* ECONNRESET : connection closed -> %s\n", __func__);
            break;
        }

        buffer[READ_SIZE - 1] = '\0';
        printf("* 받은 메세지 : \"%s\" -> %s\n", __func__, buffer);
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
    char input_buff[INPUT_SIZE] = { 0 };
    char write_buff[WRITE_SIZE] = { 0 };

    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("* 소켓을 가져올수 없습니다 -> %s\n", __func__);
        return ERROR_SOCKET;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_host);
    server_addr.sin_port = htons(atoi(server_port));

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("* 서버 연결에 실패했습니다 -> %s\n", __func__);
        return ERROR_CONNECT;
    }

    pthread_create(&s_thread_receive, NULL, receive_message, &client_socket);

    printf("* 메세지를 입력해주세요. \"주문이름#도착하는데_걸리는_시간\" -> %s\n", __func__);
    do {
        fgets(input_buff, INPUT_SIZE, stdin);
        if (input_buff[0] == '\0' || validate_input(input_buff, INPUT_SIZE) == FALSE) {
            printf("* 메세지 형식이 잘못되었습니다. 메세지를 다시 입력해주세요. \"주문이름#도착하는데_걸리는_시간\" -> %s\n", __func__);
            continue;
        }

        build_message(write_buff, input_buff);
        write(client_socket, write_buff, WRITE_SIZE);
        printf("* 메세지를 보냈습니다 : \"%s\" -> %s\n", write_buff, __func__);

    } while (TRUE);

    return SUCCESS;
}

int main(int argc, char** argv)
{
    /* register signal handler */
    signal(SIGINT, SIGINT_handler);

    create_client(argv[1], argv[2]);
}

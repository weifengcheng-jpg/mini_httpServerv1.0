#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#define SERVER_PORT 80

int get_line(int sock, char *buf, int size);
void do_http_request(int client_sock);

int main(void) {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    listen(sock, 128);

    printf("等待客户端的连接\n");
    
    int done = 1;

    while (done) {
        struct sockaddr_in client;
        int client_sock, len, i;
        char client_ip[64];
        char buf[256];

        socklen_t client_addr_len;
        client_addr_len = sizeof(client);
        client_sock = accept(sock, (struct sockaddr*)&client, &client_addr_len);

        //打印客户端IP地址和端口号
        printf("client ip: %s\t port : %d\n", 
        inet_ntop(AF_INET, (void *)&client.sin_addr.s_addr, client_ip, sizeof(client_ip)),
        ntohs(client.sin_port));

        //处理http 请求, 读取客户端发送的数据
        do_http_request(client_sock);
        close(client_sock);

    }

    close(sock);

    return 0;
}

void do_http_request(int client_sock) {
    int len = 0;
    char buf[256];

    //读取客户端发送的http请求

    //1. 读取请求行
    do {
        len = get_line(client_sock, buf, sizeof(buf));
        printf("read line : %s\n", buf);
    } while (len > 0);
}

//返回值: -1 表示读取, 等于0表示读到一个空行, 大于0表示成功读取一行
int get_line(int sock, char *buf, int size) {
    int count = 0;
    char ch = '\0';
    int len = 0;

    while ( (count < size - 1) && ch != '\n') {
        len = read(sock, &ch, 1);
        if (len == 1) {
            if (ch == '\r') {
                continue;
            } else if (ch == '\n') {
                //buf[count] = '\0';
                break;
            }

            //这里处理一般的字符
            buf[count] = ch;
            count++;
        } else if (len == -1) { //读取出错
            perror("read failed");
            count = -1;
            break;
        } else { //read返回0, 客户端关闭sock连接.
            fprintf(stderr, "client close.\n");
            count = -1;
            break;
        }
    }

    if (count >= 0) buf[count] = '\0';

    return count;
}


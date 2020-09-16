#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#define SERVER_PORT 80

static int debug = 1;

int get_line(int sock, char *buf, int size); //int get_line(int sock, char buf[], int size);
void do_http_request(int client_sock);
void do_http_response(int client_sock);

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
    char method[64];
    char url[256];
    char path[256];

    //读取客户端发送的http请求

    //1. 读取请求行
    
    len = get_line(client_sock, buf, sizeof(buf));
    
    if (len > 0) { //读到了请求行
        int i = 0, j = 0;
        while (!isspace(buf[j]) && i < (sizeof(method) - 1)) {
            method[i] = buf[j];
            i++;
            j++;
        }

        method[i] = '\0';
        if (debug) printf("request method: %s \n", method);

        if (strncasecmp(method, "GET", i) == 0) { //只处理get请求
            if (debug) printf("method = GET\n");

            //获取url
            while (isspace(buf[j++])); //跳过白空格
            i = 0;

            while (!isspace(buf[j]) && i < (sizeof(url) - 1)) {
                url[i] = buf[j];
                i++;
                j++;
            }

            url[i] = '\0';

            if (debug) printf("url: %s\n", url);

            //继续读取http头部
            do {
                len = get_line(client_sock, buf, sizeof(buf));
                if (debug) printf("read: %s\n", buf);
            } while (len > 0);

            //定位服务器本地的html文件

            //处理url 中的?
            {
                char *pos = strchr(url, '?');
                if (pos) {
                    *pos = '\0';
                    printf("real url: %s\n", url);
                }
            }

            sprintf(path, "./html_docs/%s", url);
            if (debug) printf("path: %s\n", path);

            //执行http 响应
            do_http_response(client_sock);

        } else { //非get请求, 读取http头部, 并响应客户端 501 Method Not Implemented
            fprintf(stderr, "warning! other request [%s]\n", method);
            do {
                len = get_line(client_sock, buf, sizeof(buf));
                if (debug) printf("read: %s\n", buf);
            } while (len > 0);

            //unimplemented(client_sock);
        }

        
        
    } else { //请求格式有问题, 出错处理
        //bad_request(client_sock);
    }

}

void do_http_response(int client_sock) {
    const char* main_header = "HTTP/1.0 200 OK\r\nServer: Martin Server\r\nContent -Type: text/html\r\nConnection: Close\r\n";
    const char* welcome_content = "\
    <!DOCTYPE html>\n\
<html lang=\"en\">\n\
<head>\n\
    <meta charset=\"UTF-8\">\n\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n\
    <title>Document</title>\n\
</head>\n\
<body>\n\
    这是一个迷你服务器\n\
</body>\n\
</html>\n\
    ";

    //1. 送main_header
    int len = write(client_sock, main_header, strlen(main_header));

    if (debug) fprintf(stdout, "... do_http response...\n");
    if (debug) fprintf(stdout, "write[%d]: %s", len, main_header);

    //2. 生成 Content-Length 行并发送
    char send_buf[64];
    int wc_len = strlen(welcome_content);
    len = snprintf(send_buf, 64, "Content-Length: %d\r\n\r\n", wc_len);
    len = write(client_sock, send_buf, len);

    if (debug) fprintf(stdout, "write[%d]: %s", len, send_buf);

    len = write(client_sock, welcome_content, wc_len);
    if (debug) fprintf(stdout, "write[%d]: %s", len, welcome_content);
    //3. 发送html 文件内容
}

//返回值: -1 表示读取失败, 等于0表示读到一个空行, 大于0表示成功读取一行
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


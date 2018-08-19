//
// Created by liushan on 18-8-15.
//


#include <stdio.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <memory.h>
#include <zconf.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>
#include <regex.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <limits.h>
#include <errno.h>

#include "mysignal.h"
#include "fastcgi.h"
#include "tool.h"
#include "writen_readn_readline.h"
#include "process_request.h"

struct response_header_fastcgi {
    char http_version[32];
    char content_type[64];
    char connection[64];
    char server[64];
    int status;
    char status_desc[64];
    char transfer_encoding[64];
};



void process_request_fastcgi(int connection_fd, char * filename)
{
    struct sockaddr_in servaddr;
    int sock_cli = socket(AF_INET, SOCK_STREAM, 0);
    const int buffer_size = 1024;
    char sendbuf[buffer_size];
    char recvbuf[buffer_size];

    char response[4096];
    char ch_temp[1024];

    struct response_header_fastcgi header_resonse = {
            .http_version = "HTTP/1.1",
            .content_type = "text/html",
            .transfer_encoding = "chunked",
            .connection = "keep-alive",
            .server = "Dagama",
            .status = 200,
            .status_desc = "OK"
    };

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9000);
    servaddr.sin_addr.s_addr = inet_addr("192.168.174.130");



    if (connect(sock_cli, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        printf("conn error, now exit!\n");
        exit(2);
    }

    fastcgi_request(sock_cli);
    printf("1\n");
    fastcgi_sendparams(sock_cli, "SCRIPT_FILENAME", filename);
    // fastcgi_sendparams(sock_cli, "SCRIPT_FILENAME", "/opt/application/nginx/myphp/phpinfo.php");
    printf("2\n");
    fastcgi_sendparams(sock_cli, "REQUEST_METHOD", "GET");
    fastcgi_sendendparams(sock_cli);
    fastcgi_sendendstdin(sock_cli);

    // 拼接响应头
    sprintf(response, "%s %d %s\n", header_resonse.http_version, header_resonse.status, header_resonse.status_desc);
    sprintf(ch_temp, "Content-Type: %s\n", header_resonse.content_type);
    strcat(response, ch_temp);
    sprintf(ch_temp, "Transfer-Encoding: %s\n", header_resonse.transfer_encoding);
    strcat(response, ch_temp);
    sprintf(ch_temp, "Connection: %s\n", header_resonse.connection);
    strcat(response, ch_temp);
    sprintf(ch_temp, "Server: %s\n\n", header_resonse.server);
    strcat(response, ch_temp);


    writen(connection_fd, response, strlen(response) );
    fastcgi_recv(sock_cli, connection_fd);


    printf("bye!\n");

}
//
// Created by liushan on 18-8-14.
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

#include "fastcgi.h"
#include "writen_readn_readline.h"


#define PARAMS_LEN 1024

void make_header(int request_id, int content_len, FCGI_Header * header, unsigned char type);



void fastcgi_request(int connection_fd)
{
    FCGI_BeginRequestRecord begin_record;
    int request_id = 1;
    int content_len = sizeof(begin_record.body);


    make_header(request_id, content_len, &begin_record.header, FCGI_BEGIN_REQUEST);

    begin_record.body.roleB1 = ( (FCGI_RESPONDER >> 8) & 0xff);
    begin_record.body.roleB0 = (FCGI_RESPONDER & 0xff);
    begin_record.body.flags = 0; // 请求完成后，关闭连接
    // begin_record.body.reserved

    writen(connection_fd, &begin_record, sizeof(begin_record) );

}



void fastcgi_sendparams(int connection_fd, char * key_name, char * key_value)
{
    int request_id = 1;
    int content_len;
    FCGI_Header header;
    unsigned char params[PARAMS_LEN];
    unsigned char * params_ptr = params;

    int body_len;
    size_t temp_num;

    bzero(params, sizeof(params) );


    // 使用1位，或者使用4位表示 key_name
    if (strlen(key_name) <= 127)
    {
        *params_ptr = (unsigned char) strlen(key_name);
        params_ptr++;
    }
    else
    {
        temp_num = strlen(key_name);

        *params_ptr = (unsigned char) ( ( (temp_num >> 24) & 0x7f) | 0x80);
        params_ptr++;
        *params_ptr = (unsigned char) ( (temp_num >> 16) & 0xff);
        params_ptr++;
        *params_ptr = (unsigned char) ( (temp_num >> 8) & 0xff);
        params_ptr++;
        *params_ptr = (unsigned char) (temp_num & 0xff);
        params_ptr++;
    }

    // 使用1位，或者使用4位表示 key_value
    if (strlen(key_value) <= 127)
    {
        *params_ptr = (unsigned char) strlen(key_value);
        params_ptr++;
    }
    else
    {
        temp_num = strlen(key_value);

        *params_ptr = (unsigned char) ( ( (temp_num >> 24) & 0x7f) | 0x80);
        params_ptr++;
        *params_ptr = (unsigned char) ( (temp_num >> 16) & 0xff);
        params_ptr++;
        *params_ptr = (unsigned char) ( (temp_num >> 8) & 0xff);
        params_ptr++;
        *params_ptr = (unsigned char) (temp_num & 0xff);
        params_ptr++;
    }


    memcpy(params_ptr, key_name, strlen(key_name) );
    params_ptr += strlen(key_name);
    memcpy(params_ptr, key_value, strlen(key_value) );
    params_ptr += strlen(key_value);

    // 长度，指针累计增加的大小，即为长度
    body_len = (int) (params_ptr - params);
    printf("body_len: %d\n", body_len);

    make_header(request_id, body_len, &header, FCGI_PARAMS);

    unsigned char data_sent[sizeof(header) + body_len];

    memcpy(data_sent, &header, sizeof(header) );
    memcpy(data_sent + sizeof(header), params, body_len);

    writen(connection_fd, data_sent, sizeof(data_sent) );

}


void fastcgi_sendendparams(int connection_fd)
{
    int request_id = 1;
    FCGI_Header header;
    int body_len = 0;

    make_header(request_id, body_len, &header, FCGI_PARAMS);
    writen(connection_fd, &header, sizeof(header) );
}


void fastcgi_sendendstdin(int connection_fd)
{
    int request_id = 1;
    FCGI_Header header;
    int body_len = 0;

    make_header(request_id, body_len, &header, FCGI_STDIN);
    writen(connection_fd, &header, sizeof(header) );
}


void fastcgi_recv(int sock_cli, int connection_fd)
{
    FCGI_Header header;
    int content_len;
    char response[65535];



    while (readn(sock_cli, &header, sizeof(header) ) > 0)
    {
        content_len = ( (header.contentLengthB1 << 8) & 0xff00);
        content_len += (header.contentLengthB0 & 0xff);

        printf("content_len: %d\n", content_len);
        printf("header.type: %d\n", header.type);

        if (content_len > 0 && (header.type == FCGI_STDOUT || header.type == FCGI_STDERR) )
        {
            content_len++;

            unsigned char * content_data;
            unsigned char * padding_data;

            content_data = (unsigned char *) malloc(sizeof(unsigned char) * content_len);
            padding_data = (unsigned char *) malloc(sizeof(unsigned char) * header.paddingLength);
            readn(sock_cli, content_data, content_len - 1);
            readn(sock_cli, padding_data, header.paddingLength);

            content_data[content_len - 1] = 0;

            if (header.type == FCGI_STDOUT)
            {
                bzero(response, sizeof(response) / sizeof(char));
                printf("stdout_data: \n%s\n", content_data);
                snprintf(response, 65535, "%x\n%s\n", strlen(content_data), content_data);
                writen(connection_fd, response, strlen(response));
            }
            else if (header.type == FCGI_STDERR)
            {
                printf("stderr_data: \n%s\n", content_data);
            }


            free(content_data);
            free(padding_data);
        }
        else if (content_len > 0 && header.type == FCGI_END_REQUEST)
        {
            FCGI_EndRequestBody * end_body;
            int appstatus;

            end_body = (FCGI_EndRequestBody *) malloc(sizeof(FCGI_EndRequestBody) );
            readn(sock_cli, end_body, sizeof(FCGI_EndRequestBody) );
            appstatus = ((end_body->appStatusB3 << 24) & 0xff000000);
            appstatus += ((end_body->appStatusB3 << 16) & 0xff0000);
            appstatus += ((end_body->appStatusB3 << 8) & 0xff00);
            appstatus += (end_body->appStatusB3 & 0xff);
            printf("appstatus: \n%d\n", appstatus);
            printf("protocol: \n%d\n", end_body->protocolStatus);
            printf("end_request_data: %s\n", "end_request");

            bzero(response, sizeof(response) / sizeof(char));

            snprintf(response, 65535, "%x\n\n", 0);
            writen(connection_fd, response, strlen(response));
        }

    }

}


void make_header(int request_id, int content_len, FCGI_Header * header, unsigned char type)
{
    header->version = FCGI_VERSION_1;
    header->type = type;

    header->requestIdB1 = (unsigned char)( (request_id >> 8) & 0xff);
    header->requestIdB0 = (unsigned char)(request_id & 0xff);
    header->contentLengthB1 = (unsigned char)( (content_len >> 8) & 0xff);
    header->contentLengthB0 = (unsigned char)(content_len & 0xff);
    header->paddingLength = 0;
    header->reserved = 0;
}
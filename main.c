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



#define SET_RESPONSE_STATUS_200(header_resonse)  header_resonse.status = 200; strcpy(header_resonse.status_desc, "OK")
#define SET_RESPONSE_STATUS_403(header_resonse)  header_resonse.status = 403; strcpy(header_resonse.status_desc, "forbidden")
#define SET_RESPONSE_STATUS_404(header_resonse)  header_resonse.status = 404; strcpy(header_resonse.status_desc, "file not found");
#define SET_RESPONSE_STATUS_405(header_resonse)  header_resonse.status = 405; strcpy(header_resonse.status_desc, "method not allowed");


#define FILE_PATH_MAX_LENTH 256
#define EXTENSION_NAME_LENTH 8


struct request_header {
    char method[10];
    char uri[1024];
    char http_version[32];
    char headers[100][2][1024];
    int headers_len;
};

struct response_header {
    char http_version[32];
    char content_type[64];
    unsigned long long content_length;
    char connection[64];
    char server[64];
    int status;
    char status_desc[64];
};


struct mimedict {
    char extension[8];
    char content_type[128];
};

struct default_request_file {
    char request_file_404[FILE_PATH_MAX_LENTH];
    char request_file_403[FILE_PATH_MAX_LENTH];
    char request_file_405[FILE_PATH_MAX_LENTH];
};


void  get_mimebook(const char * mime_file, struct mimedict mimebook [], int mimebook_len);



void  get_mimebook(const char * mime_file, struct mimedict mimebook [], int mimebook_len)
{
    FILE * fd_mime;
    const int buffersize = 4096;
    char line[buffersize];
    char content_type_temp[128];
    char extension_temp[EXTENSION_NAME_LENTH];
    int index_mimebook = 0, index_line_ch, index_extension;
    bool flag_content_type = true, flag_extension = false;

    if ( (fd_mime = fopen(mime_file, "r") ) == NULL)
    {
        printf("open %s failed\n", mime_file);
    }

    while (fgets(line, buffersize - 1, fd_mime) != NULL)
    {
        // 每行第一个字符不是字母或者数字的，跳过
        if (! isalnum(line[0]) )
            continue;

        // 每行，循环读取每个字符
        for (index_line_ch = 0, index_extension = 0; index_line_ch < strlen(line); index_line_ch++)
        {
            // flag_content_type开关开启时，读取的非空字符存入content_type_temp
            if (flag_content_type)
            {
                // 读到空白字符后，flag_content_type开关关闭
                if (isblank(line[index_line_ch]) )
                {
                    content_type_temp[index_line_ch] = '\0';
                    flag_content_type = false;
                }
                else
                {
                    content_type_temp[index_line_ch] = line[index_line_ch];
                }
            }
            else
            {
                // flag_extension关闭时，读取到空行，跳过
                // flag_extension开启时，读取到空行，进行赋值操作
                if (isblank(line[index_line_ch]) || line[index_line_ch] == ';')
                {
                    if (! flag_extension)
                        continue;
                    extension_temp[index_extension] = '\0';
                    if (index_mimebook >= mimebook_len)
                    {
                        fprintf(stderr, "mimefile count is more than mimebook_len, now break!\n");
                        fclose(fd_mime);
                        return;
                    }
                    strcpy(mimebook[index_mimebook].content_type, content_type_temp);
                    strcpy(mimebook[index_mimebook].extension, extension_temp);
                    index_mimebook++;
                    flag_extension = false;
                    index_extension = 0;
                    if (line[index_line_ch] == ';')
                    {
                        flag_content_type = true;
                        break;
                    }

                }
                else
                {
                    extension_temp[index_extension] = line[index_line_ch];
                    index_extension++;
                    flag_extension = true;
                }

            }

        }

    }


    fclose(fd_mime);
}

void * get_contenttype_by_filepath(char * filepath, struct mimedict mimebook [], int mimebook_len,
                                   struct response_header * header_resonse);

void * get_contenttype_by_filepath(char * filepath, struct mimedict mimebook [], int mimebook_len,
                                   struct response_header * header_resonse)
{
    char extension_name_temp[EXTENSION_NAME_LENTH], extension_name[EXTENSION_NAME_LENTH];
    int index;
    int index_extension;

    for (index = strlen(filepath) - 1, index_extension = 0; index >= 0; index--, index_extension++)
    {
        if (filepath[index] == '.')
        {
            extension_name_temp[index_extension] = '\0';
            break;
        }
        else
            extension_name_temp[index_extension] = filepath[index];
    }

    for (index = 0; index < index_extension; index++)
        extension_name[index] =  extension_name_temp[index_extension - 1 - index];
    extension_name[index] = '\0';

    for (index = 0; index < mimebook_len; index++)
    {
        if (strcmp(extension_name, mimebook[index].extension) == 0)
            strcpy(header_resonse->content_type, mimebook[index].content_type);
    }

}



int split_str_by_ch(const char *str_ori, int str_ori_len, char ch_split, char str_tgt[][1024], int ch_num);
// 字符串分割成数组
// 例如：　10.10.10.10, 10.10.10.2 分割为["10.10.10.10", "10.10.10.2"]
// 返回生成的数组长度
int split_str_by_ch(const char *str_ori, int str_ori_len, char ch_split, char str_tgt[][1024], int ch_num)
{
    int str_ori_ch_index;
    int str_index, ch_pos;
    bool flag = false;

    for (str_ori_ch_index = 0, str_index = 0, ch_pos = 0; str_ori_ch_index < str_ori_len; str_ori_ch_index++)
    {
        if (str_index >= ch_num)
        {
            str_index = ch_num;
            return str_index;
        }

        if (str_ori[str_ori_ch_index] == ch_split)
        {
            str_tgt[str_index][ch_pos] = '\0';
            str_index++;
            ch_pos = 0;
            flag = true;
        }
        else
        {
            // 刚分割的第一个字符是空格的，忽略
            if (flag)
                if (isblank(str_ori[str_ori_ch_index]) )
                    continue;
            str_tgt[str_index][ch_pos] = str_ori[str_ori_ch_index];
            ch_pos++;
            flag = false;
        }
    }
    str_tgt[str_index][ch_pos] = '\0';
    str_index++;

    return str_index;
}


void parse_header_request(char * headers_recv, struct request_header * headers_request);
void parse_header_request(char * headers_recv, struct request_header * headers_request)
{
    char line_char_s[4][1024];
    char line_read[1024];
    int index, temp_index, char_s_count, header_index;
    bool flag = true;

    for (index = 0, temp_index = 0, header_index = 0; index < strlen(headers_recv); index++)
    {
        if (headers_recv[index] == '\n')
        {
            line_read[temp_index] = '\0';

            if (flag)
            {
                char_s_count = split_str_by_ch(line_read, strlen(line_read), ' ', line_char_s, 4);
                if (char_s_count == 3)
                {
                    strcpy(headers_request->method, line_char_s[0]);
                    strcpy(headers_request->uri, line_char_s[1]);
                    strcpy(headers_request->http_version, line_char_s[2]);
                    flag = false;
                }
            } else
            {
                char_s_count = split_str_by_ch(line_read, strlen(line_read), ':', line_char_s, 4);
                if (char_s_count == 2)
                {
                    strcpy(headers_request->headers[header_index][0], line_char_s[0]);
                    strcpy(headers_request->headers[header_index][1], line_char_s[1]);
                    header_index++;
                }
            }

            temp_index = 0;
        }
        else
        {
            line_read[temp_index] = headers_recv[index];
            temp_index++;
        }
    }

    headers_request->headers_len = header_index;

}

void process_request(int connection_fd, struct sockaddr_in * client_sockaddr,
                     const struct default_request_file * request_file_default,
                     const char * doc_root, const char * file_index[], int file_index_len,
                     const char * method_allow[], int method_allow_len, const char * mime_file);

void process_request(int connection_fd, struct sockaddr_in * client_sockaddr,
                     const struct default_request_file * request_file_default,
                     const char * doc_root, const char * file_index[], int file_index_len,
                     const char * method_allow[], int method_allow_len, const char * mime_file)
{
    char cli_addr_buff[INET_ADDRSTRLEN];
    ssize_t buffer_size = 4096, read_buffer_size;
    char read_buffer[buffer_size], send_buffer[buffer_size];
    FILE * fd_request_file;
    FILE * fd_write_file;
    int len;

    struct sockaddr_in cli_sockaddr, srv_sockaddr;
    int cli_sockaddr_len, srv_sockaddr_len;
    char addr_buf[INET_ADDRSTRLEN];

    struct response_header header_resonse = {
            .http_version = "HTTP/1.1",
            .content_type = "image/jpeg",
            .content_length = 16,
            .connection = "keep-alive",
            .server = "Dagama",
            .status = 200,
            .status_desc = "OK"
    };

    memset(read_buffer, 0, sizeof(read_buffer));
    len = read(connection_fd, read_buffer, buffer_size - (ssize_t)1);
    read_buffer[len] = '\0';

    // 打印调试信息
//        printf("received:\n");
//        printf("%s", read_buffer);


    struct request_header header_request;

    char response[4096];
    char ch_temp[1024];
    const int mimebook_len = 103;

    parse_header_request(read_buffer, &header_request);

    printf("request_uri: %s\n", header_request.uri);

    struct stat statbuf;
    char request_file[512];
    int index_temp;
    bool flag_temp;

    printf("------------------\n");
    printf("this is:%s\n", file_index[0]);
    printf("this is:%s\n", file_index[1]);
    printf("------------------\n");

    struct mimedict mimebook[mimebook_len];
    get_mimebook(mime_file, mimebook, mimebook_len);


    flag_temp = false;
    for (index_temp = 0; index_temp < 3; index_temp++)
    {
        if (strcmp(method_allow[index_temp], header_request.method) == 0)
            flag_temp = true;
    }

    if (! flag_temp)
    {
        SET_RESPONSE_STATUS_405(header_resonse);
        strcpy(request_file, request_file_default->request_file_405);
    }
    else if (strcmp(header_request.method, "GET") == 0)
    {
        // 访问的uri是目录的，重写到该目录下的index文件
        if (header_request.uri[strlen(header_request.uri) - 1] == '/')
        {
            sprintf(request_file, "%s/%s%s", doc_root, header_request.uri, file_index[0]);
            if (access(request_file, F_OK) == -1)
            {
                sprintf(request_file, "%s/%s%s", doc_root, header_request.uri, file_index[1]);
                if (access(request_file, F_OK) == -1)
                {
                    SET_RESPONSE_STATUS_403(header_resonse);
                    strcpy(request_file, request_file_default->request_file_403);
                }
                else
                SET_RESPONSE_STATUS_200(header_resonse);
            }
            else
            SET_RESPONSE_STATUS_200(header_resonse);
        }
        else
        {
            sprintf(request_file, "%s/%s", doc_root, header_request.uri);

            printf("request_file: %s\n", request_file);
            // 根据文件是否存在，重新拼接请求文件，生成状态码
            // 文件存在
            if (access(request_file, F_OK) != -1)
            {
                // 获取文件信息，如果失败则403
                if (stat(request_file, &statbuf) != -1)
                {
                    // 如果为普通文件
                    if (S_ISREG(statbuf.st_mode) )
                    {
                        SET_RESPONSE_STATUS_200(header_resonse);
                    }
                    else
                    {
                        SET_RESPONSE_STATUS_404(header_resonse);
                        strcpy(request_file, request_file_default->request_file_404);
                    }
                }
                else
                {
                    printf("get file %s stat error\n", request_file);
                    SET_RESPONSE_STATUS_403(header_resonse);
                    strcpy(request_file, request_file_default->request_file_403);
                }

            }
                // 文件不存在,则404
            else
            {
                SET_RESPONSE_STATUS_404(header_resonse);
                strcpy(request_file, request_file_default->request_file_404);
            }

            printf("request_file: %s\n", request_file);

        }



    }
    else if (strcmp(header_request.method, "POST") == 0)
    {
        printf("POST is not finished yet!\n");
    }



    if (stat(request_file, &statbuf) != -1)
        header_resonse.content_length = statbuf.st_size;
    else
    {
        printf("get statbuf error!\n");
        // continue;
    }




    if ( (fd_request_file = fopen(request_file, "rb")) == NULL)
    {
        fprintf(stderr, "open file %s error!\n", request_file);
        // exit(EXIT_FAILURE);
        SET_RESPONSE_STATUS_403(header_resonse);
        strcpy(request_file, request_file_default->request_file_403);
        if ( (fd_request_file = fopen(request_file, "rb")) == NULL)
        {
            printf("open file error!\n");
            // continue;
        }

    }



    get_contenttype_by_filepath(request_file, mimebook, mimebook_len, &header_resonse);
    printf("begine send\n");

    // 拼接响应头
    sprintf(response, "%s %d %s\n", header_resonse.http_version, header_resonse.status, header_resonse.status_desc);
    sprintf(ch_temp, "Content-Type: %s\n", header_resonse.content_type);
    strcat(response, ch_temp);
    sprintf(ch_temp, "Content-Length: %llu\n", header_resonse.content_length);
    strcat(response, ch_temp);
    sprintf(ch_temp, "Connection: %s\n", header_resonse.connection);
    strcat(response, ch_temp);
    sprintf(ch_temp, "Server: %s\n\n", header_resonse.server);
    strcat(response, ch_temp);


    srv_sockaddr_len = sizeof(srv_sockaddr);
    if (getsockname(connection_fd, (struct sockaddr *)&srv_sockaddr, &srv_sockaddr_len) == -1)
        printf("getsockname() error!\n");
    else
    {
        printf("local ip: %s", inet_ntop(AF_INET, &srv_sockaddr.sin_addr, addr_buf, INET_ADDRSTRLEN));
        printf(" local port: %d\n", ntohs(srv_sockaddr.sin_port));
    }

    cli_sockaddr_len = sizeof(cli_sockaddr);
    if (getpeername(connection_fd, (struct sockaddr *)&cli_sockaddr, &cli_sockaddr_len) == -1)
        printf("getpeername() error!\n");
    else
    {
        printf("peer ip: %s", inet_ntop(AF_INET, &cli_sockaddr.sin_addr, addr_buf, INET_ADDRSTRLEN) );
        printf(" peer port: %d\n", ntohs(cli_sockaddr.sin_port) );
    }

    printf("response header:\n%s", response);

    printf("Connection from client: %s:%d\n",
           inet_ntop(AF_INET, &client_sockaddr->sin_addr, cli_addr_buff, INET_ADDRSTRLEN),
           ntohs(client_sockaddr->sin_port) );




    // 发送响应头信息
    write(connection_fd, response, strlen(response));


    if ( (fd_write_file = fdopen(connection_fd, "wb") ) == NULL)
    {
        fprintf(stderr, "get conn fd error\n");
        exit(EXIT_FAILURE);
    }


    while ( (read_buffer_size = fread(send_buffer, sizeof(char), buffer_size, fd_request_file) ) > 0)
    {
        fwrite(send_buffer, sizeof(char), read_buffer_size, fd_write_file);
        // printf("send size: %lu\n", read_buffer_size);
    }

    if (read_buffer_size < buffer_size)
    {
        if (feof(fd_request_file) != 0)
            printf("read to the end of file %s\n", request_file);
        if (ferror(fd_request_file) != 0)
        {
            fprintf(stderr, "read file %s error\n", request_file);
            exit(EXIT_FAILURE);
        }
    }

    // close(conn);
    fclose(fd_write_file);
}



int main() {
    int fd_server_sock, connection_fd;
    struct sockaddr_in server_sockaddr, client_sockaddr;
    const int myqueue = 100;
    int cli_len;
    pid_t pid;

    // 运行的配置参数
    char * doc_root = "/home/liushan/mylab/clang/dagama/webroot";
    const char * file_index[2] = {"index.html", "index.htm"};
    const char * method_allow[3] = {"GET", "DELETE", "POST"};
    char mime_file[FILE_PATH_MAX_LENTH] = "/home/liushan/mylab/clang/dagama/mime.types";
    struct default_request_file request_file_default = {
            .request_file_404 = "/home/liushan/mylab/clang/dagama/webroot/html/404.html",
            .request_file_403 = "/home/liushan/mylab/clang/dagama/webroot/html/403.html",
            .request_file_405 = "/home/liushan/mylab/clang/dagama/webroot/html/405.html"
    };





    // exit(EXIT_SUCCESS);


    bzero(&server_sockaddr, sizeof(server_sockaddr));
    fd_server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port =  htons(8080);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd_server_sock, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1)
    {
        printf("bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(fd_server_sock, myqueue) == -1)
    {
        printf("listen error\n");
        exit(EXIT_FAILURE);
    }

    // 处理子进程，以防变成僵死进程
    signal(SIGCHLD, sig_chld);

    while(1)
    {
        cli_len = sizeof(client_sockaddr);
        if ( (connection_fd = accept(fd_server_sock, (struct sockaddr *) &client_sockaddr, &cli_len)) < 0)
        {
            // 处理被中断的系统调用accept
            if (errno == EINTR)
                continue;
            else
            {
                printf("accept error!\n");
                exit(EXIT_FAILURE);
            }

        }

        if ( (pid = fork() ) < 0)
        {
            printf("fork error\n");
            exit(EXIT_FAILURE);
        }
        else if (pid != 0)
        {
            // parent
            printf("fork child: %d\n", pid);
            close(connection_fd);
        }
        else if (pid == 0)
        {
            // child
            close(fd_server_sock);
            process_request(connection_fd, &client_sockaddr,
                            &request_file_default,
                            doc_root, file_index, sizeof(file_index) / sizeof(char *),
                            method_allow, sizeof(method_allow) / sizeof(char *), mime_file);
            close(connection_fd);
            exit(0);
        }

    }

}

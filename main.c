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


struct request_header {
    char method[10];
    char uri[1024];
    char http_version[32];
    char host[256];
    char connection[64];
    char user_agent[1024];
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

#define FILE_PATH_MAX_LENTH 256
#define EXTENSION_NAME_LENTH 8

struct mimedict {
    char extension[8];
    char content_type[128];
};


void  get_mimebook(char * mime_file, struct mimedict mimebook [], int mimebook_len);



void  get_mimebook(char * mime_file, struct mimedict mimebook [], int mimebook_len)
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

void * parse_header_request(struct request_header * header_request, const char * header_recv);

void * parse_header_request(struct request_header * header_request, const char * header_recv)
{
    unsigned long index;
    int flag_method = 0, flag_uri = 0, flag_httpversion = 0;
    int myindex = 0, index_temp = 0;
    bool flag_start = true;
    char key_temp[64];

    regex_t re;
    regmatch_t subs[128];
    char matched[512];
    char errbuf[512];







    for (index = 0; index < strlen(header_recv); index++)
    {
        if (myindex <= 2)
        {
            if (myindex == 0)
            {
                if (! isspace(header_recv[index]) )
                {
                    header_request->method[index_temp] = header_recv[index];
                    index_temp++;
                }
                else
                {
                    header_request->method[index_temp] = '\0';
                    index_temp = 0;
                    myindex++;
                    flag_start = false;
                }
            }
            else if (myindex == 1)
            {
                if (! isspace(header_recv[index]) )
                {
                    header_request->uri[index_temp] = header_recv[index];
                    flag_start = true;
                    index_temp++;
                }
                else
                {
                    if (flag_start)
                    {
                        header_request->uri[index_temp] = '\0';
                        index_temp = 0;
                        myindex++;
                        flag_start = false;
                    }
                }
            }
            else if (myindex == 2)
            {
                if (! isspace(header_recv[index]) )
                {
                    header_request->http_version[index_temp] = header_recv[index];
                    flag_start = true;
                    index_temp++;
                }
                else
                {
                    if (flag_start)
                    {
                        header_request->http_version[index_temp] = '\0';
                        index_temp = 0;
                        myindex++;
                    }
                }
            }
        }



    }

    printf("### method: %s\n", header_request->method);
    printf("### uri: %s\n", header_request->uri);
    printf("### http_version: %s\n", header_request->http_version);


}




int main() {
    int fd_server_sock;
    FILE * fd_request_file;
    FILE * fd_write_file;
    struct sockaddr_in server_sockaddr;
    const int myqueue = 100;
    int conn, len;
    ssize_t buffer_size = 4096, read_buffer_size;
    char read_buffer[buffer_size], send_buffer[buffer_size];
    struct response_header header_resonse = {
            .http_version = "HTTP/1.1",
            .content_type = "image/jpeg",
            .content_length = 16,
            .connection = "keep-alive",
            .server = "Dagama",
            .status = 200,
            .status_desc = "OK"
    };

    struct request_header header_request;

    char response[4096];
    char ch_temp[1024];
    const int mimebook_len = 103;
    char * doc_root = "/home/liushan/mylab/clang/dagama/webroot";
    struct stat statbuf;
    char request_file[512];


    char mime_file[FILE_PATH_MAX_LENTH] = "/home/liushan/mylab/clang/dagama/mime.types";
    char * request_file_404 = "/home/liushan/mylab/clang/dagama/webroot/html/404.html";
    char * request_file_403 = "/home/liushan/mylab/clang/dagama/webroot/html/403.html";

    struct mimedict mimebook[mimebook_len];
    get_mimebook(mime_file, mimebook, mimebook_len);


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


    while(1)
    {
        if ( (conn = accept(fd_server_sock, (struct sockaddr *) NULL, NULL)) < 0)
        {
            printf("connect error!\n");
            exit(EXIT_FAILURE);
        }


        memset(read_buffer, 0, sizeof(read_buffer));
        len = read(conn, read_buffer, buffer_size - (ssize_t)1);
        read_buffer[len] = '\0';

        // 打印调试信息
        printf("received:\n");
        printf("%s", read_buffer);

        parse_header_request(&header_request, read_buffer);

        sprintf(request_file, "%s/%s", doc_root, header_request.uri);

        // 根据文件是否存在，重新拼接请求文件，生成状态码
        // 文件存在
        if (access(request_file, F_OK) != -1)
        {
            // 打开文件失败，则403
            if ( (fd_request_file = fopen(request_file, "rb") ) == NULL)
            {
                // printf("open file %s failed\n", request_file);
                header_resonse.status = 403;
                strcpy(header_resonse.status_desc, "forbidden");
                strcpy(request_file, request_file_403);
            }
            // 打开文件正常，则200
            else
            {
                header_resonse.status = 200;
                strcpy(header_resonse.status_desc, "OK");
            }
        }
        // 文件不存在,则404
        else
        {
            header_resonse.status = 404;
            strcpy(header_resonse.status_desc, "file not found");
            strcpy(request_file, request_file_404);
        }

        // 不是200状态的，之前打开文件失败，现在打开错误页面文件
        if (header_resonse.status != 200)
        {
            if ((fd_request_file = fopen(request_file, "rb")) == NULL)
            {
                fprintf(stderr, "open file %s error!\n", request_file);
                exit(EXIT_FAILURE);
            }
        }

        // 获取文件大小
        if (stat(request_file, &statbuf) < 0)
        {
            printf("get file %s stat error\n", request_file);
            exit(EXIT_FAILURE);
        } else
        {
            header_resonse.content_length = statbuf.st_size;
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


        printf("response header:\n%s", response);

        // 发送响应头信息
        write(conn, response, strlen(response));


        if ( (fd_write_file = fdopen(conn, "wb") ) == NULL)
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

}
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
#include <sys/epoll.h>
#include <fcntl.h>

#include "mysignal.h"
#include "fastcgi.h"
#include "tool.h"
#include "process_request.h"
#include "writen_readn_readline.h"


#define SET_RESPONSE_STATUS_200(header_resonse)  header_resonse.status = 200; strcpy(header_resonse.status_desc, "OK")
#define SET_RESPONSE_STATUS_403(header_resonse)  header_resonse.status = 403; strcpy(header_resonse.status_desc, "forbidden")
#define SET_RESPONSE_STATUS_404(header_resonse)  header_resonse.status = 404; strcpy(header_resonse.status_desc, "file not found");
#define SET_RESPONSE_STATUS_405(header_resonse)  header_resonse.status = 405; strcpy(header_resonse.status_desc, "method not allowed");


#define EXTENSION_NAME_LENTH 8
#define MAX_EPOLL_SIZE 2000

// 会话的状态
#define SESSION_READ_HEADER 1
#define SESSION_RESPONSE 2
#define SESSION_RESPONSE_FIN 3
#define SESSION_END 4

// 是否收到数据
#define SESSION_DATA_READ_READY 1
#define SESSION_DATA_WRITE_READY 2
#define SESSION_DATA_HANDLED 3

// 客户端是否断开连接
#define SESSION_RNSHUTDOWN 0
#define SESSION_RSHUTDOWN 1


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

struct DefaultReqFile {
    char request_file_404[PATH_MAX];
    char request_file_403[PATH_MAX];
    char request_file_405[PATH_MAX];
};


struct connConf {
    int connMaxTransactions;
};

struct connInfo {
    int connTransactions;
    int connFd;
    int localFileFd;
    char * recv_buf;
    unsigned int sessionStatus;
    unsigned int sessionRShutdown;
    unsigned int sessionRcvData;
};




struct runParams {
    struct connInfo * conninfo;
    const struct DefaultReqFile * default_request_file;
    char * file_index[2];
    char * method_allow[3];
    unsigned int method_allow_len;
    struct mimedict * mimebook;
    unsigned int mimebook_len;
    char * doc_root;
    struct sockaddr_in * client_sockaddr;
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
        exit(EXIT_FAILURE);
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

void init_session(struct connInfo * connSessionInfo);

void init_session(struct connInfo * connSessionInfo)
{
        connSessionInfo->recv_buf = NULL;
        connSessionInfo->localFileFd = -2;
        connSessionInfo->sessionStatus = SESSION_READ_HEADER;
        connSessionInfo->sessionRShutdown = SESSION_RNSHUTDOWN;
        connSessionInfo->sessionRcvData = SESSION_DATA_HANDLED;
}



void process_request_get_header(struct runParams *run_params, char *header_buf, ssize_t buffer_size);
void process_request_response_header(struct runParams *run_params, char * header_buf, ssize_t buffer_size);
void process_request_response_data(struct runParams *run_params);


void process_request_get_header(struct runParams *run_params, char *header_buf, ssize_t buffer_size)
{
    bool header_rcv = false;
    int index;
    ssize_t len;
    char read_buffer[buffer_size];

    memset(header_buf, 0, (size_t) buffer_size);
    memset(read_buffer, 0, sizeof(read_buffer));

    printf("now read data\n");
    printf("connfd: %d\n", run_params->conninfo->connFd);
    if ( (len = read(run_params->conninfo->connFd, read_buffer, buffer_size - (size_t)1) ) == 0)
    {
        printf("recv len 1: %ld\n", len);
        free(run_params->conninfo->recv_buf);
        run_params->conninfo->recv_buf = NULL;
        run_params->conninfo->sessionRShutdown = SESSION_RSHUTDOWN;
        // close(connSessionInfo->connFd);

        if (run_params->conninfo->sessionStatus == SESSION_READ_HEADER)
        {
            printf("header recv error\n");
            close(run_params->conninfo->connFd);
        }

        return;
    }
    else if (len < 0 && errno == EINTR)
    {
        printf("recv len 2: %ld", len);
        printf("was interuted, ignore\n");
    }
    else if (len < 0 && errno == ECONNRESET)
    {
        // 可能要关闭事件
        printf("recv len 3: %ld", len);
        free(run_params->conninfo->recv_buf);
        free(run_params->conninfo);
        // init_session(run_params->conninfo);

        close(run_params->conninfo->connFd);
        return;
    }
    else if (len < 0)
    {
        printf("recv len 4: %ld\n", len);
        fprintf(stderr, "str_echo: read error\n");
        free(run_params->conninfo->recv_buf);
        run_params->conninfo->recv_buf = NULL;
        run_params->conninfo->sessionRShutdown = SESSION_RSHUTDOWN;

        if (run_params->conninfo->sessionStatus == SESSION_READ_HEADER)
        {
            printf("header recv error\n");
            close(run_params->conninfo->connFd);
        }

        return;
        // exit(EXIT_FAILURE);
    }
    else if (len > 0)
    {
        printf("recv len 5: %ld", len);
        read_buffer[len] = '\0';
        // printf("read_buffer:\n%s\n", read_buffer);
        strncat(run_params->conninfo->recv_buf, read_buffer, buffer_size);
        // printf("recv_buf_index[%d]:\n%s\n", connSessionInfo->connFd, connSessionInfo->recv_buf);

        for (index = 0; index < strlen(run_params->conninfo->recv_buf); index++)
        {
            if (run_params->conninfo->recv_buf[index] == '\r' && run_params->conninfo->recv_buf[index+1] == '\n' &&
                run_params->conninfo->recv_buf[index+2] == '\r' && run_params->conninfo->recv_buf[index+3] == '\n')
            {
                strncpy(header_buf, run_params->conninfo->recv_buf, index + 3);
                // printf("index: %d header info:\n%s\n", index, header_buf);
                // 清空接受的数据，粗暴的丢弃后面接收的数据
                memset(run_params->conninfo->recv_buf, 0, sizeof(run_params->conninfo->recv_buf));
                header_rcv = true;
                break;
            }

        }

        if (! header_rcv)
            // 还没有读到头文件结束，下次继续读取
            return;
        // 剩余可能还会有数据，暂不处理
    }

    printf("have received header\n");

    // 打印调试信息
    printf("received:\n");
    printf("%s", header_buf);
    // 读取头完成
    run_params->conninfo->sessionStatus = SESSION_RESPONSE;
}


void process_request_response_header(struct runParams *run_params, char * header_buf, ssize_t buffer_size)
{

    char cli_addr_buff[INET_ADDRSTRLEN];
    bool is_fastcgi = false;
    struct sockaddr_in cli_sockaddr, srv_sockaddr;
    int cli_sockaddr_len, srv_sockaddr_len;
    char addr_buf[INET_ADDRSTRLEN];
    int res_io;

    struct response_header header_resonse = {
            .http_version = "HTTP/1.1",
            .content_type = "image/jpeg",
            .content_length = 16,
            .connection = "keep-alive",
            .server = "Dagama",
            .status = 200,
            .status_desc = "OK"
    };



    if (run_params->conninfo->localFileFd == -2)
    {
        struct request_header header_request;

        char response[4096];
        char ch_temp[1024];

        parse_header_request(header_buf, &header_request);

        printf("request_uri: %s\n", header_request.uri);

        struct stat statbuf;
        char request_file[512];
        int index_temp;
        bool flag_temp;


        flag_temp = false;
        for (index_temp = 0; index_temp < 3; index_temp++)
        {
            printf("method: %s   %s\n", run_params->method_allow[index_temp], header_request.method);


            if (strcmp(run_params->method_allow[index_temp], header_request.method) == 0)
                flag_temp = true;
        }

        if (str_endwith(header_request.uri, ".php"))
        {
            sprintf(request_file, "%s/%s", run_params->doc_root, header_request.uri);
            is_fastcgi = true;
        }
        else if (!flag_temp)
        {
            SET_RESPONSE_STATUS_405(header_resonse);
            strcpy(request_file, run_params->default_request_file->request_file_405);
        }
        else if (strcmp(header_request.method, "GET") == 0)
        {
            // 访问的uri是目录的，重写到该目录下的index文件
            if (header_request.uri[strlen(header_request.uri) - 1] == '/')
            {
                sprintf(request_file, "%s/%s%s", run_params->doc_root, header_request.uri, run_params->file_index[0]);
                if (access(request_file, F_OK) == -1)
                {
                    sprintf(request_file, "%s/%s%s", run_params->doc_root, header_request.uri, run_params->file_index[1]);
                    if (access(request_file, F_OK) == -1)
                    {
                        SET_RESPONSE_STATUS_403(header_resonse);
                        strcpy(request_file, run_params->default_request_file->request_file_403);
                    }
                    else
                    SET_RESPONSE_STATUS_200(header_resonse);
                }
                else
                SET_RESPONSE_STATUS_200(header_resonse);
            }
            else
            {
                sprintf(request_file, "%s/%s", run_params->doc_root, header_request.uri);

                printf("request_file: %s\n", request_file);
                // 根据文件是否存在，重新拼接请求文件，生成状态码
                // 文件存在
                if (access(request_file, F_OK) != -1)
                {
                    // 获取文件信息，如果失败则403
                    if (stat(request_file, &statbuf) != -1)
                    {
                        // 如果为普通文件
                        if (S_ISREG(statbuf.st_mode)) {
                            SET_RESPONSE_STATUS_200(header_resonse);
                        } else {
                            SET_RESPONSE_STATUS_404(header_resonse);
                            strcpy(request_file, run_params->default_request_file->request_file_404);
                        }
                    }
                    else
                    {
                        printf("get file %s stat error\n", request_file);
                        SET_RESPONSE_STATUS_403(header_resonse);
                        strcpy(request_file, run_params->default_request_file->request_file_403);
                    }

                }
                    // 文件不存在,则404
                else
                {
                    SET_RESPONSE_STATUS_404(header_resonse);
                    strcpy(request_file, run_params->default_request_file->request_file_404);
                }

                printf("request_file: %s\n", request_file);

            }


        }
        else if (strcmp(header_request.method, "POST") == 0)
        {
            printf("POST is not finished yet!\n");
        }


        if (is_fastcgi)
        {
            process_request_fastcgi(run_params->conninfo->connFd, request_file);
        }
        else
        {
            if (stat(request_file, &statbuf) != -1)
                header_resonse.content_length = statbuf.st_size;
            else
            {
                printf("get statbuf error!\n");
                // continue;
            }



            // if ( (fd_request_file = fopen(request_file, "rb")) == NULL)
            if ((run_params->conninfo->localFileFd = open(request_file, O_RDONLY)) < 0)
            {
                fprintf(stderr, "open file %s error!\n", request_file);
                // exit(EXIT_FAILURE);
                SET_RESPONSE_STATUS_403(header_resonse);
                strcpy(request_file, run_params->default_request_file->request_file_403);
                // if ( (fd_request_file = fopen(request_file, "rb")) == NULL)
                if ((run_params->conninfo->localFileFd = open(request_file, O_RDONLY)) < 0)
                {
                    printf("open file error!\n");
                    // continue;
                }

            }


            get_contenttype_by_filepath(request_file, run_params->mimebook, run_params->mimebook_len, &header_resonse);

            printf("begine send\n");

            // 拼接响应头
            sprintf(response, "%s %d %s\n", header_resonse.http_version, header_resonse.status,
                    header_resonse.status_desc);
            sprintf(ch_temp, "Content-Type: %s\n", header_resonse.content_type);
            strcat(response, ch_temp);
            sprintf(ch_temp, "Content-Length: %llu\n", header_resonse.content_length);
            strcat(response, ch_temp);
            sprintf(ch_temp, "Connection: %s\n", header_resonse.connection);
            strcat(response, ch_temp);
            sprintf(ch_temp, "Server: %s\n\n", header_resonse.server);
            strcat(response, ch_temp);


            srv_sockaddr_len = sizeof(srv_sockaddr);
            if (getsockname(run_params->conninfo->connFd, (struct sockaddr *) &srv_sockaddr, &srv_sockaddr_len) == -1)
                printf("getsockname() error!\n");
            else
            {
                printf("local ip: %s", inet_ntop(AF_INET, &srv_sockaddr.sin_addr, addr_buf, INET_ADDRSTRLEN));
                printf(" local port: %d\n", ntohs(srv_sockaddr.sin_port));
            }

            cli_sockaddr_len = sizeof(cli_sockaddr);
            if (getpeername(run_params->conninfo->connFd, (struct sockaddr *) &cli_sockaddr, &cli_sockaddr_len) == -1)
                printf("getpeername() error!\n");
            else
            {
                printf("peer ip: %s", inet_ntop(AF_INET, &cli_sockaddr.sin_addr, addr_buf, INET_ADDRSTRLEN));
                printf(" peer port: %d\n", ntohs(cli_sockaddr.sin_port));
            }

            printf("response header:\n%s", response);

            printf("Connection from client: %s:%d\n",
                   inet_ntop(AF_INET, &run_params->client_sockaddr->sin_addr, cli_addr_buff, INET_ADDRSTRLEN),
                   ntohs(run_params->client_sockaddr->sin_port));


            // 发送响应头信息
            if ((res_io = writen(run_params->conninfo->connFd, response, strlen(response))) == -1)
            {
                printf("writen header failed, continue\n");
                close(run_params->conninfo->connFd);
                return;
            }
            else
            {
                printf("writen header res: %d\n", res_io);
            }

        }
    }
    else if (run_params->conninfo->localFileFd > 0)
    {
        printf("have sent headers, continue\n");
    }
    else
    {
        printf("known error, now return\n");
        return;
    }

}


void process_request_response_data(struct runParams *run_params)
{
    ssize_t buffer_size = 4096, read_buffer_size;
    char send_buffer[buffer_size];
    int res_io;


    if ( (read_buffer_size = read(run_params->conninfo->localFileFd, send_buffer, buffer_size - (ssize_t)1) ) > 0)
    {
        if ( (res_io = writen(run_params->conninfo->connFd, send_buffer, read_buffer_size) )  == -1)
            // if ( (res_io = write(connSessionInfo->connFd, send_buffer, read_buffer_size) ) < 0)
        {
            printf("writen body failed, continue\n");
            close(run_params->conninfo->connFd);
            close(run_params->conninfo->localFileFd);
            return;
        }
        else if (res_io == read_buffer_size)
        {
            printf("writen body full, res: %d\n", res_io);
        }
        else
        {
            printf("writen body not full, res: %d read_buffer_size: %d\n", res_io, read_buffer_size);
        }
    }
    else if (read_buffer_size == 0)
    {
        printf("finish response session\n");
        (run_params->conninfo->connTransactions)++;
        printf("session count: %d\n", run_params->conninfo->connTransactions);
        if (run_params->conninfo->sessionRShutdown == SESSION_RSHUTDOWN)
        {
            // 读取输入时，已经处理recv_buf
            close(run_params->conninfo->connFd);
            free(run_params->conninfo->recv_buf);
            free(run_params->conninfo);

        }
        else
        {
//                printf("finish session, now keep alive\n");
//                free(connSessionInfo->recv_buf);
//                init_session(connSessionInfo);

            close(run_params->conninfo->connFd);
            free(run_params->conninfo->recv_buf);
            free(run_params->conninfo);
            // init_session(run_params->conninfo);



//                printf("finish session, now keep alive\n");
//                connSessionInfo->sessionStatus = SESSION_END;


        }
        return;
    }

}






void process_request(struct runParams * run_params);


void process_request(struct runParams * run_params)
{
    ssize_t buffer_size = 4096;
    char header_buf[buffer_size];


    printf("session status: %d localfile_fd: %d\n", run_params->conninfo->sessionStatus, run_params->conninfo->localFileFd);

    if (run_params->conninfo->sessionStatus == SESSION_READ_HEADER && run_params->conninfo->sessionRcvData == SESSION_DATA_READ_READY)
    {
        process_request_get_header(run_params, header_buf, buffer_size);
    }
    else
    {
        process_request_response_header(run_params, header_buf, buffer_size);
    }


    if (run_params->conninfo->sessionRcvData == SESSION_DATA_WRITE_READY && run_params->conninfo->sessionStatus == SESSION_RESPONSE)
    {
        process_request_response_data(run_params);
    }


}








int main() {
    int listen_fd, connfd, tmpConnFd;
    struct sockaddr_in server_sockaddr, client_sockaddr;
    const int myqueue = 100;
    int cli_len;
    int index, index2;

    // epoll
    struct epoll_event event;
    struct epoll_event * events;
    uint32_t tmpEvent;
    int epoll_fd, ep_fd_ready_count, ep_fd_index;
    const int mimebook_len = 103;

    struct mimedict mimebook[mimebook_len];


    struct connConf connConfLimit;

    struct connInfo * connSessionInfos;



    // 运行的配置参数
    char * doc_root = "/home/liushan/mylab/clang/dagama/webroot";
    // char * doc_root = "/opt/application/nginx/myphp";
    char * file_index[2] = {"index.html", "index.htm"};
    char * method_allow[3] = {"GET", "DELETE", "POST"};
    char mime_file[PATH_MAX] = "/home/liushan/mylab/clang/dagama/mime.types";
    struct DefaultReqFile request_file_default = {
            .request_file_404 = "/home/liushan/mylab/clang/dagama/webroot/html/404.html",
            .request_file_403 = "/home/liushan/mylab/clang/dagama/webroot/html/403.html",
            .request_file_405 = "/home/liushan/mylab/clang/dagama/webroot/html/405.html"
    };


    struct runParams run_param[MAX_EPOLL_SIZE];

    get_mimebook(mime_file, mimebook, mimebook_len);


    for (index = 0; index < MAX_EPOLL_SIZE; index++)
    {
        run_param[index].default_request_file = &request_file_default;
        for (index2 = 0; index2 < sizeof(file_index) / sizeof(char *); index2++)
            run_param[index].file_index[index2] = file_index[index2];

        for (index2 = 0; index2 < sizeof(method_allow) / sizeof(char *); index2++)
            run_param[index].method_allow[index2] = method_allow[index2];
        run_param->method_allow_len = sizeof(method_allow) / sizeof(char *);
        run_param[index].mimebook = mimebook;
        run_param[index].mimebook_len = mimebook_len;
        run_param[index].doc_root = doc_root;
        run_param[index].client_sockaddr = &client_sockaddr;
    }



    bzero(&server_sockaddr, sizeof(server_sockaddr));
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port =  htons(8080);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1)
    {
        printf("bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, myqueue) == -1)
    {
        printf("listen error\n");
        exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create(MAX_EPOLL_SIZE);
    if (epoll_fd == -1)
    {
        printf("epoll_create error\n");
        exit(EXIT_FAILURE);
    }
    event.data.fd = listen_fd;
    event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1)
    {
        printf("epoll_ctl_add error fd: %d", listen_fd);
        exit(EXIT_FAILURE);
    }

    events = malloc(MAX_EPOLL_SIZE * sizeof(event) );


    while(1)
    {
        cli_len = sizeof(client_sockaddr);
        // printf("wait event\n");
        ep_fd_ready_count = epoll_wait(epoll_fd, events, MAX_EPOLL_SIZE, -1);
        for (ep_fd_index = 0; ep_fd_index < ep_fd_ready_count; ep_fd_index++)
        {
            tmpEvent = events[ep_fd_index].events;
            tmpConnFd = events[ep_fd_index].data.fd;


            if (tmpConnFd == listen_fd)
            {
                if (tmpEvent & EPOLLIN)
                {
                    printf("EVENT: ready accept: %d\n", tmpConnFd);
                    // 收到连接请求
                    if ( (connfd = accept(listen_fd, (struct sockaddr *) &client_sockaddr, &cli_len)) < 0)
                    {
                        // 重启被中断的系统调用accept
                        if (errno == EINTR)
                            continue;
                            // accept返回前连接终止, SVR4实现
                        else if (errno == EPROTO)
                            continue;
                            // accept返回前连接终止, POSIX实现
                        else if (errno == ECONNABORTED)
                            continue;
                        else
                        {
                            printf("accept error!\n");
                            exit(EXIT_FAILURE);
                        }

                    }


                    memset(&event, 0, sizeof(event));

                    event.data.fd = connfd;
                    event.events = EPOLLIN | EPOLLOUT;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &event) == -1)
                    {
                        printf("epoll_ctl connfd add error fd: %d\n", connfd);
                        exit(EXIT_FAILURE);
                    }

                    connSessionInfos = malloc(sizeof(struct connInfo));
                    init_session(connSessionInfos);

                    connSessionInfos->recv_buf = malloc(sizeof(char) * MAX_EPOLL_SIZE);
                    memset(connSessionInfos->recv_buf, 0, sizeof(*connSessionInfos->recv_buf) );
                    printf("receive conn:%d\n", connfd);

                    connSessionInfos->connFd = connfd;
                    connSessionInfos->connTransactions = 0;
                    run_param[connfd].conninfo = connSessionInfos;
                }
            }
            else if (tmpConnFd >= 4)
            {
                if (tmpEvent & EPOLLIN)
                {
                    if (run_param[tmpConnFd].conninfo->sessionStatus != SESSION_END)
                    {
                        run_param[tmpConnFd].conninfo->sessionRcvData = SESSION_DATA_READ_READY;
                        printf("EVENT: ready read: %d\n", tmpConnFd);
                        process_request(&run_param[tmpConnFd]);
                    }
                }

                if (tmpEvent & EPOLLOUT)
                {
                    if (run_param[tmpConnFd].conninfo->sessionStatus == SESSION_RESPONSE)
                    {
                        run_param[tmpConnFd].conninfo->sessionRcvData = SESSION_DATA_WRITE_READY;
                        printf("EVENT: ready write: %d\n", tmpConnFd);
                        process_request(&run_param[tmpConnFd]);
                    }
                }
            }


        }


    }

}

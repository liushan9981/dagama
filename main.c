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

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "mysignal.h"
#include "fastcgi.h"
#include "mystring.h"
#include "process_request.h"
#include "writen_readn_readline.h"

#include "main.h"
#include "handle_config.h"
#include "myutils.h"


void get_value_by_header(const char * header, const char * header_key, char * header_value)
{
    int blank_num = 0;
    int index = 0;

    for (index = strlen(header_key) + 1, blank_num = 0; index < strlen(header); index++, blank_num++)
        if (! isblank(header[index]))
            break;
    memcpy(header_value, header + strlen(header_key) + 1 + blank_num, strlen(header) - (strlen(header_key) + 1) );

    // 最后一个特殊字符'\r'改为'\0'
    if (header_value[strlen(header_value) - 1] == '\r')
        header_value[strlen(header_value) - 1] = '\0';
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


void parse_header_request(char * headers_recv, struct request_header * headers_request);

void parse_header_request(char * headers_recv, struct request_header * headers_request)
{
    char line_char_s[4][MAX_STR_SPLIT_SIZE];
    char line_read[1024];
    int index, temp_index, char_s_count, header_index, header_value_index;
    bool flag = true;

    for (index = 0, temp_index = 0, header_index = 0; index < strlen(headers_recv); index++)
    {
        if (headers_recv[index] == '\n')
        {
            line_read[temp_index] = '\0';

            if (flag)
            {
                char_s_count = str_split(line_read, ' ', line_char_s, 4);
                if (char_s_count == 3)
                {
                    strcpy(headers_request->method, line_char_s[0]);
                    strcpy(headers_request->uri, line_char_s[1]);
                    strcpy(headers_request->http_version, line_char_s[2]);
                    flag = false;
                }
            }
            else
            {
                char_s_count = str_split(line_read, ':', line_char_s, 4);
                if (char_s_count == 2)
                {
                    strcpy(headers_request->headers[header_index][0], line_char_s[0]);
                    strcpy(headers_request->headers[header_index][1], line_char_s[1]);

                    printf("## recv headers: %s#%s\n",
                           headers_request->headers[header_index][0], headers_request->headers[header_index][1]);

                    if (strcmp(headers_request->headers[header_index][0], "Host") == 0)
                        strcpy(headers_request->host, headers_request->headers[header_index][1]);

                    header_index++;
                }
                else if (char_s_count > 2)
                {
                    strcpy(headers_request->headers[header_index][0], line_char_s[0]);
                    get_value_by_header(line_read, headers_request->headers[header_index][0],
                            headers_request->headers[header_index][1]);

                    printf("## recv headers: %s#%s\n",
                           headers_request->headers[header_index][0], headers_request->headers[header_index][1]);

                    if (strcmp(headers_request->headers[header_index][0], "Host") == 0)
                        strcpy(headers_request->host, headers_request->headers[header_index][1]);

                    header_index++;
                }
                else
                {
                    fprintf(stderr, "recv error header: %s\n", line_read);
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



void process_request_get_header(struct RunParams *run_params, char *header_buf, ssize_t buffer_size);
void process_request_response_header(struct RunParams *run_params, char * header_buf, ssize_t buffer_size);
void process_request_response_data(struct RunParams *run_params);


void process_request_get_header(struct RunParams *run_params, char *header_buf, ssize_t buffer_size)
{
    bool header_rcv = false;
    int index;
    ssize_t len;
    char read_buffer[buffer_size];

    memset(header_buf, 0, (size_t) buffer_size);
    memset(read_buffer, 0, sizeof(read_buffer));

    printf("now read data\n");
    printf("connfd: %d\n", run_params->conninfo->connFd);
    if ( (run_params->conninfo->is_https) )
        len = SSL_read(run_params->conninfo->ssl, read_buffer, buffer_size - (size_t)1);
    else
        len = read(run_params->conninfo->connFd, read_buffer, buffer_size - (size_t)1);


    if ( len == 0)
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



void process_request_response_header(struct RunParams *run_params, char * header_buf, ssize_t buffer_size)
{

    char cli_addr_buff[INET_ADDRSTRLEN];
    bool is_fastcgi = false;
    struct sockaddr_in cli_sockaddr, srv_sockaddr;
    int cli_sockaddr_len, srv_sockaddr_len;
    char addr_buf[INET_ADDRSTRLEN];
    int res_io;

    int host_index;

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
        struct stat statbuf;
        char request_file[512];
        int index_temp;
        bool flag_temp;

        parse_header_request(header_buf, &header_request);

        for (host_index = 0; host_index < 2; host_index++)
        {
            printf("debug hostname:\n#%s#\n#%s#\n", header_request.host, run_params->hostvar[host_index].host);
            if (strcmp(header_request.host, run_params->hostvar[host_index].host) == 0)
                run_params->hostvar += host_index;
        }


        printf("debug2: docroot:%s\n", run_params->hostvar->doc_root);
        flag_temp = false;
        for (index_temp = 0; index_temp < run_params->hostvar->method_allowed_len; index_temp++)
            if (strcmp(run_params->hostvar->method_allowed[index_temp], header_request.method) == 0)
            {
                flag_temp = true;
                break;
            }

        if (str_endwith(header_request.uri, ".php"))
        {
            sprintf(request_file, "%s/%s", run_params->hostvar->doc_root, header_request.uri);
            is_fastcgi = true;
        }
        else if (!flag_temp)
        {
            SET_RESPONSE_STATUS_405(header_resonse);
            strcpy(request_file, run_params->hostvar->request_file_405);
        }
        else if (strcmp(header_request.method, "GET") == 0)
        {
            // 访问的uri是目录的，重写到该目录下的index文件
            if (header_request.uri[strlen(header_request.uri) - 1] == '/')
            {
                sprintf(request_file, "%s/%s%s", run_params->hostvar->doc_root, header_request.uri, run_params->hostvar->file_indexs[0]);
                if (access(request_file, F_OK) == -1)
                {
                    sprintf(request_file, "%s/%s%s", run_params->hostvar->doc_root, header_request.uri, run_params->hostvar->file_indexs[1]);
                    if (access(request_file, F_OK) == -1)
                    {
                        SET_RESPONSE_STATUS_403(header_resonse);
                        strcpy(request_file, run_params->hostvar->request_file_403);
                    }
                    else
                    SET_RESPONSE_STATUS_200(header_resonse);
                }
                else
                SET_RESPONSE_STATUS_200(header_resonse);
            }
            else
            {
                sprintf(request_file, "%s/%s", run_params->hostvar->doc_root, header_request.uri);

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
                            strcpy(request_file, run_params->hostvar->request_file_404);
                        }
                    }
                    else
                    {
                        printf("get file %s stat error\n", request_file);
                        SET_RESPONSE_STATUS_403(header_resonse);
                        strcpy(request_file, run_params->hostvar->request_file_403);
                    }

                }
                    // 文件不存在,则404
                else
                {
                    SET_RESPONSE_STATUS_404(header_resonse);
                    strcpy(request_file, run_params->hostvar->request_file_404);
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
            printf("request_file: %s\n", request_file);


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
                strcpy(request_file, run_params->hostvar->request_file_403);
                // if ( (fd_request_file = fopen(request_file, "rb")) == NULL)
                if ((run_params->conninfo->localFileFd = open(request_file, O_RDONLY)) < 0)
                {
                    printf("open file error!\n");
                    // continue;
                }

            }


            get_contenttype_by_filepath(request_file, run_params->hostvar->mimebook, MAX_MIMEBOOK_SIZE, &header_resonse);

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
            if (run_params->conninfo->is_https)
                res_io = SSL_write(run_params->conninfo->ssl, response, strlen(response));
            else
                res_io = writen(run_params->conninfo->connFd, response, strlen(response));


            if (res_io < 0)
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


void process_request_response_data(struct RunParams *run_params)
{
    ssize_t buffer_size = 4096, read_buffer_size;
    char send_buffer[buffer_size];
    int res_io;



    if ( (read_buffer_size = read(run_params->conninfo->localFileFd, send_buffer, buffer_size - (ssize_t)1) ) > 0)
    {
        if (run_params->conninfo->is_https)
            res_io = SSL_write(run_params->conninfo->ssl, send_buffer, read_buffer_size);
        else
            res_io = writen(run_params->conninfo->connFd, send_buffer, read_buffer_size);


        if (res_io  == -1)
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






void process_request(struct RunParams * run_params);


void process_request(struct RunParams * run_params)
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




int create_listen_sock(int port, struct sockaddr_in * server_sockaddr);

int create_listen_sock(int port, struct sockaddr_in * server_sockaddr)
{
    int listen_fd;
    // struct sockaddr_in server_sockaddr;
    const int myqueue = 100;

    bzero(server_sockaddr, sizeof(*server_sockaddr));
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    printf("listen_fd: %d\n", listen_fd);

    server_sockaddr->sin_family = AF_INET;
    server_sockaddr->sin_port =  htons(port);
    server_sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *)server_sockaddr, sizeof(*server_sockaddr)) == -1)
    {
        printf("bind error errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, myqueue) == -1)
    {
        printf("listen error\n");
        exit(EXIT_FAILURE);
    }

    return listen_fd;
}



void init_openssl();
SSL_CTX * create_context_ssl(void);
void configure_context_ssl(SSL_CTX * ctx);


void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}


SSL_CTX * create_context_ssl(void)
{
    const SSL_METHOD *method;
    SSL_CTX * ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        printf("create context failed\n");
        exit(EXIT_FAILURE);
    }

    return ctx;
}


void configure_context_ssl(SSL_CTX * ctx)
{
    char * crt = "/home/liushan/mylab/clang/cert/kubelet_node-4.crt";
    char * key = "/home/liushan/mylab/clang/cert/kubelet_node-4.key";

    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_use_certificate_file(ctx, crt, SSL_FILETYPE_PEM) <= 0)
    {
        printf("use cert file error\n");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0)
    {
        printf("use cert file error\n");
        exit(EXIT_FAILURE);
    }

}


void get_app_cwdir(int argc, char ** argv, char * app_cwdir)
{
    char temp_key_value[128][MAX_STR_SPLIT_SIZE];
    char dest_path[PATH_MAX], cw_dir[PATH_MAX];
    char * join_str = "/";
    int str_split_num;

    // 获取程序的绝对路径
    if (str_startwith(argv[0], join_str) )
        strncpy(dest_path, argv[0], strlen(argv[0]) );
    else
    {
        if (getcwd(cw_dir, PATH_MAX) == NULL)
            err_exit("getcwd() error\n");
        strncpy(dest_path, cw_dir, MAX_STR_SPLIT_SIZE);
        strncat(dest_path, join_str, strlen(join_str) );
        strncat(dest_path, argv[0], strlen(argv[0]) );
    }

    // 去掉末尾的文件名
    str_split_num = str_split(dest_path, '/', temp_key_value, 128);
    str_join(join_str, strlen(join_str), temp_key_value, str_split_num - 1, dest_path, MAX_STR_SPLIT_SIZE);

    strncpy(app_cwdir, join_str, MAX_STR_SPLIT_SIZE);
    strncat(app_cwdir, dest_path, strlen(dest_path) );
}



void get_config_file_path(int argc, char ** argv, char * config_file_path);

void get_config_file_path(int argc, char ** argv, char * config_file_path)
{
    char dest_path2[PATH_MAX];
    char * join_str = "/";
    char app_cwdir[PATH_MAX];

    // 后面跟参数的，认为是文件路径，覆盖默认的地址
    if (argc > 1)
        strncpy(config_file_path, argv[1], PATH_MAX);

    // 如果路径是绝对路径，直接返回
    if (str_startwith(config_file_path, join_str) )
        return;

    get_app_cwdir(argc, argv, app_cwdir);

    strncpy(dest_path2, app_cwdir, PATH_MAX);
    strncat(dest_path2, join_str, strlen(join_str) );
    strncat(dest_path2, config_file_path, strlen(config_file_path) );

    strncpy(config_file_path, dest_path2, PATH_MAX);
}


void test_module(int argc, char ** argv);

void test_module(int argc, char ** argv)
{


    exit(EXIT_SUCCESS);
}


int main(int argc, char ** argv) {

    // test_module(argc, argv);



    int http_listen_fd, https_listen_fd, connfd, tmpConnFd, cli_len;
    struct sockaddr_in client_sockaddr, http_server_sockaddr, https_server_sockaddr;
    int index, index2;

    SSL_CTX * ctx;

    // 设置ssl
    init_openssl();
    ctx = create_context_ssl();
    configure_context_ssl(ctx);

    // epoll
    struct epoll_event event;
    struct epoll_event * events;
    uint32_t tmpEvent;
    int epoll_fd, ep_fd_ready_count, ep_fd_index;

    struct connConf connConfLimit;
    struct connInfo * connSessionInfos;
    struct RunParams run_param[MAX_EPOLL_SIZE];

    struct hostVar * host_var_ptr;
    char config_file_path[PATH_MAX] = "../conf/dagama.conf";
    int host_num_count;

    get_config_file_path(argc, argv, config_file_path);
    printf("config_file_path: %s\n", config_file_path);
    host_num_count = get_config_host_num(config_file_path);

    host_var_ptr = malloc(sizeof(struct hostVar) * host_num_count);

    init_config(host_var_ptr, config_file_path);

    http_listen_fd = create_listen_sock(8080, &http_server_sockaddr);
    https_listen_fd = create_listen_sock(8043, &https_server_sockaddr);

    epoll_fd = epoll_create(MAX_EPOLL_SIZE);
    if (epoll_fd == -1)
    {
        printf("epoll_create error\n");
        exit(EXIT_FAILURE);
    }

    // https_listen http_listen 加入event
    event.data.fd = http_listen_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, http_listen_fd, &event) == -1)
    {
        printf("epoll_ctl_add error fd: %d", http_listen_fd);
        exit(EXIT_FAILURE);
    }

    event.data.fd = https_listen_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, https_listen_fd, &event) == -1)
    {
        printf("epoll_ctl_add error fd: %d", https_listen_fd);
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


            if (tmpConnFd == http_listen_fd)
            {
                if (tmpEvent & EPOLLIN)
                {
                    printf("EVENT: http ready accept: %d\n", tmpConnFd);
                    // 收到连接请求
                    if ( (connfd = accept(http_listen_fd, (struct sockaddr *) &client_sockaddr, &cli_len)) < 0)
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

                    run_param[connfd].conninfo->is_https = false;
                    run_param[connfd].conninfo->https_ssl_have_conned = false;
                    run_param[connfd].hostvar = host_var_ptr;
                    run_param[connfd].client_sockaddr = &client_sockaddr;
                }
            }
            else if (tmpConnFd == https_listen_fd)
            {
                if (tmpEvent & EPOLLIN)
                {
                    printf("EVENT: https ready accept: %d\n", tmpConnFd);
                    // 收到连接请求
                    if ( (connfd = accept(https_listen_fd, (struct sockaddr *) &client_sockaddr, &cli_len)) < 0)
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

                    run_param[connfd].conninfo->is_https = true;
                    run_param[connfd].conninfo->https_ssl_have_conned = false;
                    run_param[connfd].hostvar = host_var_ptr;
                    run_param[connfd].client_sockaddr = &client_sockaddr;

                    printf("run_param[connfd].hostvar: %s %s\n",
                            run_param[connfd].hostvar->host, run_param[connfd].hostvar->doc_root);


                    SSL * ssl;
                    ssl = SSL_new(ctx);
                    SSL_set_fd(ssl, connfd);
                }
            }
            else if (tmpConnFd >= 5)
            {
                // 有数据到达，可以读
                if (tmpEvent & EPOLLIN)
                {
                    // https访问ssl未建立连接
                    if (run_param[tmpConnFd].conninfo->is_https &&
                    (! run_param[tmpConnFd].conninfo->https_ssl_have_conned)
                    )
                    {
                        SSL * ssl;
                        ssl = SSL_new(ctx);
                        SSL_set_fd(ssl, tmpConnFd);



                        if (SSL_accept(ssl) <= 0)
                        {
                            printf("#2 ssl accept error errno: %d\n", errno);
                            if (run_param[tmpConnFd].conninfo->https_ssl_have_conned)
                            {
                                printf("run_param[tmpConnFd].https_ssl_have_conned): true\n");
                                printf("tmpConnFd: %d\n", tmpConnFd);

                            }
                            else
                            {
                                printf("run_param[tmpConnFd].https_ssl_have_conned): false\n");
                                printf("tmpConnFd: %d\n", tmpConnFd);
                            }

                            close(tmpConnFd);
                        }
                        else
                        {
                            printf("ssl established %d\n", tmpConnFd);
                            run_param[tmpConnFd].conninfo->https_ssl_have_conned = true;
                            run_param[tmpConnFd].conninfo->ssl = ssl;
                        }
                    }
                    else
                    {
                        if (run_param[tmpConnFd].conninfo->sessionStatus != SESSION_END)
                        {
                            run_param[tmpConnFd].conninfo->sessionRcvData = SESSION_DATA_READ_READY;
                            printf("EVENT: ready read: %d\n", tmpConnFd);
                            process_request(&run_param[tmpConnFd]);
                        }
                    }

                }

                // 可以写
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

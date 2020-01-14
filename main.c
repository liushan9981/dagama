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
#include <limits.h>
#include <errno.h>
#include <fcntl.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "mysignal.h"
#include "fastcgi.h"
#include "mystring.h"
#include "process_request_fastcgi.h"
#include "writen_readn_readline.h"
#include "main.h"
#include "handle_config.h"
#include "myutils.h"
#include "request_header.h"
#include "response_data.h"
#include "session.h"



static int http_listen_fd, https_listen_fd;
static struct SessionRunParams session_run_param[MAX_EPOLL_SIZE];
struct ParamsRun run_params;
SSL_CTX * ctx;
static char config_file_path[PATH_MAX] = "../conf/dagama.conf";
static struct sockaddr_in http_server_sockaddr, https_server_sockaddr;
struct sockaddr_in client_sockaddr;


void read_header(struct SessionRunParams *session_params_ptr)
{
    int index;
    ssize_t len;
    const int buffer_size = 4096;
    char read_buffer[buffer_size];

    memset(read_buffer, 0, sizeof(read_buffer));

    if ( (session_params_ptr->session_info->is_https) )
        len = SSL_read(session_params_ptr->session_info->ssl, read_buffer, buffer_size - (size_t)1);
    else
        len = read(session_params_ptr->session_info->connFd, read_buffer, buffer_size - (size_t)1);


    if ( len == 0)
    {
        // 之前已经读完请求头
        if (session_params_ptr->session_info->sessionStatus == SESSION_RESPONSE_HEADER)
        {
            session_params_ptr->session_info->sessionRShutdown = SESSION_RSHUTDOWN;
        }
        else if (session_params_ptr->session_info->sessionStatus == SESSION_READ_HEADER)
        {
            session_close(session_params_ptr);
            printf("have closed session == 0\n");
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
        session_close(session_params_ptr);
        return;
    }
    else if (len < 0)
    {
        if (session_params_ptr->session_info->sessionStatus == SESSION_READ_HEADER)
        {
            printf("header recv error < 0\n");
            // close(run_params->session_info->connFd);
            session_close(session_params_ptr);
        }
        else if (session_params_ptr->session_info->sessionStatus == SESSION_RESPONSE_HEADER)
        {
            printf("recv len 4: %ld\n", len);
            session_params_ptr->session_info->sessionRShutdown = SESSION_RSHUTDOWN;
        }

        return;
        // exit(EXIT_FAILURE);
    }
    else if (len > 0)
    {
        printf("recv len 5: %ld", len);
        read_buffer[len] = '\0';
        strncat(session_params_ptr->session_info->recv_buf, read_buffer, buffer_size);

        for (index = 0; index < strlen(session_params_ptr->session_info->recv_buf); index++)
        {
            if (session_params_ptr->session_info->recv_buf[index] == '\r' && session_params_ptr->session_info->recv_buf[index+1] == '\n' &&
                session_params_ptr->session_info->recv_buf[index+2] == '\r' && session_params_ptr->session_info->recv_buf[index+3] == '\n')
            {
                memcpy(session_params_ptr->session_info->header_buf, session_params_ptr->session_info->recv_buf, index + 4);
                session_params_ptr->session_info->header_buf[index + 4] = '\0';
                // 清空接受的数据，粗暴的丢弃后面接收的数据
                memset(session_params_ptr->session_info->recv_buf, 0, sizeof(char) * MAX_EPOLL_SIZE);
                session_params_ptr->session_info->sessionStatus = SESSION_RESPONSE_HEADER;
                break;
            }

        }
        // 剩余可能还会有数据，暂不处理
    }

}



void get_host_var_by_header(struct SessionRunParams * session_params_ptr,
        const struct request_header * header_request_ptr, struct ParamsRun * run_params_ptr)
{
    int host_index;

    for (host_index = 0; host_index < run_params_ptr->host_count; host_index++)
    {
        printf("debug hostname:\n#%s#\n#%s#\n", header_request_ptr->host, run_params_ptr->hostvar[host_index].host);
        printf("debug cmp: %d\n", strcmp(header_request_ptr->host, run_params_ptr->hostvar[host_index].host) );
        if (strcmp(header_request_ptr->host, run_params_ptr->hostvar[host_index].host) == 0)
        {
            session_params_ptr->hostvar = &(run_params_ptr->hostvar[host_index]);
            return;
        }
    }

    // 默认等于第一个
    session_params_ptr->hostvar = run_params_ptr->hostvar;
}


void get_access_log(struct SessionRunParams * session_params_ptr)
{
    char response_bytes[16];
    char * log_msg_ptr;
    log_msg_ptr = session_params_ptr->conn_info.log_msg;
    char status[4];

    snprintf(response_bytes, 16, "%lld", session_params_ptr->session_info->response_bytes);
    snprintf(status, 4, "%d", session_params_ptr->session_info->hd_response.status);

    if ( (
            strlen(session_params_ptr->session_info->hd_request.user_agent) +
            strlen(session_params_ptr->conn_info.client_ip) +
            strlen(response_bytes) + 2
            ) > 4096
            )
        strncpy(session_params_ptr->conn_info.log_msg, "log too long", 4096);
    else
    {
        strncpy(log_msg_ptr, session_params_ptr->conn_info.client_ip, 4096);
        strcat(log_msg_ptr, LOG_SPLIT_STR);

        strncat(log_msg_ptr, session_params_ptr->session_info->hd_request.uri,
                strlen(session_params_ptr->session_info->hd_request.uri) );
        strcat(log_msg_ptr, LOG_SPLIT_STR);

        strncat(log_msg_ptr, session_params_ptr->session_info->hd_request.user_agent,
                strlen(session_params_ptr->session_info->hd_request.user_agent) );
        strcat(log_msg_ptr, LOG_SPLIT_STR);

        strncat(log_msg_ptr, response_bytes, strlen(response_bytes) );
        strcat(log_msg_ptr, LOG_SPLIT_STR);

        strncat(log_msg_ptr, status, 3);
    }
}


void response_header(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr)
{
    get_response_header(session_params_ptr, run_params_ptr);

    if (session_params_ptr->session_info == NULL)
    {
        printf("func->response_header: session_params_ptr->session_info == NULL, now return\n");
        return;
    }

    session_params_ptr->session_info->send_buffer_size = strlen(session_params_ptr->session_info->header_response);
    session_params_ptr->session_info->send_buffer = session_params_ptr->session_info->header_response;

    write_response_data(session_params_ptr);
    if (session_params_ptr->session_info != NULL)
    {
        session_params_ptr->session_info->sessionStatus = SESSION_RESPONSE_BODY;
        return;
    }

}


void write_response_data(struct SessionRunParams *session_params_ptr)
{
    ssize_t read_buffer_size;
    char * send_buffer;
    int res_io;

    read_buffer_size = session_params_ptr->session_info->send_buffer_size;
    send_buffer = session_params_ptr->session_info->send_buffer;

    if (session_params_ptr->session_info->is_https)
        res_io = SSL_write(session_params_ptr->session_info->ssl, send_buffer, read_buffer_size);
    else
        res_io = writen(session_params_ptr->session_info->connFd, send_buffer, read_buffer_size);

    if (res_io  == -1)
    {
        printf("writen body failed, continue\n");
        session_close(session_params_ptr);
        return;
    }
    else if (res_io == read_buffer_size)
    {
        session_params_ptr->session_info->response_bytes += res_io;
    }
    else
    {
        session_params_ptr->session_info->response_bytes += res_io;
        printf("writen body not full, res: %d read_buffer_size: %ld\n", res_io, read_buffer_size);
    }
}


void response_data(struct SessionRunParams *session_params_ptr)
{
    ssize_t * read_buffer_size;
    char * send_buffer;
    ssize_t buffer_size;
    int local_file_fd;

    read_buffer_size = &(session_params_ptr->session_info->response_data.read_buffer_size);
    buffer_size = session_params_ptr->session_info->response_data.buffer_size;
    send_buffer = session_params_ptr->session_info->response_data.send_buffer;
    local_file_fd = session_params_ptr->session_info->localFileFd;

    if (local_file_fd == -3)
        process_request_response_500(session_params_ptr);
    else if (lseek(local_file_fd, session_params_ptr->session_info->localFdPos, SEEK_SET) != -1)
    {
        if ( (*read_buffer_size = read(local_file_fd, send_buffer, buffer_size - (ssize_t)1) ) > 0)
        {
            session_params_ptr->session_info->send_buffer_size = *read_buffer_size;
            session_params_ptr->session_info->send_buffer = send_buffer;
            session_params_ptr->session_info->localFdPos += (*read_buffer_size);
            write_response_data(session_params_ptr);
        }
        else if (*read_buffer_size == 0)
            process_request_fin_response(session_params_ptr);
    }
    else
    {
        fprintf(stderr, "lseek error\n");
        session_close(session_params_ptr);
    }

}


void process_request(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    printf("session_status: %d\n", session_params_ptr->session_info->sessionRcvData);

    if (session_params_ptr->session_info->sessionRcvData == SESSION_RST)
    {
        printf("recv rst event now close session\n");
        session_close(session_params_ptr);
    }
    else if (session_params_ptr->session_info->sessionRcvData == SESSION_DATA_READ_READY)
    {
        if (session_params_ptr->session_info->sessionStatus == SESSION_READ_HEADER)
        {
            read_header(session_params_ptr);
            if (session_params_ptr->session_info != NULL && session_params_ptr->session_info->sessionStatus == SESSION_RESPONSE_HEADER)
            {
                run_params_ptr->event.data.fd = session_params_ptr->session_info->connFd;
                run_params_ptr->event.events = EPOLLOUT|EPOLLHUP;
                event_update(&(run_params_ptr->event), run_params_ptr->epoll_fd, session_params_ptr->session_info->connFd);
            }
        }
    }
    else if (session_params_ptr->session_info->sessionRcvData == SESSION_DATA_WRITE_READY)
    {
        if (session_params_ptr->session_info->sessionStatus == SESSION_RESPONSE_BODY)
        {
            response_data(session_params_ptr);
            if (session_params_ptr->session_info != NULL && session_params_ptr->session_info->sessionStatus == SESSION_READ_HEADER)
            {
                run_params_ptr->event.data.fd = session_params_ptr->session_info->connFd;
                run_params_ptr->event.events = EPOLLIN;
                event_update(&(run_params_ptr->event), run_params_ptr->epoll_fd, session_params_ptr->session_info->connFd);
            }
        }
        else if (session_params_ptr->session_info->sessionStatus == SESSION_RESPONSE_HEADER)
        {
            response_header(session_params_ptr, run_params_ptr);
            // TODO
            // HEAD方法，只发送响应头后，需要继续读取，以后写剩下语句
        }
    }

}



int create_listen_sock(int port, struct sockaddr_in * server_sockaddr)
{
    int listen_fd;
    const int myqueue = 100;

    bzero(server_sockaddr, sizeof(*server_sockaddr));
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

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


void get_config_file_path(int argc, char ** argv)
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


void event_add(struct epoll_event * event, int epoll_fd, int fd)
{

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, event) == -1)
    {
        printf("epoll_ctl_add error fd: %d", fd);
        exit(EXIT_FAILURE);
    }
}


void event_update(struct epoll_event * event, int epoll_fd, int fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, event) == -1)
    {
        printf("epoll_ctl_mod error fd: %d", fd);
        exit(EXIT_FAILURE);
    }
}


void handle_session(void)
{
    int ep_fd_ready_count, ep_fd_index, connfd, tmpConnFd, cli_len;
    struct epoll_event * events;
    uint32_t tmpEvent;

    events = malloc(MAX_EPOLL_SIZE * sizeof(run_params.event) );
    cli_len = sizeof(client_sockaddr);

    while(1)
    {
        ep_fd_ready_count = epoll_wait(run_params.epoll_fd, events, MAX_EPOLL_SIZE, -1);
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
                    if ( (connfd = accept_session(http_listen_fd, &cli_len) ) < 0)
                        continue;
                    else
                        new_http_session(session_run_param, &run_params, connfd, &client_sockaddr);
                }
            }
            else if (tmpConnFd == https_listen_fd)
            {
                if (tmpEvent & EPOLLIN)
                {
                    printf("EVENT: https ready accept: %d\n", tmpConnFd);
                    // 收到连接请求
                    if ( (connfd = accept_session(https_listen_fd, &cli_len)) < 0)
                        continue;
                    else
                        new_https_session(session_run_param, &run_params, connfd, &client_sockaddr, ctx);
                }
            }
            else if (tmpConnFd >= 5)
            {
                printf("recv data >= 5:\n");
                // 有数据到达，可以读
                if (tmpEvent & EPOLLIN)
                {
                    printf("debug recv EPOLLIN\n");
                    // https访问ssl未建立连接
                    if (session_run_param[tmpConnFd].session_info->is_https &&
                        (! session_run_param[tmpConnFd].session_info->https_ssl_have_conned)
                            )
                    {
                        new_ssl_session(session_run_param, tmpConnFd);
                    }
                    else
                    {
                        if (session_run_param[tmpConnFd].session_info->sessionStatus == SESSION_READ_HEADER)
                        {
                            session_run_param[tmpConnFd].session_info->sessionRcvData = SESSION_DATA_READ_READY;
                            process_request(&(session_run_param[tmpConnFd]), &run_params);
                        }
                    }

                }

                if (tmpEvent & EPOLLHUP)
                {
                    printf("debug recv EPOLLHUP\n");
                    if (session_run_param[tmpConnFd].session_info == NULL)
                        continue;
                    session_run_param[tmpConnFd].session_info->sessionRcvData = SESSION_RST;
                    process_request(&(session_run_param[tmpConnFd]), &run_params);
                }

                // 可以写
                if (tmpEvent & EPOLLOUT)
                {
                    printf("debug recv EPOLLOUT\n");
                    // TODO 连接已经释放，临时处理这种情况
                    if (session_run_param[tmpConnFd].session_info == NULL)
                        continue;
                    if (session_run_param[tmpConnFd].session_info->sessionStatus == SESSION_RESPONSE_HEADER ||
                        session_run_param[tmpConnFd].session_info->sessionStatus == SESSION_RESPONSE_BODY)
                    {
                        session_run_param[tmpConnFd].session_info->sessionRcvData = SESSION_DATA_WRITE_READY;
                        process_request(&(session_run_param[tmpConnFd]), &run_params);
                    }
                }

            }

        }

    }

}




static void sig_pipe(int signo)
{
    printf("recv SIGPIPE\n");
}


void install_signal(void)
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        err_exit("signal(SIGPIPE) error");
    // TODO term信号
}

void init_request_file_open_book(struct RequestFileOpenBook * request_file_open_book_ptr);

void init_request_file_open_book(struct RequestFileOpenBook * request_file_open_book_ptr)
{
    int index;

    for (index = 0; index < MAX_EPOLL_SIZE; index++)
    {
        request_file_open_book_ptr[index].fd = -1;
        request_file_open_book_ptr[index].file_size = -1;
        request_file_open_book_ptr[index].reference_count = 0;
        request_file_open_book_ptr[index].myerrno = 0;
    }


}

void get_run_params(int argc, char ** argv)
{
    get_config_file_path(argc, argv);
    printf("config file path: %s\n", config_file_path);


    run_params.host_count = get_config_host_num(config_file_path);
    run_params.hostvar = malloc(sizeof(struct hostVar) * run_params.host_count);
    init_config(run_params.hostvar, config_file_path);

    init_request_file_open_book(run_params.request_file_open_book);

    int index;

    for (index = 0; index < 3; index++)
    {
        printf("debug %d: %d\n", index, run_params.request_file_open_book[index].fd);
    }

}



void start_listen(void)
{
    // 设置ssl
    init_openssl();
    ctx = create_context_ssl();
    configure_context_ssl(ctx);

    http_listen_fd = create_listen_sock(8080, &http_server_sockaddr);
    https_listen_fd = create_listen_sock(8043, &https_server_sockaddr);

    if ( (run_params.epoll_fd = epoll_create(MAX_EPOLL_SIZE) ) == -1)
        err_exit("epoll_create error\n");
    // https_listen http_listen 加入event
    run_params.event.data.fd = http_listen_fd;
    run_params.event.events = EPOLLIN;
    event_add(&(run_params.event), run_params.epoll_fd, http_listen_fd);

    run_params.event.data.fd = https_listen_fd;
    run_params.event.events = EPOLLIN;
    event_add(&(run_params.event), run_params.epoll_fd, https_listen_fd);
}




void test_module(int argc, char ** argv)
{
    printf("this is %d\n", FOPEN_MAX);

    exit(EXIT_SUCCESS);

}


int main(int argc, char ** argv) {
    // test_module(argc, argv);

    install_signal();
    get_run_params(argc, argv);
    start_listen();
    // 循环处理会话
    int index;

    for (index = 0; index < 3; index++)
    {
        printf("debug in main() %d: %d\n", index, run_params.request_file_open_book[index].fd);
    }
    handle_session();
}


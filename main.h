//
// Created by liushan on 19-11-9.
//

#ifndef DAGAMA_MAIN_H
#define DAGAMA_MAIN_H

#include <stdbool.h>
#include <limits.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "write_log.h"

#define SET_RESPONSE_STATUS_200(header_resonse)  header_resonse->status = 200; strcpy(header_resonse->status_desc, "OK")
#define SET_RESPONSE_STATUS_403(header_resonse)  header_resonse->status = 403; strcpy(header_resonse->status_desc, "forbidden")
#define SET_RESPONSE_STATUS_404(header_resonse)  header_resonse->status = 404; strcpy(header_resonse->status_desc, "file not found")
#define SET_RESPONSE_STATUS_405(header_resonse)  header_resonse->status = 405; strcpy(header_resonse->status_desc, "method not allowed")
#define SET_RESPONSE_STATUS_500(header_resonse)  header_resonse->status = 500; strcpy(header_resonse->status_desc, "Internal Server Error")

#define EXTENSION_NAME_LENTH 8
#define MAX_EPOLL_SIZE 10000
#define MAX_MIMEBOOK_SIZE 128
#define MAX_HEADER_METHOD_ALLOW_NUM 8
#define MAX_HEADER_METHOD_ALLOW_SIZE 8
#define MAX_HEADER_FILE_INDEX_NUM 16
#define MAX_HEADER_RESPONSE_SIZE 4096

#define ACCEPT_CONTINUE_FLAG -3


#define HEADER_VALUE_NOT_AVAIABLE "NA"
#define HEADER_METHOD_GET "GET"


// 会话的状态
#define SESSION_READ_HEADER 1
#define SESSION_RESPONSE_HEADER 2
#define SESSION_RESPONSE_BODY 3
#define SESSION_RST 4


// 是否收到数据
#define SESSION_DATA_READ_READY 1
#define SESSION_DATA_WRITE_READY 2
#define SESSION_DATA_HANDLED 3

// 客户端是否断开连接
#define SESSION_RNSHUTDOWN 0
#define SESSION_RSHUTDOWN 1

#define LOG_SPLIT_STR "    "



struct request_header {
    char host[PATH_MAX];
    char method[10];
    char uri[1024];
    char http_version[32];
    char headers[100][2][1024];
    char all_header_line[100][1024];
    char user_agent[4096];
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

typedef struct ResponseData {
    char fallback_500_data_buf[4096];
    char send_buffer[4096];
    ssize_t read_buffer_size;
    ssize_t buffer_size;
} ResponseData;


struct mimedict {
    char extension[8];
    char content_type[128];
};

struct DefaultReqFile {
    char request_file_404[PATH_MAX];
    char request_file_403[PATH_MAX];
    char request_file_405[PATH_MAX];
};


struct ResponseStatus {
    bool is_header_illegal;
    bool is_method_allowd;
};


struct sessionInfo {
    // int                    connTransactions;
    int                    connFd;
    int                    localFileFd;
    off_t                  localFdPos;
    char *                 recv_buf;
    char                   header_buf[MAX_HEADER_RESPONSE_SIZE];
    char                   header_response[MAX_HEADER_RESPONSE_SIZE];
    unsigned int           sessionStatus;
    unsigned int           sessionRShutdown;
    unsigned int           sessionRcvData;
    struct request_header  hd_request;
    struct response_header hd_response;
    char                   request_file[PATH_MAX];
    int                    redirect_count;
    // bool                   is_request_file_set;
    struct ResponseStatus  response_status;
    bool                   is_https;
    bool                   upstream_is_fastcgi;
    bool                   upstream_is_local_http;
    bool                   upstream_is_proxy_http;
    bool                   https_ssl_have_conned;
    ResponseData           response_data;
    long long              response_bytes;
    SSL *                  ssl;
    ssize_t                send_buffer_size;
    char *                 send_buffer;
};


struct FileOpenBook {
        char file_path[PATH_MAX];
        FILE * f;
};


struct RequestFileOpenBook {
    char file_path[PATH_MAX];
    int fd;
    unsigned long long file_size;
    int reference_count;
    int myerrno;
};


struct hostVar {
    char   host[PATH_MAX];
    char   doc_root[PATH_MAX];
    char   file_indexs[MAX_HEADER_FILE_INDEX_NUM][PATH_MAX];
    int    file_indexs_len;
    char   method_allowed[MAX_HEADER_METHOD_ALLOW_NUM][MAX_HEADER_METHOD_ALLOW_SIZE];
    int    method_allowed_len;
    char   request_file_403[PATH_MAX];
    char   request_file_404[PATH_MAX];
    char   request_file_405[PATH_MAX];
    char   log_host_path[PATH_MAX];
    FILE * f_host_log;
    char   log_level_host[LOG_LEVEL_MAX_LEN];
    struct mimedict mimebook[MAX_MIMEBOOK_SIZE];
};


struct ParamsRun {
    int host_count;
    struct hostVar * hostvar;
    int epoll_fd;
    struct epoll_event event;
    struct RequestFileOpenBook request_file_open_book[MAX_EPOLL_SIZE];
};

typedef struct ConnInfo {
    char client_ip[INET_ADDRSTRLEN];
//    char user_agent[4096];
//    int http_status;
    char log_msg[4096];
} ConnInfo;

struct SessionRunParams {
    struct sessionInfo * session_info;
    struct sockaddr_in * client_sockaddr;
    struct hostVar * hostvar;
    ConnInfo conn_info;
};









// ssl
void init_openssl();
SSL_CTX * create_context_ssl(void);
void configure_context_ssl(SSL_CTX * ctx);

void event_add(struct epoll_event * event, int epoll_fd, int fd);
void event_update(struct epoll_event * event, int epoll_fd, int fd);

void get_config_file_path(int argc, char ** argv);
void process_request(struct SessionRunParams *, struct ParamsRun *);

// header
void get_host_var_by_header(struct SessionRunParams * session_params_ptr,
        const struct request_header * header_request_ptr, struct ParamsRun *);
void read_header(struct SessionRunParams *session_params_ptr);
void get_response_header(struct SessionRunParams * session_params_ptr, struct ParamsRun *);
void response_header(struct SessionRunParams *session_params_ptr, struct ParamsRun *);

void response_data(struct SessionRunParams *session_params_ptr);
void write_response_data(struct SessionRunParams *session_params_ptr);


int create_listen_sock(int port, struct sockaddr_in * server_sockaddr);
void test_module(int argc, char ** argv);

void install_signal(void);
void get_run_params(int argc, char ** argv);
void start_listen(void);
void handle_session(void);

void get_access_log(struct SessionRunParams * session_params_ptr);
void get_client_ip(struct SessionRunParams * session_params_ptr);


static void sig_pipe(int signo);
#endif //DAGAMA_MAIN_H

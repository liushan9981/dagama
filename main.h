//
// Created by liushan on 19-11-9.
//

#ifndef DAGAMA_MAIN_H
#define DAGAMA_MAIN_H

#include <stdbool.h>
#include <limits.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define SET_RESPONSE_STATUS_200(header_resonse)  header_resonse.status = 200; strcpy(header_resonse.status_desc, "OK")
#define SET_RESPONSE_STATUS_403(header_resonse)  header_resonse.status = 403; strcpy(header_resonse.status_desc, "forbidden")
#define SET_RESPONSE_STATUS_404(header_resonse)  header_resonse.status = 404; strcpy(header_resonse.status_desc, "file not found");
#define SET_RESPONSE_STATUS_405(header_resonse)  header_resonse.status = 405; strcpy(header_resonse.status_desc, "method not allowed");


#define EXTENSION_NAME_LENTH 8
#define MAX_EPOLL_SIZE 10000
#define MAX_MIMEBOOK_SIZE 128
#define MAX_HEADER_METHOD_ALLOW_NUM 8
#define MAX_HEADER_METHOD_ALLOW_SIZE 8
#define MAX_HEADER_FILE_INDEX_NUM 16
#define MAX_HEADER_RESPONSE_SIZE 4096

// 会话的状态
#define SESSION_READ_HEADER 1
#define SESSION_RESPONSE_HEADER 2
#define SESSION_RESPONSE_BODY 3
#define SESSION_END 4

// 是否收到数据
#define SESSION_DATA_READ_READY 1
#define SESSION_DATA_WRITE_READY 2
#define SESSION_DATA_HANDLED 3

// 客户端是否断开连接
#define SESSION_RNSHUTDOWN 0
#define SESSION_RSHUTDOWN 1


struct request_header {
    char host[PATH_MAX];
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
    char header_buf[MAX_HEADER_RESPONSE_SIZE];
    char header_response[MAX_HEADER_RESPONSE_SIZE];
    unsigned int sessionStatus;
    unsigned int sessionRShutdown;
    unsigned int sessionRcvData;

    bool is_https;
    bool https_ssl_have_conned;
    SSL * ssl;
};


struct hostVar {
    char host[PATH_MAX];
    char doc_root[PATH_MAX];
    char file_indexs[MAX_HEADER_FILE_INDEX_NUM][PATH_MAX];
    int file_indexs_len;
    char method_allowed[MAX_HEADER_METHOD_ALLOW_NUM][MAX_HEADER_METHOD_ALLOW_SIZE];
    int method_allowed_len;
    char request_file_403[PATH_MAX];
    char request_file_404[PATH_MAX];
    char request_file_405[PATH_MAX];
    struct mimedict mimebook[MAX_MIMEBOOK_SIZE];
};



struct ParamsRun {
    int host_count;
};


struct SessionRunParams {
    struct connInfo * conninfo;
    struct sockaddr_in * client_sockaddr;
    struct hostVar * hostvar;
    struct ParamsRun params_run_ptr;

//    const struct DefaultReqFile * default_request_file;
//    char * file_index[2];
//    char * method_allow[3];
//    unsigned int method_allow_len;
//    struct mimedict * mimebook;
//    unsigned int mimebook_len;
//    char * doc_root;
};



void get_value_by_header(const char * header, const char * header_key, char * header_value);
void session_close(struct SessionRunParams *run_params);
void get_config_file_path(int argc, char ** argv, char * config_file_path);

// header
void get_host_var_by_header(struct SessionRunParams * run_params_ptr, const struct request_header * header_request_ptr);
void process_request_get_header(struct SessionRunParams *run_params);
void process_request_get_response_header(struct SessionRunParams * run_params_ptr);
void process_request_response_header(struct SessionRunParams *run_params);

void process_request_response_data(struct SessionRunParams *run_params);

void pr_client_info(struct SessionRunParams *run_params_ptr);

#endif //DAGAMA_MAIN_H

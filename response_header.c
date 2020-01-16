//
// Created by liushan on 19-12-8.
//

#include "response_header.h"
#include "main.h"
#include "request_header.h"

#include <sys/stat.h>
#include "mystring.h"
#include <zconf.h>
#include <unistd.h>
#include "process_request_fastcgi.h"
#include <fcntl.h>


extern struct ParamsRun run_params;
extern char * response_500_msg;


void parse_header_request(struct SessionRunParams * session_params_ptr)
{
    get_header_request_all_line(session_params_ptr);

    get_header_request_method(session_params_ptr);
    get_header_request_uri(session_params_ptr);
    get_header_request_host(session_params_ptr);
    get_header_request_ua(session_params_ptr);

}


void get_client_ip(struct SessionRunParams * session_params_ptr)
{
    char cli_addr_buff[INET_ADDRSTRLEN];
    struct sockaddr_in cli_sockaddr;
    socklen_t len;

    len = sizeof(cli_sockaddr);

    if (getpeername(session_params_ptr->session_info->connFd, (struct sockaddr *)&cli_sockaddr, &len) == 0)
    {
        inet_ntop(AF_INET, &(cli_sockaddr.sin_addr), cli_addr_buff, INET_ADDRSTRLEN);
        memcpy(session_params_ptr->conn_info.client_ip, cli_addr_buff, strlen(cli_addr_buff) + 1);
    }
}


void get_contenttype_by_filepath(char * filepath, struct mimedict mimebook [], int mimebook_len,
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



bool check_file_path_accessible(char * file_path)
{
    struct stat statbuf;

    if (access(file_path, F_OK) != -1 && stat(file_path, &statbuf) != -1 && S_ISREG(statbuf.st_mode) )
        return true;
    return false;
}



bool get_index_file(struct SessionRunParams * session_params_ptr)
{
    int index_file_count, index;
    char (* file_index) [PATH_MAX];
    char file_path[PATH_MAX];

    index_file_count = session_params_ptr->hostvar->file_indexs_len;
    file_index = session_params_ptr->hostvar->file_indexs;
    printf("get_index_file, index_file_count: %d\n", index_file_count);


    for (index = 0; index < index_file_count; index++)
    {
        sprintf(file_path, "%s/%s%s",
                session_params_ptr->hostvar->doc_root,
                session_params_ptr->session_info->hd_request.uri,
                file_index[index]);

        if (check_file_path_accessible(file_path) )
        {
            printf("index file is accessable: %s\n", file_path);
            memcpy(session_params_ptr->session_info->request_file, file_path, strlen(file_path) + 1);
            printf("get_index_file: after memcpy:%s\n", session_params_ptr->session_info->request_file);
            return true;
        }
        else
            printf("index file is not accessable: %s\n", file_path);

    }

    return false;
}


void get_response_header_file_path(struct SessionRunParams * session_params_ptr)
{
    struct request_header * header_request;
    struct response_header * header_resonse;
    char * request_file;

    header_resonse = &(session_params_ptr->session_info->hd_response);
    header_request = &(session_params_ptr->session_info->hd_request);
    request_file = session_params_ptr->session_info->request_file;

    // 访问的uri是目录的，重写到该目录下的index文件
    if (str_endwith(header_request->uri, "/") )
    {
        if (! get_index_file(session_params_ptr) )
        {
            SET_RESPONSE_STATUS_403(header_resonse);
            strcpy(request_file, session_params_ptr->hostvar->request_file_403);
        }

    }
    else
        sprintf(request_file, "%s/%s", session_params_ptr->hostvar->doc_root, header_request->uri);
}



bool get_response_header_check_client_is_allowed(struct SessionRunParams * session_params_ptr)
{
    char * request_file;
    struct response_header * header_resonse;

    header_resonse = &(session_params_ptr->session_info->hd_response);
    request_file = session_params_ptr->session_info->request_file;

    printf("strcmp(session_params_ptr->conn_info.client_ip, \"127.0.0.1\"): %d\n", strcmp(session_params_ptr->conn_info.client_ip, "127.0.0.1"));
    printf("host: %s\n", session_params_ptr->session_info->hd_request.uri);

    if (strcmp(session_params_ptr->conn_info.client_ip, "127.0.0.1") != 0)
    {
        SET_RESPONSE_STATUS_403(header_resonse);
        strcpy(request_file, session_params_ptr->hostvar->request_file_403);
//        session_params_ptr->session_info->is_request_file_set = true;
        memcpy(session_params_ptr->session_info->hd_request.method, HEADER_METHOD_GET, strlen(HEADER_METHOD_GET) + 1);
        printf("client ip is not allowed\n");
        printf("request file: %s\n", request_file);

        return false;
    }
    printf("2 session_params_ptr->conn_info.client_ip: %s\n", session_params_ptr->conn_info.client_ip);
    return true;
}



bool get_response_header_check_method_is_allowed(struct SessionRunParams * session_params_ptr)
{
    char * request_file;
    struct response_header * header_resonse;
    header_resonse = &(session_params_ptr->session_info->hd_response);

    request_file = session_params_ptr->session_info->request_file;

    printf("now run get_response_header_check_method_is_allowed()\n");

    if (! session_params_ptr->session_info->response_status.is_method_allowd)
    {
        SET_RESPONSE_STATUS_405(header_resonse);
        strcpy(request_file, session_params_ptr->hostvar->request_file_405);
        // session_params_ptr->session_info->is_request_file_set = true;
        memcpy(session_params_ptr->session_info->hd_request.method, HEADER_METHOD_GET, strlen(HEADER_METHOD_GET) + 1);
        return false;
    }

    return true;
}


bool is_request_file_set(struct SessionRunParams * session_params_ptr)
{
    return (strlen(session_params_ptr->session_info->request_file) > 0)?true: false;
}



bool is_response_status_set(struct SessionRunParams * session_params_ptr)
{
    return (session_params_ptr->session_info->hd_response.status == 0)?false: true;
}



int getOrNewReqFd(struct SessionRunParams *session_params_ptr, struct ParamsRun *run_params_ptr)
{
    char * request_file;
    int index;
    struct response_header * header_resonse;
    int * localFileFd;
    unsigned long long * content_length;
    struct RequestFileOpenBook * request_file_open_book_ptr;
    struct stat statbuf;
    bool flag = false;

    header_resonse = &(session_params_ptr->session_info->hd_response);
    request_file = session_params_ptr->session_info->request_file;
    localFileFd = &(session_params_ptr->session_info->localFileFd);
    content_length = &(header_resonse->content_length);

    printf("### debug now run get_fd_or_open_file()\n");

    for (index = 0; index < MAX_EPOLL_SIZE; index++)
        if (strcmp(request_file, run_params_ptr->request_file_open_book[index].file_path) == 0)
        {
            request_file_open_book_ptr = &(run_params_ptr->request_file_open_book[index]);
            *localFileFd = request_file_open_book_ptr->fd;
            *content_length = request_file_open_book_ptr->file_size;
            printf("debug content_length: %lld\n", *content_length);
            request_file_open_book_ptr[index].reference_count++;
            return 0;
        }

    if ( (*localFileFd = open(request_file, O_RDONLY) ) > 0 && stat(request_file, &statbuf) != -1)
        flag = true;

    for (index = 0; index < MAX_EPOLL_SIZE; index++)
        if (run_params_ptr->request_file_open_book[index].fd == -1)
        {
            request_file_open_book_ptr = &(run_params_ptr->request_file_open_book[index]);
            memcpy(request_file_open_book_ptr->file_path, request_file, strlen(request_file) + 1);
            if (flag)
            {
                request_file_open_book_ptr->fd = *localFileFd;
                *content_length = request_file_open_book_ptr->file_size = statbuf.st_size;
                request_file_open_book_ptr->reference_count++;
                request_file_open_book_ptr->myerrno = 0;
                return request_file_open_book_ptr->myerrno;
            }
            else
            {
                request_file_open_book_ptr->myerrno = errno;
                return request_file_open_book_ptr->myerrno;
            }

        }

    return 0;
}


void closeReqFd(struct sessionInfo *connSessionInfo)
{
    int index;
    char * request_file;
    struct RequestFileOpenBook * request_file_open_book_ptr = NULL;


    request_file = connSessionInfo->request_file;

    for (index = 0; index < MAX_EPOLL_SIZE; index++)
        if (strcmp(request_file, run_params.request_file_open_book[index].file_path) == 0)
        {
            request_file_open_book_ptr = &(run_params.request_file_open_book[index]);
            (request_file_open_book_ptr->reference_count)--;
            printf("debug from close_local_fd filepath %s ref count is %d\n",
                   request_file_open_book_ptr->file_path,
                   request_file_open_book_ptr->reference_count);
            break;
        }

    if (request_file_open_book_ptr == NULL)
        return;
    else if (request_file_open_book_ptr->reference_count <= 0)
    {
        printf("debug: from close_local_fd() now close fd: %d\n", request_file_open_book_ptr->fd);
        close(request_file_open_book_ptr->fd);
        request_file_open_book_ptr->fd = -1;
        request_file_open_book_ptr->file_size = -1;
        request_file_open_book_ptr->reference_count = 0;
        request_file_open_book_ptr->myerrno = 0;
        request_file_open_book_ptr->file_path[0] = '\0';
    }

}


void open_request_file(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    char * request_file;
    struct response_header * header_resonse;
    int * redirect_count;
    int myerrno;

    request_file = session_params_ptr->session_info->request_file;
    header_resonse = &(session_params_ptr->session_info->hd_response);
    redirect_count = &(session_params_ptr->session_info->redirect_count);

    if (*redirect_count > 10)
    {
        // SET 500
        session_params_ptr->session_info->localFileFd = -3;
        memcpy(session_params_ptr->session_info->response_data.fallback_500_data_buf, response_500_msg, strlen(response_500_msg) + 1);
        SET_RESPONSE_STATUS_500(header_resonse);
        session_params_ptr->session_info->hd_response.content_length = strlen(response_500_msg);
        return;
    }

    myerrno = getOrNewReqFd(session_params_ptr, run_params_ptr);
    printf("mydebug myerrno: %d\n", myerrno);

    if (session_params_ptr->session_info->localFileFd > 0
    && myerrno == 0)
    {
        if (header_resonse->status == 0)
        {
            SET_RESPONSE_STATUS_200(header_resonse);
        }
    }
    else
    {
        (*redirect_count)++;

        if (is_response_status_set(session_params_ptr) )
            return;

        if (myerrno == EACCES)
        {
            fprintf(stderr, "open file %s %s\n", request_file, strerror(EACCES) );
            SET_RESPONSE_STATUS_403(header_resonse);
            strcpy(request_file, session_params_ptr->hostvar->request_file_403);
        }
        else if (myerrno == ENOENT)
        {
            fprintf(stderr, "open file %s %s\n", request_file, strerror(ENOENT) );
            SET_RESPONSE_STATUS_404(header_resonse);
            strcpy(request_file, session_params_ptr->hostvar->request_file_404);
        }
    }

}


void get_local_http_response_header(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    char * request_file;
    struct response_header * header_resonse;
    int local_file_fd;

    local_file_fd = session_params_ptr->session_info->localFileFd;
    request_file = session_params_ptr->session_info->request_file;
    header_resonse = &(session_params_ptr->session_info->hd_response);

    printf("status: %d\n", session_params_ptr->session_info->hd_response.status);

    if (! is_request_file_set(session_params_ptr) && local_file_fd != -3)
    {
        get_response_header_file_path(session_params_ptr);
        printf("func->get_local_http_response_header: request_file1: %s\n", request_file);
    }

    while (local_file_fd != -3 && local_file_fd < 0)
    {
        open_request_file(session_params_ptr, run_params_ptr);
        local_file_fd = session_params_ptr->session_info->localFileFd;
    }

    printf("func->get_local_http_response_header: request_file2: %s\n", request_file);
    printf("func->get_local_http_response_header: local_fd: %d\n", session_params_ptr->session_info->localFileFd);

    get_contenttype_by_filepath(request_file, session_params_ptr->hostvar->mimebook, MAX_MIMEBOOK_SIZE, header_resonse);

}





void merge_response_header(struct SessionRunParams * session_params_ptr)
{
    char ch_temp[1024];
    struct response_header * header_resonse;

    if (session_params_ptr->session_info == NULL)
        return;

    header_resonse = &(session_params_ptr->session_info->hd_response);

    // 拼接响应头
    sprintf(session_params_ptr->session_info->header_response, "%s %d %s\n", header_resonse->http_version, header_resonse->status,
            header_resonse->status_desc);
    sprintf(ch_temp, "Content-Type: %s\n", header_resonse->content_type);
    strcat(session_params_ptr->session_info->header_response, ch_temp);
    sprintf(ch_temp, "Content-Length: %llu\n", header_resonse->content_length);
    strcat(session_params_ptr->session_info->header_response, ch_temp);
    sprintf(ch_temp, "Connection: %s\n", header_resonse->connection);
    strcat(session_params_ptr->session_info->header_response, ch_temp);
    sprintf(ch_temp, "Server: %s\n\n", header_resonse->server);
    strcat(session_params_ptr->session_info->header_response, ch_temp);

}



void get_upstream_type(struct SessionRunParams * session_params_ptr)
{
    struct request_header * header_request;

    header_request = &(session_params_ptr->session_info->hd_request);

    if (str_endwith(header_request->uri, ".php"))
        session_params_ptr->session_info->upstream_is_fastcgi = true;
    else
        session_params_ptr->session_info->upstream_is_local_http = true;

}


void set_default_header_response(struct SessionRunParams * session_params_ptr)
{
    struct response_header * header_resonse;
    header_resonse = &(session_params_ptr->session_info->hd_response);

    memcpy(header_resonse->http_version, "HTTP/1.1", strlen("HTTP/1.1") + 1);
    memcpy(header_resonse->connection, "keep-alive", strlen("keep-alive") + 1);
    memcpy(header_resonse->server, "Dagama", strlen("Dagama") + 1);
}


void get_response_header(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    struct request_header * header_request;
    char * request_file;


    header_request = &(session_params_ptr->session_info->hd_request);
    parse_header_request(session_params_ptr);
    get_host_var_by_header(session_params_ptr, header_request, run_params_ptr);
    set_default_header_response(session_params_ptr);

    get_client_ip(session_params_ptr);
    if (get_response_header_check_client_is_allowed(session_params_ptr) )
        get_response_header_check_method_is_allowed(session_params_ptr);

    get_upstream_type(session_params_ptr);

    if (session_params_ptr->session_info->upstream_is_fastcgi)
    {
        request_file = session_params_ptr->session_info->request_file;
        sprintf(request_file, "%s/%s", session_params_ptr->hostvar->doc_root, header_request->uri);
        process_request_fastcgi(session_params_ptr->session_info->connFd, request_file);
    }
    else if (session_params_ptr->session_info->upstream_is_local_http)
    {
        printf("check now is session_params_ptr->session_info->upstream_is_local_http\n");
        if (strcmp(session_params_ptr->session_info->hd_request.method, HEADER_METHOD_GET) == 0)
        {
            get_local_http_response_header(session_params_ptr, run_params_ptr);
            merge_response_header(session_params_ptr);
        }
        else
        {
            // 其他方法
        }
    }
    else if (session_params_ptr->session_info->upstream_is_proxy_http)
    {
        // TODO proxy http
    }

}


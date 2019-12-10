//
// Created by liushan on 19-12-8.
//

#include "response_header.h"
#include "main.h"

#include <sys/stat.h>
#include "mystring.h"
#include <zconf.h>
#include <unistd.h>
#include "process_request_fastcgi.h"
#include <fcntl.h>



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



void get_response_header_get(struct SessionRunParams * session_params_ptr)
{
    struct request_header * header_request;
    struct stat statbuf;
    struct response_header * header_resonse;
    char * request_file;


    header_resonse = &(session_params_ptr->conninfo->hd_response);
    header_request = &(session_params_ptr->conninfo->hd_request);
    request_file = session_params_ptr->conninfo->request_file;

    // 访问的uri是目录的，重写到该目录下的index文件
    if (str_endwith(header_request->uri, "/") )
    {
        sprintf(request_file, "%s/%s%s", session_params_ptr->hostvar->doc_root, header_request->uri, session_params_ptr->hostvar->file_indexs[0]);
        if (access(request_file, F_OK) == -1)
        {
            sprintf(request_file, "%s/%s%s", session_params_ptr->hostvar->doc_root, header_request->uri, session_params_ptr->hostvar->file_indexs[1]);
            if (access(request_file, F_OK) == -1)
            {
                SET_RESPONSE_STATUS_403(header_resonse);
                strcpy(request_file, session_params_ptr->hostvar->request_file_403);
            }
            else
            {
                SET_RESPONSE_STATUS_200(header_resonse);
            }

        }
        else
        {
            SET_RESPONSE_STATUS_200(header_resonse);
        }

    }
    else
    {
        sprintf(request_file, "%s/%s", session_params_ptr->hostvar->doc_root, header_request->uri);
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
                    strcpy(request_file, session_params_ptr->hostvar->request_file_404);
                }
            }
            else
            {
                printf("get file %s stat error\n", request_file);
                SET_RESPONSE_STATUS_403(header_resonse);
                strcpy(request_file, session_params_ptr->hostvar->request_file_403);
            }

        }
            // 文件不存在,则404
        else
        {
            SET_RESPONSE_STATUS_404(header_resonse);
            strcpy(request_file, session_params_ptr->hostvar->request_file_404);
        }

        printf("request_file: %s\n", request_file);

    }

    session_params_ptr->conninfo->is_request_file_set = true;

}




bool get_response_header_check_client_is_allowed(struct SessionRunParams * session_params_ptr)
{
    char * request_file;
    struct response_header * header_resonse;

    header_resonse = &(session_params_ptr->conninfo->hd_response);
    request_file = session_params_ptr->conninfo->request_file;

    printf("1 session_params_ptr->accessLog.client_ip: %s\n", session_params_ptr->accessLog.client_ip);
    printf("strcmp(session_params_ptr->accessLog.client_ip, \"127.0.0.1\"): %d", strcmp(session_params_ptr->accessLog.client_ip, "127.0.0.1") );
    if (strcmp(session_params_ptr->accessLog.client_ip, "127.0.0.1") != 0)
    {
        SET_RESPONSE_STATUS_403(header_resonse);
        strcpy(request_file, session_params_ptr->hostvar->request_file_403);
        session_params_ptr->conninfo->is_request_file_set = true;
        memcpy(session_params_ptr->conninfo->hd_request.method, HEADER_METHOD_GET, strlen(HEADER_METHOD_GET) + 1);
        printf("client ip is now allowed\n");
        printf("request file: %s\n", request_file);

        return false;
    }
    printf("2 session_params_ptr->accessLog.client_ip: %s\n", session_params_ptr->accessLog.client_ip);
    return true;
}



bool get_response_header_check_method_is_allowed(struct SessionRunParams * session_params_ptr)
{
    char * request_file;
    struct response_header * header_resonse;
    header_resonse = &(session_params_ptr->conninfo->hd_response);

    request_file = session_params_ptr->conninfo->request_file;

    printf("now run get_response_header_check_method_is_allowed()\n");

    if (! session_params_ptr->conninfo->response_status.is_method_allowd)
    {
        SET_RESPONSE_STATUS_405(header_resonse);
        strcpy(request_file, session_params_ptr->hostvar->request_file_405);
        session_params_ptr->conninfo->is_request_file_set = true;
        memcpy(session_params_ptr->conninfo->hd_request.method, HEADER_METHOD_GET, strlen(HEADER_METHOD_GET) + 1);
    }

    return session_params_ptr->conninfo->is_request_file_set;
}




void get_local_http_response_header(struct SessionRunParams * session_params_ptr)
{
    char * request_file;
    struct stat statbuf;
    struct response_header * header_resonse;
    header_resonse = &(session_params_ptr->conninfo->hd_response);

    request_file = session_params_ptr->conninfo->request_file;

    printf("now is get_local_http_response_header()\n");
    printf("request_file: %s\n", request_file);

    if (stat(request_file, &statbuf) != -1)
        header_resonse->content_length = statbuf.st_size;
    else {
        printf("get statbuf error!\n");
        // continue;
    }

    if ((session_params_ptr->conninfo->localFileFd = open(request_file, O_RDONLY)) < 0) {
        fprintf(stderr, "open file %s error!\n", request_file);
        SET_RESPONSE_STATUS_403(header_resonse);
        strcpy(request_file, session_params_ptr->hostvar->request_file_403);
        if ((session_params_ptr->conninfo->localFileFd = open(request_file, O_RDONLY)) < 0) {
            fprintf(stderr, "open file %s error!\n", request_file);
            session_close(session_params_ptr);
            return;
        }

    }

    get_contenttype_by_filepath(request_file, session_params_ptr->hostvar->mimebook, MAX_MIMEBOOK_SIZE, header_resonse);
}




void merge_response_header(struct SessionRunParams * session_params_ptr)
{
    char ch_temp[1024];
    struct response_header * header_resonse;
    header_resonse = &(session_params_ptr->conninfo->hd_response);


    // 拼接响应头
    sprintf(session_params_ptr->conninfo->header_response, "%s %d %s\n", header_resonse->http_version, header_resonse->status,
            header_resonse->status_desc);
    sprintf(ch_temp, "Content-Type: %s\n", header_resonse->content_type);
    strcat(session_params_ptr->conninfo->header_response, ch_temp);
    sprintf(ch_temp, "Content-Length: %llu\n", header_resonse->content_length);
    strcat(session_params_ptr->conninfo->header_response, ch_temp);
    sprintf(ch_temp, "Connection: %s\n", header_resonse->connection);
    strcat(session_params_ptr->conninfo->header_response, ch_temp);
    sprintf(ch_temp, "Server: %s\n\n", header_resonse->server);
    strcat(session_params_ptr->conninfo->header_response, ch_temp);

}



void get_upstream_type(struct SessionRunParams * session_params_ptr)
{
    struct request_header * header_request;

    header_request = &(session_params_ptr->conninfo->hd_request);

    if (str_endwith(header_request->uri, ".php"))
        session_params_ptr->conninfo->upstream_is_fastcgi = true;
    else
        session_params_ptr->conninfo->upstream_is_local_http = true;

}




void set_default_header_response(struct SessionRunParams * session_params_ptr)
{
    struct response_header * header_resonse;
    header_resonse = &(session_params_ptr->conninfo->hd_response);

//    struct response_header header_resonse = {
//            .http_version = "HTTP/1.1",
//            .content_type = "image/jpeg",
//            .content_length = 16,
//            .connection = "keep-alive",
//            .server = "Dagama",
//            .status = 200,
//            .status_desc = "OK"
//    };

    memcpy(header_resonse->http_version, "HTTP/1.1", strlen("HTTP/1.1") + 1);
    memcpy(header_resonse->connection, "keep-alive", strlen("keep-alive") + 1);
    memcpy(header_resonse->server, "Dagama", strlen("Dagama") + 1);
}


void get_response_header(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    struct request_header * header_request;
    char * request_file;


    header_request = &(session_params_ptr->conninfo->hd_request);
    parse_header_request(session_params_ptr);
    get_host_var_by_header(session_params_ptr, header_request, run_params_ptr);
    set_default_header_response(session_params_ptr);

    get_client_ip(session_params_ptr);
    if (get_response_header_check_client_is_allowed(session_params_ptr) )
        get_response_header_check_method_is_allowed(session_params_ptr);

    get_upstream_type(session_params_ptr);

    if (session_params_ptr->conninfo->upstream_is_fastcgi)
    {
        request_file = session_params_ptr->conninfo->request_file;
        sprintf(request_file, "%s/%s", session_params_ptr->hostvar->doc_root, header_request->uri);
        process_request_fastcgi(session_params_ptr->conninfo->connFd, request_file);
    }
    else if (session_params_ptr->conninfo->upstream_is_local_http)
    {
        printf("check now is session_params_ptr->conninfo->upstream_is_local_http\n");
        if (strcmp(session_params_ptr->conninfo->hd_request.method, HEADER_METHOD_GET) == 0)
        {
            if (! session_params_ptr->conninfo->is_request_file_set)
                get_response_header_get(session_params_ptr);
            get_local_http_response_header(session_params_ptr);
            merge_response_header(session_params_ptr);
        }
        else
        {
            // 其他方法
        }
    }
    else if (session_params_ptr->conninfo->upstream_is_proxy_http)
    {
        // TODO proxy http
    }

}

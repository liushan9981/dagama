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
#include "process_request.h"
#include "writen_readn_readline.h"
#include "main.h"
#include "handle_config.h"
#include "myutils.h"



void get_header_value_by_key(const char *header, const char *header_key, char *header_value)
{
    char * key_pre;
    int header_key_len = strlen(header_key) + 1;

    key_pre = malloc(sizeof(char) * (header_key_len + 1) );
    memcpy(key_pre, header_key, header_key_len);
    memcpy(header_value, header, strlen(header) + 1);
    strcat(key_pre, ":");
    str_lstrip_str(header_value, key_pre);
    free(key_pre);
    str_strip(header_value);
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


void get_header_request_method(struct SessionRunParams * session_params_ptr);

void get_header_request_method(struct SessionRunParams * session_params_ptr)
{
    int index;
    char * headers_recv;
    char * method_allowed;
    char (* all_method_allowed_ptr)[MAX_HEADER_METHOD_ALLOW_SIZE];

    headers_recv = session_params_ptr->conninfo->header_buf;
    all_method_allowed_ptr = session_params_ptr->hostvar->method_allowed;

    for (index = 0; index < MAX_HEADER_METHOD_ALLOW_NUM; index++)
        if (str_startwith(headers_recv, all_method_allowed_ptr[index]) )
        {
            printf("debug: get_header_request_method: cmp = 0\n");

            memcpy(session_params_ptr->conninfo->header_request.method,
                   all_method_allowed_ptr[index],
                   strlen(all_method_allowed_ptr[index]) + 1
            );
            printf("after get_header_request_method:%s\n", session_params_ptr->conninfo->header_request.method);
            return;
        }

    session_params_ptr->conninfo->response_status.is_method_allowd = false;
}


void get_header_request_uri(struct SessionRunParams * session_params_ptr);

void get_header_request_uri(struct SessionRunParams * session_params_ptr)
{
    int index, char_s_count;
    char * headers_recv;
    const int first_line_max_len = 14 + PATH_MAX;
    char first_line[first_line_max_len];
    char line_char_s[4][MAX_STR_SPLIT_SIZE];
    char * uri_ptr;

    headers_recv = session_params_ptr->conninfo->header_buf;
    // GET / HTTP/1.1
    // 请求头，路径最大加上其他的值
    for (index = 0; index < first_line_max_len; index++)
    {
        if (headers_recv[index] == '\r' && headers_recv[index + 1] == '\n')
        {
            memcpy(first_line, headers_recv, index);
            first_line[index] = '\0';
            break;
        }
    }

    printf("first line:%s\n", first_line);
    char_s_count = str_split(first_line, ' ', line_char_s, 4);
    uri_ptr = line_char_s[1];

    if (char_s_count == 3)
    {
        if (str_startwith(uri_ptr, "/") )
        {
            memcpy(session_params_ptr->conninfo->header_request.uri, uri_ptr, sizeof(uri_ptr) + 1);
            printf("after get_header_request_uri uri:%s\n", session_params_ptr->conninfo->header_request.uri);
            return;
        }
    }

    session_params_ptr->conninfo->response_status.is_header_illegal = true;
}


void get_header_request_host(struct SessionRunParams * session_params_ptr);

void get_header_request_host(struct SessionRunParams * session_params_ptr)
{
    char * headers_recv;
}



void get_header_request_all_line(struct SessionRunParams * session_params_ptr);

void get_header_request_all_line(struct SessionRunParams * session_params_ptr)
{
    int header_index, line_start, line_end, copy_len;
    char * headers_recv;
    char (* all_header_line) [1024];

    int index;

    headers_recv = session_params_ptr->conninfo->header_buf;
    all_header_line = session_params_ptr->conninfo->header_request.all_header_line;

    printf("all_line: %s\n", headers_recv);

    if (str_endwith(headers_recv, "\r\n\r\n") )
        printf("str_endwith(headers_recv, rnrn)\n");

    // TODO 100写死
    for (header_index = 0, line_start = 0, line_end = 0;
    header_index < 100, line_end < MAX_HEADER_RESPONSE_SIZE;
    line_end++)
    {
        if (headers_recv[line_end] == '\r' && headers_recv[line_end + 1] == '\n')
        {
            printf("line_end: %d\nline_start: %d\n", line_end, line_start);

            copy_len = line_end - line_start;
            if (copy_len == 0)
            {
                printf("debug From get_header_request_all_line: copy_len == 0\n");
                return;
            }
            printf("copy_len: %d\n", copy_len);

            memcpy(all_header_line[header_index], headers_recv + line_start, copy_len);
            printf("after memcpy line_end: %d\nline_start: %d\n", line_end, line_start);
            all_header_line[header_index][copy_len] = '\0';
            line_start = line_end + 2;
            header_index++;
        }
    }

    if (header_index == 100)
    {
        // TODO 请求头太大
        printf("head request too large\n");
    }

    printf("get_header_request_all_line result:\n");
    for (index = 0; index < header_index; index++)
        printf("#%s#\n", all_header_line[index]);

}


void parse_header_request(struct SessionRunParams * session_params_ptr)
{
    char line_char_s[4][MAX_STR_SPLIT_SIZE];
    char line_read[1024];
    int index, temp_index, char_s_count, header_index, header_value_index;
    bool flag = true;

    char * headers_recv;
    struct request_header * headers_request;

    headers_recv = session_params_ptr->conninfo->header_buf;
    headers_request = &(session_params_ptr->conninfo->header_request);


    get_header_request_all_line(session_params_ptr);
    get_header_request_method(session_params_ptr);
    get_header_request_uri(session_params_ptr);

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
                if (strcmp(line_char_s[0], "User-Agent") == 0)
                {
                    // TODO 长度写死
                    strncpy(headers_request->user_agent, line_read + 11, 4096);
                    str_strip(headers_request->user_agent);
                    printf("debug user agent: %s\n", headers_request->user_agent);
                }
                else if (char_s_count == 2)
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
                    get_header_value_by_key(line_read, headers_request->headers[header_index][0],
                                            headers_request->headers[header_index][1]);

                    printf("## recv headers > 2: %s#%s\n",
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



void process_request_get_header(struct SessionRunParams * session_params_ptr)
{
    bool header_rcv = false;
    int index;
    ssize_t len;
    const int buffer_size = 4096;
    char read_buffer[buffer_size];

    memset(session_params_ptr->conninfo->header_buf, 0, (size_t) MAX_HEADER_RESPONSE_SIZE);
    memset(read_buffer, 0, sizeof(read_buffer));

    printf("connfd: %d\n", session_params_ptr->conninfo->connFd);
    if ( (session_params_ptr->conninfo->is_https) )
        len = SSL_read(session_params_ptr->conninfo->ssl, read_buffer, buffer_size - (size_t)1);
    else
        len = read(session_params_ptr->conninfo->connFd, read_buffer, buffer_size - (size_t)1);


    if ( len == 0)
    {
        // 之前已经读完请求头
        if (session_params_ptr->conninfo->sessionStatus == SESSION_RESPONSE_HEADER)
        {
//            printf("recv len 1: %ld\n", len);
//            free(run_params->conninfo->recv_buf);
//            run_params->conninfo->recv_buf = NULL;
            session_params_ptr->conninfo->sessionRShutdown = SESSION_RSHUTDOWN;
        }
        else if (session_params_ptr->conninfo->sessionStatus == SESSION_READ_HEADER)
        {
            printf("header recv error == 0\n");
            // close(run_params->conninfo->connFd);
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
        if (session_params_ptr->conninfo->sessionStatus == SESSION_READ_HEADER)
        {
            printf("header recv error < 0\n");
            // close(run_params->conninfo->connFd);
            session_close(session_params_ptr);
        }
        else if (session_params_ptr->conninfo->sessionStatus == SESSION_RESPONSE_HEADER)
        {
            printf("recv len 4: %ld\n", len);
//            fprintf(stderr, "str_echo: read error\n");
//            free(run_params->conninfo->recv_buf);
//            run_params->conninfo->recv_buf = NULL;
            session_params_ptr->conninfo->sessionRShutdown = SESSION_RSHUTDOWN;
        }

        return;
        // exit(EXIT_FAILURE);
    }
    else if (len > 0)
    {
        printf("recv len 5: %ld", len);
        read_buffer[len] = '\0';
        // printf("read_buffer:\n%s\n", read_buffer);
        strncat(session_params_ptr->conninfo->recv_buf, read_buffer, buffer_size);
        // printf("recv_buf_index[%d]:\n%s\n", connSessionInfo->connFd, connSessionInfo->recv_buf);

        if (str_endwith(session_params_ptr->conninfo->recv_buf, "\r\n\r\n") )
            printf("debug2 session_params_ptr->conninfo->recv_buf end with rnrn\n");

        for (index = 0; index < strlen(session_params_ptr->conninfo->recv_buf); index++)
        {
            if (session_params_ptr->conninfo->recv_buf[index] == '\r' && session_params_ptr->conninfo->recv_buf[index+1] == '\n' &&
                session_params_ptr->conninfo->recv_buf[index+2] == '\r' && session_params_ptr->conninfo->recv_buf[index+3] == '\n')
            {
                // TODO 接受的数据rnrn丢失
                strncpy(session_params_ptr->conninfo->header_buf, session_params_ptr->conninfo->recv_buf, index + 3);
                if (str_endwith(session_params_ptr->conninfo->header_buf, "\r\n\r\n") )
                    printf("session_params_ptr->conninfo->header_buf end with rnrn\n");
                // printf("index: %d header info:\n%s\n", index, header_buf);
                // 清空接受的数据，粗暴的丢弃后面接收的数据
                memset(session_params_ptr->conninfo->recv_buf, 0, sizeof(char) * MAX_EPOLL_SIZE);
                header_rcv = true;
                break;
            }

        }

        if (! header_rcv)
            // 还没有读到头文件结束，下次继续读取
            return;
        // 剩余可能还会有数据，暂不处理
    }


    if (header_rcv)
    {
        printf("have received header\n");

        // 打印调试信息
        printf("received:\n");
        printf("%s", session_params_ptr->conninfo->header_buf);
        // 读取头完成
        session_params_ptr->conninfo->sessionStatus = SESSION_RESPONSE_HEADER;
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


void process_request_get_response_header(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    struct request_header * header_request;
    // char response[4096];
    char ch_temp[1024];
    struct stat statbuf;
    char request_file[512];
    int index_temp;
    // bool flag_temp;
    bool is_fastcgi = false;

    struct response_header header_resonse = {
            .http_version = "HTTP/1.1",
            .content_type = "image/jpeg",
            .content_length = 16,
            .connection = "keep-alive",
            .server = "Dagama",
            .status = 200,
            .status_desc = "OK"
    };

    header_request = &(session_params_ptr->conninfo->header_request);

    parse_header_request(session_params_ptr);
    get_host_var_by_header(session_params_ptr, header_request, run_params_ptr);

    printf("debug2: docroot:%s\n", session_params_ptr->hostvar->doc_root);
//    flag_temp = false;
//    for (index_temp = 0; index_temp < session_params_ptr->hostvar->method_allowed_len; index_temp++)
//        if (strcmp(session_params_ptr->hostvar->method_allowed[index_temp], header_request->method) == 0)
//        {
//            flag_temp = true;
//            break;
//        }

    if (str_endwith(header_request->uri, ".php"))
    {
        sprintf(request_file, "%s/%s", session_params_ptr->hostvar->doc_root, header_request->uri);
        is_fastcgi = true;
    }
    else if (! session_params_ptr->conninfo->response_status.is_method_allowd)
    {
        SET_RESPONSE_STATUS_405(header_resonse);
        strcpy(request_file, session_params_ptr->hostvar->request_file_405);
    }
    else if (strcmp(header_request->method, "GET") == 0)
    {
        // 访问的uri是目录的，重写到该目录下的index文件
        if (header_request->uri[strlen(header_request->uri) - 1] == '/')
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


    }
    else if (strcmp(header_request->method, "POST") == 0)
    {
        printf("POST is not finished yet!\n");
    }


    if (is_fastcgi)
    {
        process_request_fastcgi(session_params_ptr->conninfo->connFd, request_file);
    }
    else
    {
        printf("request_file: %s\n", request_file);

        if (stat(request_file, &statbuf) != -1)
            header_resonse.content_length = statbuf.st_size;
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

        get_contenttype_by_filepath(request_file, session_params_ptr->hostvar->mimebook, MAX_MIMEBOOK_SIZE,
                                    &header_resonse);

        // 拼接响应头
        sprintf(session_params_ptr->conninfo->header_response, "%s %d %s\n", header_resonse.http_version, header_resonse.status,
                header_resonse.status_desc);
        sprintf(ch_temp, "Content-Type: %s\n", header_resonse.content_type);
        strcat(session_params_ptr->conninfo->header_response, ch_temp);
        sprintf(ch_temp, "Content-Length: %llu\n", header_resonse.content_length);
        strcat(session_params_ptr->conninfo->header_response, ch_temp);
        sprintf(ch_temp, "Connection: %s\n", header_resonse.connection);
        strcat(session_params_ptr->conninfo->header_response, ch_temp);
        sprintf(ch_temp, "Server: %s\n\n", header_resonse.server);
        strcat(session_params_ptr->conninfo->header_response, ch_temp);
    }

}


void get_client_ip(struct SessionRunParams * session_params_ptr)
{
    char cli_addr_buff[INET_ADDRSTRLEN];
    strncpy(session_params_ptr->accessLog.client_ip,
            inet_ntop(AF_INET, &session_params_ptr->client_sockaddr->sin_addr, cli_addr_buff, INET_ADDRSTRLEN),
            16);
}


void get_access_log(struct SessionRunParams * session_params_ptr)
{
    char response_bytes[16];
    char * log_msg_ptr;
    log_msg_ptr = session_params_ptr->accessLog.log_msg;

    snprintf(response_bytes, 16, "%lld", session_params_ptr->accessLog.response_bytes);

    if ( (
            strlen(session_params_ptr->conninfo->header_request.user_agent) +
            strlen(session_params_ptr->accessLog.client_ip) +
            strlen(response_bytes) + 2
            ) > 4096
            )
        strncpy(session_params_ptr->accessLog.log_msg, "log too long", 4096);
    else
    {
        strncpy(log_msg_ptr, session_params_ptr->accessLog.client_ip, 4096);
        strcat(log_msg_ptr, LOG_SPLIT_STR);
        strncat(log_msg_ptr, session_params_ptr->conninfo->header_request.user_agent,
                strlen(session_params_ptr->conninfo->header_request.user_agent) );
        strcat(log_msg_ptr, LOG_SPLIT_STR);
        strncat(log_msg_ptr, response_bytes, strlen(response_bytes) );
    }
}


void process_request_response_header(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr)
{
    int write_len;
    process_request_get_response_header(session_params_ptr, run_params_ptr);

    if (session_params_ptr->conninfo == NULL)
        return;

    get_client_ip(session_params_ptr);

    // 发送响应头信息
    if (session_params_ptr->conninfo->is_https)
        write_len = SSL_write(session_params_ptr->conninfo->ssl, session_params_ptr->conninfo->header_response,
                strlen(session_params_ptr->conninfo->header_response));
    else
        write_len = writen(session_params_ptr->conninfo->connFd, session_params_ptr->conninfo->header_response,
                strlen(session_params_ptr->conninfo->header_response));

    if (write_len < 0)
    {
        printf("writen header failed, continue\n");
        // close(run_params_ptr->conninfo->connFd);
        session_close(session_params_ptr);
        return;
    }
    else
    {
        printf("writen header res: %d\n", write_len);
        session_params_ptr->accessLog.response_bytes = write_len;
        session_params_ptr->conninfo->sessionStatus = SESSION_RESPONSE_BODY;
    }

}



void process_request_response_data(struct SessionRunParams *session_params_ptr)
{
    ssize_t buffer_size = 4096, read_buffer_size;
    char send_buffer[buffer_size], log_level_notice[] = LOG_LEVEL_NOTICE;
    int res_io;


    if ( (read_buffer_size = read(session_params_ptr->conninfo->localFileFd, send_buffer, buffer_size - (ssize_t)1) ) > 0)
    {
        if (session_params_ptr->conninfo->is_https)
            res_io = SSL_write(session_params_ptr->conninfo->ssl, send_buffer, read_buffer_size);
        else
            res_io = writen(session_params_ptr->conninfo->connFd, send_buffer, read_buffer_size);

        if (res_io  == -1)
        {
            printf("writen body failed, continue\n");
            session_close(session_params_ptr);
            return;
        }
        else if (res_io == read_buffer_size)
        {
            session_params_ptr->accessLog.response_bytes += res_io;
        }
        else
        {
            session_params_ptr->accessLog.response_bytes += res_io;
            printf("writen body not full, res: %d read_buffer_size: %ld\n", res_io, read_buffer_size);
        }
    }
    else if (read_buffer_size == 0)
    {
        (session_params_ptr->conninfo->connTransactions)++;
        // printf("session count: %d\n", session_params_ptr->conninfo->connTransactions);


        get_access_log(session_params_ptr);
        write_log(log_level_notice, session_params_ptr->hostvar->log_level_host, session_params_ptr->accessLog.log_msg,
                  session_params_ptr->hostvar->f_host_log);

        if (session_params_ptr->conninfo->sessionRShutdown == SESSION_RSHUTDOWN)
            // 读取输入时，已经处理recv_buf
            session_close(session_params_ptr);
        else
        {
            // 已经发送完毕，保持连接，等待下一个事务
            session_params_ptr->conninfo->sessionStatus = SESSION_READ_HEADER;
            close(session_params_ptr->conninfo->localFileFd);
            session_params_ptr->conninfo->localFileFd = -2;
        }
        return;
    }

}


void process_request(struct SessionRunParams * session_params_ptr, struct ParamsRun * run_params_ptr)
{
    printf("session_status: %d\n", session_params_ptr->conninfo->sessionRcvData);

    if (session_params_ptr->conninfo->sessionRcvData == SESSION_RST)
    {
        printf("recv rst event now close session\n");
        session_close(session_params_ptr);
    }
    else if (session_params_ptr->conninfo->sessionRcvData == SESSION_DATA_READ_READY)
    {
        if (session_params_ptr->conninfo->sessionStatus == SESSION_READ_HEADER)
        {
            process_request_get_header(session_params_ptr);
            if (session_params_ptr->conninfo != NULL && session_params_ptr->conninfo->sessionStatus == SESSION_RESPONSE_HEADER)
            {
                run_params_ptr->event.data.fd = session_params_ptr->conninfo->connFd;
                run_params_ptr->event.events = EPOLLOUT|EPOLLHUP;
                event_update(&(run_params_ptr->event), run_params_ptr->epoll_fd, session_params_ptr->conninfo->connFd);
            }
        }
    }
    else if (session_params_ptr->conninfo->sessionRcvData == SESSION_DATA_WRITE_READY)
    {
        if (session_params_ptr->conninfo->sessionStatus == SESSION_RESPONSE_BODY)
        {
            process_request_response_data(session_params_ptr);
            if (session_params_ptr->conninfo != NULL && session_params_ptr->conninfo->sessionStatus == SESSION_READ_HEADER)
            {
                run_params_ptr->event.data.fd = session_params_ptr->conninfo->connFd;
                run_params_ptr->event.events = EPOLLIN;
                event_update(&(run_params_ptr->event), run_params_ptr->epoll_fd, session_params_ptr->conninfo->connFd);
            }
        }
        else if (session_params_ptr->conninfo->sessionStatus == SESSION_RESPONSE_HEADER)
        {
            process_request_response_header(session_params_ptr, run_params_ptr);
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


void init_session(struct connInfo * connSessionInfo)
{
    connSessionInfo->recv_buf = NULL;
    connSessionInfo->localFileFd = -2;
    connSessionInfo->sessionStatus = SESSION_READ_HEADER;
    connSessionInfo->sessionRShutdown = SESSION_RNSHUTDOWN;
    connSessionInfo->sessionRcvData = SESSION_DATA_HANDLED;
    connSessionInfo->response_status.is_header_illegal = false;
    connSessionInfo->response_status.is_method_allowd = true;
}


void session_close(struct SessionRunParams * session_params_ptr)
{
    close(session_params_ptr->conninfo->connFd);
    printf("closed connFd: %d\n", session_params_ptr->conninfo->connFd);
    if (session_params_ptr->conninfo->localFileFd > 0)
    {
        close(session_params_ptr->conninfo->localFileFd);
        session_params_ptr->conninfo->localFileFd = -2;
        printf("closed localFileFd: %d\n", session_params_ptr->conninfo->localFileFd);
    }

    if (session_params_ptr->conninfo->recv_buf != NULL)
    {
        free(session_params_ptr->conninfo->recv_buf);
        session_params_ptr->conninfo->recv_buf = NULL;
        printf("have freed run_params->conninfo->recv_buf\n");
    }
    if (session_params_ptr->conninfo != NULL)
    {
        free(session_params_ptr->conninfo);
        session_params_ptr->conninfo = NULL;
        printf("have freed run_params->conninfo\n");
    }
}


void new_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd)
{
    struct connInfo * connSessionInfos;

    memset(&(run_params_ptr->event), 0, sizeof(run_params_ptr->event));

    run_params_ptr->event.data.fd = connfd;
    run_params_ptr->event.events = EPOLLIN;
    event_add(&(run_params_ptr->event), run_params_ptr->epoll_fd, connfd);

    connSessionInfos = malloc(sizeof(struct connInfo));
    init_session(connSessionInfos);

    connSessionInfos->recv_buf = malloc(sizeof(char) * MAX_EPOLL_SIZE);
    memset(connSessionInfos->recv_buf, 0, sizeof(char) * MAX_EPOLL_SIZE );
    printf("receive conn:%d\n", connfd);

    connSessionInfos->connFd = connfd;
    connSessionInfos->connTransactions = 0;
    session_params_ptr[connfd].conninfo = connSessionInfos;
}


void new_http_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd, struct sockaddr_in * client_sockaddr)
{
    new_session(session_params_ptr, run_params_ptr, connfd);

    session_params_ptr[connfd].conninfo->is_https = false;
    session_params_ptr[connfd].conninfo->https_ssl_have_conned = false;
    session_params_ptr[connfd].hostvar = run_params_ptr->hostvar;
    session_params_ptr[connfd].client_sockaddr = client_sockaddr;
}


void new_https_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd, struct sockaddr_in * client_sockaddr, SSL_CTX * ctx)
{
    new_session(session_params_ptr, run_params_ptr, connfd);

    session_params_ptr[connfd].conninfo->is_https = true;
    session_params_ptr[connfd].conninfo->https_ssl_have_conned = false;
    session_params_ptr[connfd].hostvar = run_params_ptr->hostvar;
    session_params_ptr[connfd].client_sockaddr = client_sockaddr;

    printf("run_param[connfd].hostvar: %s %s\n",
           session_params_ptr[connfd].hostvar->host,
           session_params_ptr[connfd].hostvar->doc_root);

    SSL * ssl;
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, connfd);
}


int accept_session(int listen_fd, unsigned int * cli_len)
{
    int connfd;

    if ( (connfd = accept(listen_fd, (struct sockaddr *) &client_sockaddr, cli_len)) < 0)
    {
        // 重启被中断的系统调用accept
        if (errno == EINTR)
            connfd = ACCEPT_CONTINUE_FLAG;
            // accept返回前连接终止, SVR4实现
        else if (errno == EPROTO)
            connfd = ACCEPT_CONTINUE_FLAG;
            // accept返回前连接终止, POSIX实现
        else if (errno == ECONNABORTED)
            connfd = ACCEPT_CONTINUE_FLAG;
        else
        {
            printf("accept error!\n");
            connfd = ACCEPT_CONTINUE_FLAG;
            // exit(EXIT_FAILURE);
        }

    }

    return connfd;

}


void new_ssl_session(struct SessionRunParams * session_params_ptr, int connfd)
{
    SSL * ssl;
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, connfd);

    if (SSL_accept(ssl) <= 0)
    {
        printf("#2 ssl accept error errno: %d\n", errno);
        if (session_params_ptr[connfd].conninfo->https_ssl_have_conned)
        {
            printf("run_param[tmpConnFd].https_ssl_have_conned): true\n");
            printf("tmpConnFd: %d\n", connfd);

        }
        else
        {
            printf("run_param[tmpConnFd].https_ssl_have_conned): false\n");
            printf("tmpConnFd: %d\n", connfd);
        }

        close(connfd);
    }
    else
    {
        printf("ssl established %d\n", connfd);
        session_params_ptr[connfd].conninfo->https_ssl_have_conned = true;
        session_params_ptr[connfd].conninfo->ssl = ssl;
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
                    if (session_run_param[tmpConnFd].conninfo->is_https &&
                        (! session_run_param[tmpConnFd].conninfo->https_ssl_have_conned)
                            )
                    {
                        new_ssl_session(session_run_param, tmpConnFd);
                    }
                    else
                    {
                        if (session_run_param[tmpConnFd].conninfo->sessionStatus == SESSION_READ_HEADER)
                        {
                            session_run_param[tmpConnFd].conninfo->sessionRcvData = SESSION_DATA_READ_READY;
                            process_request(&(session_run_param[tmpConnFd]), &run_params);
                        }
                    }

                }

                if (tmpEvent & EPOLLHUP)
                {
                    printf("debug recv EPOLLHUP\n");
                    if (session_run_param[tmpConnFd].conninfo == NULL)
                        continue;
                    session_run_param[tmpConnFd].conninfo->sessionRcvData = SESSION_RST;
                    process_request(&(session_run_param[tmpConnFd]), &run_params);
                }

                // 可以写
                if (tmpEvent & EPOLLOUT)
                {
                    printf("debug recv EPOLLOUT\n");
                    // TODO 连接已经释放，临时处理这种情况
                    if (session_run_param[tmpConnFd].conninfo == NULL)
                        continue;
                    if (session_run_param[tmpConnFd].conninfo->sessionStatus == SESSION_RESPONSE_HEADER ||
                        session_run_param[tmpConnFd].conninfo->sessionStatus == SESSION_RESPONSE_BODY)
                    {
                        session_run_param[tmpConnFd].conninfo->sessionRcvData = SESSION_DATA_WRITE_READY;
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



void get_run_params(int argc, char ** argv)
{
    get_config_file_path(argc, argv);
    printf("config file path: %s\n", config_file_path);

    run_params.host_count = get_config_host_num(config_file_path);
    run_params.hostvar = malloc(sizeof(struct hostVar) * run_params.host_count);
    init_config(run_params.hostvar, config_file_path);
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
    char ch[] = "Host: www.baidu.com\r\n";
    char * ch_ptr = "Host";
    char value[1024];


    str_lstrip_str(ch, ch_ptr);
    printf("%s\n", ch);

//    get_header_value_by_key2(ch, ch_ptr, value);
//
//    printf("after str_lstrip_str:\n%s\n", value);



    exit(EXIT_SUCCESS);

}


int main(int argc, char ** argv) {
    // test_module(argc, argv);

    install_signal();
    get_run_params(argc, argv);
    start_listen();
    // 循环处理会话
    handle_session();
}


//
// Created by liushan on 19-12-7.
//

#include "request_header.h"
#include "mystring.h"



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


void get_header_request_all_line(struct SessionRunParams * session_params_ptr)
{
    int header_index, line_start, line_end, copy_len;
    char * headers_recv;
    char (* all_header_line) [1024];

    int index;

    headers_recv = session_params_ptr->session_info->header_buf;
    all_header_line = session_params_ptr->session_info->hd_request.all_header_line;

    // TODO 100写死
    for (header_index = 0, line_start = 0, line_end = 0;
         header_index < 100, line_end < MAX_HEADER_RESPONSE_SIZE;
         line_end++)
    {
        if (headers_recv[line_end] == '\r' && headers_recv[line_end + 1] == '\n')
        {
            copy_len = line_end - line_start;
            if (copy_len == 0)
            {
                printf("debug From get_header_request_all_line: copy_len == 0\n");
                return;
            }

            memcpy(all_header_line[header_index], headers_recv + line_start, copy_len);
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




void get_header_request_method(struct SessionRunParams * session_params_ptr)
{
    int index;
    char (* all_header_line) [1024];
    char (* all_method_allowed_ptr)[MAX_HEADER_METHOD_ALLOW_SIZE];

    all_header_line = session_params_ptr->session_info->hd_request.all_header_line;
    all_method_allowed_ptr = session_params_ptr->hostvar->method_allowed;

    for (index = 0; index < MAX_HEADER_METHOD_ALLOW_NUM; index++)
        if (str_startwith(all_header_line[0], all_method_allowed_ptr[index]) )
        {
            printf("debug: get_header_request_method: cmp = 0\n");

            memcpy(session_params_ptr->session_info->hd_request.method,
                   all_method_allowed_ptr[index],
                   strlen(all_method_allowed_ptr[index]) + 1
            );
            printf("after get_header_request_method:%s\n", session_params_ptr->session_info->hd_request.method);
            return;
        }

    session_params_ptr->session_info->response_status.is_method_allowd = false;
}




void get_header_request_uri(struct SessionRunParams * session_params_ptr)
{
    int char_s_count;
    char line_char_s[4][MAX_STR_SPLIT_SIZE];
    char * uri_ptr;
    char (* all_header_line) [1024];

    all_header_line = session_params_ptr->session_info->hd_request.all_header_line;

    printf("first line:%s\n", all_header_line[0]);
    char_s_count = str_split(all_header_line[0], ' ', line_char_s, 4);
    uri_ptr = line_char_s[1];

    if (char_s_count == 3)
    {
        if (str_startwith(uri_ptr, "/") )
        {
            memcpy(session_params_ptr->session_info->hd_request.uri, uri_ptr, strlen(uri_ptr) + 1);
            printf("after get_header_request_uri uri:%s\n", session_params_ptr->session_info->hd_request.uri);
            return;
        }
    }

    session_params_ptr->session_info->response_status.is_header_illegal = true;
}



void get_header_request_host(struct SessionRunParams * session_params_ptr)
{
    int index;
    char (* all_header_line) [1024];
    // TODO 1024 写死了
    char tmp_ch[1024];

    all_header_line = session_params_ptr->session_info->hd_request.all_header_line;

    // TODO 100写死了
    for (index = 0; index < 100; index++)
        if (str_startwith(all_header_line[index], "Host:") )
            break;

    if (index == 100)
    {
        session_params_ptr->session_info->response_status.is_header_illegal = true;
        return;
    }
    else
        get_header_value_by_key(all_header_line[index], "Host", session_params_ptr->session_info->hd_request.host);

}


void get_header_request_ua(struct SessionRunParams * session_params_ptr)
{
    int index;
    char (* all_header_line) [1024];
    // TODO 1024 写死了
    char tmp_ch[1024];

    all_header_line = session_params_ptr->session_info->hd_request.all_header_line;

    // TODO 100写死了
    for (index = 0; index < 100; index++)
        if (str_startwith(all_header_line[index], "User-Agent:") )
            break;

    if (index == 100)
        memcpy(session_params_ptr->session_info->hd_request.user_agent, HEADER_VALUE_NOT_AVAIABLE, strlen(HEADER_VALUE_NOT_AVAIABLE) + 1);
    else
        get_header_value_by_key(all_header_line[index], "User-Agent", session_params_ptr->session_info->hd_request.user_agent);

    printf("session_params_ptr->session_info->hd_request.user_agent:%s\n", session_params_ptr->session_info->hd_request.user_agent);
}



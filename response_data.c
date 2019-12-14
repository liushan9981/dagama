//
// Created by liushan on 19-12-14.
//

#include "response_data.h"


#include <unistd.h>
#include "session.h"



void process_request_response_500(struct SessionRunParams *session_params_ptr)
{
    ssize_t * read_buffer_size;
    char * send_buffer, * send_buffer_500;

    send_buffer = session_params_ptr->session_info->response_data.send_buffer;
    send_buffer_500 = session_params_ptr->session_info->response_data.fallback_500_data_buf;
    read_buffer_size = &(session_params_ptr->session_info->response_data.read_buffer_size);
    *read_buffer_size = strlen(send_buffer_500);
    memcpy(send_buffer, send_buffer_500, *read_buffer_size);

    session_params_ptr->session_info->send_buffer_size = *read_buffer_size;
    session_params_ptr->session_info->send_buffer = send_buffer;

    write_response_data(session_params_ptr);
    process_request_fin_response(session_params_ptr);
}


void process_request_fin_response(struct SessionRunParams *session_params_ptr)
{
    char log_level_notice[] = LOG_LEVEL_NOTICE;

    get_access_log(session_params_ptr);
    write_log(log_level_notice, session_params_ptr->hostvar->log_level_host, session_params_ptr->conn_info.log_msg,
              session_params_ptr->hostvar->f_host_log);

    if (session_params_ptr->session_info->sessionRShutdown == SESSION_RSHUTDOWN)
        // 读取输入时，已经处理recv_buf
        session_close(session_params_ptr);
    else
    {
        // 已经发送完毕，保持连接，等待下一个事务
        session_fin_transaction(session_params_ptr->session_info);
    }


}



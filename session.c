//
// Created by liushan on 19-12-14.
//

#include "session.h"

#include <unistd.h>


void init_session(struct sessionInfo * connSessionInfo)
{
    connSessionInfo->recv_buf = NULL;
    connSessionInfo->localFileFd = -2;
    connSessionInfo->sessionStatus = SESSION_READ_HEADER;
    connSessionInfo->sessionRShutdown = SESSION_RNSHUTDOWN;
    connSessionInfo->sessionRcvData = SESSION_DATA_HANDLED;
    connSessionInfo->response_status.is_header_illegal = false;
    connSessionInfo->response_status.is_method_allowd = true;

    connSessionInfo->upstream_is_fastcgi = false;
    connSessionInfo->upstream_is_local_http = false;
    connSessionInfo->upstream_is_proxy_http = false;

    memset(connSessionInfo->request_file, 0, sizeof(char) * PATH_MAX);
    connSessionInfo->hd_response.status = 0;
    connSessionInfo->redirect_count = 0;
    connSessionInfo->response_data.buffer_size = 4096;
    connSessionInfo->response_bytes = 0;
    memset(connSessionInfo->header_buf, 0, (size_t) MAX_HEADER_RESPONSE_SIZE);
}

void session_fin_transaction(struct sessionInfo * connSessionInfo)
{
    close(connSessionInfo->localFileFd);
    connSessionInfo->localFileFd = -2;
    connSessionInfo->sessionStatus = SESSION_READ_HEADER;
    connSessionInfo->response_status.is_header_illegal = false;
    connSessionInfo->response_status.is_method_allowd = true;
    connSessionInfo->upstream_is_fastcgi = false;
    connSessionInfo->upstream_is_local_http = false;
    connSessionInfo->upstream_is_proxy_http = false;
    memset(connSessionInfo->request_file, 0, sizeof(char) * PATH_MAX);
    connSessionInfo->hd_response.status = 0;
    connSessionInfo->redirect_count = 0;
    connSessionInfo->response_bytes = 0;
    memset(connSessionInfo->header_buf, 0, (size_t) MAX_HEADER_RESPONSE_SIZE);
}


void session_close(struct SessionRunParams * session_params_ptr)
{
    close(session_params_ptr->session_info->connFd);
    printf("closed connFd: %d\n", session_params_ptr->session_info->connFd);
    if (session_params_ptr->session_info->localFileFd > 0)
    {
        close(session_params_ptr->session_info->localFileFd);
        session_params_ptr->session_info->localFileFd = -2;
        printf("closed localFileFd: %d\n", session_params_ptr->session_info->localFileFd);
    }

    if (session_params_ptr->session_info->recv_buf != NULL)
    {
        free(session_params_ptr->session_info->recv_buf);
        session_params_ptr->session_info->recv_buf = NULL;
        printf("have freed run_params->session_info->recv_buf\n");
    }
    if (session_params_ptr->session_info != NULL)
    {
        free(session_params_ptr->session_info);
        session_params_ptr->session_info = NULL;
        printf("have freed run_params->session_info\n");
    }
}


void new_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd)
{
    struct sessionInfo * connSessionInfos;

    memset(&(run_params_ptr->event), 0, sizeof(run_params_ptr->event));

    run_params_ptr->event.data.fd = connfd;
    run_params_ptr->event.events = EPOLLIN;
    event_add(&(run_params_ptr->event), run_params_ptr->epoll_fd, connfd);

    connSessionInfos = malloc(sizeof(struct sessionInfo));
    init_session(connSessionInfos);

    connSessionInfos->recv_buf = malloc(sizeof(char) * MAX_EPOLL_SIZE);
    memset(connSessionInfos->recv_buf, 0, sizeof(char) * MAX_EPOLL_SIZE );
    printf("receive conn:%d\n", connfd);

    connSessionInfos->connFd = connfd;
    // connSessionInfos->connTransactions = 0;
    session_params_ptr[connfd].session_info = connSessionInfos;
}


void new_http_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd, struct sockaddr_in * client_sockaddr)
{
    new_session(session_params_ptr, run_params_ptr, connfd);

    session_params_ptr[connfd].session_info->is_https = false;
    session_params_ptr[connfd].session_info->https_ssl_have_conned = false;
    session_params_ptr[connfd].hostvar = run_params_ptr->hostvar;
    session_params_ptr[connfd].client_sockaddr = client_sockaddr;
}


void new_https_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd, struct sockaddr_in * client_sockaddr, SSL_CTX * ctx)
{
    new_session(session_params_ptr, run_params_ptr, connfd);

    session_params_ptr[connfd].session_info->is_https = true;
    session_params_ptr[connfd].session_info->https_ssl_have_conned = false;
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
        if (session_params_ptr[connfd].session_info->https_ssl_have_conned)
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
        session_params_ptr[connfd].session_info->https_ssl_have_conned = true;
        session_params_ptr[connfd].session_info->ssl = ssl;
    }
}


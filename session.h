//
// Created by liushan on 19-12-14.
//

#ifndef DAGAMA_SESSION_H
#define DAGAMA_SESSION_H

#include "main.h"

void init_session(struct sessionInfo * connSessionInfo);
void session_fin_transaction(struct sessionInfo * connSessionInfo);
void session_close(struct SessionRunParams *session_params_ptr);
void new_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd);
void new_http_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd, struct sockaddr_in * client_sockaddr);
void new_https_session(struct SessionRunParams *session_params_ptr, struct ParamsRun * run_params_ptr, int connfd, struct sockaddr_in * client_sockaddr, SSL_CTX * ctx);
int accept_session(int listen_fd, unsigned int * cli_len);
void new_ssl_session(struct SessionRunParams * session_params_ptr, int connfd);
void init_localFileFd(struct sessionInfo * connSessionInfo);


#endif //DAGAMA_SESSION_H

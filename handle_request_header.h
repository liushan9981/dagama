//
// Created by liushan on 19-12-7.
//

#ifndef DAGAMA_HANDLE_HEADER_H
#define DAGAMA_HANDLE_HEADER_H


#include "main.h"

void get_header_value_by_key(const char *header, const char *header_key, char *header_value);
void get_header_request_all_line(struct SessionRunParams * session_params_ptr);

void get_header_request_method(struct SessionRunParams * session_params_ptr);
void get_header_request_uri(struct SessionRunParams * session_params_ptr);
void get_header_request_host(struct SessionRunParams * session_params_ptr);
void get_header_request_ua(struct SessionRunParams * session_params_ptr);


#endif //DAGAMA_HANDLE_HEADER_H

//
// Created by liushan on 19-12-8.
//

#ifndef DAGAMA_RESPONSE_HEADER_H
#define DAGAMA_RESPONSE_HEADER_H

#include "main.h"

void get_contenttype_by_filepath(char * filepath, struct mimedict mimebook [], int mimebook_len,
                                 struct response_header * header_resonse);
void get_response_header_get(struct SessionRunParams * session_params_ptr);
void get_upstream_type(struct SessionRunParams * session_params_ptr);
void set_default_header_response(struct SessionRunParams * session_params_ptr);
void merge_response_header(struct SessionRunParams * session_params_ptr);
void get_local_http_response_header(struct SessionRunParams * session_params_ptr);
bool get_response_header_check_method_is_allowed(struct SessionRunParams * session_params_ptr);
bool get_response_header_check_client_is_allowed(struct SessionRunParams * session_params_ptr);

#endif //DAGAMA_RESPONSE_HEADER_H

//
// Created by liushan on 19-12-8.
//

#ifndef DAGAMA_RESPONSE_HEADER_H
#define DAGAMA_RESPONSE_HEADER_H

#include "main.h"

void get_contenttype_by_filepath(char * filepath, struct mimedict mimebook [], int mimebook_len,
                                 struct response_header * header_resonse);
void get_upstream_type(struct SessionRunParams * session_params_ptr);
void set_default_header_response(struct SessionRunParams * session_params_ptr);
void merge_response_header(struct SessionRunParams * session_params_ptr);
void get_local_http_response_header(struct SessionRunParams * session_params_ptr);
bool get_response_header_check_method_is_allowed(struct SessionRunParams * session_params_ptr);
bool get_response_header_check_client_is_allowed(struct SessionRunParams * session_params_ptr);
void open_request_file(struct SessionRunParams * session_params_ptr);
bool get_index_file(struct SessionRunParams * session_params_ptr);
bool check_file_path_accessible(char * file_path);
void parse_header_request(struct SessionRunParams * session_params_ptr);
bool is_response_status_set(struct SessionRunParams * session_params_ptr);
#endif //DAGAMA_RESPONSE_HEADER_H

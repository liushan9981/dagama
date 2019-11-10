//
// Created by liushan on 19-11-9.
//

#ifndef DAGAMA_HANDLE_CONFIG_H
#define DAGAMA_HANDLE_CONFIG_H

#include "main.h"

int get_config_host_num(void);
void init_config(struct hostVar * , int);
void init_host_run_params(struct RunParams * [], struct hostVar * [], int);
void  get_mimebook(const char * mime_file, struct mimedict mimebook [], int mimebook_len);
#endif //DAGAMA_HANDLE_CONFIG_H

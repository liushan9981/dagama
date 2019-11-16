//
// Created by liushan on 19-11-9.
//

#ifndef DAGAMA_HANDLE_CONFIG_H
#define DAGAMA_HANDLE_CONFIG_H

#include "main.h"

#define CONFIG_FILE_KEY_HOST "host:"
#define CONFIG_FILE_HOST_MAX_KEY_NUM 1024
#define CONFIG_FILE_LINE_MAX_SIZE 4096
#define CONFIG_FILE_LINE_COMMENT_PREFIX "#"
#define CONFIG_FILE_CUTOFF_STR "---"
#define CONFIG_FILE_LINE_KV_SPLIT_CH ':'

struct config_key_value {
    char key[128];
    char value[4096];
};


struct config_host_kv {
    struct config_key_value config_kv[CONFIG_FILE_HOST_MAX_KEY_NUM];
    int current_key_num;
};


struct config_all_host_kv {
    struct config_host_kv * host_config_kv;
    int host_num;
    FILE * f_config;
};


void get_config_host_num(struct config_all_host_kv * all_host_kv);
void init_config(struct hostVar * , int);
void init_host_run_params(struct RunParams * [], struct hostVar * [], int);
void  get_mimebook(const char * mime_file, struct mimedict mimebook [], int mimebook_len);
void get_all_host_config(struct config_all_host_kv * all_host_kv);
#endif //DAGAMA_HANDLE_CONFIG_H

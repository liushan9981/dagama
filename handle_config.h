//
// Created by liushan on 19-11-9.
//

#ifndef DAGAMA_HANDLE_CONFIG_H
#define DAGAMA_HANDLE_CONFIG_H

#include "main.h"


#define CONFIG_FILE_HOST_MAX_KEY_NUM 1024
#define CONFIG_FILE_LINE_MAX_SIZE 4096
#define CONFIG_FILE_LINE_COMMENT_PREFIX "#"
#define CONFIG_FILE_CUTOFF_STR "---"
#define CONFIG_FILE_LINE_KV_SPLIT_CH ':'
#define CONFIG_FILE_ITEM_SPLIT_CH ','

#define CONFIG_FILE_KEY_HOST            "host"
#define CONFIG_FILE_KEY_DOCROOT         "doc_root"
#define CONFIG_FILE_KEY_MIMEFILE        "mime_file"
#define CONFIG_FILE_KEY_METHOD_ALLOWED  "method_allowed"
#define CONFIG_FILE_KEY_INDEX_FILE      "index"
#define CONFIG_FILE_KEY_403PAGE         "403_page"
#define CONFIG_FILE_KEY_404PAGE         "404_page"
#define CONFIG_FILE_KEY_405PAGE         "405_page"
#define CONFIG_FILE_KEY_HTTPS           "https"
#define CONFIG_FILE_KEY_SSLCERT         "ssl_cert"
#define CONFIG_FILE_KEY_SSLKEY          "ssl_key"
#define CONFIG_FILE_KEY_UPSTREAM_TYPE   "upstream_type"
#define CONFIG_FILE_KEY_UPSTREAM        "upstream"


#define CONFIG_FILE_VALUE_FLAG_ON                   "on"
#define CONFIG_FILE_VALUE_FLAG_OFF                  "off"
#define CONFIG_FILE_VALUE_NA                        "N/A"
#define CONFIG_FILE_VALUE_UPSTREAM_TYPE_LOCAL       "local"
#define CONFIG_FILE_VALUE_UPSTREAM_TYPE_FASTCGI     "fastcgi"
#define CONFIG_FILE_VALUE_UPSTREAM_TYPE_HTTP        "http"



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
    char config_file_path[PATH_MAX];
};

// TODO 相对路径，转为绝对路径
static struct config_host_kv defalt_config_host_kv = {
        .config_kv[0].key = "host",
        .config_kv[0].value = "localhost",

        .config_kv[1].key = "doc_root",
        .config_kv[1].value = "./webroot",

        .config_kv[2].key = "mime_file",
        .config_kv[2].value = "/home/liushan/mylab/clang/dagama/conf/mime.types",

        .config_kv[3].key = "method_allowed",
        .config_kv[3].value = "GET, DELETE, POST",

        .config_kv[4].key = "index",
        .config_kv[4].value = "index.html, index.htm",

        .config_kv[5].key = "403_page",
        .config_kv[5].value = "./webroot/html/403.html",

        .config_kv[6].key = "404_page",
        .config_kv[6].value = "./webroot/html/404.html",

        .config_kv[7].key = "405_page",
        .config_kv[7].value = "./webroot/html/405.html",

        .config_kv[8].key = "https",
        .config_kv[8].value = "off",

        .config_kv[9].key = "ssl_cert",
        .config_kv[9].value = CONFIG_FILE_VALUE_NA,

        .config_kv[10].key = "ssl_key",
        .config_kv[10].value = CONFIG_FILE_VALUE_NA,

        .config_kv[11].key = "upstream_type",
        .config_kv[11].value = CONFIG_FILE_VALUE_UPSTREAM_TYPE_LOCAL,

        .config_kv[12].key = "upstream",
        .config_kv[12].value = CONFIG_FILE_VALUE_NA,

        .current_key_num = 13
};


bool get_value_by_key(struct config_key_value * config_kv_ptr, char * * ch, int num);
int get_config_host_num(const char *);
void init_config(struct hostVar * host_var_ptr, char * config_file_path);
void init_host_run_params(struct RunParams * [], struct hostVar * [], int);
void  get_mimebook(const char * mime_file, struct mimedict mimebook [], int mimebook_len);
void get_all_host_config(struct config_all_host_kv * all_host_kv);
void set_host_config(struct config_all_host_kv * all_host_kv, struct hostVar *);
#endif //DAGAMA_HANDLE_CONFIG_H

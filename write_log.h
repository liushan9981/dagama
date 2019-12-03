//
// Created by liushan on 19-11-28.
//

#ifndef DAGAMA_WRITE_LOG_H
#define DAGAMA_WRITE_LOG_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define CUR_DATE_LEN 25
#define LOG_FORMAT "%-23s  [%s]  %s\n"
#define LOG_LEVEL_MAX_LEN  16
#define LOG_LEVEL_INFO     "INFO"
#define LOG_LEVEL_NOTICE   "NOTICE"
#define LOG_LEVEL_WARN     "WARN"
#define LOG_LEVEL_ERROR    "ERROR"


// 日志名称和日志级别，级别为整数值，值从低到高表示严重级别从低到高
struct log_grade {
    char level_name[10];
    int grade;
};

// 定义日志级别，4个级别
struct log_level {
    struct log_grade info;
    struct log_grade notice;
    struct log_grade warn;
    struct log_grade error;
};

//　４个日志级别的名称和值
static struct log_level log_write_level = {
        .info = {LOG_LEVEL_INFO, 0},
        .notice = {LOG_LEVEL_NOTICE, 1},
        .warn = {LOG_LEVEL_WARN, 2},
        .error = {LOG_LEVEL_ERROR, 3}
};

void write_log(const char * cur_log_level, const char * log_level, const char * log_msg, FILE * f_log_file);
void write_log_get_cur_date(char *cur_date);

#endif //DAGAMA_WRITE_LOG_H

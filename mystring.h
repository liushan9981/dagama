//
// Created by liushan on 18-8-15.
//

#include <stdbool.h>

#ifndef DAGAMA_TOOL_H
#define DAGAMA_TOOL_H

#define MAX_STR_SPLIT_SIZE 4096


bool str_endwith(const char * str, const char * cmp_str);
bool str_startwith(const char * str, const char * cmp_str);
bool str_isspace(const char * str);

// 返回截断的空白字符数
int str_rstrip(char * str);
int str_lstrip(char * str);
int str_strip(char * str);


// 字符串分割成数组
// 例如：　10.10.10.10, 10.10.10.2 分割为["10.10.10.10", "10.10.10.2"]
// 返回生成的数组长度
int str_split(const char *str_ori, char ch_split, char str_tgt[][MAX_STR_SPLIT_SIZE], int ch_num);


#endif //DAGAMA_TOOL_H

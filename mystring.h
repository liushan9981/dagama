//
// Created by liushan on 18-8-15.
//

#include <stdbool.h>

#ifndef DAGAMA_TOOL_H
#define DAGAMA_TOOL_H

bool str_endwith(char * str, char * cmp_str);


// 字符串分割成数组
// 例如：　10.10.10.10, 10.10.10.2 分割为["10.10.10.10", "10.10.10.2"]
// 返回生成的数组长度
int split_str_by_ch(const char *str_ori, int str_ori_len, char ch_split, char str_tgt[][1024], int ch_num);


#endif //DAGAMA_TOOL_H

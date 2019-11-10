//
// Created by liushan on 18-8-15.
//

#include "mystring.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>



bool str_endwith(char * str, char * cmp_str)
{
    char * temp;

    if (strlen(cmp_str) > strlen(str) )
        return false;

    temp = str + strlen(str) - strlen(cmp_str);

    return (strcmp(temp, cmp_str) == 0);
}


int split_str_by_ch(const char *str_ori, int str_ori_len, char ch_split, char str_tgt[][1024], int ch_num)
{
    int str_ori_ch_index;
    int str_index, ch_pos;
    bool flag = false;

    for (str_ori_ch_index = 0, str_index = 0, ch_pos = 0; str_ori_ch_index < str_ori_len; str_ori_ch_index++)
    {
        if (str_index >= ch_num)
        {
            str_index = ch_num;
            return str_index;
        }

        if (str_ori[str_ori_ch_index] == ch_split)
        {
            str_tgt[str_index][ch_pos] = '\0';
            str_index++;
            ch_pos = 0;
            flag = true;
        }
        else
        {
            // 刚分割的第一个字符是空格的，忽略
            if (flag)
                if (isblank(str_ori[str_ori_ch_index]) )
                    continue;
            str_tgt[str_index][ch_pos] = str_ori[str_ori_ch_index];
            ch_pos++;
            flag = false;
        }
    }
    str_tgt[str_index][ch_pos] = '\0';
    str_index++;

    return str_index;
}



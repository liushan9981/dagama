//
// Created by liushan on 18-8-15.
//

#include "mystring.h"
#include "myutils.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>


bool str_endwith(const char * str, const char * cmp_str)
{
    const char * temp;

    if (strlen(cmp_str) > strlen(str) )
        return false;

    temp = str + strlen(str) - strlen(cmp_str);

    return (strcmp(temp, cmp_str) == 0);
}


bool str_startwith(const char * str, const char * cmp_str)
{
    return (strncmp(str, cmp_str, strlen(cmp_str) ) == 0);
}


int str_rstrip(char * str)
{
    char * ptr;
    int count;

    for (ptr = str + strlen(str) -1, count = 0; ptr >= str; ptr--, count++)
        if (! isspace(*ptr))
        {
            *(ptr + 1) = '\0';
            break;
        }

    if ( ptr < str)
        str[0] = '\0';

    return count;
}


int str_lstrip(char * str)
{
    char * ptr;
    int count;

    for (ptr = str, count = 0; ptr < str + strlen(str); ptr++, count++)
        if (! isspace(*ptr) )
            break;

    memmove(str, ptr,  str + strlen(str) - ptr);
    str[str + strlen(str) - ptr] = '\0';

    return count;
}


int str_strip(char * str)
{
    int count;

    count = str_lstrip(str);
    count += str_rstrip(str);
    return count;
}


bool str_isspace(const char * str)
{
    char * str_ptr;

    if ( (str_ptr = malloc(sizeof(char) * (strlen(str) + 1) ) ) == NULL)
        err_exit("malloc error\n");

    return (sscanf(str, "%s", str_ptr) <= 0);
}


int str_split(const char *str_ori, char ch_split, char str_tgt[][MAX_STR_SPLIT_SIZE], int ch_num)
{
    int str_ori_ch_index;
    int str_index, ch_pos;
    bool flag = false;

    for (str_ori_ch_index = 0, str_index = 0, ch_pos = 0; str_ori_ch_index < strlen(str_ori); str_ori_ch_index++)
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



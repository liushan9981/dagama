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


char * str_join(char * ch_split, int split_len,
                char str_ori[][MAX_STR_SPLIT_SIZE], int ch_num, char * str_tgt, int str_tgt_len)
{
    long sum_len;
    int str_ori_index;

    sum_len = (ch_num * MAX_STR_SPLIT_SIZE) - ch_num + 1 + (ch_num - 1) * split_len;
    if (sum_len < str_tgt_len)
        return NULL;

    str_tgt[0] = '\0';
    for (str_ori_index = 1; str_ori_index < ch_num; str_ori_index++)
    {
        if (strlen(str_ori[str_ori_index]) > 0)
        {
            strncat(str_tgt, str_ori[str_ori_index], strlen(str_ori[str_ori_index]) );
            strncat(str_tgt, ch_split, strlen(ch_split) );
        }
    }

    str_tgt[strlen(str_tgt) - strlen(ch_split)] = '\0';

    return str_tgt;
}


int str_split(const char *str_ori, char ch_split, char str_tgt[][MAX_STR_SPLIT_SIZE], int ch_num)
{
    int str_ori_ch_index;
    int str_index, ch_pos;

    memset(str_tgt, 0, sizeof(char) *MAX_STR_SPLIT_SIZE * ch_num);

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
        }
        else
        {
            str_tgt[str_index][ch_pos] = str_ori[str_ori_ch_index];
            ch_pos++;
        }
    }
    str_tgt[str_index][ch_pos] = '\0';
    str_index++;

    return str_index;
}


// 优化分割字符串，新版本
int str_split2(const char *str_ori, char ch_split, char * * str_tgt, int len1, int len2)
{
    const char * ch_start, * ch_end;
    int copy_len, len1_index;
    const int illegal_res = -1;

    for (len1_index = 0, ch_start = ch_end = str_ori; ;len1_index++)
    {
        if ( (len1_index + 1) > len1)
            return illegal_res;

        while (*ch_end != ch_split && *ch_end != '\0')
            ch_end++;

        if ( (copy_len = ch_end - ch_start) > 0)
        {
            if ( (copy_len + 1) > len2)
                return illegal_res;

            strncpy(str_tgt[len1], ch_start, copy_len);
            str_tgt[len1][copy_len] = '\0';
            ch_start = ch_end;
        }
    }

}
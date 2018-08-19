//
// Created by liushan on 18-8-15.
//

#include "tool.h"
#include <string.h>
#include <stdio.h>



bool str_endwith(char * str, char * cmp_str)
{
    char * temp;

    if (strlen(cmp_str) > strlen(str) )
        return false;

    temp = str + strlen(str) - strlen(cmp_str);

    return (strcmp(temp, cmp_str) == 0);
}
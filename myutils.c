//
// Created by liushan on 19-11-16.
//

#include "myutils.h"

#include <stdio.h>
#include <stdlib.h>

void err_exit(const char * str)
{
    fprintf(stderr, "%s\n", str);
    exit(2);
}
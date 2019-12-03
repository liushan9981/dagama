//
// Created by liushan on 19-11-28.
//

#include "write_log.h"

int get_log_grade_by_level(const char * level_name);

int get_log_grade_by_level(const char * cur_log_level)
{
    int cur_level_grade = -1;

    // 获取最小记录日志名称对应的级别
    if (strcmp(cur_log_level, log_write_level.info.level_name) == 0)
        cur_level_grade = log_write_level.info.grade;
    else if (strcmp(cur_log_level, log_write_level.notice.level_name) == 0)
        cur_level_grade = log_write_level.notice.grade;
    else if (strcmp(cur_log_level, log_write_level.warn.level_name) == 0)
        cur_level_grade = log_write_level.warn.grade;
    else if (strcmp(cur_log_level, log_write_level.error.level_name) == 0)
        cur_level_grade = log_write_level.error.grade;

    return cur_level_grade;
}


void write_log(const char * cur_log_level, const char * log_level, const char * log_msg, FILE * f_log_file)
{
    char cur_date[CUR_DATE_LEN]; // 时间字符串
    int cur_level_grade, level_grade; //记录日志的最小级别

    if ( (cur_level_grade = get_log_grade_by_level(cur_log_level) ) < 0)
    {
        printf("error! log_level_para \"%s\" is not legal\n", cur_log_level);
        exit(EXIT_FAILURE);
    }

    if ( (level_grade = get_log_grade_by_level(log_level) ) < 0)
    {
        printf("error! log_level_para \"%s\" is not legal\n", log_level);
        exit(EXIT_FAILURE);
    }

    // 记录日志级别大于等于最小日志记录级别时，则记录日志
    if (cur_level_grade >= level_grade)
    {
        write_log_get_cur_date(cur_date); //获取当前时间
        // 时间字符串，保留23位，写死的
        fprintf(f_log_file, LOG_FORMAT, cur_date, cur_log_level, log_msg);
        fflush(f_log_file);
    }
}


void write_log_get_cur_date(char *cur_date)
{
    char * wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    time_t timep;
    struct tm * p;

    time(&timep);
    p = localtime(&timep);
    sprintf(cur_date, "%d/%d/%d %s %d:%d:%d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
            wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);

}





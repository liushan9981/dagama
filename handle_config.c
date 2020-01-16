//
// Created by liushan on 19-11-9.
//


#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "main.h"
#include "myutils.h"
#include "handle_config.h"
#include "mystring.h"


extern struct ParamsRun run_params;


bool get_value_by_key(struct config_key_value * config_kv_ptr, char * * ch, int num)
{
    int index;

    for (index = 0; index < num; index++)
        if (strcmp(config_kv_ptr[index].key, *ch) == 0)
        {
            *ch = config_kv_ptr[index].value;
            return true;
        }

    return false;
}



void  get_mimebook(const char * mime_file, struct mimedict mimebook [], int mimebook_len)
{
    FILE * fd_mime;
    const int buffersize = 4096;
    char line[buffersize];
    char content_type_temp[128];
    char extension_temp[EXTENSION_NAME_LENTH];
    int index_mimebook = 0, index_line_ch, index_extension;
    bool flag_content_type = true, flag_extension = false;

    if ( (fd_mime = fopen(mime_file, "r") ) == NULL)
    {
        printf("open %s failed\n", mime_file);
        exit(EXIT_FAILURE);
    }

    while (fgets(line, buffersize - 1, fd_mime) != NULL)
    {
        // 每行第一个字符不是字母或者数字的，跳过
        if (! isalnum(line[0]) )
            continue;

        // 每行，循环读取每个字符
        for (index_line_ch = 0, index_extension = 0; index_line_ch < strlen(line); index_line_ch++)
        {
            // flag_content_type开关开启时，读取的非空字符存入content_type_temp
            if (flag_content_type)
            {
                // 读到空白字符后，flag_content_type开关关闭
                if (isblank(line[index_line_ch]) )
                {
                    content_type_temp[index_line_ch] = '\0';
                    flag_content_type = false;
                }
                else
                {
                    content_type_temp[index_line_ch] = line[index_line_ch];
                }
            }
            else
            {
                // flag_extension关闭时，读取到空行，跳过
                // flag_extension开启时，读取到空行，进行赋值操作
                if (isblank(line[index_line_ch]) || line[index_line_ch] == ';')
                {
                    if (! flag_extension)
                        continue;
                    extension_temp[index_extension] = '\0';
                    if (index_mimebook >= mimebook_len)
                    {
                        fprintf(stderr, "mimefile count is more than mimebook_len, now break!\n");
                        fclose(fd_mime);
                        return;
                    }
                    strcpy(mimebook[index_mimebook].content_type, content_type_temp);
                    strcpy(mimebook[index_mimebook].extension, extension_temp);
                    index_mimebook++;
                    flag_extension = false;
                    index_extension = 0;
                    if (line[index_line_ch] == ';')
                    {
                        flag_content_type = true;
                        break;
                    }

                }
                else
                {
                    extension_temp[index_extension] = line[index_line_ch];
                    index_extension++;
                    flag_extension = true;
                }

            }

        }

    }


    fclose(fd_mime);
}


int get_config_host_num(const char * config_f_path)
{
    char line[CONFIG_FILE_LINE_MAX_SIZE];
    char host_key_pre[CONFIG_FILE_LINE_MAX_SIZE];
    FILE * f;
    int host_num_count = 0;

    strncpy(host_key_pre, CONFIG_FILE_KEY_HOST, CONFIG_FILE_LINE_MAX_SIZE);
    strncat(host_key_pre, ":", CONFIG_FILE_LINE_MAX_SIZE - strlen(host_key_pre) );

    if ( (f = fopen(config_f_path, "r") ) == NULL)
        err_exit("can't open config file\n");

    while (fgets(line, CONFIG_FILE_LINE_MAX_SIZE, f) != NULL)
        if (str_startwith(line, CONFIG_FILE_KEY_HOST) )
            host_num_count++;

    fclose(f);
    return host_num_count;
}


void get_all_host_config(struct config_all_host_kv * all_host_kv)
{
    char line[CONFIG_FILE_LINE_MAX_SIZE];
    int host_index = 0, host_key_index = 0, splited_num;
    char temp_key_value[128][MAX_STR_SPLIT_SIZE];
    FILE * f;

    if ( (f = fopen(all_host_kv->config_file_path, "r") ) == NULL)
        err_exit("can't open config file\n");

    while (fgets(line, CONFIG_FILE_LINE_MAX_SIZE, f) != NULL)
    {
        if (str_startwith(line, CONFIG_FILE_LINE_COMMENT_PREFIX) || str_isspace(line) )
            continue;

        if (str_startwith(line, CONFIG_FILE_CUTOFF_STR) )
        {
            host_key_index = 0;
            host_index++;
            continue;
        }

        splited_num = str_split(line, CONFIG_FILE_LINE_KV_SPLIT_CH, temp_key_value, 128);
        strncpy(all_host_kv->host_config_kv[host_index].config_kv[host_key_index].key,
                temp_key_value[0], strlen(temp_key_value[0]) + 1);
        if (splited_num == 2)
            strncpy(all_host_kv->host_config_kv[host_index].config_kv[host_key_index].value,
                    temp_key_value[1], strlen(temp_key_value[1]) + 1);
        else if (splited_num > 2)
            strncpy(all_host_kv->host_config_kv[host_index].config_kv[host_key_index].value,
                    line + strlen(temp_key_value[0]) + 1, CONFIG_FILE_LINE_MAX_SIZE);
        else
        {
            fprintf(stderr, "parse config file error, line: %s\n", line);
            err_exit("now exit\n");
        }

        host_key_index++;
        all_host_kv->host_config_kv[host_index].current_key_num = host_key_index;
    }
}



void get_or_open_file(struct FileOpenBook  * file_open_book, const char * file_path, FILE ** f)
{
    int index;

    for (index = 0; index < CONFIG_FILE_HOST_MAX_KEY_NUM; index++)
        if (strcmp(file_open_book[index].file_path, file_path) == 0)
        {
            *f = file_open_book[index].f;
            return;
        }

    if ( (*f = fopen(file_path, "a") ) != NULL)
        for (index = 0; index < CONFIG_FILE_HOST_MAX_KEY_NUM; index++)
            if (strlen(file_open_book[index].file_path) == 0)
            {
                strncpy(file_open_book[index].file_path, file_path, PATH_MAX);
                file_open_book[index].f = *f;
                return;
            }
}


void init_open_file_book(struct FileOpenBook file_open_book [])
{
    int index;

    for (index = 0; index < CONFIG_FILE_HOST_MAX_KEY_NUM; index++)
    {
         file_open_book[index].file_path[0] = '\0';
         file_open_book[index].f = NULL;
    }
}


void set_host_config(struct config_all_host_kv * all_host_kv, struct hostVar * host_var_ptr)
{
    int tmp_index_host, tmp_split_index;
    struct hostVar * cur_host_var_ptr;
    struct config_host_kv * cur_config_host_kv_ptr;
    char temp_splited_value[1024][MAX_STR_SPLIT_SIZE];
    char * tmp_str, * ori_ptr;
    struct FileOpenBook * file_open_book;

    file_open_book = malloc(sizeof(struct FileOpenBook) * CONFIG_FILE_HOST_MAX_KEY_NUM);
    init_open_file_book(file_open_book);

    ori_ptr = malloc(sizeof(char) * CONFIG_FILE_LINE_MAX_SIZE);
    tmp_str = ori_ptr;

    for (tmp_index_host = 0; tmp_index_host < all_host_kv->host_num; tmp_index_host++)
    {

        cur_host_var_ptr = host_var_ptr + tmp_index_host;
        cur_config_host_kv_ptr = all_host_kv->host_config_kv + tmp_index_host;

        strncpy(tmp_str, CONFIG_FILE_KEY_HOST, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->host, tmp_str, PATH_MAX);
        str_strip(cur_host_var_ptr->host);

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_DOCROOT, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->doc_root, tmp_str, PATH_MAX);
        str_strip(cur_host_var_ptr->doc_root);

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_MIMEFILE, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        str_strip(tmp_str);
        printf("mime file: %s\n", tmp_str);
        get_mimebook(tmp_str, cur_host_var_ptr->mimebook, MAX_MIMEBOOK_SIZE);

        printf("mimebook %s %s\n", cur_host_var_ptr->mimebook[2].extension,cur_host_var_ptr->mimebook[2].content_type);

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_METHOD_ALLOWED, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);

        cur_host_var_ptr->method_allowed_len = str_split(tmp_str, CONFIG_FILE_ITEM_SPLIT_CH, temp_splited_value, 1024);

        for (tmp_split_index = 0; tmp_split_index < cur_host_var_ptr->method_allowed_len; tmp_split_index++)
        {
            str_strip(temp_splited_value[tmp_split_index]);
            strncpy(cur_host_var_ptr->method_allowed[tmp_split_index], temp_splited_value[tmp_split_index],
                    MAX_HEADER_METHOD_ALLOW_SIZE);
        }

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_INDEX_FILE, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        cur_host_var_ptr->file_indexs_len = str_split(tmp_str, CONFIG_FILE_ITEM_SPLIT_CH, temp_splited_value, 1024);
        for (tmp_split_index = 0; tmp_split_index < cur_host_var_ptr->file_indexs_len; tmp_split_index++)
        {
            str_strip(temp_splited_value[tmp_split_index]);
            strncpy(cur_host_var_ptr->file_indexs[tmp_split_index], temp_splited_value[tmp_split_index], PATH_MAX);
        }

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_403PAGE, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->request_file_403, tmp_str, PATH_MAX);
        str_strip(cur_host_var_ptr->request_file_403);

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_404PAGE, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->request_file_404, tmp_str, PATH_MAX);
        str_strip(cur_host_var_ptr->request_file_404);

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_405PAGE, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->request_file_405, tmp_str, PATH_MAX);
        str_strip(cur_host_var_ptr->request_file_405);

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_LOG_HOST_PATH, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->log_host_path, tmp_str, PATH_MAX);
        str_strip(cur_host_var_ptr->log_host_path);
        printf("log_host_path: %s\n", cur_host_var_ptr->log_host_path);

        get_or_open_file(file_open_book, cur_host_var_ptr->log_host_path, &(cur_host_var_ptr->f_host_log) );
        if ( cur_host_var_ptr->f_host_log == NULL)
        {
            fprintf(stderr, "open log file: %s failed\n", cur_host_var_ptr->log_host_path);
            exit(EXIT_FAILURE);
        }

        tmp_str = ori_ptr;
        strncpy(tmp_str, CONFIG_FILE_KEY_LOG_LEVEL, CONFIG_FILE_LINE_MAX_SIZE);
        if (! get_value_by_key(cur_config_host_kv_ptr->config_kv, &tmp_str, cur_config_host_kv_ptr->current_key_num) )
            get_value_by_key(defalt_config_host_kv.config_kv, &tmp_str, defalt_config_host_kv.current_key_num);
        strncpy(cur_host_var_ptr->log_level_host, tmp_str, LOG_LEVEL_MAX_LEN);
        str_strip(cur_host_var_ptr->log_level_host);
        printf("log_level_host: %s\n", cur_host_var_ptr->log_level_host);

    }

}



void init_request_file_open_book(void)
{
    int index;
    struct RequestFileOpenBook * request_file_open_book_ptr;

    request_file_open_book_ptr = run_params.request_file_open_book;

    for (index = 0; index < MAX_EPOLL_SIZE; index++)
    {
        request_file_open_book_ptr[index].fd = -1;
        request_file_open_book_ptr[index].file_size = -1;
        request_file_open_book_ptr[index].reference_count = 0;
        request_file_open_book_ptr[index].myerrno = 0;
    }


}


void init_config(struct hostVar * host_var_ptr, char * config_f_path)
{
    struct config_all_host_kv all_host_kv;


    strncpy(all_host_kv.config_file_path, config_f_path, PATH_MAX);
    all_host_kv.host_num = get_config_host_num(config_f_path);
    all_host_kv.host_config_kv = (struct config_host_kv *) malloc(sizeof(struct config_host_kv) * all_host_kv.host_num);
    get_all_host_config(&all_host_kv);
    set_host_config(&all_host_kv, host_var_ptr);
    init_request_file_open_book();

    int index;

    for (index = 0; index < 3; index++)
    {
        printf("debug %d: %d\n", index, run_params.request_file_open_book[index].fd);
    }


    // exit(EXIT_SUCCESS);

    // 调试用，打印出来所有配置的值
//    int index, index2;
//    printf("----------------\n");
//    for (index = 0; index < 3; index++)
//    {
//        printf("host: %s\n", host_var_ptr[index].host);
//        printf("doc root: %s\n", host_var_ptr[index].doc_root);
//        printf("mimebook: %s %s\n", host_var_ptr[index].mimebook[5].extension,
//                host_var_ptr[index].mimebook[5].content_type);
//        for (index2 = 0; index2 < host_var_ptr[index].file_indexs_len; index2++)
//        {
//            printf("index:%s\n",host_var_ptr[index].file_indexs[index2]);
//        }
//
//        for (index2 = 0; index2 < host_var_ptr[index].method_allowed_len; index2++)
//            printf("method allowed:%s\n", host_var_ptr[index].method_allowed[index2]);
//        printf("403:%s\n", host_var_ptr[index].request_file_403);
//        printf("404:%s\n", host_var_ptr[index].request_file_404);
//        printf("405:%s\n", host_var_ptr[index].request_file_405);
//        printf("*****\n");
//
//    }

    // exit(EXIT_SUCCESS);

}



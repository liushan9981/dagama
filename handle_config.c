//
// Created by liushan on 19-11-9.
//


#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "handle_config.h"




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


int get_config_host_num(void)
{
    return 2;
}




//    // 运行的配置参数
//    char * doc_root = "/home/liushan/mylab/clang/dagama/webroot";
//    // char * doc_root = "/opt/application/nginx/myphp";
//    char * file_index[2] = {"index.html", "index.htm"};
//    char * method_allow[3] = {"GET", "DELETE", "POST"};
//
//    struct DefaultReqFile request_file_default = {
//            .request_file_404 = "/home/liushan/mylab/clang/dagama/webroot/html/404.html",
//            .request_file_403 = "/home/liushan/mylab/clang/dagama/webroot/html/403.html",
//            .request_file_405 = "/home/liushan/mylab/clang/dagama/webroot/html/405.html"
//    };

void init_config(struct hostVar * host_var_ptr, int num)
{
    char mime_file[PATH_MAX] = "/home/liushan/mylab/clang/dagama/mime.types";


    strncpy(host_var_ptr[0].host, "192.168.123.173:8043", PATH_MAX);
    strncpy(host_var_ptr[0].doc_root, "/home/liushan/mylab/clang/dagama/webroot", PATH_MAX);

    strncpy(host_var_ptr[0].file_indexs[0], "index.html", PATH_MAX);
    strncpy(host_var_ptr[0].file_indexs[1], "index.htm", PATH_MAX);
    host_var_ptr[0].file_indexs_len = 2;

    strncpy(host_var_ptr[0].method_allowed[0], "GET", 8);
    strncpy(host_var_ptr[0].method_allowed[1], "DELETE", 8);
    strncpy(host_var_ptr[0].method_allowed[2], "POST", 8);
    host_var_ptr[0].method_allowed_len = 3;

    strncpy(host_var_ptr[0].request_file_403, "/home/liushan/mylab/clang/dagama/webroot/html/403.html", PATH_MAX);
    strncpy(host_var_ptr[0].request_file_404, "/home/liushan/mylab/clang/dagama/webroot/html/404.html", PATH_MAX);
    strncpy(host_var_ptr[0].request_file_405, "/home/liushan/mylab/clang/dagama/webroot/html/405.html", PATH_MAX);


    get_mimebook(mime_file, host_var_ptr[0].mimebook, MAX_MIMEBOOK_SIZE);



    strncpy(host_var_ptr[1].host, "192.168.123.173:8080", PATH_MAX);
    strncpy(host_var_ptr[1].doc_root, "/home/liushan/mylab/clang/dagama/webroot-2", PATH_MAX);

    strncpy(host_var_ptr[1].file_indexs[0], "index.html", PATH_MAX);
    strncpy(host_var_ptr[1].file_indexs[1], "index.htm", PATH_MAX);
    host_var_ptr[1].file_indexs_len = 2;

    strncpy(host_var_ptr[1].method_allowed[0], "GET", 8);
    strncpy(host_var_ptr[1].method_allowed[1], "DELETE", 8);
    strncpy(host_var_ptr[1].method_allowed[2], "POST", 8);
    host_var_ptr[1].method_allowed_len = 3;

    strncpy(host_var_ptr[1].request_file_403, "/home/liushan/mylab/clang/dagama/webroot/html/403.html", PATH_MAX);
    strncpy(host_var_ptr[1].request_file_404, "/home/liushan/mylab/clang/dagama/webroot/html/404.html", PATH_MAX);
    strncpy(host_var_ptr[1].request_file_405, "/home/liushan/mylab/clang/dagama/webroot/html/405.html", PATH_MAX);

    get_mimebook(mime_file, host_var_ptr[1].mimebook, MAX_MIMEBOOK_SIZE);

}



//void init_host_run_params(struct RunParams * host_run_params_ptr[], struct hostVar * host_var[], int num)
//{
//    int index1, index2;
//    struct RunParams * temp_host_run_params_ptr;
//
//    for (index1 = 0; index1 < num; index1++)
//    {
//        temp_host_run_params_ptr = host_run_params_ptr[index1];
//        temp_host_run_params_ptr->hostvar = host_var[index1];
//
//        for (index2 = 0; index2 < MAX_EPOLL_SIZE; index2++)
//        {
//
////            temp_host_run_params_ptr->conninfo->is_https = false;
////            temp_host_run_params_ptr->conninfo->https_ssl_have_conned = false;
////            temp_host_run_params_ptr->conninfo->ssl = NULL;
//
//
////            temp_host_run_params_ptr[index2].default_request_file = &request_file_default;
////            for (index2 = 0; index2 < sizeof(file_index) / sizeof(char *); index2++)
////                run_param[index2].file_index[index2] = file_index[index2];
////
////            for (index2 = 0; index2 < sizeof(method_allow) / sizeof(char *); index2++)
////                run_param[index2].method_allow[index2] = method_allow[index2];
////            run_param->method_allow_len = sizeof(method_allow) / sizeof(char *);
////            run_param[index2].mimebook = mimebook;
////            run_param[index2].mimebook_len = mimebook_len;
////            run_param[index2].doc_root = doc_root;
////            run_param[index2].client_sockaddr = &client_sockaddr;
////
////            run_param[index2].is_https = false;
////            run_param[index2].https_ssl_have_conned = false;
////            run_param[index2].ssl = NULL;
//        }
//    }
//
//
//}
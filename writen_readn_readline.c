#include <stdio.h>
#include <sys/unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>


#define MAXLINE 4096

ssize_t readn(int fd, void * vptr, size_t n)
{
    char * ptr;
    size_t nleft;
    ssize_t nread;

    ptr = vptr;
    nleft = n;

    while (nleft > 0)
    {
        if ( (nread = read(fd, ptr, nleft) ) < 0)
        {
            if (errno == EINTR)
                nread = 0;
            else
                return (-1);
        }
        else if (nread == 0)
            break;

        nleft -= nread;
        ptr += nread;
    }

    // 期望读取n字节，可能读取不到n字节的时候，已经到达EOF
    return (n - nleft);
}



ssize_t writen(int fd, const void * vptr, size_t n)
{
    size_t nleft;
    ssize_t nwriten;
    const char * ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0)
    {
        if ( (nwriten = write(fd, ptr, nleft) ) <= 0)
        {
            if (nwriten < 0 && errno == EINTR)
                nwriten = 0;
            else
                return (-1);
        }
        nleft -= nwriten;
        ptr += nwriten;
    }

    //不存在这种情况： 期望读取n字节，可能读取不到n字节的时候，已经到达EOF
    // 所以直接返回n
    return n;
}

// read_cnt为全局变量，系统会初始化为0
static int read_cnt = 0;
static char * read_ptr;
static char read_buf[MAXLINE];


// 每次调用myread操作一个字符，myread一次读取MAXLINE个字符
static ssize_t myread(int fd, char * ptr)
{
    if (read_cnt <= 0)
    {
        again:
            if ( (read_cnt = read(fd, read_buf, sizeof(read_buf) ) ) < 0)
            {
                if (errno == EINTR)
                    goto again;
                return -1;
            }
            else if (read_cnt == 0)
                return 0;
        // 每次读取后，read_ptr指向read_buf的第一个元素的位置
        read_ptr = read_buf;
    }

    // 每次调用，计数减一
    read_cnt--;
    // 每次调用，read_ptr前移一次
    *ptr = *read_ptr++;
    return 1;
}





ssize_t readline(int fd, void * vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;

    for (n = 1; n < maxlen; n++)
    {
        if ( (rc = myread(fd, &c) ) == 1)
        {
            *ptr++ = c;
            if (c == '\n')
                break;
        }
        else if (rc == 0)
        {
            *ptr = 0;
            return n - 1;
        }
        else
            return -1;
    }

    *ptr = 0;
    return n;
}


// 如果还有数据没有读取则把调用方的指针指向read_ptr
ssize_t readlinebuf(void ** vptrptr)
{
    if (read_cnt)
        *vptrptr = read_ptr;
    return read_cnt;
}



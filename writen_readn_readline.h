
ssize_t readn(int fd, void * vptr, size_t n);

ssize_t writen(int fd, const void * vptr, size_t n);

static ssize_t myread(int fd, char * ptr);

ssize_t readline(int fd, void * vptr, size_t maxlen);

ssize_t readlinebuf(void ** vptrptr);


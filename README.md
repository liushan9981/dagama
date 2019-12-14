
### 简介
- dagama 名字取自航海家达伽马
- 该项目是个web服务，简单支持fastcgi协议，并发使用epoll
- readn writen readline等函数，抄自《Unix网络编程》一书

### 特性
- 支持http
- 支持https
- 支持fastcgi
- 仅支持GET方法
- 支持keepalive
- 使用epoll 支持并发
- 支持虚拟主机
- 支持自定义索引文件
- 支持自定义mime类型
- 支持配置文件
- 目前仅支持GET,响应200 403 404 405 500
- 访问目录时，支持index文件响应

### 用途
- 学习操作系统
- 学习网络
- 学习http协议
- 学习fastcgi协议
- 理解内存溢出、内存泄漏等

### 配置文件
```
doc_root: /home/liushan/mylab/clang/dagama/webroot-2
mime_file: /home/liushan/mylab/clang/dagama/conf/mime.types
host: 127.0.0.1:8080
https: on
ssl_cert: /home/liushan/mylab/clang/cert/kubelet_node-4.crt
ssl_key: /home/liushan/mylab/clang/cert/kubelet_node-4.key
method_allowed: GET
index: index.html
403_page: /home/liushan/mylab/clang/dagama/webroot/html/403.html
404_page: /home/liushan/mylab/clang/dagama/webroot/html/404.html
405_page: /home/liushan/mylab/clang/dagama/webroot/html/405.html
```

### 访问日志
```
2019/12/14 Sat 18:3:31   [NOTICE]  127.0.0.1    /01.jpg    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    1192459    200
2019/12/14 Sat 18:3:33   [NOTICE]  127.0.0.1    /01.jpg    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    369005    200
2019/12/14 Sat 18:3:34   [NOTICE]  127.0.0.1    /02.jpg    Apache-HttpClient/4.5.5 (Java/1.8.0_232)    802723    200
2019/12/14 Sat 18:3:35   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:35   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:35   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:35   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:36   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:36   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:36   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:37   [NOTICE]  127.0.0.1    /01.jpg    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    369005    200
2019/12/14 Sat 18:3:38   [NOTICE]  127.0.0.1    /01.jpg    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    369005    200
2019/12/14 Sat 18:3:38   [NOTICE]  127.0.0.1    /01.jpg    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    369005    200
2019/12/14 Sat 18:3:39   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:39   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
2019/12/14 Sat 18:3:39   [NOTICE]  192.168.42.13    /    Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36    113    403
```

### 安装
linux环境下，使用cmake 生成可执行文件即可




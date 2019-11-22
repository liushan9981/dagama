
### 简介
- dagama 名字取自航海家达伽马
- 该项目是个web服务，简单支持fastcgi协议，并发使用epoll
- 目前仅支持GET,响应200或者403 404 405
- 访问目录时，支持index文件响应
- readn writen readline等函数，抄自《Unix网络编程》一书

### 特性
- 支持http
- 支持https
- 支持fastcgi
- 仅支持GET方法
- 使用epoll 支持并发
- 支持虚拟主机
- 支持自定义索引文件
- 支持自定义mime类型
- 支持配置文件

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

### 安装
linux环境下，使用cmake 生成可执行文件即可




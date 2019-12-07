cmake_minimum_required(VERSION 2.8)
project(dagama)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lssl")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lcrypto")

set(SOURCE_FILES main.c mysignal.c  mysignal.h fastcgi.c fastcgi.h
        writen_readn_readline.c writen_readn_readline.h mystring.c mystring.h process_request.c process_request.h handle_config.c handle_config.h main.h myutils.c myutils.h write_log.c write_log.h handle_header.c handle_header.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

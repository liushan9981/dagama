cmake_minimum_required(VERSION 2.8)
project(dagama)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lssl")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lcrypto")

set(SOURCE_FILES main.c mysignal.c  mysignal.h fastcgi.c fastcgi.h
        writen_readn_readline.c writen_readn_readline.h tool.c tool.h process_request.c process_request.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

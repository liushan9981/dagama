cmake_minimum_required(VERSION 2.8)
project(dagama)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c mysignal.c  mysignal.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

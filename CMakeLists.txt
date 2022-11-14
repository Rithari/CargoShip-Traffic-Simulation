cmake_minimum_required(VERSION 3.23)
project(OS_Project C)

set(CMAKE_C_STANDARD 90)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(GCC_COVERAGE_COMPILE_FLAGS "-v -p -D DEBUG -Wall -O0")
    add_executable(OS_Project main.c utils/source/logger.c utils/logger.h)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(GCC_COVERAGE_COMPILE_FLAGS "-p -Wall -O2")
    add_executable(OS_Project main.c)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

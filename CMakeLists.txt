cmake_minimum_required(VERSION 3.23)
project(Progetto_SO C)

set(CMAKE_C_STANDARD 90)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(GCC_COVERAGE_COMPILE_FLAGS "-v -D DEBUG -p -Wall -O0")
    add_executable(Progetto_SO main.c utils/source/logger.c utils/header/logger.h)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(GCC_COVERAGE_COMPILE_FLAGS "-p -Wall -O2")
    add_executable(Progetto_SO main.c)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

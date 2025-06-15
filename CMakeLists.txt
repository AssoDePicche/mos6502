cmake_minimum_required(VERSION 3.10)

project(mos6502 C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES source/*.c)

add_executable(${PROJECT_NAME} ${SOURCES})

set(COMPILE_OPTIONS -Wall -Wextra -Wpedantic -g)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

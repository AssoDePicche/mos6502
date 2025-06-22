cmake_minimum_required(VERSION 3.14)

project(mos6502 C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(COMPILE_OPTIONS -Wall -Wextra -Wpedantic -g)

file(GLOB LIB_SOURCES source/*.c)

add_library(mos6502_lib STATIC ${LIB_SOURCES})

target_compile_options(mos6502_lib PRIVATE ${COMPILE_OPTIONS})

add_executable(${PROJECT_NAME} source/main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE mos6502_lib)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

add_library(unity STATIC unity/src/unity.c)

target_compile_options(unity PRIVATE ${COMPILE_OPTIONS})

enable_testing()

file(GLOB TEST_SOURCES tests/*.c)

add_executable(tests ${TEST_SOURCES})

target_compile_options(tests PRIVATE ${COMPILE_OPTIONS})

target_link_libraries(tests PRIVATE mos6502_lib unity)

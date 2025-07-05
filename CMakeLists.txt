cmake_minimum_required(VERSION 3.14)

project(mos6502 C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(COMPILE_OPTIONS -Wall -Wextra -Wpedantic -g)

add_library(mos6502_lib STATIC source/mos6502.c)
target_compile_options(mos6502_lib PRIVATE ${COMPILE_OPTIONS})
target_include_directories(mos6502_lib PRIVATE include)


find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(Parser
    ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.h
)

FLEX_TARGET(Lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer.l
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.c
)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_executable(${PROJECT_NAME}
    source/main.c
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE mos6502_lib)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

add_library(unity STATIC unity/src/unity.c)
target_compile_options(unity PRIVATE ${COMPILE_OPTIONS})

file(GLOB TEST_SOURCES tests/*.c)
add_executable(tests ${TEST_SOURCES})
target_compile_options(tests PRIVATE ${COMPILE_OPTIONS})
target_link_libraries(tests PRIVATE mos6502_lib unity)

enable_testing()

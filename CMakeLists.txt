cmake_minimum_required(VERSION 3.14)

project(mos6502 C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Incluir diretórios
include_directories(include)

# Opções de compilação
set(COMPILE_OPTIONS -Wall -Wextra -Wpedantic -g)

# Arquivos da biblioteca principal
file(GLOB LIB_SOURCES source/*.c)

# Criar biblioteca principal
add_library(mos6502_lib STATIC ${LIB_SOURCES})
target_compile_options(mos6502_lib PRIVATE ${COMPILE_OPTIONS})

# Flex e Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Corrigido: uso de CMAKE_CURRENT_BINARY_DIR para saída dos arquivos gerados
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

# Executável principal
add_executable(${PROJECT_NAME}
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
    source/main.c
)

# Incluir diretório para parser.tab.h
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE mos6502_lib)
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

# Biblioteca de testes
add_library(unity STATIC unity/src/unity.c)
target_compile_options(unity PRIVATE ${COMPILE_OPTIONS})

# Executável de testes
file(GLOB TEST_SOURCES tests/*.c)
add_executable(tests ${TEST_SOURCES})
target_compile_options(tests PRIVATE ${COMPILE_OPTIONS})
target_link_libraries(tests PRIVATE mos6502_lib unity)

# Enable testing
enable_testing()

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "mos6502.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int yylex();
extern int yylineno;

uint16_t current_address = 0x0000;

extern MOS6502 *CPU;

#define MAX_TOKENS 1024

typedef enum {
    TOKEN_REF_ABS_ADDR,
    TOKEN_REF_REL_OFFSET,
    TOKEN_LABEL,
} TokenType;

typedef struct {
    TokenType type;
    uint16_t address;
    char *buffer;
} Token;

Token token_table[MAX_TOKENS];
size_t token_count = 0;

Token reference_table[MAX_TOKENS];
size_t reference_count = 0;

void add_token(const char* buffer, uint16_t address) {
    if (MAX_TOKENS <= token_count) {
        fprintf(stderr, "Yacc: Max %d tokens allowed. Exiting.\n", MAX_TOKENS);
        exit(1);
    }

    for (size_t index = 0; index < token_count; ++index) {
        if (strcmp(token_table[index].buffer, buffer) == 0) {
            fprintf(stderr, "Yacc: Duplicate token '%s' defined at 0x%04X. Already defined at 0x%04X. Exiting.\n",
                            buffer, address, token_table[index].address);
            exit(1);
        }
    }

    token_table[token_count].buffer = strdup(buffer);
    if (token_table[token_count].buffer == NULL) {
        perror("Yacc: Failed to allocate memory for token buffer");
        exit(1);
    }

    token_table[token_count].address = address;
    token_table[token_count].type = TOKEN_LABEL;
    ++token_count;
}

int get_token_address(const char* buffer, uint16_t* address) {
    for (size_t index = 0; index < token_count; ++index) {
        if (strcmp(token_table[index].buffer, buffer) == 0) {
            *address = token_table[index].address;
            return 1;
        }
    }
    return 0;
}

void add_forward_ref(uint16_t address, const char* buffer, TokenType type) {
    if (reference_count >= MAX_TOKENS) {
        fprintf(stderr, "Yacc: Max %d forward references allowed. Exiting.\n", MAX_TOKENS);
        exit(1);
    }

    reference_table[reference_count].address = address;

    reference_table[reference_count].buffer = strdup(buffer);
    if (reference_table[reference_count].buffer == NULL) {
        perror("Yacc: Failed to allocate memory for reference buffer");
        exit(1);
    }

    reference_table[reference_count].type = type;
    ++reference_count;
}

void resolve_forward_references() {
    fprintf(stdout, "Yacc: Resolving forward references...\n");
    for (size_t index = 0; index < reference_count; ++index) {
        const Token token = reference_table[index];

        uint16_t target_address;

        if (!get_token_address(token.buffer, &target_address)) {
            fprintf(stderr, "Yacc: Undefined token '%s' referenced at 0x%04X. Exiting.\n", token.buffer, token.address);
            exit(1);
        }

        if (token.type == TOKEN_REF_ABS_ADDR) {
            mos6502_write(CPU, token.address, (target_address & 0xFF));
            mos6502_write(CPU, token.address + 1, ((target_address >> 8) & 0xFF));
        } else if (token.type == TOKEN_REF_REL_OFFSET) {
            int16_t offset = target_address - (token.address + 1);

            if (offset < -128 || offset > 127) {
                fprintf(stderr, "Yacc: Branch target '%s' (0x%04X) is out of range for relative branch from 0x%04X (offset %d). Exiting.\n",
                        token.buffer, target_address, token.address - 1, offset);
                exit(1);
            }
            mos6502_write(CPU, token.address, (int8_t)(offset & 0xFF));
        }
    }
    fprintf(stdout, "Yacc: Forward references resolved.\n");
}

void cleanup_tables() {
    for (size_t i = 0; i < token_count; ++i) {
        free(token_table[i].buffer);
        token_table[i].buffer = NULL;
    }
    for (size_t i = 0; i < reference_count; ++i) {
        free(reference_table[i].buffer);
        reference_table[i].buffer = NULL;
    }
}

%}

%union {
    int ival;
    char* sval;
}

%token NEWLINE
%token <ival> HEX_VALUE DEC_VALUE
%token <sval> STRING_LITERAL LABEL_DEF LABEL_REF

%token LDX_OP LDA_OP BEQ_OP STA_OP INX_OP JMP_OP BRK_OP

%token ORG_DIR BYTE_DIR

%token HASH
%token COMMA
%token REG_X

%type <ival> address_operand immediate_operand
%type <sval> buffer

%%

program:
    lines
    {
        resolve_forward_references();

        printf("Yacc: MOS6502 execution started.\n");

        if (CPU == NULL) {
            fprintf(stderr, "Yacc: CPU not initialized. Exiting.\n");
            exit(1);
        }

        CPU->PC = current_address;

        mos6502_execute(CPU);

        mos6502_dump(CPU, stdout);

        printf("Yacc: MOS6502 execution finished.\n");

        cleanup_tables();
    }
;

lines:
    /* empty */
    | lines line
;

line:
    NEWLINE
    | token_definition NEWLINE
    | instruction NEWLINE
    | directive NEWLINE
    | token_definition instruction NEWLINE
    | token_definition directive NEWLINE
;

token_definition:
    LABEL_DEF {
        add_token($1, current_address);
        free($1);
    }
;

instruction:
    LDX_OP HASH immediate_operand {
        mos6502_write(CPU, current_address++, MOS6502_LDX_IMMEDIATE_MODE);
        mos6502_write(CPU, current_address++, $3);
    }
    | LDA_OP buffer COMMA REG_X {
        mos6502_write(CPU, current_address++, MOS6502_LDA_ABSOLUTE_X_MODE);
        add_forward_ref(current_address, $2, TOKEN_REF_ABS_ADDR);
        current_address += 2;
        free($2);
    }
    | BEQ_OP buffer {
        mos6502_write(CPU, current_address++, MOS6502_BEQ_RELATIVE_MODE);
        add_forward_ref(current_address, $2, TOKEN_REF_REL_OFFSET);
        current_address++;
        free($2);
    }
    | STA_OP address_operand {
        mos6502_write(CPU, current_address++, MOS6502_STA_ABSOLUTE_MODE);
        mos6502_write(CPU, current_address++, ($2 & 0xFF));
        mos6502_write(CPU, current_address++, (($2 >> 8) & 0xFF));
    }
    | INX_OP {
        mos6502_write(CPU, current_address++, MOS6502_INX_IMPLIED_MODE);
    }
    | JMP_OP address_operand {
        mos6502_write(CPU, current_address++, MOS6502_JMP_ABSOLUTE_MODE);
        mos6502_write(CPU, current_address++, ($2 & 0xFF));
        mos6502_write(CPU, current_address++, (($2 >> 8) & 0xFF));
    }
    | JMP_OP buffer {
        mos6502_write(CPU, current_address++, MOS6502_JMP_ABSOLUTE_MODE);
        add_forward_ref(current_address, $2, TOKEN_REF_ABS_ADDR);
        current_address += 2;
        free($2);
    }
    | BRK_OP {
        mos6502_write(CPU, current_address++, MOS6502_BRK_IMPLIED_MODE);
    }
;

directive:
    ORG_DIR HEX_VALUE {
        current_address = $2;
    }
    | BYTE_DIR byte_list {
    }
;

byte_list:
    byte_item
    | byte_list COMMA byte_item
;

byte_item:
    HEX_VALUE {
        mos6502_write(CPU, current_address++, $1);
    }
    | DEC_VALUE {
        mos6502_write(CPU, current_address++, $1);
    }
    | STRING_LITERAL {
        for (size_t index = 0; index < strlen($1); ++index) {
            mos6502_write(CPU, current_address++, $1[index]);
        }
        free($1);
    }
;

address_operand:
    HEX_VALUE { $$ = $1; }
    | buffer {
        uint16_t address;
        if (!get_token_address($1, &address)) {
             fprintf(stderr, "Yacc: Undefined label '%s' used as direct address at 0x%04X. This type of reference is not handled as a forward reference for this instruction (STA/JMP direct). Exiting.\n",
                             $1, current_address);
             free($1);
             exit(1);
        }
        $$ = address;
        free($1);
    }
;

immediate_operand:
    HEX_VALUE { $$ = $1; }
    | DEC_VALUE { $$ = $1; }
;

buffer:
    LABEL_REF { $$ = $1; }
;

%%

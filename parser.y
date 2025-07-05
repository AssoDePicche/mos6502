%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mos6502.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int yylex();
extern int yylineno;

extern int yylval_int;
extern char* yylval_str;

uint16_t current_address = 0x0000;

extern MOS6502 *cpu_instance;

#define MAX_LABELS 100

typedef enum {
    REF_TYPE_ABS_ADDR,
    REF_TYPE_REL_OFFSET
} RefType;

typedef struct {
    char name[32];
    uint16_t address;
} LabelEntry;

LabelEntry label_table[MAX_LABELS];
int label_count = 0;

typedef struct {
    uint16_t address;
    char label_name[32];
    RefType type;
} ForwardRefEntry;

ForwardRefEntry forward_refs[MAX_LABELS];
int forward_ref_count = 0;

void add_label(const char* name, uint16_t address) {
    if (label_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Too many labels. Max %d allowed.\n", MAX_LABELS);
        exit(1);
    }
    for (int i = 0; i < label_count; ++i) {
        if (strcmp(label_table[i].name, name) == 0) {
            fprintf(stderr, "Error: Duplicate label '%s' defined at 0x%04X. Already defined at 0x%04X.\n",
                    name, address, label_table[i].address);
            exit(1);
        }
    }
    strncpy(label_table[label_count].name, name, sizeof(label_table[label_count].name) - 1);
    label_table[label_count].name[sizeof(label_table[label_count].name) - 1] = '\0';
    label_table[label_count].address = address;
    label_count++;
}

int get_label_address(const char* name, uint16_t* address) {
    for (int i = 0; i < label_count; ++i) {
        if (strcmp(label_table[i].name, name) == 0) {
            *address = label_table[i].address;
            return 1;
        }
    }
    return 0;
}

void add_forward_ref(uint16_t address_to_fill, const char* label_name, RefType type) {
    if (forward_ref_count >= MAX_LABELS) {
        fprintf(stderr, "Error: Too many forward references. Max %d allowed.\n", MAX_LABELS);
        exit(1);
    }
    forward_refs[forward_ref_count].address = address_to_fill;
    strncpy(forward_refs[forward_ref_count].label_name, label_name, sizeof(forward_refs[forward_ref_count].label_name) - 1);
    forward_refs[forward_ref_count].label_name[sizeof(forward_refs[forward_ref_count].label_name) - 1] = '\0';
    forward_refs[forward_ref_count].type = type;
    forward_ref_count++;
}

void resolve_forward_references() {
    for (int i = 0; i < forward_ref_count; ++i) {
        uint16_t label_addr;
        if (get_label_address(forward_refs[i].label_name, &label_addr)) {
            if (forward_refs[i].type == REF_TYPE_ABS_ADDR) {
                mos6502_write(cpu_instance, forward_refs[i].address, (uint8_t)(label_addr & 0xFF));
                mos6502_write(cpu_instance, forward_refs[i].address + 1, (uint8_t)((label_addr >> 8) & 0xFF));
            } else if (forward_refs[i].type == REF_TYPE_REL_OFFSET) {
                int16_t offset = label_addr - (forward_refs[i].address + 1);

                if (offset < -128 || offset > 127) {
                    fprintf(stderr, "Error: Branch target '%s' (0x%04X) is out of range for relative branch from 0x%04X (offset %d).\n",
                            forward_refs[i].label_name, label_addr, forward_refs[i].address - 1, offset);
                    exit(1);
                }
                mos6502_write(cpu_instance, forward_refs[i].address, (uint8_t)(offset & 0xFF));
            }
        } else {
            fprintf(stderr, "Error: Undefined label '%s' referenced at 0x%04X.\n",
                    forward_refs[i].label_name, forward_refs[i].address);
            exit(1);
        }
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

%token ORG_DIR BYTE_DIR

%token LDX_OP LDA_OP BEQ_OP STA_OP INX_OP JMP_OP BRK_OP

%token HASH
%token COMMA
%token REG_X

%type <ival> address_operand immediate_operand
%type <sval> label_name

%%

program:
    lines
    {
        resolve_forward_references();
        printf("\n--- Assembly completed. Starting MOS 6502 simulation ---\n");
        cpu_instance->PC = current_address;
        while (!mos6502_should_stop(cpu_instance)) {
            mos6502_execute(cpu_instance);
        }
        printf("\n--- Simulation finished ---\n");
    }
;

lines:
    | lines line
;

line:
    NEWLINE
    | label_definition NEWLINE
    | instruction NEWLINE
    | directive NEWLINE
    | label_definition instruction NEWLINE
    | label_definition directive NEWLINE
;

label_definition:
    LABEL_DEF {
        add_label($1, current_address);
        free($1);
    }
;

instruction:
    LDX_OP HASH immediate_operand {
        mos6502_write(cpu_instance, current_address++, MOS6502_LDX_IMMEDIATE_MODE);
        mos6502_write(cpu_instance, current_address++, (uint8_t)$3);
    }
    | LDA_OP label_name COMMA REG_X {
        mos6502_write(cpu_instance, current_address++, MOS6502_LDA_ABSOLUTE_X_MODE);
        add_forward_ref(current_address, $2, REF_TYPE_ABS_ADDR);
        current_address += 2;
        free($2);
    }
    | BEQ_OP label_name {
        mos6502_write(cpu_instance, current_address++, MOS6502_BEQ_RELATIVE_MODE);
        add_forward_ref(current_address, $2, REF_TYPE_REL_OFFSET);
        current_address++;
        free($2);
    }
    | STA_OP address_operand {
        mos6502_write(cpu_instance, current_address++, MOS6502_STA_ABSOLUTE_MODE);
        mos6502_write(cpu_instance, current_address++, (uint8_t)($2 & 0xFF));
        mos6502_write(cpu_instance, current_address++, (uint8_t)(($2 >> 8) & 0xFF));
    }
    | INX_OP {
        mos6502_write(cpu_instance, current_address++, MOS6502_INX_IMPLIED_MODE);
    }
    | JMP_OP address_operand {
        mos6502_write(cpu_instance, current_address++, MOS6502_JMP_ABSOLUTE_MODE);
        mos6502_write(cpu_instance, current_address++, (uint8_t)($2 & 0xFF));
        mos6502_write(cpu_instance, current_address++, (uint8_t)(($2 >> 8) & 0xFF));
    }
    | JMP_OP label_name {
        mos6502_write(cpu_instance, current_address++, MOS6502_JMP_ABSOLUTE_MODE);
        add_forward_ref(current_address, $2, REF_TYPE_ABS_ADDR);
        current_address += 2;
        free($2);
    }
    | BRK_OP {
        mos6502_write(cpu_instance, current_address++, MOS6502_BRK_IMPLIED_MODE);
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
        mos6502_write(cpu_instance, current_address++, (uint8_t)$1);
    }
    | DEC_VALUE {
        mos6502_write(cpu_instance, current_address++, (uint8_t)$1);
    }
    | STRING_LITERAL {
        for (int i = 0; i < strlen($1); ++i) {
            mos6502_write(cpu_instance, current_address++, (uint8_t)$1[i]);
        }
        free($1);
    }
;

address_operand:
    HEX_VALUE { $$ = $1; }
    | label_name {
        uint16_t addr;
        if (get_label_address($1, &addr)) {
            $$ = addr;
        } else {
            $$ = 0;
        }
        free($1);
    }
;

immediate_operand:
    HEX_VALUE { $$ = $1; }
    | DEC_VALUE { $$ = $1; }
;

label_name:
    LABEL_REF { $$ = $1; }
;

%%

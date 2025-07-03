%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void yyerror(const char* s);
int yylex();

%}

%union {
    int num;
    char* id;
}

%token <num> NUMBER
%token <id> IDENTIFIER LABEL_DEF
%token INSTR REGISTER COMMA EOL

%%

program:
    lines
    ;

lines:
    lines line
  | line
  ;

line:
    LABEL_DEF EOL
  | instruction EOL
  | EOL
  ;

instruction:
    INSTR operand_list
  ;

operand_list:
  | operand
  | operand COMMA REGISTER
  ;

operand:
    NUMBER
  | IDENTIFIER
  | REGISTER
  ;

%%

void yyerror(const char *buffer) {
    fprintf(stderr, "Error: %s\n", buffer);
}

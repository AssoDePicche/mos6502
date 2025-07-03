%{
#include "parser.tab.h"
%}

DIGITO    [0-9]
HEXDIG    [0-9a-fA-F]
ID        [a-zA-Z_][a-zA-Z0-9_]*
HEXNUM    \$[0-9a-fA-F]+
DECNUM    [0-9]+

%%

;                       { return EOL; }
[ \t]+                 ;

LDA|STA|ADC|SBC|LDX|LDY|STX|STY|JMP|JSR|RTS|BRK   { return INSTR; }
A|X|Y                 { return REGISTER; }

{HEXNUM}              { yylval.num = strtol(yytext + 1, NULL, 16); return NUMBER; }
{DECNUM}              { yylval.num = atoi(yytext); return NUMBER; }

{ID}:                 { yylval.id = strdup(yytext); return LABEL_DEF; }
{ID}                  { yylval.id = strdup(yytext); return IDENTIFIER; }

","                   { return COMMA; }

.                     { fprintf(stderr, "Unexpected char: %s\n", yytext); }

%%

int yywrap() {
    return 1;
}

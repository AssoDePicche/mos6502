%{
#include <string.h>
#include <stdlib.h>

#include "parser.tab.h"

extern uint16_t current_address;

%}

%option noyywrap

%option yylineno

%%

[ \t]+          ;

\n              { return NEWLINE; }

"//".* ;
";".* ;

\.ORG           { return ORG_DIR; }
\.BYTE          { return BYTE_DIR; }

LDX             { return LDX_OP; }
LDA             { return LDA_OP; }
BEQ             { return BEQ_OP; }
STA             { return STA_OP; }
INX             { return INX_OP; }
JMP             { return JMP_OP; }
BRK             { return BRK_OP; }

"#"             { return HASH; }
","             { return COMMA; }
"X"             { return REG_X; }

\$[0-9a-fA-F]{2,4} {
  yylval.ival = (int)strtol(yytext + 1, NULL, 16);

  return HEX_VALUE;
}

[0-9]+  {
  yylval.ival = atoi(yytext);

  return DEC_VALUE;
}

\"([^"\\]|\\.)*\" {
  yylval.sval = strdup(yytext + 1);

  yylval.sval[strlen(yylval.sval) - 1] = '\0';

  return STRING_LITERAL;
}

[a-zA-Z_][a-zA-Z0-9_]*: {
  yylval.sval = strdup(yytext);

  yylval.sval[strlen(yylval.sval) - 1] = '\0';

  return LABEL_DEF;
}

[a-zA-Z_][a-zA-Z0-9_]* {
  yylval.sval = strdup(yytext);

  return LABEL_REF;
}

. {
  fprintf(stderr, "Lex: Unexpected '%s' at line %d\n", yytext, yylineno);
}

%%

#include <stdio.h>
#include <stdlib.h>  // For exit

#include "mos6502.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int yylineno;

MOS6502 *CPU = NULL;

void yyerror(const char *s) {
  fprintf(stderr, "Parse error at line %d: %s\n", yylineno, s);
}

int main(const int argc, const char **argv) {
  if (2 != argc) {
    fprintf(stderr, "error: you must provide an .asm file\n");
    return 1;
  }

  const char *filename = argv[1];

  yyin = fopen(filename, "r");

  if (NULL == yyin) {
    fprintf(stderr, "error: unable to open the '%s' file\n", filename);
    return 1;
  }

  CPU = mos6502_construct();

  if (NULL == CPU) {
    fprintf(stderr, "error: mos6502 could not be started\n");
    fclose(yyin);
    return 1;
  }

  int parse_result = yyparse();

  fclose(yyin);

  mos6502_destruct(CPU);

  if (parse_result != 0) {
    fprintf(stderr, "Error: Parsing failed.\n");
    return 1;
  }

  return 0;
}

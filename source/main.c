#include <stdio.h>

#include "mos6502.h"

int main(const int argc, const char **argv) {
  if (2 != argc) {
    fprintf(stderr, "error: you must provide an .asm file\n");

    return 1;
  }

  const char *filename = argv[1];

  FILE *stream = fopen(filename, "r");

  if (NULL == stream) {
    fprintf(stderr, "error: unable to open the '%s' file\n", filename);

    return 1;
  }

  fclose(stream);

  MOS6502 *CPU = mos6502_construct();

  if (NULL == CPU) {
    fprintf(stderr, "error: mos6502 could not be started\n");
  }

  while (!mos6502_should_stop(CPU)) {
    mos6502_execute(CPU);
  }

  mos6502_dump(CPU, stdout);

  mos6502_destruct(CPU);

  return 0;
}

#include <stdio.h>

int main(const int argc, const char **argv) {
  if (argc != 2) {
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

  return 0;
}

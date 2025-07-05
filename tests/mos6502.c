#include "mos6502.h"

#include <unity.h>

typedef void (*test_t)(void);

static MOS6502 *CPU = NULL;

void setUp(void) { CPU = mos6502_construct(); }

void tearDown(void) { mos6502_destruct(CPU); }

static const test_t TESTS[] = {
    NULL,
};

int main(void) {
  UNITY_BEGIN();

  for (size_t index = 0; index < sizeof(TESTS) / sizeof(test_t); ++index) {
    const test_t test = TESTS[index];

    if (NULL == test) {
      fprintf(stderr, "Error: could not run test");

      break;
    }

    RUN_TEST(test);
  }

  return UNITY_END();
}

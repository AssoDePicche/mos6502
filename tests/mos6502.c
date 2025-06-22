#include "mos6502.h"

#include <unity.h>

static MOS6502 *CPU = NULL;

void setUp(void) { CPU = mos6502_construct(); }

void tearDown(void) { mos6502_destruct(CPU); }

void LDA_Immediate_Can_Load_A_Value_Into_Register_A(void) {
  mos6502_write(CPU, 0xFFFC, MOS6502_LDA_IMMEDIATE_MODE);

  mos6502_write(CPU, 0xFFFD, 0x84);

  CPU->current_instruction_cycles = 2;

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_HEX8(CPU->A, 0x84);
}

void LDA_Zero_Page_Can_Load_A_Value_Into_Register_A(void) {
  mos6502_write(CPU, 0xFFFC, MOS6502_LDA_ZERO_PAGE_MODE);

  mos6502_write(CPU, 0xFFFD, 0x42);

  mos6502_write(CPU, 0x0042, 0x37);

  CPU->current_instruction_cycles = 3;

  TEST_ASSERT_EQUAL_HEX8(CPU->A, 0x37);
}

void LDA_Zero_Page_X_Can_Load_A_Value_Into_Register_A(void) {
  CPU->X = 5;

  mos6502_write(CPU, 0xFFFC, MOS6502_LDA_ZERO_PAGE_X_MODE);

  mos6502_write(CPU, 0xFFFD, 0x42);

  mos6502_write(CPU, 0x0047, 0x37);

  CPU->current_instruction_cycles = 4;

  TEST_ASSERT_EQUAL_HEX8(CPU->A, 0x37);
}

void LDA_Zero_Page_X_Can_Load_A_Value_Into_Register_A_When_It_Wraps(void) {
  CPU->X = 0xFF;

  mos6502_write(CPU, 0xFFFC, MOS6502_LDA_ZERO_PAGE_X_MODE);

  mos6502_write(CPU, 0xFFFD, 0x80);

  mos6502_write(CPU, 0x007F, 0x37);

  CPU->current_instruction_cycles = 4;

  TEST_ASSERT_EQUAL_HEX8(CPU->A, 0x37);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(LDA_Immediate_Can_Load_A_Value_Into_Register_A);

  RUN_TEST(LDA_Zero_Page_Can_Load_A_Value_Into_Register_A);

  RUN_TEST(LDA_Zero_Page_X_Can_Load_A_Value_Into_Register_A);

  RUN_TEST(LDA_Zero_Page_X_Can_Load_A_Value_Into_Register_A_When_It_Wraps);

  return UNITY_END();
}

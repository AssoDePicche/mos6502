#include "mos6502.h"

#include <unity.h>

typedef void (*test_t)(void);

static MOS6502 *CPU = NULL;

void setUp(void) {
  CPU = mos6502_construct();

  TEST_ASSERT_NOT_NULL(CPU);

  setvbuf(stdout, NULL, _IOFBF, 0);

  setvbuf(stderr, NULL, _IOFBF, 0);
}

void tearDown(void) {
  mos6502_destruct(CPU);

  setvbuf(stdout, NULL, _IOFBF, 0);

  setvbuf(stderr, NULL, _IOFBF, 0);
}

void test_mos6502_read_write(void) {
  uint16_t address = 0x1234;
  uint8_t value = 0xAB;
  mos6502_write(CPU, address, value);
  TEST_ASSERT_EQUAL_UINT8(value, mos6502_read(CPU, address));
}

void test_mos6502_set_get_clear_status(void) {
  CPU->P = 0x00;
  mos6502_set_status(CPU, MOS6502_STATUS_C);
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_C));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_Z));

  mos6502_clear_status(CPU, MOS6502_STATUS_C);
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_C));
}

void test_mos6502_push_pop(void) {
  uint8_t initial_sp = CPU->SP;
  uint8_t value1 = 0x11;
  uint8_t value2 = 0x22;

  mos6502_push(CPU, value1);
  TEST_ASSERT_EQUAL_UINT8(initial_sp - 1, CPU->SP);
  TEST_ASSERT_EQUAL_UINT8(value1, CPU->BUS[MOS6502_STACK + initial_sp]);

  mos6502_push(CPU, value2);
  TEST_ASSERT_EQUAL_UINT8(initial_sp - 2, CPU->SP);
  TEST_ASSERT_EQUAL_UINT8(value2, CPU->BUS[MOS6502_STACK + initial_sp - 1]);

  TEST_ASSERT_EQUAL_UINT8(value2, mos6502_pop(CPU));
  TEST_ASSERT_EQUAL_UINT8(initial_sp - 1, CPU->SP);

  TEST_ASSERT_EQUAL_UINT8(value1, mos6502_pop(CPU));
  TEST_ASSERT_EQUAL_UINT8(initial_sp, CPU->SP);
}

void test_mos6502_update_z_n_status_zero(void) {
  CPU->P = 0x00;
  mos6502_update_z_n_status(CPU, 0x00);
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_mos6502_update_z_n_status_positive(void) {
  CPU->P = 0xFF;  // Set all flags
  mos6502_update_z_n_status(CPU, 0x01);
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_mos6502_update_z_n_status_negative(void) {
  CPU->P = 0x00;
  mos6502_update_z_n_status(CPU, 0x80);
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_LDX_IMMEDIATE_MODE(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1001] = 0xCD;  // Operand
  CPU->X = 0x00;
  CPU->P = 0xFF;  // Clear all flags initially

  LDX_IMMEDIATE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT8(0xCD, CPU->X);
  TEST_ASSERT_EQUAL_UINT16(0x1002, CPU->PC);
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_TRUE(
      mos6502_get_status(CPU, MOS6502_STATUS_N));  // 0xCD is negative
}

void test_LDX_IMMEDIATE_MODE_zero(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1001] = 0x00;  // Operand
  CPU->X = 0xFF;
  CPU->P = 0x00;

  LDX_IMMEDIATE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT8(0x00, CPU->X);
  TEST_ASSERT_EQUAL_UINT16(0x1002, CPU->PC);
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_LDA_ABSOLUTE_X_MODE(void) {
  CPU->PC = 0x1000;
  CPU->X = 0x05;
  CPU->BUS[0x1001] = 0x00;           // Low byte of address 0x2000
  CPU->BUS[0x1002] = 0x20;           // High byte of address 0x2000
  CPU->BUS[0x2000 + CPU->X] = 0xEF;  // Value at effective address
  CPU->A = 0x00;
  CPU->P = 0x00;

  LDA_ABSOLUTE_X_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT8(0xEF, CPU->A);
  TEST_ASSERT_EQUAL_UINT16(0x1003, CPU->PC);
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_BEQ_RELATIVE_MODE_branch_taken(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1001] = 0x0A;  // Positive offset
  mos6502_set_status(CPU, MOS6502_STATUS_Z);

  BEQ_RELATIVE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT16(0x1000 + 2 + 0x0A, CPU->PC);
}

void test_BEQ_RELATIVE_MODE_branch_taken_negative_offset(void) {
  CPU->PC = 0x1050;
  CPU->BUS[0x1051] = (uint8_t)-0x10;  // Negative offset (-16)
  mos6502_set_status(CPU, MOS6502_STATUS_Z);

  BEQ_RELATIVE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT16(0x1050 + 2 - 0x10, CPU->PC);
}

void test_BEQ_RELATIVE_MODE_branch_not_taken(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1001] = 0x0A;  // Offset
  mos6502_clear_status(CPU, MOS6502_STATUS_Z);

  BEQ_RELATIVE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT16(0x1002, CPU->PC);
}

void test_STA_ABSOLUTE_MODE(void) {
  CPU->PC = 0x1000;
  CPU->A = 0x55;
  CPU->BUS[0x1001] = 0x34;  // Low byte of address 0x1234
  CPU->BUS[0x1002] = 0x12;  // High byte of address 0x1234
  CPU->BUS[0x1234] = 0x00;  // Ensure it's not the value we expect

  STA_ABSOLUTE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT8(0x55, CPU->BUS[0x1234]);
  TEST_ASSERT_EQUAL_UINT16(0x1003, CPU->PC);
}

void test_INX_IMPLIED_MODE(void) {
  CPU->PC = 0x1000;
  CPU->X = 0x0F;
  CPU->P = 0x00;

  INX_IMPLIED_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT8(0x10, CPU->X);
  TEST_ASSERT_EQUAL_UINT16(0x1001, CPU->PC);
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_INX_IMPLIED_MODE_wraparound_zero(void) {
  CPU->PC = 0x1000;
  CPU->X = 0xFF;  // Will wrap to 0x00
  CPU->P = 0x00;

  INX_IMPLIED_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT8(0x00, CPU->X);
  TEST_ASSERT_EQUAL_UINT16(0x1001, CPU->PC);
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_Z));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_N));
}

void test_JMP_ABSOLUTE_MODE(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1001] = 0xAA;  // Low byte of target 0xBBAA
  CPU->BUS[0x1002] = 0xBB;  // High byte of target 0xBBAA

  JMP_ABSOLUTE_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT16(0xBBAA, CPU->PC);
}

void test_BRK_IMPLIED_MODE(void) {
  CPU->PC = 0x1000;
  CPU->P = 0x00;   // Initial status flags
  CPU->SP = 0xFD;  // Initial stack pointer

  // Set IRQ vector
  CPU->BUS[MOS6502_VEC_IRQ] = 0xAD;
  CPU->BUS[MOS6502_VEC_IRQ + 1] = 0xDE;  // Target 0xDEAD

  BRK_IMPLIED_MODE(CPU);

  TEST_ASSERT_EQUAL_UINT16(0xDEAD, CPU->PC);
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_I));
  TEST_ASSERT_FALSE(mos6502_get_status(
      CPU, MOS6502_STATUS_B));  // B flag cleared after pushing

  // Verify stack content
  TEST_ASSERT_EQUAL_UINT8(0xFC, CPU->SP);  // SP decremented three times
  TEST_ASSERT_EQUAL_UINT8(0x10,
                          CPU->BUS[MOS6502_STACK + 0xFD]);  // PC+2 high byte
  TEST_ASSERT_EQUAL_UINT8(0x02,
                          CPU->BUS[MOS6502_STACK + 0xFC]);  // PC+2 low byte
  TEST_ASSERT_EQUAL_UINT8(0x00 | MOS6502_STATUS_B,
                          CPU->BUS[MOS6502_STACK + 0xFB]);  // P with B flag set
}

void test_mos6502_execute_LDX_IMMEDIATE(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1000] = MOS6502_LDX_IMMEDIATE_MODE;
  CPU->BUS[0x1001] = 0xAA;
  CPU->X = 0x00;

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT8(0xAA, CPU->X);
  TEST_ASSERT_EQUAL_UINT16(0x1002, CPU->PC);
}

void test_mos6502_execute_LDA_ABSOLUTE_X(void) {
  CPU->PC = 0x1000;
  CPU->X = 0x03;
  CPU->BUS[0x1000] = MOS6502_LDA_ABSOLUTE_X_MODE;
  CPU->BUS[0x1001] = 0x00;  // Low byte of address 0x2000
  CPU->BUS[0x1002] = 0x20;  // High byte of address 0x2000
  CPU->BUS[0x2000 + CPU->X] = 0xBB;
  CPU->A = 0x00;

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT8(0xBB, CPU->A);
  TEST_ASSERT_EQUAL_UINT16(0x1003, CPU->PC);
}

void test_mos6502_execute_BEQ_RELATIVE_taken(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1000] = MOS6502_BEQ_RELATIVE_MODE;
  CPU->BUS[0x1001] = 0x05;  // Offset
  mos6502_set_status(CPU, MOS6502_STATUS_Z);

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT16(0x1000 + 2 + 5, CPU->PC);
}

void test_mos6502_execute_STA_ABSOLUTE(void) {
  CPU->PC = 0x1000;
  CPU->A = 0xCC;
  CPU->BUS[0x1000] = MOS6502_STA_ABSOLUTE_MODE;
  CPU->BUS[0x1001] = 0x00;  // Low byte of address 0x3000
  CPU->BUS[0x1002] = 0x30;  // High byte of address 0x3000

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT8(0xCC, CPU->BUS[0x3000]);
  TEST_ASSERT_EQUAL_UINT16(0x1003, CPU->PC);
}

void test_mos6502_execute_INX_IMPLIED(void) {
  CPU->PC = 0x1000;
  CPU->X = 0x0F;
  CPU->BUS[0x1000] = MOS6502_INX_IMPLIED_MODE;

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT8(0x10, CPU->X);
  TEST_ASSERT_EQUAL_UINT16(0x1001, CPU->PC);
}

void test_mos6502_execute_JMP_ABSOLUTE(void) {
  CPU->PC = 0x1000;
  CPU->BUS[0x1000] = MOS6502_JMP_ABSOLUTE_MODE;
  CPU->BUS[0x1001] = 0xEF;  // Low byte of target 0xABCD
  CPU->BUS[0x1002] = 0xCD;  // High byte of target 0xABCD

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT16(0xCDEF, CPU->PC);
}

void test_mos6502_execute_BRK_IMPLIED(void) {
  CPU->PC = 0x1000;
  CPU->P = 0x00;
  CPU->SP = 0xFD;
  CPU->BUS[0x1000] = MOS6502_BRK_IMPLIED_MODE;
  CPU->BUS[MOS6502_VEC_IRQ] = 0x00;
  CPU->BUS[MOS6502_VEC_IRQ + 1] = 0xC0;  // Target 0xC000

  mos6502_execute(CPU);

  TEST_ASSERT_EQUAL_UINT16(0xC000, CPU->PC);
  TEST_ASSERT_TRUE(mos6502_get_status(CPU, MOS6502_STATUS_I));
  TEST_ASSERT_FALSE(mos6502_get_status(CPU, MOS6502_STATUS_B));
  TEST_ASSERT_EQUAL_UINT8(0xFC, CPU->SP);  // SP decremented three times
  TEST_ASSERT_EQUAL_UINT8(0x10,
                          CPU->BUS[MOS6502_STACK + 0xFD]);  // PC+2 high byte
  TEST_ASSERT_EQUAL_UINT8(0x02,
                          CPU->BUS[MOS6502_STACK + 0xFC]);  // PC+2 low byte
  TEST_ASSERT_EQUAL_UINT8(0x00 | MOS6502_STATUS_B,
                          CPU->BUS[MOS6502_STACK + 0xFB]);  // P with B flag set
}

static const test_t TESTS[] = {
    test_mos6502_read_write,
    test_mos6502_set_get_clear_status,
    test_mos6502_push_pop,
    test_mos6502_update_z_n_status_zero,
    test_mos6502_update_z_n_status_positive,
    test_mos6502_update_z_n_status_negative,
    test_LDX_IMMEDIATE_MODE,
    test_LDX_IMMEDIATE_MODE_zero,
    test_LDA_ABSOLUTE_X_MODE,
    test_BEQ_RELATIVE_MODE_branch_taken,
    test_BEQ_RELATIVE_MODE_branch_taken_negative_offset,
    test_BEQ_RELATIVE_MODE_branch_not_taken,
    test_STA_ABSOLUTE_MODE,
    test_INX_IMPLIED_MODE,
    test_INX_IMPLIED_MODE_wraparound_zero,
    test_JMP_ABSOLUTE_MODE,
    test_BRK_IMPLIED_MODE,
    test_mos6502_execute_LDX_IMMEDIATE,
    test_mos6502_execute_LDA_ABSOLUTE_X,
    test_mos6502_execute_BEQ_RELATIVE_taken,
    test_mos6502_execute_STA_ABSOLUTE,
    test_mos6502_execute_INX_IMPLIED,
    test_mos6502_execute_JMP_ABSOLUTE,
    test_mos6502_execute_BRK_IMPLIED,
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

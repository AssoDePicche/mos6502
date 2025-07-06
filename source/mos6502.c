#include "mos6502.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*mos6502_instruction_handler)(MOS6502 *);

static void mos6502_update_z_n_status(MOS6502 *this, const uint8_t value) {
  if (0 == value) {
    mos6502_set_status(this, MOS6502_STATUS_Z);
  } else {
    mos6502_clear_status(this, MOS6502_STATUS_Z);
  }

  if (value & 0x80) {
    mos6502_set_status(this, MOS6502_STATUS_N);
  } else {
    mos6502_clear_status(this, MOS6502_STATUS_N);
  }
}

static void LDX_IMMEDIATE_MODE(MOS6502 *this) {
  const uint8_t operand = mos6502_read(this, this->PC + 1);

  fprintf(stdout, "MOS6502: Loading %02X into REG X (LDX #$%02X)\n", operand,
          operand);

  this->X = operand;

  this->PC += 2;

  mos6502_update_z_n_status(this, this->X);
}

static void LDA_ABSOLUTE_X_MODE(MOS6502 *this) {
  uint16_t base_address = mos6502_read(this, this->PC + 1);
  base_address |= ((uint16_t)mos6502_read(this, this->PC + 2) << 8);

  fprintf(stdout,
          "MOS6502: Loading into REG A from absolute address (LDA $%04X,X)\n",
          base_address);

  const uint16_t address = base_address + this->X;

  this->A = mos6502_read(this, address);

  this->PC += 3;

  mos6502_update_z_n_status(this, this->A);
}

static void BEQ_RELATIVE_MODE(MOS6502 *this) {
  int8_t offset = (int8_t)mos6502_read(this, this->PC + 1);
  this->PC += 2;

  if (mos6502_get_status(this, MOS6502_STATUS_Z)) {
    this->PC += offset;
    fprintf(stdout, "MOS6502: Branch taken to 0x%04X (BEQ offset %d)\n",
            this->PC, offset);
  } else {
    fprintf(stdout, "MOS6502: Branch not taken (BEQ).\n");
  }
}

static void STA_ABSOLUTE_MODE(MOS6502 *this) {
  uint16_t address = mos6502_read(this, this->PC + 1);
  address |= ((uint16_t)mos6502_read(this, this->PC + 2) << 8);

  fprintf(
      stdout,
      "MOS6502: Writing REG A (0x%02X) value to absolute address (STA $%04X)\n",
      this->A, address);

  mos6502_write(this, address, this->A);

  this->PC += 3;
}

static void INX_IMPLIED_MODE(MOS6502 *this) {
  fprintf(stdout, "MOS6502: Incrementing REG X\n");

  ++this->X;

  this->PC += 1;

  mos6502_update_z_n_status(this, this->X);
}

static void JMP_ABSOLUTE_MODE(MOS6502 *this) {
  uint16_t target_address = mos6502_read(this, this->PC + 1);
  target_address |= ((uint16_t)mos6502_read(this, this->PC + 2) << 8);

  fprintf(stdout, "MOS6502: Jumping to absolute address 0x%04X (JMP $%04X)\n",
          target_address, target_address);

  this->PC = target_address;
}

static void BRK_IMPLIED_MODE(MOS6502 *this) {
  fprintf(stdout,
          "MOS6502: Break command (BRK). Pushing PC and P, jumping to IRQ/BRK "
          "vector.\n");

  mos6502_push(this, (this->PC + 2) >> 8);
  mos6502_push(this, (this->PC + 2) & 0xFF);

  mos6502_push(this, this->P | MOS6502_STATUS_B);

  mos6502_set_status(this, MOS6502_STATUS_I);
  mos6502_clear_status(this, MOS6502_STATUS_B);

  this->PC = mos6502_read(this, MOS6502_VEC_IRQ) |
             ((uint16_t)mos6502_read(this, MOS6502_VEC_IRQ + 1) << 8);
}

static const mos6502_instruction_handler MOS6502_INSTRUCTIONS_TABLE[] = {
    [MOS6502_LDX_IMMEDIATE_MODE] = LDX_IMMEDIATE_MODE,
    [MOS6502_LDA_ABSOLUTE_X_MODE] = LDA_ABSOLUTE_X_MODE,
    [MOS6502_BEQ_RELATIVE_MODE] = BEQ_RELATIVE_MODE,
    [MOS6502_STA_ABSOLUTE_MODE] = STA_ABSOLUTE_MODE,
    [MOS6502_INX_IMPLIED_MODE] = INX_IMPLIED_MODE,
    [MOS6502_JMP_ABSOLUTE_MODE] = JMP_ABSOLUTE_MODE,
    [MOS6502_BRK_IMPLIED_MODE] = BRK_IMPLIED_MODE,
};

MOS6502 *mos6502_construct(void) {
  MOS6502 *this = (MOS6502 *)malloc(sizeof(MOS6502));

  if (NULL == this) {
    return NULL;
  }

  memset(this, 0, sizeof(MOS6502));

  this->PC = MOS6502_VEC_RESET;

  this->SP = 0xFD;

  return this;
}

void mos6502_destruct(MOS6502 *this) {
  assert(NULL != this);

  free(this);
}

uint8_t mos6502_read(const MOS6502 *this, const uint16_t address) {
  fprintf(stdout, "MOS6502: Reading address '0x%04X'\n", address);

  return this->BUS[address];
}

void mos6502_write(MOS6502 *this, const uint16_t address, const uint8_t value) {
  fprintf(stdout, "MOS6502: Writing '0x%02X' on '0x%04X' address\n", value,
          address);

  this->BUS[address] = value;
}

void mos6502_set_status(MOS6502 *this, const uint8_t status) {
  this->P |= status;
}

uint8_t mos6502_get_status(const MOS6502 *this, const uint8_t status) {
  return this->P & status;
}

void mos6502_clear_status(MOS6502 *this, const uint8_t status) {
  this->P &= ~status;
}

void mos6502_push(MOS6502 *this, const uint8_t value) {
  fprintf(stdout, "MOS6502: Pushing 0x%02X on STACK 0x%04X address\n", value,
          MOS6502_STACK + this->SP);

  this->BUS[MOS6502_STACK + this->SP] = value;

  --this->SP;
  fprintf(stdout, "MOS6502: Decrementing STACK POINTER to 0x%02X\n", this->SP);
}

uint8_t mos6502_pop(MOS6502 *this) {
  ++this->SP;
  fprintf(stdout, "MOS6502: Incrementing STACK POINTER to 0x%02X\n", this->SP);

  uint8_t value = this->BUS[MOS6502_STACK + this->SP];
  fprintf(stdout, "MOS6502: Popping 0x%02X from 0x%04X address\n", value,
          MOS6502_STACK + this->SP);

  return value;
}

void mos6502_execute(MOS6502 *this) {
  const uint8_t opcode = mos6502_read(this, this->PC);

  fprintf(stdout, "MOS6502: Executing instruction 0x%02X at 0x%04X\n", opcode,
          this->PC);

  const mos6502_instruction_handler handler =
      MOS6502_INSTRUCTIONS_TABLE[opcode];

  if (NULL == handler) {
    fprintf(stderr,
            "Instruction 0x%02X on address 0x%04X not implemented. Halting.\n",
            opcode, this->PC);
    exit(1);
    return;
  }

  handler(this);
}

void mos6502_dump(const MOS6502 *this, FILE *stream) {
  int change_region = 0;
  char region[1024];

  for (uint32_t index = 0; index < MOS6502_BUS_SIZE; ++index) {
    switch (index) {
      case MOS6502_ZERO_PAGE:
        strcpy(region, "ZERO PAGE");
        change_region = 1;
        break;
      case MOS6502_STACK:
        strcpy(region, "STACK");
        change_region = 1;
        break;
      case MOS6502_RAM:
        strcpy(region, "RAM");
        change_region = 1;
        break;
      case MOS6502_ROM:
        strcpy(region, "ROM");
        change_region = 1;
        break;
      case MOS6502_VEC_NMI:
        strcpy(region, "VEC NMI");
        change_region = 1;
        break;
      case MOS6502_VEC_RESET:
        strcpy(region, "VEC RESET");
        change_region = 1;
        break;
      case MOS6502_VEC_IRQ:
        strcpy(region, "VEC IRQ/BRK");
        change_region = 1;
        break;
    }

    const uint8_t byte_val = this->BUS[index];

    if (0 == byte_val) {
      continue;
    }

    if (change_region) {
      for (int8_t i = 0; i < 43; ++i) {
        fprintf(stream, "-");
      }
      fprintf(stream, "\n%s\n", region);
      change_region = 0;
    }

    fprintf(stream, "ADDRESS 0x%04X %02X ", index, byte_val);
    fprintf(stream, "%c", isprint(byte_val) ? byte_val : '.');
    fprintf(stream, "\n");
  }

  static const uint8_t nil[MOS6502_BUS_SIZE] = {0};

  if (0 == memcmp(this->BUS, nil, MOS6502_BUS_SIZE)) {
    for (int8_t i = 0; i < 43; ++i) {
      fprintf(stream, "-");
    }
    fprintf(stream, "\nNo data in BUS (all zeros)\n");
  }

  for (int8_t i = 0; i < 43; ++i) {
    fprintf(stream, "-");
  }

  fprintf(stream, "\n");

  fprintf(stream,
          "|%-6s|%-6s|%-6s|%-6s|%-6s|%-6s|\n|0x%04X|0x%04X|0x%04X|0x%04X|0x%"
          "04X|0x%04X|\n",
          "PC", "SP", "REG. A", "REG. X", "REG. Y", "REG. P", this->PC,
          this->SP, this->A, this->X, this->Y, this->P);

  for (int8_t i = 0; i < 43; ++i) {
    fprintf(stream, "-");
  }

  mos6502_dump_status(this, stream);
}

void mos6502_dump_status(const MOS6502 *this, FILE *stream) {
  fprintf(stream, "\n|%-13s|\n", "REG. P FLAGS");

  for (int8_t i = 0; i < 15; ++i) {
    fprintf(stream, "-");
  }

  fprintf(stream, "\n|N|V|B|D|I|Z|C|\n|%d|%d|%d|%d|%d|%d|%d|\n",
          mos6502_get_status(this, MOS6502_STATUS_N) ? 1 : 0,
          mos6502_get_status(this, MOS6502_STATUS_V) ? 1 : 0,
          mos6502_get_status(this, MOS6502_STATUS_B) ? 1 : 0,
          mos6502_get_status(this, MOS6502_STATUS_D) ? 1 : 0,
          mos6502_get_status(this, MOS6502_STATUS_I) ? 1 : 0,
          mos6502_get_status(this, MOS6502_STATUS_Z) ? 1 : 0,
          mos6502_get_status(this, MOS6502_STATUS_C) ? 1 : 0);

  for (int8_t i = 0; i < 15; ++i) {
    fprintf(stream, "-");
  }

  fprintf(stream, "\n");
}

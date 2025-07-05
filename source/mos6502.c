#include "mos6502.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef void (*mos6502_instruction_handler)(MOS6502 *);

static uint8_t mos6502_fetch_byte(const MOS6502 *this) {
  return mos6502_read(this, this->PC + 1) |
         (mos6502_read(this, this->PC + 2) << 8);
}

static void mos6502_update_z_n_status(MOS6502 *this, const uint16_t address) {
  if (0 == mos6502_get_status(this, MOS6502_STATUS_Z)) {
    mos6502_set_status(this, MOS6502_STATUS_Z);
  } else {
    mos6502_clear_status(this, MOS6502_STATUS_Z);
  }

  if (address & 0x80) {
    mos6502_set_status(this, MOS6502_STATUS_N);
  } else {
    mos6502_clear_status(this, MOS6502_STATUS_N);
  }
}

static void LDX_IMMEDIATE_MODE(MOS6502 *this) {
  const uint8_t operand =
      mos6502_fetch_byte(this);  // TODO: maybe just the PC+1

  fprintf(stdout, "Loading %02X into REG X (LDX #$%02XX)\n", operand, operand);

  this->X = operand;

  this->PC += 2;

  mos6502_update_z_n_status(this, this->X);
}

static void LDA_ABSOLUTE_X_MODE(MOS6502 *this) {
  const uint8_t operand = mos6502_fetch_byte(this);

  fprintf(stdout, "Loading into REG A from absolute address (LDA $%02X,X)\n",
          operand);

  const uint16_t address = operand + this->X;

  this->A = mos6502_read(this, address);

  this->PC += 3;

  mos6502_update_z_n_status(this, this->A);
}

static void BEQ_RELATIVE_MODE(MOS6502 *this) {
  if (1 == mos6502_get_status(this, MOS6502_STATUS_Z)) {
    const uint8_t offset = mos6502_read(this, this->PC + 1);

    this->PC += offset;

    fprintf(stderr, "Jump to %02X address (BEQ %02X)\n", this->PC, offset);
  }

  this->PC += 2;
}

static void STA_ABSOLUTE_MODE(MOS6502 *this) {
  const uint8_t operand = mos6502_fetch_byte(this);

  fprintf(stdout, "Writing REG A value to absolute address (STA $%02X)\n",
          operand);

  mos6502_write(this, operand, this->A);

  this->PC += 3;
}

static void INX_IMPLIED_MODE(MOS6502 *this) {
  fprintf(stdout, "Incrementing REG X\n");

  ++this->X;

  ++this->PC;

  mos6502_update_z_n_status(this, this->X);
}

static void JMP_ABSOLUTE_MODE(MOS6502 *this) {
  const uint8_t operand = mos6502_fetch_byte(this);

  fprintf(stdout, "Jumping to absolute address %02X (JMP $%02X)\n", operand,
          operand);

  this->PC = operand;
}

static void BRK_IMPLIED_MODE(MOS6502 *this) {
  fprintf(stdout, "Break command (BRK). Jumping into BRK vector address\n");

  this->PC += 2;

  mos6502_push(this, this->P);

  mos6502_set_status(this, MOS6502_STATUS_B | MOS6502_STATUS_I);

  this->PC = MOS6502_VEC_RESET;
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
  fprintf(stdout, "Reading address %02X\n", address);

  return this->BUS[address];
}

void mos6502_write(MOS6502 *this, const uint16_t address, const uint8_t value) {
  this->BUS[address] = value;

  fprintf(stdout, "Writing '%02X' on '%02X' address\n", value, address);
}

void mos6502_set_status(MOS6502 *this, const uint8_t status) {
  this->P |= (status & 0xFF);
}

uint8_t mos6502_get_status(const MOS6502 *this, const uint8_t status) {
  return this->P & status;
}

void mos6502_clear_status(MOS6502 *this, const uint8_t status) {
  this->P &= ~status;
}

void mos6502_push(MOS6502 *this, const uint8_t value) {
  fprintf(stdout, "Pushing %X on STACK %X address\n", value,
          MOS6502_STACK + this->SP);

  fprintf(stdout, "Decrementing STACK POINTER\n");

  this->BUS[MOS6502_STACK + this->SP] = value;

  --this->SP;
}

uint8_t mos6502_pop(MOS6502 *this) {
  ++this->SP;

  fprintf(stdout, "Incrementing STACK POINTER\n");

  fprintf(stdout, "Poping %02X from %02X address\n",
          this->BUS[MOS6502_STACK + this->SP], MOS6502_STACK + this->SP);

  return this->BUS[MOS6502_STACK + this->SP];
}

void mos6502_execute(MOS6502 *this) {
  const uint8_t opcode = mos6502_read(this, this->PC);

  fprintf(stdout, "Executing instruction %X\n", opcode);

  const mos6502_instruction_handler handler =
      MOS6502_INSTRUCTIONS_TABLE[opcode];

  if (NULL == handler) {
    fprintf(stderr, "Instruction %X on address %X not implemented\n", opcode,
            this->PC);

    return;
  }

  handler(this);
}

bool mos6502_should_stop(const MOS6502 *this) {
  return mos6502_get_status(this, MOS6502_STATUS_B) ||
         mos6502_get_status(this, MOS6502_STATUS_I);
}

void mos6502_dump(const MOS6502 *this, FILE *stream) {
  int change_region = 0;

  for (uint32_t index = 0; index < MOS6502_BUS_SIZE; ++index) {
    char region[1024];

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
    }

    const uint64_t byte = this->BUS[index];

    if (0 == byte) {
      continue;
    }

    if (change_region) {
      for (int8_t i = 0; i < 43; ++i) {
        fprintf(stream, "-");
      }
      fprintf(stream, "\n%s\n", region);
      change_region = 0;
    }

    fprintf(stream, "ADD 0x%04X ", index);

    for (int64_t bit = 7; 0 <= bit; --bit) {
      fprintf(stream, "%02lX", (byte >> (bit * 8)) & 0xFF);

      if (bit > 0) {
        fprintf(stream, " ");
      }
    }

    fprintf(stream, " ");

    for (int8_t bit = 7; 0 <= bit; --bit) {
      int8_t ch = (byte >> (bit * 8)) & 0xFF;

      fprintf(stream, "%c", isprint(ch) ? ch : '.');
    }

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

  fprintf(
      stream,
      "|%-6s|%-6s|%-6s|%-6s|%-6s|%-6s|\n|%06u|%06u|%06u|%06u|%06u|0x%04X|\n",
      "PC", "SP", "REG. A", "REG. X", "REG. Y", "REG. P", this->PC, this->SP,
      this->A, this->X, this->Y, this->P);

  for (int8_t i = 0; i < 43; ++i) {
    fprintf(stream, "-");
  }

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

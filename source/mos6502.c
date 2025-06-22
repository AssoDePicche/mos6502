#include "mos6502.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef void (*mos6502_instruction_handler)(MOS6502 *);

static void LDA_Immediate_Mode(MOS6502 *this) {
  this->A = mos6502_fetch_byte(this);

  const uint8_t status = (0x0 == this->A) ? MOS6502_STATUS_Z : MOS6502_STATUS_N;

  mos6502_set_status(this, status);
}

static void LDA_Zero_Page_Mode(MOS6502 *this) {
  const uint8_t zero_page_address = mos6502_fetch_byte(this);

  this->A = mos6502_read(this, zero_page_address);

  --this->current_instruction_cycles;

  const uint8_t status = (0x0 == this->A) ? MOS6502_STATUS_Z : MOS6502_STATUS_N;

  mos6502_set_status(this, status);
}

static void LDA_Zero_Page_X_Mode(MOS6502 *this) {
  const uint8_t zero_page_x_address = mos6502_fetch_byte(this) + this->X;

  --this->current_instruction_cycles;

  this->A = mos6502_read(this, zero_page_x_address);

  --this->current_instruction_cycles;

  const uint8_t status = (0x0 == this->A) ? MOS6502_STATUS_Z : MOS6502_STATUS_N;

  mos6502_set_status(this, status);
}

static void JSR_Absolute_Mode(MOS6502 *this) {
  const uint8_t subroutine_address = mos6502_fetch_word(this);

  mos6502_write_word(this, this->PC - 1, subroutine_address);

  ++this->SP;

  this->PC = subroutine_address;

  --this->current_instruction_cycles;
}

static const mos6502_instruction_handler MOS6502_INSTRUCTIONS_TABLE[256] = {
    [MOS6502_LDA_IMMEDIATE_MODE] = LDA_Immediate_Mode,
    [MOS6502_LDA_ZERO_PAGE_MODE] = LDA_Zero_Page_Mode,
    [MOS6502_LDA_ZERO_PAGE_X_MODE] = LDA_Zero_Page_X_Mode,
    [MOS6502_JSR_ABSOLUTE_MODE] = JSR_Absolute_Mode,
};

MOS6502 *mos6502_construct(void) {
  MOS6502 *this = (MOS6502 *)malloc(sizeof(MOS6502));

  if (NULL == this) {
    return NULL;
  }

  mos6502_reset(this);

  return this;
}

void mos6502_destruct(MOS6502 *this) {
  assert(NULL != this);

  free(this);
}

void mos6502_reset(MOS6502 *this) {
  memset(this->BUS, 0, MOS6502_BUS_SIZE);

  this->current_instruction_cycles = 0;

  this->PC = MOS6502_VEC_RESET;

  this->A = 0;

  this->X = 0;

  this->Y = 0;

  mos6502_set_status(this, MOS6502_STATUS_I | MOS6502_STATUS_B);

  this->SP = 0xFD;
}

uint8_t mos6502_read(const MOS6502 *this, const uint16_t address) {
  return this->BUS[address];
}

void mos6502_write(MOS6502 *this, const uint16_t address, const uint8_t value) {
  this->BUS[address] = value;
}

void mos6502_set_status(MOS6502 *this, const uint8_t status) {
  this->P |= (status & 0xFF);
}

uint8_t mos6502_get_status(MOS6502 *this, const uint8_t status) {
  return (this->P & status) != 0;
}

void mos6502_clear_status(MOS6502 *this, const uint8_t status) {
  this->P &= ~status;
}

void mos6502_push(MOS6502 *this, const uint8_t value) {
  assert(0x00 != this->SP);

  this->BUS[MOS6502_STACK + this->SP] = value;

  --this->SP;
}

uint8_t mos6502_pop(MOS6502 *this) {
  assert(0xFF != this->SP);

  ++this->SP;

  return this->BUS[MOS6502_STACK + this->SP];
}

uint8_t mos6502_fetch_byte(MOS6502 *this) {
  --this->current_instruction_cycles;

  return this->BUS[this->PC++];
}

static int is_big_endian(void) {
  static const unsigned int i = 1;

  static const char *c = (char *)&i;

  return (*c == 0);
}

uint8_t mos6502_fetch_word(MOS6502 *this) {
  this->current_instruction_cycles -= 2;

  if (is_big_endian()) {
    return (this->BUS[this->PC] << 8) | this->BUS[++this->PC];
  }

  return this->BUS[this->PC] | (this->BUS[++this->PC] << 8);
}

void mos6502_write_word(MOS6502 *this, const uint8_t word,
                        const uint16_t address) {
  this->BUS[address] = word & 0xFF;

  this->BUS[address + 1] = (word >> 8);

  this->current_instruction_cycles -= 2;
}

void mos6502_execute(MOS6502 *this) {
  while (0 != this->current_instruction_cycles) {
    const uint8_t instruction = mos6502_fetch_byte(this);

    fprintf(stdout, "Executing instruction %X\n", instruction);

    const mos6502_instruction_handler handler =
        MOS6502_INSTRUCTIONS_TABLE[instruction];

    if (NULL == handler) {
      fprintf(stderr, "Instruction %X not implemented\n", instruction);
      break;
    }

    handler(this);
  }
}

void mos6502_debug(const MOS6502 *this, FILE *stream) {
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

  fprintf(stream, "\n");
}

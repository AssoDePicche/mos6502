#include "mos6502.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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
  memset(this->memory, 0, MOS6502_MEM_SIZE);

  this->PC = MOS6502_VEC_RESET;

  this->A = 0;

  this->X = 0;

  this->Y = 0;

  mos6502_set_status(this, MOS6502_STATUS_I | MOS6502_STATUS_B);

  this->SP = 0xFD;
}

uint8_t mos6502_read(const MOS6502 *this, const uint16_t address) {
  return this->memory[address];
}

void mos6502_write(MOS6502 *this, const uint16_t address, const uint8_t value) {
  this->memory[address] = value;
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

  this->memory[MOS6502_STACK + this->SP] = value;

  --this->SP;
}

uint8_t mos6502_pop(MOS6502 *this) {
  assert(0xFF != this->SP);

  ++this->SP;

  return this->memory[MOS6502_STACK + this->SP];
}

void mos6502_debug(const MOS6502 *this, FILE *stream) {
  int change_region = 0;

  for (uint32_t index = 0; index < MOS6502_MEM_SIZE; ++index) {
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

    const uint64_t byte = this->memory[index];

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

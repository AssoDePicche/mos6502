#include "mos6502.h"

#include <assert.h>
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

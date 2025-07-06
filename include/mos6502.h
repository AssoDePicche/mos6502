#ifndef __MOS6502__
#define __MOS6502__

#include <stdint.h>
#include <stdio.h>

#define MOS6502_STATUS_C 0x01
#define MOS6502_STATUS_Z 0x02
#define MOS6502_STATUS_I 0x04
#define MOS6502_STATUS_D 0x08
#define MOS6502_STATUS_B 0x10
#define MOS6502_STATUS_V 0x40
#define MOS6502_STATUS_N 0x80

#define MOS6502_BUS_SIZE 0x10000
#define MOS6502_ZERO_PAGE 0x0000
#define MOS6502_STACK 0x0100
#define MOS6502_RAM 0x0200
#define MOS6502_ROM 0x8000
#define MOS6502_VEC_NMI 0xFFFA
#define MOS6502_VEC_RESET 0xFFFC
#define MOS6502_VEC_IRQ 0xFFFE

typedef enum {
  MOS6502_LDX_IMMEDIATE_MODE = 0xA2,
  MOS6502_LDA_ABSOLUTE_X_MODE = 0xBD,
  MOS6502_BEQ_RELATIVE_MODE = 0xF0,
  MOS6502_STA_ABSOLUTE_MODE = 0x8D,
  MOS6502_INX_IMPLIED_MODE = 0xE8,
  MOS6502_JMP_ABSOLUTE_MODE = 0x4C,
  MOS6502_BRK_IMPLIED_MODE = 0x00,
} MOS6502_Opcode;

typedef struct {
  uint8_t BUS[MOS6502_BUS_SIZE];
  uint16_t PC;
  uint8_t A;
  uint8_t X;
  uint8_t Y;
  uint8_t P;
  uint8_t SP;
} MOS6502;

MOS6502 *mos6502_construct(void);

void mos6502_destruct(MOS6502 *);

uint8_t mos6502_read(const MOS6502 *, const uint16_t);

void mos6502_write(MOS6502 *, const uint16_t, const uint8_t);

void mos6502_set_status(MOS6502 *, const uint8_t);

uint8_t mos6502_get_status(const MOS6502 *, const uint8_t);

void mos6502_clear_status(MOS6502 *, const uint8_t);

void mos6502_push(MOS6502 *, const uint8_t);

uint8_t mos6502_pop(MOS6502 *);

void mos6502_execute(MOS6502 *);

void mos6502_dump(const MOS6502 *, FILE *);

void mos6502_dump_status(const MOS6502 *, FILE *);

void mos6502_update_z_n_status(MOS6502 *, const uint8_t);

void LDX_IMMEDIATE_MODE(MOS6502 *);

void LDA_ABSOLUTE_X_MODE(MOS6502 *);

void BEQ_RELATIVE_MODE(MOS6502 *);

void STA_ABSOLUTE_MODE(MOS6502 *);

void INX_IMPLIED_MODE(MOS6502 *);

void JMP_ABSOLUTE_MODE(MOS6502 *);

void BRK_IMPLIED_MODE(MOS6502 *);

#endif

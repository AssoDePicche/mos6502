# MOS 6502

Um emulador de MOS6502 com as seguintes instruções implementadas:

- LDX Immediate Mode
- LDA Absolute X Mode
- BEQ Relative Mode
- STA Absolute Mode
- INX Implied Mode
- JMP Absolute Mode
- BRK Implied Mode

Obs: Immediate, Absolute X, Relative, Absolute e Implied se referem ao modo com o endereçamento de acesso na memória é realizado. Para mais informações acesse:

- [SY6500 Datasheet](https://www.princeton.edu/~mae412/HANDOUTS/Datasheets/6502.pdf)
- [MCS6500 Datasheet](https://www.mdawson.net/vic20chrome/cpu/mos_6500_mpu_preliminary_may_1976.pdf)
- [W65C02S Datasheet](https://eater.net/datasheets/w64c02s.pdf)
- [MOS Technology 6502](https://en.wikipedia.org/wiki/MOS_Technology_6502)
- [6502 Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)
- [Instruction reference](https://www.nesdev.org/wiki/Instruction_reference)

## Dependências

- CMake
- GCC
- FLEX
- YACC

## Como executar

Com as dependências instaladas basta executar o arquivo **build.sh** na raiz do diretório. Após isso use o comando abaixo para testar arquivos .asm compatíveis com a arquitetura do MOS6502.


```bash
./build/mos6502 6502.asm
```

## Exemplo

Para um arquivo como abaixo:

```asm
.ORG $0300

START:
    LDX #$00

LOOP:
    LDA MESSAGE,X

    BEQ END_PROGRAM

    STA $FD00

    INX
    JMP LOOP

END_PROGRAM:
    BRK

MESSAGE:
    .BYTE "HELLO, WORLD!", $0D, $0A, $00
```

A saída é esta:

```bash
MOS6502: Writing '0xA2' on '0x0300' address
MOS6502: Writing '0x00' on '0x0301' address
MOS6502: Writing '0xBD' on '0x0302' address
MOS6502: Writing '0xF0' on '0x0305' address
MOS6502: Writing '0x8D' on '0x0307' address
MOS6502: Writing '0x00' on '0x0308' address
MOS6502: Writing '0xFD' on '0x0309' address
MOS6502: Writing '0xE8' on '0x030A' address
MOS6502: Writing '0x4C' on '0x030B' address
MOS6502: Writing '0x00' on '0x030E' address
MOS6502: Writing '0x48' on '0x030F' address
MOS6502: Writing '0x45' on '0x0310' address
MOS6502: Writing '0x4C' on '0x0311' address
MOS6502: Writing '0x4C' on '0x0312' address
MOS6502: Writing '0x4F' on '0x0313' address
MOS6502: Writing '0x2C' on '0x0314' address
MOS6502: Writing '0x20' on '0x0315' address
MOS6502: Writing '0x57' on '0x0316' address
MOS6502: Writing '0x4F' on '0x0317' address
MOS6502: Writing '0x52' on '0x0318' address
MOS6502: Writing '0x4C' on '0x0319' address
MOS6502: Writing '0x44' on '0x031A' address
MOS6502: Writing '0x21' on '0x031B' address
MOS6502: Writing '0x0D' on '0x031C' address
MOS6502: Writing '0x0A' on '0x031D' address
MOS6502: Writing '0x00' on '0x031E' address
Yacc: Resolving forward references...
MOS6502: Writing '0x0F' on '0x0303' address
MOS6502: Writing '0x03' on '0x0304' address
MOS6502: Writing '0x07' on '0x0306' address
MOS6502: Writing '0x02' on '0x030C' address
MOS6502: Writing '0x03' on '0x030D' address
Yacc: Forward references resolved.
Yacc: MOS6502 execution started.
MOS6502: Reading address '0x031F'
MOS6502: Executing instruction 0x00 at 0x031F
MOS6502: Break command (BRK). Pushing PC and P, jumping to IRQ/BRK vector.
MOS6502: Pushing 0x03 on STACK 0x01FD address
MOS6502: Decrementing STACK POINTER to 0xFC
MOS6502: Pushing 0x21 on STACK 0x01FC address
MOS6502: Decrementing STACK POINTER to 0xFB
MOS6502: Pushing 0x10 on STACK 0x01FB address
MOS6502: Decrementing STACK POINTER to 0xFA
MOS6502: Reading address '0xFFFE'
MOS6502: Reading address '0xFFFF'
-------------------------------------------
STACK
ADDRESS 0x01FB 10 .
ADDRESS 0x01FC 21 !
ADDRESS 0x01FD 03 .
-------------------------------------------
RAM
ADDRESS 0x0300 A2 .
ADDRESS 0x0302 BD .
ADDRESS 0x0303 0F .
ADDRESS 0x0304 03 .
ADDRESS 0x0305 F0 .
ADDRESS 0x0306 07 .
ADDRESS 0x0307 8D .
ADDRESS 0x0309 FD .
ADDRESS 0x030A E8 .
ADDRESS 0x030B 4C L
ADDRESS 0x030C 02 .
ADDRESS 0x030D 03 .
ADDRESS 0x030F 48 H
ADDRESS 0x0310 45 E
ADDRESS 0x0311 4C L
ADDRESS 0x0312 4C L
ADDRESS 0x0313 4F O
ADDRESS 0x0314 2C ,
ADDRESS 0x0315 20
ADDRESS 0x0316 57 W
ADDRESS 0x0317 4F O
ADDRESS 0x0318 52 R
ADDRESS 0x0319 4C L
ADDRESS 0x031A 44 D
ADDRESS 0x031B 21 !
ADDRESS 0x031C 0D .
ADDRESS 0x031D 0A .
-------------------------------------------
|PC    |SP    |REG. A|REG. X|REG. Y|REG. P|
|0x0000|0x00FA|0x0000|0x0000|0x0000|0x0004|
-------------------------------------------
|REG. P FLAGS |
---------------
|N|V|B|D|I|Z|C|
|0|0|0|0|1|0|0|
---------------
Yacc: MOS6502 execution finished.
```

Ao fim da execução do programa em assembly carregado no emulador é possível ver o dump de sua memória (separada em seções) e de seus registradores.

Obs: Para executar os testes unitários implementados em **tests/mos6502.c** é preciso executar o seguinte comando após compilar o programa:

```bash
./build/tests
```

## Equipe

Samuel do Prado Rodrigues (GU3052788)

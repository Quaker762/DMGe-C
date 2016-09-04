/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        dmgcpu.h

Abstract:
        Interface for CPU related functions

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#ifndef DMGCPU_H_INCLUDED
#define DMGCPU_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#define ZERO_FLAG   0x80
#define SUB_FLAG    0x40
#define HC_FLAG     0x20
#define CARRY_FLAG  0x10

// Register data type.
// Able to access both bytes individually, as well as the entire word
typedef union
{
    struct
    {
        uint8_t lo;
        uint8_t hi;
    };
    uint16_t word;
} register16_t;

// Not the best to use
typedef struct
{
    //Our basic register, AF, BC, DE, HL
    register16_t AF, BC, DE, HL;

    //Special Registers
    register16_t SP, PC;
} registers_t;

typedef struct
{
    uint32_t m, t; // Machine cycles
} clock_t;

typedef struct
{
    bool        running;
    clock_t     clock;
} cpu_t;

typedef struct
{
    const char* name;
    void*       operation;
    uint32_t    m_cycles, t_cycles;
} instruction_t;

void cpu_init(void* gb);
void cpu_cycle();


#endif // DMGCPU_H_INCLUDED

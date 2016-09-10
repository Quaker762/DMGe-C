/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        apu.h

Abstract:
        Defines the Audio Processing Unit data structure.

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#ifndef APU_H_INCLUDED
#define APU_H_INCLUDED

#include <stdint.h>

typedef struct
{
    void        (*write_reg)(uint16_t address, uint8_t data);
    uint8_t     (*read_reg)(uint16_t address);

    //TODO: WHAT DATA DO WE NEED TO HAVE HERE?!?!?!?!
} apu_t;

void apu_init(void* gb);
void apu_cycle(uint32_t clock);

#endif // APU_H_INCLUDED

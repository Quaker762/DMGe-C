/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        mmu.h

Abstract:
        Memory and ROM related functions.

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#ifndef MMU_H_INCLUDED
#define MMU_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    void        (*write8)(uint16_t addr, uint8_t data);
    void        (*write16)(uint16_t addr, uint16_t data);
    uint8_t     (*read8)(uint16_t addr);
    uint16_t    (*read16)(uint16_t addr);

    void        (*map_rom)(void);
    bool        biosmapped;
} mmu_t;

void mmu_init(void* gb);
void load_rom(const char* rom_name);



#endif // MMU_H_INCLUDED

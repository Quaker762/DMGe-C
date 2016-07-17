/*++

Copyright (c) 2016  Radial Technologies

Module Name:

Abstract:

Author:

Environment:

Notes:

Revision History:

--*/
#ifndef MMU_H_INCLUDED
#define MMU_H_INCLUDED

#include <stdint.h>

typedef struct
{
    void        (*write8)(uint16_t addr, uint8_t data);
    void        (*write16)(uint16_t addr, uint16_t data);
    uint8_t     (*read8)(uint16_t addr);
    uint16_t    (*read16)(uint16_t addr);
} mmu_t;

void mmu_init(void* gb);



#endif // MMU_H_INCLUDED

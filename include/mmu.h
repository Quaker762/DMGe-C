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

#define INTFLAG_VBLANK  0x01
#define INTFLAG_LCD     0x02
#define INTFLAG_TIMER   0x04
#define INTFLAG_SERIAL  0x08
#define INTFLAG_JOYPAD  0x10

typedef struct
{
    void        (*write8)(uint16_t addr, uint8_t data);
    void        (*write16)(uint16_t addr, uint16_t data);
    uint8_t     (*read8)(uint16_t addr);
    uint16_t    (*read16)(uint16_t addr);

    void        (*map_rom)(void);
    void        (*set_ime_bit)(uint8_t bit);
    void        (*unset_ime_bit)(uint8_t bit);
    void        (*set_int_bit)(uint8_t bit);
    void        (*unset_int_bit)(uint8_t bit);
    bool        biosmapped;
} mmu_t;

void mmu_init(void* gb);
void load_rom(const char* rom_name);



#endif // MMU_H_INCLUDED

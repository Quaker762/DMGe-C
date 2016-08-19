/*++

Copyright (c) 2016  Radial Technologies

Module Name:
        gpu.c

Abstract:
        Interface for GPU related functions

Author:
        Quaker762

Environment:

Notes:

Revision History:

--*/
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <gameboy.h>
#include <gpu.h>

#define VRAM_OFFSET 0x8000

gameboy_t* gameboy;

uint8_t vram_read(uint16_t address)
{
    return gameboy->mmu.read8(address)
}

void gpu_init()
{
    gameboy = (gameboy_t*)gb;
    gb->gpu.vram_read = &vram_read();
}

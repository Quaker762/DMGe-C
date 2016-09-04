/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        gpu.c

Abstract:
        Interface for GPU related functions

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#include <stdio.h>
#include <string.h>
#include <gameboy.h>
#include <mmu.h>
#include <gpu.h>

#define VRAM_OFFSET 0x8000

gameboy_t* gameboy;

uint8_t vram_read(uint16_t address)
{
    return gameboy->mmu.read8(address);
}

void gpu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;
    //gameboy->gpu.vram_read = &vram_read;

    printf("GPU Initialised Successfully!\n");
}

void gpu_cycle(void)
{

}

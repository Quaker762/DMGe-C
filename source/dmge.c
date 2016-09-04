/*++

Copyright (c) 2016  Mosaic Software

Module Name:

Abstract:

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#include <stdlib.h>
#include <gameboy.h>
#include <dmgcpu.h>
#include <mmu.h>
#include <cart.h>
#include <gpu.h>

gameboy_t* gb;

int main()
{
    char* romname[128];

    gb = (gameboy_t*)malloc(sizeof(gameboy_t));

    cpu_init(gb);
    mmu_init(gb);
    gpu_init(gb);

    load_rom("roms/tetris.gb");

    gb->cpu.running = true;

    while(gb->cpu.running)
    {
        cpu_cycle();
        gpu_cycle(gb->cpu.clock.m);
    }

    free(&gb);
    return 0;
}

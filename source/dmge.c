/*++

Copyright (c) 2016  Mosaic Software

Module Name:

Abstract:

Author:

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
    gb = (gameboy_t*)malloc(sizeof(gameboy_t));

    cpu_init(gb);
    mmu_init(gb);
    gpu_init(gb);

    gb->cpu.running = true;
    load_rom("roms/tetris.gb");

    while(gb->cpu.running)
    {
        cpu_cycle();
    }

    free(&gb);
    return 0;
}

/*++

Copyright (c) 2016  Radial Technologies

Module Name:

Abstract:

Author:

Environment:

Notes:

Revision History:

--*/
#include <gameboy.h>
#include <dmgcpu.h>
#include <mmu.h>

gameboy_t gb;

int main()
{
    cpu_init(&gb);
    mmu_init(&gb);

    gb.cpu.running = true;
    load_rom("../roms/bios.bin");
    cycle();

    return 0;
}

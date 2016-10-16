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
#include <apu.h>
#include <sdl_win.h>

#include <windows.h>

gameboy_t* gb;

HANDLE emuthread;

void dmge_quit(void)
{
    TerminateThread(emuthread, 0x00);
    exit(0);
}

DWORD WINAPI dmge_cycle(void* arg)
{
    while(gb->cpu.running)
    {
        cpu_cycle();
        gpu_cycle(gb->cpu.clock.m);
    }
}

int main()
{
    char* romname[128];

    gb = (gameboy_t*)malloc(sizeof(gameboy_t));

    vid_init();

    cpu_init(gb);
    mmu_init(gb);
    gpu_init(gb);
    apu_init(gb);

    load_rom("roms/tetris.gb");

    gb->cpu.running = true;
    gb->mmu.biosmapped = true;

    emuthread = CreateThread(NULL, 0, dmge_cycle, NULL, 0, 0); // Apparently argument 3 is an 'incompatible pointer type'. Fuck off GCC, I'm doing what MS is telling me to do.

    while(true)
    {
        vid_refresh();
    }

    WaitForSingleObject(emuthread, INFINITE);

    gb->cpu.running = false;
    free(&gb);

    return 0;
}

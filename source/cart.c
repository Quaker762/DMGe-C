/*++

Copyright (c) 2016  Radial Technologies

Module Name:

Abstract:

Author:

Environment:

Notes:

Revision History:

--*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmu.h>

int load_rom(const char* fname)
{
    uint32_t    size;
    uint8_t*    rombuff;
    FILE*       rom;

    rom = fopen(fname, "rb");
    fseek(rom, 0L, SEEK_END);
    size = ftell(rom);
    rewind(rom);

    fread(rombuff, 1, 0x8000, rom); // Read the first 8KiB of the ROM into RAM


    return 0;
}

/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        cart.c

Abstract:
        Game Cart related functions.

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmu.h>

//int load_rom(const char* fname)
//{
//    uint32_t    size    = 0;
//    uint8_t*    rombuff = (uint8_t*)malloc(0x8000);
//    FILE*       rom;
//
//    if((rom = fopen(fname, "rb")) == 0)
//    {
//        fseek(rom, 0, SEEK_END);
//        size = ftell(rom);
//        rewind(rom);
//    }
//    else
//    {
//        printf("FAILED TO LOAD ROM, %s!\n", fname);
//        return -1;
//    }
//
//
//    fread(rombuff, 1, 0x8000, rom); // Read the first 8KiB of the ROM into RAM
//    fclose(rom);
//
//    return 0;
//}

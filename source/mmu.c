/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        mmu.c

Abstract:
        Memory related functions

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <gameboy.h>

#define RAM_SIZE 0xFFFF

uint8_t     ram[0xFFFF]; //64-KiB RAM
uint8_t     rom[0x8000]; // ROM including all possible banks.
gameboy_t*  gameboy;

// VERY RUDIMENTARY!!
static uint8_t read8(uint16_t address)
{
    return ram[address];
}

// VERY RUDIMENTARY!!
static uint16_t read16(uint16_t address)
{
    return (read8(address) | (read8(address+1) << 8));
}

static void write8(uint16_t address, uint8_t data)
{
    ram[address] = data;
}

static void write16(uint16_t address, uint16_t data)
{
    ram[address]    = data & 0xFF;
    ram[address+1]  = data >> 8;
}

static void map_rom()
{
    memcpy(ram, rom, 0x8000);
}

static int load_bios()
{
    char    bios[256];
    FILE*   rom;

    rom = fopen("roms/bios.bin", "rb");
    memset(bios, 0x00, 256);
    fread(bios, 256, sizeof(uint8_t), rom);
    fclose(rom);

    memcpy(ram, bios, 256);

    return 0;
}

void load_rom(const char* romname)
{
    FILE*       from;

    if((from = fopen(romname, "rb")) != 0)
    {
        fread(rom, 0x8000, sizeof(uint8_t), from);
        memcpy(ram + 0x100, rom + 0x100, 0x8000 - 0x100);
    }
    else
    {
        printf("Unable to load ROM: %s!\n", romname);
    }
    fclose(from);
}

void mmu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;

    memset(ram, 0x00, RAM_SIZE); // Zero out the contents of RAM
    gameboy->mmu.read8   = &read8;
    gameboy->mmu.read16  = &read16;
    gameboy->mmu.write8  = &write8;
    gameboy->mmu.write16 = &write16;
    gameboy->mmu.map_rom = &map_rom;

    load_bios();
    gameboy->mmu.biosmapped = true;

    printf("MMU Initialised Succesfuly!\n");
}


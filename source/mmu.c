/*++

Copyright (c) 2016  Radial Technologies

Module Name:

Abstract:

Author:

Environment:

Notes:

Revision History:

--*/
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <gameboy.h>

gameboy_t*  gameboy;
uint8_t     ram[0xFFFF]; //64-KiB RAM

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

void mmu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;

    memset(ram, 0x00, 0xFFFF); // Zero out the contents of RAM
    gameboy->mmu.read8   = &read8;
    gameboy->mmu.read16  = &read16;
    gameboy->mmu.write8  = &write8;
    gameboy->mmu.write16 = &write16;

    printf("MMU Initialised Succesfuly!\n");
}


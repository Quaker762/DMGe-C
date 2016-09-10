/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        apu.c

Abstract:
        Implementation of apu.h

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#include <apu.h>
#include <gameboy.h>

gameboy_t* gameboy;

static uint8_t read_reg(uint16_t address)
{
    switch(address)
    {
        printf("apu: attempting read of register 0x%04x...\n", address);
    }

    return 0x00;
}

static void write_reg(uint16_t address, uint8_t data)
{
    switch(address)
    {
        printf("apu: attempting to write register 0x%04x with value 0x%04x...\n", address, data);

        default:
        printf("wot...\n");
    }
}

void apu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;

    gameboy->apu.read_reg   = &read_reg;
    gameboy->apu.write_reg  = &write_reg;

    printf("APU Initialised Successfully!\n");
}

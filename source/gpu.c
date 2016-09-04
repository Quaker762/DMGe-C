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
    if(address < VRAM_BASE || address > VRAM_END) // Probably not necessary, but for debugging purposes sorta handy
    {
        printf("gpu: Error reading address %x (out of bounds)");
        return 0x0000; // This will fuck some shit up tbh fam
    }
    return gameboy->mmu.read8(address);
}

void vram_write(uint16_t address, uint8_t val)
{
    if(address < VRAM_BASE || address > VRAM_END)
    {
        printf("gpu: Error writing address %x (out of bounds)");
        return;
    }
    gameboy->mmu.write8(address, val);
}

void gpu_cycle(uint32_t clock)
{
    gameboy->gpu.stateclock += clock;

    // This is probably wrong
    switch(gameboy->gpu.state)
    {
        case STATE_OAM_READ:
            if(gameboy->gpu.stateclock >= 20)
            {
                gameboy->gpu.stateclock = 0;
                gameboy->gpu.state = STATE_VRAM_READ;
            }
        break;

        case STATE_VRAM_READ:
            if(gameboy->gpu.stateclock >= 43)
            {
                gameboy->gpu.stateclock = 0;
                gameboy->gpu.state = STATE_HBLANK;
                gameboy->gpu.render_scanline();
            }
        break;

        case STATE_HBLANK:
            if(gameboy->gpu.stateclock > 143)
            {
                gameboy->gpu.stateclock = 0;
                gameboy->gpu.line++;

                if(gameboy->gpu.line == 143)
                {
                    gameboy->gpu.state = STATE_VBLANK;
                    // Refresh the actual SDL frame or something here
                    gameboy->mmu.set_int_bit(INTFLAG_VBLANK);
                }
                else
                {
                    gameboy->gpu.state = STATE_OAM_READ;
                }
            }
        break;

        case STATE_VBLANK:
            if(gameboy->gpu.stateclock >= 114)
            {
                gameboy->gpu.stateclock = 0;
                gameboy->gpu.line++;

                if(gameboy->gpu.line > 153)
                {
                    gameboy->gpu.state = STATE_OAM_READ;
                    gameboy->gpu.line = 0;
                }
            }
        break;

        default:
            printf("gpu: state error, resetting state to OAM_READ\n");
            gameboy->gpu.state = STATE_OAM_READ;
    }
}

void write_reg(uint16_t address, uint8_t value)
{
    printf("gpu: attempting write to register 0x%04x with value 0x%02x...\n", address, value);

    switch(address)
    {
        case 0xFF40: //LCDC
        break;

        case 0xFF41: //STAT
        break;

        case 0xFF42: //SCY
            gameboy->gpu.scy = value;
        break;
    }
}

void read_reg(uint16_t address)
{
    printf("gpu: attempting read of register 0x%04x...\n", address);

    switch(address)
    {
        case 0xFF42:
            return gameboy->gpu.scy;

        case 0xFF44:
            return gameboy->gpu.line;
    }
}

void render_scanline()
{

}

void gpu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;
    //gameboy->gpu.vram_read = &vram_read;

    gameboy->gpu.render_scanline    = &render_scanline;
    gameboy->gpu.write_reg          = &write_reg;
    gameboy->gpu.read_reg           = &read_reg;

    printf("GPU Initialised Successfully!\n");
}

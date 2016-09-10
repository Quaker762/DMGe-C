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

    switch(gameboy->gpu.state)
    {
        case STATE_HBLANK:
            if(gameboy->gpu.stateclock >= 204)
            {
                gameboy->gpu.line++;

                if(gameboy->gpu.line == 143)
                {
                    gameboy->mmu.set_int_bit(INTFLAG_VBLANK);
                    gameboy->gpu.state = STATE_VBLANK;
                }
                else
                {
                    gameboy->gpu.state = STATE_OAM_READ;
                }
                gameboy->gpu.stateclock -= 204;
            }
        break;

        case STATE_VBLANK:
            if(gameboy->gpu.stateclock >= 456)
            {
                gameboy->gpu.line++;

                if(gameboy->gpu.line > 153)
                {
                    gameboy->gpu.line = 0;
                    gameboy->gpu.state = STATE_OAM_READ;
                }
                gameboy->gpu.stateclock -= 456;
            }
        break;

        case STATE_OAM_READ:
            if(gameboy->gpu.stateclock >= 80)
            {
                gameboy->gpu.state = STATE_VRAM_READ;
                gameboy->gpu.stateclock -= 80;
            }
        break;

        case STATE_VRAM_READ:
            if(gameboy->gpu.stateclock >= 172)
            {
                gameboy->gpu.state = STATE_HBLANK;
                gameboy->gpu.stateclock -= 172;
            }
        break;

        default: // If we get here, something has seriously fucked up, I wouldn't be surprised if the emulator gets into an undefined state and crashes...
            printf("gpu: invalid gpu state 0x%x! Resetting...\n");
            gameboy->gpu.state = STATE_OAM_READ;
        break;
    }
}

static void write_reg(uint16_t address, uint8_t data)
{
    printf("gpu: attempting write to register 0x%04x with data 0x%02x...\n", address, data);

    switch(address)
    {
        case 0xFF40: //LCDC
        break;

        case 0xFF41: //STAT
        break;

        case 0xFF42: //SCY
            gameboy->gpu.scy = data;
        break;

        case 0xFF47: //WRITE PALETTE
            gameboy->gpu.palette[3] = (data & 0xC0) >> 6;
            gameboy->gpu.palette[2] = (data & 0x30) >> 4;
            gameboy->gpu.palette[1] = (data & 0x0C) >> 2;
            gameboy->gpu.palette[0] = (data & 0x03);
        break;
    }
}

static uint8_t read_reg(uint16_t address)
{
    //printf("gpu: attempting read of register 0x%04x...\n", address);

    switch(address)
    {
        case 0xFF42:
            return gameboy->gpu.scy;
        break;

        case 0xFF44:
            return gameboy->gpu.line;
        break;
    }

    return 0x00;
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

    gameboy->gpu.line = 0;
    gameboy->gpu.scx = 0;
    gameboy->gpu.scy = 0;
    gameboy->gpu.stateclock = 0;

    printf("GPU Initialised Successfully!\n");
}

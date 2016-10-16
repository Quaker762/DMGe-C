/*++

Copyright (c) 2016  Mosaic Software & Darren Anderson

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
#include <sdl_win.h>

#define VRAM_BASE 0x8000
#define VRAM_SIZE 0x2000


gameboy_t* gameboy;

pixel_t paletteref[4];

uint8_t vram[VRAM_SIZE];
uint8_t tileset[384][8][8];

uint8_t vram_read(uint16_t address)
{
    if(address < VRAM_BASE || address > VRAM_END) // Probably not necessary, but for debugging purposes sorta handy
    {
        printf("gpu: Error reading address %x (out of bounds)");
        return 0x0000; // This will fuck some shit up tbh fam
    }
    return vram[address];
}

void vram_write(uint16_t address, uint8_t val)
{
    if(address < VRAM_BASE || address > VRAM_END)
    {
        printf("gpu: Error writing address %x (out of bounds)");
        return;
    }
    vram[address] = val;
}

// This needs to be fuckin fixed ASAP!
void update_tile(uint16_t addr, uint16_t data)
{
    uint16_t addrtrans = addr & 0x1FFE;
    uint16_t tileindex = (addrtrans >> 4);
    uint8_t y = (addrtrans >> 1) & 7;

    uint8_t sx, x;

    for(x = 0; x < 8; x++)
    {
        sx = 1 << (7-x);

        tileset[tileindex][y][x] = ((gameboy->mmu.read8(addrtrans) & sx) ? 1 : 0)
        + ((gameboy->mmu.read8(addrtrans + 1) & sx) ? 2 : 0);
    }
}

void render_scanline()
{
    //TILEMAP 0 STARTS AT 0x8000!
    uint16_t tilemapbase = VRAM_BASE;
    uint16_t offsetbase = tilemapbase + ((((gameboy->gpu.line+gameboy->gpu.scy) & 255) >> 3) << 5);
    uint16_t x, y, tindex;

    y = (gameboy->gpu.line + gameboy->gpu.scy) & 0x07;

    for(x = 0; x < 160; x++)
    {
        tindex = gameboy->mmu.read8(offsetbase + (x/8));
        put_pixel(x, gameboy->gpu.line, paletteref[gameboy->gpu.palette[tileset[tindex][y][x % 8]]]);
    }
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
                gameboy->gpu.stateclock = 0;
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
                gameboy->gpu.stateclock = 0;
            }
        break;

        case STATE_OAM_READ:
            if(gameboy->gpu.stateclock >= 80)
            {
                gameboy->gpu.state = STATE_VRAM_READ;
                gameboy->gpu.stateclock = 0;
            }
        break;

        case STATE_VRAM_READ:
            if(gameboy->gpu.stateclock >= 172)
            {
                gameboy->gpu.state = STATE_HBLANK;

                render_scanline();
                gameboy->gpu.stateclock = 0;
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
    //printf("gpu: attempting write to register 0x%04x with data 0x%02x...\n", address, data);

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

void gpu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;
    gameboy->gpu.vram_write         = &vram_write;
    gameboy->gpu.vram_read          = &vram_read;

    gameboy->gpu.render_scanline    = &render_scanline;
    gameboy->gpu.write_reg          = &write_reg;
    gameboy->gpu.read_reg           = &read_reg;

    gameboy->gpu.line = 0;
    gameboy->gpu.scx = 0;
    gameboy->gpu.scy = 0;
    gameboy->gpu.stateclock = 0;

    memset(&tileset, 0x00, 384*8*8);

    paletteref[0] = (pixel_t){156, 189, 15};
    paletteref[1] = (pixel_t){140, 173, 15};
    paletteref[2] = (pixel_t){48, 98, 48};
    paletteref[3] = (pixel_t){15, 56, 15};

    printf("GPU Initialised Successfully!\n");
}

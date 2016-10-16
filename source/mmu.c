/*++

Copyright (c) 2016  Mosaic Software & Darren Anderson

Module Name:
        mmu.c

Abstract:
        Memory related functions

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

    TODO: Separate the BIOS and RAM into different arrays so we don't have to worry about remapping the lower 256 bytes of the ROM (the interrupt handlers)

Revision History:

--*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <mmu.h>
#include <gameboy.h>
#include <cart.h>

#define RAM_SIZE 0xFFFF

uint8_t     ram[0xFFFF]; //64-KiB RAM
uint8_t     rom[0x8000]; // ROM including all possible banks.
uint8_t     bios[0xFF];

gameboy_t*  gameboy;
cart_t*     cart;

const char* interrupts = {"VBLANK", "LCD_STAT", "TIMER", "SERIAL", "JOYPAD"};

// VERY RUDIMENTARY!!
static uint8_t read8(uint16_t address)
{
    if(address >= 0xFF10 && address <= 0xFF3F)
        return gameboy->apu.read_reg(address);

    if(address >= 0xFF40 && address <= 0xFF4B)
        return gameboy->gpu.read_reg(address);

    return ram[address];
}

// VERY RUDIMENTARY!!
static uint16_t read16(uint16_t address)
{
    return (read8(address) | (read8(address+1) << 8));
}

static void write8(uint16_t address, uint8_t data)
{
    if(address >= 0xFF10 && address <= 0xFF3F)
        gameboy->apu.write_reg(address, data);

    if(address >= 0xFF40 && address <= 0xFF4B)
        gameboy->gpu.write_reg(address, data);

    if(address >= 0x8000 && address <= 0x9FFF)
        update_tile(address, data);

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
    FILE*   rom;

    rom = fopen("roms/bios.bin", "rb");
    memset(bios, 0x00, 256);
    fread(bios, 0xFF, sizeof(uint8_t), rom);
    fclose(rom);

    return 0;
}

void load_rom(const char* romname)
{
    FILE*       from;

    if((from = fopen(romname, "rb")) != 0)
    {
        fread(rom, 0x8000, sizeof(uint8_t), from);
        memcpy(ram, rom, 0x8000);
        memcpy(ram, bios, 0xFF);
        memcpy(cart, ram + 0x134, sizeof(cart_t));
    }
    else
    {
        printf("Unable to load ROM: %s!\n", romname);
    }
    fclose(from);
}

void set_ime_bit(uint8_t bit)
{
    uint8_t ime_curr = gameboy->mmu.read8(0xFFFF);

    ime_curr |= bit;
    gameboy->mmu.write8(0xFFFF, ime_curr);
}

void unset_ime_bit(uint8_t bit)
{
    uint8_t ime_curr = gameboy->mmu.read8(0xFF0F);

    ime_curr &= ~bit;
    gameboy->mmu.write8(0xFF0F, ime_curr);
}

void set_int_bit(uint8_t bit)
{
    uint8_t ime_curr = gameboy->mmu.read8(0xFF0F);

    ime_curr |= bit;
    gameboy->mmu.write8(0xFF0F, ime_curr);
}

void unset_int_bit(uint8_t bit)
{
    uint8_t ime_curr = gameboy->mmu.read8(0xFFFF);

    ime_curr &= ~bit;
    gameboy->mmu.write8(0xFFFF, ime_curr);
}

void mmu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;
    cart = (cart_t*)malloc(sizeof(cart_t));

    memset(ram, 0x00, RAM_SIZE); // Zero out the contents of RAM
    memset(cart, 0x00, sizeof(cart_t));
    gameboy->mmu.read8          = &read8;
    gameboy->mmu.read16         = &read16;
    gameboy->mmu.write8         = &write8;
    gameboy->mmu.write16        = &write16;
    gameboy->mmu.map_rom        = &map_rom;
    gameboy->mmu.set_int_bit    = &set_int_bit;
    gameboy->mmu.unset_int_bit  = &unset_int_bit;
    gameboy->mmu.set_ime_bit    = &set_ime_bit;
    gameboy->mmu.unset_ime_bit  = &unset_ime_bit;

    load_bios();
    gameboy->mmu.biosmapped = true;

    printf("MMU Initialised Succesfuly!\n");
}

/*++

Copyright (c) 2016  Mosaic Software

Module Name
        cart.h

Abstract:
        Defines a logical GameBoy Game Cart and related functions.

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#ifndef CART_H_INCLUDED
#define CART_H_INCLUDED

#define ROM_ONLY            0x00
#define MBC1                0x01
#define MBC1_RAM            0x02
#define MBC1_RAM_BATT       0x03
#define MBC2                0x05
#define MBC2_BATT           0x06
#define RAM                 0x08
#define RAM_BATT            0x09
#define MMM01               0x0B
#define MMM01_SRAM          0x0C
#define MMM01_SRAM_BATT     0x0D
#define MBC3_TIMER_BATT     0x0F
#define MBC3_TIMER_RAM_BATT 0x10
#define MBC3                0x11
#define MBC3_RAM            0x12
#define MBC3_RAM_BATT       0x13
#define MBC5                0x19
#define MBC5_BATT           0x1A
#define MBC5_RAM_BATT       0x1B
#define MBC5_RUMBLE         0x1C
#define MBC5_RUMBLE_SRAM    0x1D
#define MBC5_RUMB_SRAM_BATT 0x1E
#define POCKET_CAMERA       0x1F
#define BANDAI_TAMA5        0xFD
#define HUDSON_HUC_3        0xFE
#define HUDSON_HUC_1        0xFF

typedef struct
{
    char        title[16];  // 11 character game title
    uint8_t     cgb_flag;   // GameBoy Colour flag
    uint16_t    license_code;
    uint8_t     sgb_flag;
    uint8_t     cart_type;
    uint8_t     rom_size;
    uint8_t     ram_size;
    uint8_t     destination_code;
    uint8_t     license_code_old;
    uint8_t     mask_rom_vernum;
    uint8_t     complement_number;
    uint16_t    checksum; // Ignored by GameBoy
} cart_t;

//int load_rom(const char* fname);



#endif // CART_H_INCLUDED

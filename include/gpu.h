/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        gpu.h

Abstract:
        Interface for GPU related functions

Author:
        Quaker762

Environment:

Notes:

Revision History:




--------------------------- FFFF  | 32kB ROMs are non-switchable and occupy
 I/O ports + internal RAM         | 0000-7FFF are. Bigger ROMs use one of two
--------------------------- FF00  | different bank switches. The type of a
 Internal RAM                     | bank switch can be determined from the
--------------------------- C000  | internal info area located at 0100-014F
 8kB switchable RAM bank          | in each cartridge.
--------------------------- A000  |
 16kB VRAM                        | MBC1 (Memory Bank Controller 1):
--------------------------- 8000  | Writing a value into 2000-3FFF area will
 16kB switchable ROM bank         | select an appropriate ROM bank at
--------------------------- 4000  | 4000-7FFF. Writing a value into 4000-5FFF
 16kB ROM bank #0                 | area will select an appropriate RAM bank
--------------------------- 0000  | at A000-C000.
                                  |
                                  | MBC2 (Memory Bank Controller 2):
                                  | Writing a value into 2100-21FF area will
                                  | select an appropriate ROM bank at
                                  | 4000-7FFF. RAM switching is not provided.
--*/
#ifndef GPU_H_INCLUDED
#define GPU_H_INCLUDED

#define STATE_HBLANK        0
#define STATE_VBLANK        1
#define STATE_OAM_READ      2
#define STATE_VRAM_READ     3

#define VRAM_BASE           0x8000
#define VRAM_END            0xA000
typedef struct
{
    void    (*vram_write)(uint16_t addr, uint8_t data);
    uint8_t (*vram_read)(uint16_t addr);
    void    (*render_scanline)();
    void    (*write_reg)(uint16_t address, uint8_t data);
    uint8_t (*read_reg)(uint16_t address);

    uint32_t    stateclock;
    uint8_t     state;
    uint8_t     line;
    uint8_t     tileset[384][8][8];
    uint8_t     scx;
    uint8_t     scy;
    uint8_t     wndx;
    uint8_t     wndy;
    uint8_t     bgbuffer[256][256];
    uint8_t     palette[4];
} gpu_t;

void gpu_init(void* gb);
void gpu_cycle(uint32_t clock);
void update_tile(uint16_t addr, uint16_t data);
#endif // GPU_H_INCLUDED

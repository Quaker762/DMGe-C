/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        sdl_win.h

Abstract:
        Functions and data related to the SDL interface.

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#ifndef SDL_WIN_H_INCLUDED
#define SDL_WIN_H_INCLUDED

#include <stdint.h>
#include <windows.h>

#define WINDOW_WIDTH    160
#define WINDOW_HEIGHT   144

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} pixel_t;

void put_pixel(uint8_t x, uint8_t y, pixel_t pixel);
void scale(uint8_t factor);

void            vid_init(void);
DWORD WINAPI    vid_refresh(void);


#endif // SDL_WIN_H_INCLUDED

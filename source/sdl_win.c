/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        sdl_win.c

Abstract:
        Implementation of sdl_win.h

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdio.h>

#include <sdl_win.h>
#include <gameboy.h>


SDL_Window*     gbhwnd;
SDL_Surface*    gbsurf;
SDL_Renderer*   gbrender;
SDL_Event       event;

pixel_t         pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
GLuint          tex;

void vid_init(void)
{
    memset(pixels, 0x00, sizeof(pixels)); // Clear the framebuffer

    // Initialise SDL iteself
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("sdl: Call to SDL_INIT_VIDEO failed: %s", SDL_GetError());
        exit(-1);
    }

    // Open a 160x144 SDL window
    gbhwnd = SDL_CreateWindow("DMGe", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if(gbhwnd == NULL)
    {
        printf("sdl: Call to SDL_CreateWindow() failed: %s", SDL_GetError());
        exit(-1);
    }

    gbrender = SDL_CreateRenderer(gbhwnd, -1, 0);
    SDL_GL_CreateContext(gbhwnd);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}

DWORD WINAPI vid_refresh(void)
{
    SDL_PumpEvents();
    SDL_PollEvent(&event);

    if(event.type == SDL_QUIT)
        dmge_quit();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, 5, 100);
    glTranslatef(0.0f, 0.0f, -6.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 160, 144, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixels);

	glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0f, 1.0f, 0.0f);					// Top Left
        glTexCoord2f(1.0, 0.0);
        glVertex3f( 1.0f, 1.0f, 0.0f);					// Top Right
        glTexCoord2f(1.0, 1.0);
        glVertex3f( 1.0f,-1.0f, 0.0f);					// Bottom Right
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0f,-1.0f, 0.0f);					// Bottom Left
    glEnd();

    SDL_GL_SwapWindow(gbhwnd);
}

void put_pixel(uint8_t x, uint8_t y, pixel_t pixel)
{
    pixels[(WINDOW_WIDTH*y) + x] = pixel;
}

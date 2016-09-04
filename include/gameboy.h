/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        gameboy.h

Abstract:
        Defines a logical 'gameboy', i.e, all the components attached to the actual device. Sort of like a C++ 'class'.
        Except it's not C++, because we all know C++ is a bloated crock of fucking shit.

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:

Revision History:

--*/
#ifndef GAMEBOY_H_INCLUDED
#define GAMEBOY_H_INCLUDED

#include <dmgcpu.h>
#include <mmu.h>
#include <gpu.h>

typedef struct
{
    cpu_t cpu;
    mmu_t mmu;
    gpu_t gpu;
} gameboy_t;


#endif // GAMEBOY_H_INCLUDED

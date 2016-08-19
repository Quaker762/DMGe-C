/*++

Copyright (c) 2016  Radial Technologies

Module Name:

Abstract:

Author:

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

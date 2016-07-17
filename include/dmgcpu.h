/*++

Copyright (c) 2016  Radial Technologies

Module Name:
        dmgcpu.h

Abstract:
        Interface for CPU related functions

Author:
        Quaker762

Environment:

Notes:

Revision History:

--*/
#ifndef DMGCPU_H_INCLUDED
#define DMGCPU_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#define ZERO_FLAG   0x80
#define SUB_FLAG    0x40
#define HC_FLAG     0x20
#define CARRY_FLAG  0x10

// Register data type.
// Able to access both bytes individually, as well as the entire word
typedef union
{
    struct
    {
        uint8_t lo;
        uint8_t hi;
    };
    uint16_t word;
} register16_t;

// Not the best to use
typedef struct
{
    //Our basic register, AF, BC, DE, HL
    register16_t AF, BC, DE, HL;

    //Special Registers
    register16_t SP, PC;
} registers_t;

typedef struct
{
    uint32_t m, t; // Machine cycles
} clock_t;

typedef struct
{
    bool running;
} cpu_t;

typedef struct
{
    const char* name;
    void*       operation;
} instruction_t;

void cpu_init(void* gb);
void cycle();

void nop();
void ld_bc_d16();
void ld_bcp_a();
void bc_inc();
void b_inc();
void b_dec();
void ld_b_d8();
void rlca();
void ld_a16_sp();
void add_hl_bc();
void ld_a_bcp();
void bc_dec();
void c_inc();
void c_dec();
void ld_c_d8();
void rrca(); // 0x0F

void stop();
void ld_de_d16();
void ld_dep_a();
void de_inc();
void d_inc();
void d_dec();
void ld_d_d8();
void rla();
void jr_r8();
void add_hl_de();
void ld_a_dep();
void de_dec();
void e_inc();
void e_dec();
void ld_e_d8();
void rra(); // 0x1F

void jr_nz_r8();
void ld_hl_d16();
void ldi_hlp_a();
void hl_inc();
void h_inc();
void h_dec();
void ld_h_d8();
void daa();
void jr_z_r8();
void add_hl_hl();
void ldi_a_hl();
void hl_dec();
void l_inc();
void l_dec();
void ld_l_d8();
void cpl(); //0x2F

void jr_nc_r8();
void ld_sp_d16();
void ldd_hlp_a();
void sp_inc();
void hlp_inc();
void hlp_dec();
void ld_hlp_d8();
void scf();
void jr_c_r8();
void add_hl_sp();
void ldd_a_hlp();
void sp_dec();
void a_inc();
void a_dec();
void ld_a_d8();
void ccf(); //0x3F







#endif // DMGCPU_H_INCLUDED

/*++

Copyright (c) 2016  Radial Technologies

Module Name:

Abstract:

Author:

Environment:

Notes:


Revision History:

--*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dmgcpu.h>
#include <gameboy.h>

gameboy_t* gameboy;

// General Purpose registers
static register16_t AF;
static register16_t BC;
static register16_t DE;
static register16_t HL;

// RESERVED REGISTERS. 16-bit ACCESS ONLY!!!
static register16_t PC;
static register16_t SP;

// Set true by STOP and HALT
// On a real GameBoy, STOP and HALT act (kinda) differently,
// but it doesn't matter on a PC because we don't have AA
// batteries in the back ahahaha!
static bool         halted = false;

instruction_t instructions[0xFF] =
{
    {"NOP",                 &nop},
    {"LD BC,d16",           &ld_bc_d16},
    {"LD (BC),A",           &ld_bcp_a},
    {"INC BC",              &bc_inc},
    {"INC B",               &b_inc},
    {"DEC B",               &b_dec},
    {"LD B,d8",             &ld_b_d8},
    {"RLCA",                &rlca},
    {"LD (a16),SP",         &ld_a16_sp},
    {"ADD HL,BC",           &add_hl_bc},
    {"LD A,(BC)",           &ld_a_bcp},
    {"DEC BC",              &bc_dec},
    {"INC C",               &c_inc},
    {"DEC C",               &c_dec},
    {"LD C,d8",             &ld_c_d8},
    {"RRCA",                &rrca},
    {"STOP 0",              &stop},
    {"LD DE,d16",           &ld_de_d16},
    {"LD (DE),A",           &ld_dep_a},
    {"INC DE",              &de_inc},
    {"INC D",               &d_inc},
    {"DEC D",               &d_dec},
    {"LD D,d8",             &ld_d_d8},
    {"RLA",                 &rla},
    {"JR r8",               &jr_r8},
    {"ADD HL,DE",           &add_hl_de},
    {"LD A,(DE)",           &ld_a_dep},
    {"DEC DE",              &de_dec},
    {"INC E",               &e_inc},
    {"DEC E",               &e_dec},
    {"LD E,d8",             &ld_e_d8},
    {"RRA",                 &rra},
    {"JR NZ,r8",            &jr_nz_r8},
    {"LD HL,d16",           &ld_hl_d16},
    {"LD (HL+),A",          &ldi_hlp_a},
    {"INC HL",              &hl_inc},
    {"INC H",               &h_inc},
    {"DEC H",               &h_dec},
    {"LD H,d8",             &ld_h_d8},
    {"DAA",                 &daa},
    {"JR Z,r8",             &jr_z_r8},
    {"ADD HL,HL",           &add_hl_hl},
    {"LD A,(HL+)",          &ldi_a_hl},
    {"DEC HL",              &hl_dec},
    {"INC L",               &l_inc},
    {"DEC L",               &l_dec},
    {"LD L,d8",             &ld_l_d8},
    {"CPL",                 &cpl},
    {"JR NC,r8",            &jr_nc_r8},
    {"LD SP,d16",           &ld_sp_d16},
    {"LD (HL-),A",          &ldd_hlp_a},
    {"INC SP",              &sp_inc},
    {"INC (HL)",            &hlp_inc},
    {"DEC (HL)",            &hlp_dec},
    {"LD (HL),d8",          &ld_hlp_d8},
    {"SCF",                 &scf},
    {"JR C,r8",             &jr_c_r8},
    {"ADD HL,SP",           &add_hl_sp},
    {"LD A,(HL-)",          &ldd_a_hlp},
    {"DEC SP",              &sp_dec},
    {"INC A",               &a_inc},
    {"DEC A",               &a_dec},
    {"LD A,d8",             &ld_a_d8},
    {"CCF",                 &ccf},
    {"LD B,B",              &ld_b_b},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
    {"NOP", NULL},
};

instruction_t instructionscb[0xFF] =
{

};

void cpu_init(void* gb)
{
    gameboy = (gameboy_t*)gb;

    AF.word = 0x0000;
    BC.word = 0x0000;
    DE.word = 0x0000;
    HL.word = 0x0000;
    SP.word = 0x0000;
    PC.word = 0x0000;
}

// Main Processor loop
void cycle()
{
    while(gameboy->cpu.running && halted == false)
    {
        switch(gameboy->mmu.read16(PC.word))
        {
            case 0xCB:
            {

            }
            default:
            {
                if(instructions[PC.word].operation != (void*)NULL)
                {

                }
                else
                {
                    gameboy->cpu.running = false;
                    printf("ILLEGAL INSTRUCTION %s AT ADDRESS 0x%4x! SYSTEM HALTED!\n",instructions[PC.word].name, PC.word);
                }
            }
        }
    }
}
///////////////////HELPER FUNCTIONS////////////////////////////////////

static inline void set_flag(uint8_t flag)
{
    AF.lo |= flag;
}

static inline void unset_flag(uint8_t flag)
{
    AF.lo = (AF.lo & (~flag));
}

// These functions are ONLY for operations in which the FLAGS register
// is affected to save us a lot of typing.
void inc(uint8_t* value)
{
    if((*value & 0x0F) == 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    *value++; // Perform increment
    unset_flag(SUB_FLAG);

    if(*value != 0)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);


}

void dec(uint8_t* value)
{
    if((*value & 0x0F) == 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    *value--; // Perform increment
    set_flag(SUB_FLAG);

    if(*value != 0)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);
}

void add8(uint8_t* reg, uint8_t value)
{
    uint16_t result = *reg + value;

    if(result & 0xFF00)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    *reg += value;

    if(*reg)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);

    if(((*reg & 0x0F) + (value & 0x0F)) > 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    unset_flag(SUB_FLAG);
}

void add16(uint16_t* reg, uint16_t value)
{
    uint32_t result = *reg + value;

    if(result & 0xFFFF0000)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    *reg += value;

    if(((*reg & 0x0F) + (value & 0x0F)) > 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    unset_flag(SUB_FLAG);
}

void sub(uint8_t value)
{
    if(value > AF.hi)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    if((value & 0x0F) > (AF.hi & 0x0F))
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    AF.hi -= value;

    if(AF.hi == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);

    set_flag(SUB_FLAG);

}
///////////////////////////////////////////////////////////////////////

// 0x00
void nop()
{
    PC.word++;
}

//0x01
void ld_bc_d16()
{
    BC.word = gameboy->mmu.read16(PC.word + 1);
    PC.word += 3;
}

//0x02
void ld_bcp_a()
{
    gameboy->mmu.write8(BC.word, AF.hi);
    PC.word++;
}

//0x03
void bc_inc()
{
    BC.word++;
    PC.word++;
}

//0x04
void b_inc()
{
    inc(&BC.hi);
    PC.word++;
}

//0x05
void b_dec()
{
    dec(&BC.hi);
    PC.word++;
}

//0x06
void ld_b_d8()
{
    BC.hi = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x07
void rlca()
{
    uint8_t carry = AF.hi >> 7;

    if(carry != 0)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    AF.hi <<= 1;
    AF.hi |= carry;

    unset_flag(ZERO_FLAG | HC_FLAG | SUB_FLAG);
    PC.word++;
}

//0x08
void ld_a16_sp()
{
    gameboy->mmu.write16(gameboy->mmu.read16(PC.word + 1), SP.word);
    PC.word += 3;
}

//0x09
void add_hl_bc()
{
    add16(HL.word, BC.word);
    PC.word++;
}

//0x0A
void ld_a_bcp()
{
    AF.hi = gameboy->mmu.read8(BC.word);
    PC.word++;
}

//0x0B
void bc_dec()
{
    BC.word--;
    PC.word++;
}

//0x0C
void c_inc()
{
    inc(BC.lo);
    PC.word++;
}

//0x0D
void c_dec()
{
    dec(BC.lo);
    PC.word++;
}

//0x0E
void ld_c_d8()
{
    BC.lo = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x0F
void rrca()
{
    uint8_t carry = AF.hi & 0x01;

    if(carry != 0)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    AF.hi >>= 1;
    AF.hi |= carry << 7;

    unset_flag(ZERO_FLAG | HC_FLAG | SUB_FLAG);
    PC.word++;
}

//0x10
void stop()
{
    halted = true;
}

//0x11
void ld_de_d16()
{
    DE.word = gameboy->mmu.read16(PC.word + 1);
    PC.word += 3;
}

//0x12
void ld_dep_a()
{
    gameboy->mmu.write8(DE.word, AF.hi);
    PC.word++;
}

//0x13
void de_inc()
{
    DE.word++;
    PC.word++;
}

//0x14void d_inc()
{
    inc(&DE.hi);
    PC.word++;
}

//0x15
void d_dec()
{
    dec(&DE.hi);
    PC.word++;
}

//0x16
void ld_d_d8()
{
    DE.hi = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x17
void rla()
{
    uint8_t carry = AF.hi >> 7;

    if(AF.hi & 0x80)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    AF.hi <<= 1;
    AF.hi |= carry;

    unset_flag(CARRY_FLAG | HC_FLAG | SUB_FLAG);
    PC.word++;
}

//0x18
void jr_r8()
{
    PC.word += (int8_t)gameboy->mmu.read8(PC.word + 1);
}

//0x19
void add_hl_de()
{
    add16(&HL.word, DE.word);
    PC.word++;
}

//0x1A
void ld_a_dep()
{
    AF.hi = gameboy->mmu.read8(DE.word);
    PC.word++;
}

//0x1B
void de_dec()
{
    DE.word--;
    PC.word++;
}

//0x1C
void e_inc()
{
    inc(&DE.lo);
    PC.word++;
}

//0x1D
void e_dec()
{
    dec(&DE.lo);
    PC.word++;
}

//0x1E
void ld_e_d8()
{
    DE.lo = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x1F
void rra()
{
    uint8_t carry = AF.hi & 0x1;

    if(AF.hi & 0x1)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    AF.hi >>= 1;
    AF.hi |= carry << 7;

    unset_flag(CARRY_FLAG | HC_FLAG | SUB_FLAG);
    PC.word++;
}

//0x20
void jr_nz_r8()
{
    if(!(AF.lo & CARRY_FLAG))
    {
        PC.word += (int8_t)gameboy->mmu.read8(PC.word + 1);
    }
    else
    {
        PC.word += 2;
    }
}

//0x21
void ld_hl_d16()
{
    HL.word = gameboy->mmu.read16(PC.word + 1);
    PC.word += 3;
}

//0x22
void ldi_hlp_a()
{
    gameboy->mmu.write8(HL.word, AF.hi);
    HL.word++;
    PC.word++;
}

//0x23
void hl_inc()
{
    HL.word++;
    PC.word++;
}

//0x24
void h_inc()
{
    inc(&HL.hi);
    PC.word++;
}

//0x25
void h_dec()
{
    dec(&HL.hi);
    PC.word++;
}

//0x26
void ld_h_d8()
{
    HL.hi = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x27 (Decimal Adjust A) Thanks Darren!
void daa()
{
    uint8_t a = AF.hi;

    if(AF.lo & SUB_FLAG)
    {
        if(AF.lo & HC_FLAG)
            a = (a - 0x06) & 0xFF;
        else
            a -= 0x60;
    }
    else
    {
        if(AF.lo & HC_FLAG || (a & 0x0F) > 9)
            a += 0x06;
        if(AF.lo & HC_FLAG || a > 0x9F)
            a += 0x60;
    }

    AF.hi = a;

    if(!AF.hi)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);

    if(a >= 0x100)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    PC.word++;
}

//0x28
void jr_z_r8()
{
    if(AF.lo & ZERO_FLAG)
    {
        PC.word += (int8_t)gameboy->mmu.read8(PC.word + 1);
    }
    else
    {
        PC.word += 2;
    }
}

//0x29
void add_hl_hl()
{
    add16(&HL.word, HL.word);
    PC.word++;
}

//0x2A
void ldi_a_hl()
{
    AF.hi = gameboy->mmu.read8(HL.word);
    HL.word++;
    PC.word++;
}

//0x2B
void hl_dec()
{
    HL.word--;
    PC.word++;
}

//0x2C
void l_inc()
{
    inc(&HL.lo);
    PC.word++;
}

//0x2D
void l_dec()
{
    dec(&HL.lo);
    PC.word++;
}

//0x2E
void ld_l_d8()
{
    HL.lo = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x2F
void cpl()
{
    AF.hi = ~AF.hi;
    set_flag(HC_FLAG);
    set_flag(SUB_FLAG);
    PC.word++;
}

//0x30
void jr_nc_r8()
{
    if(!(AF.lo & CARRY_FLAG))
    {
        PC.word += (int8_t)gameboy->mmu.read8(PC.word + 1);
    }
    else
    {
        PC.word += 2;
    }
}

//0x31
void ld_sp_d16()
{
    SP.word = gameboy->mmu.read16(PC.word + 1);
    PC.word += 3;
}

//0x32
void ldd_hlp_a()
{
    gameboy->mmu.write16(HL.word, AF.hi);
    HL.word--;
    PC.word++;
}

//0x33
void sp_inc()
{
    SP.word++;
    PC.word++;
}

//0x34
void hlp_inc()
{
    uint8_t value = gameboy->mmu.read8(HL.word);
    inc(&value);
    gameboy->mmu.write8(HL.word, value);
    PC.word++;
}

//0x35
void hlp_dec()
{
    uint8_t value = gameboy->mmu.read8(HL.word);
    dec(&value);
    gameboy->mmu.write8(HL.word, value);
    PC.word++;
}

//0x36
void ld_hlp_d8()
{
    gameboy->mmu.write8(HL.word, gameboy->mmu.read(PC.word + 1));
    PC.word += 2;
}

//0x37 (Set Carry Flag)
void scf()
{
    set_flag(CARRY_FLAG);
    unset_flag(HC_FLAG);
    unset_flag(SUB_FLAG);
    PC.word++;
}

//0x38
void jr_c_r8()
{
    if(AF.lo & CARRY_FLAG)
    {
        PC.word += (int8_t)gameboy->mmu.read8(PC.word + 1);
    }
    else
    {
        PC.word += 2;
    }
}

//0x39
void add_hl_sp()
{
    add16(&HL.word, SP.word);
    PC.word++;
}

//0x3A
void ldd_a_hlp()
{
    AF.hi = gameboy->mmu.read8(HL.word);
    HL.word--;
    PC.word++;
}

//0x3B
void sp_dec()
{
    SP.word--;
    PC.word++;
}

//0x3C
void a_inc()
{
    inc(&AF.hi);
    PC.word++;
}

//0x3D
void a_dec()
{
    dec(&AF.hi);
    PC.word++;
}

//0x3E
void ld_a_d8()
{
    AF.hi = gameboy->mmu.read8(PC.word + 1);
    PC.word += 2;
}

//0x3F
void ccf()
{
    unset_flag(CARRY_FLAG);
    unset_flag(HC_FLAG);
    unset_flag(SUB_FLAG);
    PC.word++;
}

//0x40
void ld_b_b()
{
    BC.hi = BC.hi;
    PC.word++;
}

//0x41
void ld_b_c()
{
    BC.hi = BC.lo;
    PC.word++;
}

//0x42
void ld_b_d()
{
    BC.hi = DE.hi;
    PC.word++;
}

//0x43
void ld_b_e()
{
    BC.hi = DE.lo;
    PC.word++;
}

//0x44
void ld_b_h()
{
    BC.hi = HL.hi;
    PC.word++;
}

//0x45
void ld_b_l()
{
    BC.hi = HL.lo;
    PC.word++;
}

//0x46
void ld_b_hlp()
{
    BC.hi = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x47
void ld_b_a()
{
    BC.hi = AF.hi;
    PC.word++;
}

//0x48
void ld_c_b()
{
    BC.lo = BC.hi;
    PC.word++;
}

//0x49
void ld_c_c()
{
    BC.lo = BC.lo;
    PC.word++;
}

//0x4A
void ld_c_d()
{
    BC.lo = DE.hi;
    PC.word++;
}

//0x4B
void ld_c_e()
{
    BC.lo = DE.lo;
    PC.word++;
}

//0x4C
void ld_c_h()
{
    BC.lo = HL.hi;
    PC.word++;
}

//0x4D
void ld_c_l()
{
    BC.lo = HL.lo;
    PC.word++;
}


//0x4E
void ld_c_hlp()
{
    BC.lo = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x4F
void ld_c_a()
{
    BC.lo = AF.hi;
    PC.word++;
}

//0x50
void ld_d_b()
{
    DE.hi = BC.hi;
    PC.word++;
}

//0x51
void ld_d_c()
{
    DE.hi = BC.lo();
    PC.word++;
}

//0x52
void ld_d_d()
{
    DE.hi = DE.hi;
    PC.word++;
}

//0x53
void ld_d_e()
{
    DE.hi = DE.lo;
    PC.word++;
}

//0x54
void ld_d_h()
{
    DE.hi = HL.hi;
    PC.word++;
}

//0x55
void ld_d_l()
{
    DE.hi = HL.lo;
    PC.word++;
}

//0x56
void ld_d_hlp()
{
    DE.hi = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x57
void ld_d_a()
{
    DE.hi = AF.hi;
    PC.word++;
}

//0x58
void ld_e_b()
{
    DE.lo = BC.hi;
    PC.word++;
}

//0x59
void ld_e_c()
{
    DE.lo = BC.lo;
    PC.word++;
}

//0x5A
void ld_e_d()
{
    DE.lo = DE.hi;
    PC.word++;
}

//0x5B
void ld_e_e()
{
    DE.lo = DE.lo;
    PC.word++;
}

//0x5C
void ld_e_h()
{
    DE.lo = HL.hi;
    PC.word++;
}

//0x5D
void ld_e_l()
{
    DE.lo = HL.lo;
    PC.word++;
}

//0x5E
void ld_e_hlp()
{
    DE.lo = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x5F
void ld_e_a()
{
    DE.lo = AF.hi;
    PC.word++;
}

//0x60
void ld_h_b()
{
    HL.hi = BC.hi;
    PC.word++;
}

//0x61
void ld_h_c()
{
    HL.hi = BC.lo;
    PC.word++;
}

//0x62
void ld_h_d()
{
    HL.hi = DE.hi;
    PC.word++;
}

//0x63
void ld_h_e()
{
    HL.hi = DE.lo;
    PC.word++;
}

//0x64
void ld_h_h()
{
    HL.hi = HL.hi;
    PC.word++;
}

//0x65
void ld_h_l()
{
    HL.hi = HL.lo;
    PC.word++;
}

//0x66
void ld_h_hlp()
{
    HL.hi = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x67
void ld_h_a()
{
    HL.hi = AF.hi;
    PC.word++;
}

//0x68
void ld_l_b()
{
    HL.lo = BC.hi;
    PC.word++;
}

//0x69
void ld_l_c()
{
    HL.lo = BC.hi;
    PC.word++;
}

//0x6A
void ld_l_d()
{
    HL.lo = DE.hi;
    PC.word++;
}

//0x6B
void ld_l_e()
{
    HL.lo = DE.lo;
    PC.word++;
}

//0x6C
void ld_l_h()
{
    HL.lo = = HL.hi;
    PC.word++;
}

//0x6D
void ld_l_l()
{
    HL.lo = HL.lo;
    PC.word++;
}

//0x6E
void ld_l_hlp()
{
    HL.lo = = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x6F
void ld_l_a()
{
    HL.lo = AF.hi;
    PC.word++;
}

//0x70
void ld_hlp_b()
{
    gameboy->mmu.write8(HL.word, BC.hi);
    PC.word++;
}

//0x71
void ld_hlp_c()
{
    gameboy->mmu.write8(HL.word, BC.lo);
    PC.word++;
}

//0x72
void ld_hlp_d()
{
    gameboy->mmu.write8(HL.word, BC.lo);
    PC.word++;
}

//0x73
void ld_hlp_e()
{
    gameboy->mmu.write8(HL.word, DE.lo);
    PC.word++;
}

//0x74
void ld_hlp_h()
{
    gameboy->mmu.write8(HL.word, HL.lo);
    PC.word++;
}

//0x75
void ld_hlp_l()
{
    gameboy->mmu.write8(HL.word, HL.lo);
    PC.word++;
}

//0x76
void halt
{
    halted = true;
}

//0x77
void ld_hlp_a()
{
    gameboy->mmu.write8(HL.word, AF.hi);
    PC.word++;
}

//0x78
void ld_a_b()
{
    AF.hi = BC.hi;
    PC.word++;
}

//0x79
void ld_a_c()
{
    AF.hi = BC.lo;
    PC.word++;
}

//0x7A
void ld_a_d()
{
    AF.hi = DE.hi;
    PC.word++;
}

//0x7B
void ld_a_e()
{
    AF.hi = DE.lo;
    PC.word++;
}

//0x7C
void ld_a_h()
{
    AF.hi = HL.hi;
    PC.word++;
}

//0x7d
void ld_a_l()
{
    AF.hi = HL.lo();
    PC.word++;
}

//0x7E
void ld_a_hlp()
{
    AF.hi = gameboy->mmu.read8(HL.word);
    PC.word++;
}

//0x7F
void ld_a_a()
{
    AF.hi = AF.hi;
    PC.word++;
}

//0x80
void add_a_b()
{
    add(&AF.hi, BC.hi);
}

//0x81
void add_a_c()
{
    add(&AF.hi, BC.lo)
}

//0x82
void add_a_d()
{
    add(&AF.hi, DE.hi);
}

//0x83
void add_a_e()
{
    add(&AF.hi, DE.lo);
}

//0x84
void add_a_h()
{
    add(&AF.hi, HL.hi);
}

//0x85
void add_a_l()
{
    add(&AF.hi, HL.lo);
}

//0x86
void add_a_hlp()
{
    add(&AF.hi, gameboy->mmu.read8(HL.word));
}

//0x87
void add_a_a()
{
    add(&AF.hi, AF.hi);
}

//0x88

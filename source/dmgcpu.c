/*++

Copyright (c) 2016  Mosaic Software

Module Name:
        dmgcpu.c

Abstract:
        Implementation of dmgcpu.h

Author:
        jbuhagiar [Quaker762]

Environment:

Notes:
        Lol, we can't pass by reference in C..

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
static bool         halted  = false;
static bool         ime     = false;
static FILE*        trace;

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
static void inc(uint8_t* value)
{
    if((*value & 0x0F) == 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    (*value)++; // Perform increment
    unset_flag(SUB_FLAG);

    if(*value != 0)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);


}

static void dec(uint8_t* value)
{
    if((*value & 0x0F) == 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    (*value)--; // Perform increment
    set_flag(SUB_FLAG);

    if(*value != 0)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);
}

static void add(uint8_t* reg, uint8_t value)
{
    uint16_t result = *reg + value;

    if(result & 0xFF00)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) += value;

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

static void adc(uint8_t* reg, uint8_t value)
{
    uint16_t result = *reg + value;
    result |= (AF.lo >> 4) & 0x01;

    if(result & 0xFF00)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) += value;

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

static void add16(uint16_t* reg, uint16_t value)
{
    uint32_t result = *reg + value;

    if(result & 0xFFFF0000)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) += value;

    if(((*reg & 0x0F) + (value & 0x0F)) > 0x0F)
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);

    unset_flag(SUB_FLAG);
}

static void sub(uint8_t value)
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

static void sbc(uint8_t value)
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
    AF.hi -= (AF.lo >> 7) & 0x01;

    if(AF.hi == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);

    set_flag(SUB_FLAG);
}

// LOGICAL INSTRUCTIONS!!
static void and(uint8_t value)
{
    unset_flag(SUB_FLAG);
    set_flag(HC_FLAG);
    unset_flag(CARRY_FLAG);

    AF.hi &= value;

    if(AF.hi == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);
}

static void xor(uint8_t value)
{
    unset_flag(CARRY_FLAG);
    unset_flag(SUB_FLAG);
    unset_flag(HC_FLAG);

    AF.hi ^= value;

    if(AF.hi == 0)
        set_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);
}

static void or(uint8_t value)
{
    unset_flag(CARRY_FLAG);
    unset_flag(SUB_FLAG);
    unset_flag(HC_FLAG);

    AF.hi |= value;

    if(AF.hi == 0)
        set_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);
}

static void cp(uint8_t value)
{
    set_flag(SUB_FLAG);

    if(AF.hi == value)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);

    if(AF.hi < value)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    if((value & 0x0F) > (AF.hi & 0x0F))
        set_flag(HC_FLAG);
    else
        unset_flag(HC_FLAG);
}

// HELPER FUNCTIONS

// RLC, bit 7 goes to Carry Flag and bit0
// Shift left by one
static void rlc(uint8_t* reg)
{
    uint8_t carry = (*reg & 0x80) >> 7;

    if(carry)
        set_flag(CARRY_FLAG);
    else
        set_flag(CARRY_FLAG);

    (*reg) <<= 1;
    (*reg) |= carry;

    if(*reg == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);
}

// RRC, same as above, but to the right this time!
static void rrc(uint8_t* reg)
{
    uint8_t carry = *reg & 0x01;

    if(carry)
        set_flag(CARRY_FLAG);
    else
        set_flag(CARRY_FLAG);

    (*reg) >>= 1;
    (*reg) |= (carry << 7);

    if(*reg == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);
}

// RL, Bit 7 to Carry flag, carry flag to bit 0
static void rl(uint8_t* reg)
{
    uint8_t carry = (AF.lo >> 4) & 0x01;
    uint8_t bit7 = *reg & 0x80;

    unset_flag(SUB_FLAG);
    unset_flag(HC_FLAG);


    if(bit7)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) <<= 1;
    (*reg) |= carry;
}

// RL, Bit 7 to Carry flag, carry flag to bit 0
static void rr(uint8_t* reg)
{
    uint8_t carry = (AF.lo >> 4) & 0x01;
    uint8_t bit1 = *reg & 0x01;

    unset_flag(SUB_FLAG);
    unset_flag(HC_FLAG);

    if(bit1)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) >>= 1;
    (*reg) |= (carry << 7);
}

static void sla(uint8_t* reg)
{
    unset_flag(HC_FLAG);
    unset_flag(SUB_FLAG);

    if(*reg & 0x80)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) <<= 1;

    if(*reg == 0)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);
}

static void sra(uint8_t* reg)
{
    unset_flag(HC_FLAG);
    unset_flag(SUB_FLAG);

    uint8_t bit7 = *reg & 0x80;

    if(*reg & 0x01)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) >>= 1;
    (*reg) |= bit7;

    if(*reg == 0)
        unset_flag(ZERO_FLAG);
    else
        set_flag(ZERO_FLAG);
}

static void srl(uint8_t* reg)
{
    unset_flag(SUB_FLAG);
    unset_flag(HC_FLAG);

    if(*reg & 0x01)
        set_flag(CARRY_FLAG);
    else
        unset_flag(CARRY_FLAG);

    (*reg) >>= 1;

    if(*reg == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);
}

static void swap(uint8_t* reg)
{
    uint8_t tmp;

    unset_flag(SUB_FLAG);
    unset_flag(HC_FLAG);
    unset_flag(CARRY_FLAG);

    tmp = (*reg) & 0x0F;
    (*reg) >>= 4;
    (*reg) &= (tmp << 4);

    if(*reg == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);
}

static void set_bit(uint8_t* reg, uint8_t bit)
{
    (*reg) |= (0x1 << bit);
}

static void unset_bit(uint8_t* reg, uint8_t bit)
{
    (*reg) &= ~(0x1 << bit);
}

static void test_bit(uint8_t* reg, uint8_t bit)
{
    unset_flag(SUB_FLAG);
    set_flag(HC_FLAG);

    if((*reg & (0x1 << bit)) == 0)
        set_flag(ZERO_FLAG);
    else
        unset_flag(ZERO_FLAG);
}


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
    uint8_t carry = (AF.hi & 0x80) >> 7;

    if(carry)
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
    add16(&HL.word, BC.word);
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
    inc(&BC.lo);
    PC.word++;
}

//0x0D
void c_dec()
{
    dec(&BC.lo);
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

//0x14
void d_inc()
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
    PC.word += (int8_t)(gameboy->mmu.read8(PC.word + 1)) + 2;
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
    if(!(AF.lo & ZERO_FLAG))
    {
        PC.word += (int8_t)(gameboy->mmu.read8(PC.word + 1)) + 2;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
    else
    {
        PC.word += 2;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
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
        PC.word += ((int8_t)gameboy->mmu.read8(PC.word + 1)) + 2;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
    else
    {
        PC.word += 2;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
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
        PC.word += (int8_t)(gameboy->mmu.read8(PC.word + 1)) + 2;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
    else
    {
        PC.word += 2;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
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
    gameboy->mmu.write8(HL.word, gameboy->mmu.read8(PC.word + 1));
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
        PC.word += (int8_t)(gameboy->mmu.read8(PC.word + 1)) + 2;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
    else
    {
        PC.word += 2;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
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
    DE.hi = BC.lo;
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
    HL.lo = HL.hi;
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
    HL.lo = gameboy->mmu.read8(HL.word);
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
void halt()
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
    AF.hi = HL.lo;
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
    PC.word++;
}

//0x81
void add_a_c()
{
    add(&AF.hi, BC.lo);
    PC.word++;
}

//0x82
void add_a_d()
{
    add(&AF.hi, DE.hi);
    PC.word++;
}

//0x83
void add_a_e()
{
    add(&AF.hi, DE.lo);
    PC.word++;
}

//0x84
void add_a_h()
{
    add(&AF.hi, HL.hi);
    PC.word++;
}

//0x85
void add_a_l()
{
    add(&AF.hi, HL.lo);
    PC.word++;
}

//0x86
void add_a_hlp()
{
    add(&AF.hi, gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0x87
void add_a_a()
{
    add(&AF.hi, AF.hi);
    PC.word++;
}

//0x88
void adc_a_b()
{
    adc(&AF.hi, BC.hi);
    PC.word++;
}

//0x89
void adc_a_c()
{
    adc(&AF.hi, BC.lo);
    PC.word++;
}

//0x8A
void adc_a_d()
{
    adc(&AF.hi, DE.hi);
    PC.word++;
}

//0x8B
void adc_a_e()
{
    adc(&AF.hi, DE.lo);
    PC.word++;
}

//0x8C
void adc_a_h()
{
    adc(&AF.hi, HL.hi);
    PC.word++;
}

//0x8D
void adc_a_l()
{
    adc(&AF.hi, HL.lo);
    PC.word++;
}

//0x8E
void adc_a_hlp()
{
    adc(&AF.hi, gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0x8F
void adc_a_a()
{
    adc(&AF.hi, AF.hi);
    PC.word++;
}

//0x90
void sub_b()
{
    sub(BC.hi);
    PC.word++;
}

//0x91
void sub_c()
{
    sub(BC.lo);
    PC.word++;
}

//0x92
void sub_d()
{
    sub(DE.hi);
    PC.word++;
}

//0x93
void sub_e()
{
    sub(DE.lo);
    PC.word++;
}

//0x94
void sub_h()
{
    sub(HL.hi);
    PC.word++;
}

//0x95
void sub_l()
{
    sub(HL.lo);
    PC.word++;
}

//0x96
void sub_hlp()
{
    sub(gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0x97
void sub_a()
{
    sub(AF.hi);
    PC.word++;
}

//0x98
void sbc_a_b()
{
    sbc(BC.hi);
    PC.word++;
}

//0x99
void sbc_a_c()
{
    sbc(BC.lo);
    PC.word++;
}

//0x9A
void sbc_a_d()
{
    sbc(DE.hi);
    PC.word++;
}

//0x9B
void sbc_a_e()
{
    sbc(DE.lo);
    PC.word++;
}

//0x9C
void sbc_a_h()
{
    sbc(HL.hi);
    PC.word++;
}

//0x9D
void sbc_a_l()
{
    sbc(HL.lo);
    PC.word++;
}

//0x9E
void sbc_a_hlp()
{
    sbc(gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0x9F
void sbc_a_a()
{
    sbc(AF.hi);
    PC.word++;
}

//0xA0
void and_b()
{
    and(BC.hi);
    PC.word++;
}

//0xA1
void and_c()
{
    and(BC.lo);
    PC.word++;
}

//0xA2
void and_d()
{
    and(DE.hi);
    PC.word++;
}

//0xA3
void and_e()
{
    and(DE.lo);
    PC.word++;
}

//0xA4
void and_h()
{
    and(HL.hi);
    PC.word++;
}

//0xA5
void and_l()
{
    and(HL.lo);
    PC.word++;
}

//0xA6
void and_hlp()
{
    and(gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0xA7
void and_a()
{
    and(AF.hi);
    PC.word++;
}

//0xA8
void xor_b()
{
    xor(BC.hi);
    PC.word++;
}

//0xA9
void xor_c()
{
    xor(BC.lo);
    PC.word++;
}

//0xAA
void xor_d()
{
    xor(DE.hi);
    PC.word++;
}

//0xAB
void xor_e()
{
    xor(DE.lo);
    PC.word++;
}

//0xAC
void xor_h()
{
    xor(HL.hi);
    PC.word++;
}

//0xAD
void xor_l()
{
    xor(HL.lo);
    PC.word++;
}

//0xAE
void xor_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    xor(val);
    gameboy->mmu.write8(HL.word, val);
    PC.word++;
}

//0xAF
void xor_a()
{
    xor(AF.hi);
    PC.word++;
}

//0xB0
void or_b()
{
    or(BC.hi);
    PC.word++;
}

//0xB1
void or_c()
{
    or(BC.lo);
    PC.word++;
}

//0xB2
void or_d()
{
    or(DE.hi);
    PC.word++;
}

//0xB3
void or_e()
{
    or(DE.lo);
    PC.word++;
}

//0xB4
void or_h()
{
    or(HL.hi);
    PC.word++;
}

//0xB5
void or_l()
{
    or(HL.lo);
    PC.word++;
}

//0xB6
void or_hlp()
{
    or(gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0xB7
void or_a()
{
    or(AF.hi);
    PC.word++;
}

//0xB8
void cp_b()
{
    cp(BC.hi);
    PC.word++;
}

//0xB9
void cp_c()
{
    cp(BC.lo);
    PC.word++;
}

//0xBA
void cp_d()
{
    cp(DE.hi);
    PC.word++;
}

//0xBB
void cp_e()
{
    cp(DE.lo);
    PC.word++;
}

//0xBC
void cp_h()
{
    cp(HL.hi);
    PC.word++;
}

//0xBD
void cp_l()
{
    cp(HL.lo);
    PC.word++;
}

//0xBE
void cp_hlp()
{
    cp(gameboy->mmu.read8(HL.word));
    PC.word++;
}

//0xBF
void cp_a()
{
    cp(AF.hi);
    PC.word++;
}

//0xC0
// FIX ME PLEASE!
void ret_nz()
{
    uint8_t flags = AF.lo;

    // Zero flag is NOT set!
    // Pop the return value off of the stack and jump to it!
    if((flags & ZERO_FLAG) == 0)
    {
        uint16_t addr = gameboy->mmu.read16(SP.word);
        SP.word += 2; // The stack grows downwards so we INCREMENT the stack pointer!
        PC.word = addr;

        gameboy->cpu.clock.t += 20;
        gameboy->cpu.clock.m += 5;
    }
    else
    {
        PC.word++;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
    }
}

//0xC1
void pop_bc()
{
    BC.word = gameboy->mmu.read16(SP.word);
    SP.word += 2;
    PC.word++;
}

//0xC2
void jp_nz_a16()
{
    uint8_t flags = AF.lo;

    if((flags & ZERO_FLAG) == 0)
    {
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 16;
        gameboy->cpu.clock.m += 4;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 4;
    }
}

//0xC3
void jp_a16()
{
    PC.word = gameboy->mmu.read16(PC.word + 1);
}

//0xC4
void call_nz_a16()
{
    uint8_t flags = AF.lo;

    if((flags & ZERO_FLAG) == 0)
    {
        SP.word -= 2;
        gameboy->mmu.write16(SP.word, PC.word);
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 24;
        gameboy->cpu.clock.m += 6;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xC5
void push_bc()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, BC.word);
    PC.word++;
}

//0xC6
void add_a_d8()
{
    add(&AF.hi, PC.word + 1);
    PC.word += 2;
}

//0xC7
void rst_00()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x00;
}

//0xC8
void ret_z()
{
    uint8_t flags = AF.lo;

    if((flags & ZERO_FLAG) == ZERO_FLAG) // Fucked up comparison ahahaha!
    {
        PC.word = gameboy->mmu.read16(SP.word);
        SP.word += 2;

        gameboy->cpu.clock.t += 20;
        gameboy->cpu.clock.m += 5;
    }
    else
    {
        PC.word++;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
    }
}

//0xC9
void ret()
{
    PC.word = gameboy->mmu.read16(SP.word);
    SP.word += 2;
}

//0xCA
void jp_z_a16()
{
    uint8_t flags = AF.lo;

    if((flags & ZERO_FLAG) == ZERO_FLAG)
    {
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 16;
        gameboy->cpu.clock.m += 4;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xCB
// PREFIX CB! RESERVED INSTRUCTION!

//0xCC
void call_z_a16()
{
    uint8_t flags = AF.lo;

    if((flags & ZERO_FLAG) == 0)
    {
        SP.word -= 2;
        gameboy->mmu.write16(SP.word, PC.word);
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t = 24;
        gameboy->cpu.clock.m = 6;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xCD
void call_a16()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word + 3); // Store address of next instruction.
    PC.word = gameboy->mmu.read16(PC.word + 1);
}

//0xCE
void adc_a_d8()
{
    adc(&AF.hi, PC.word + 1);
    PC.word += 2;
}

//0xCF
void rst_08()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x08;
}

//0xD0
void ret_nc()
{
    uint8_t flags = AF.lo;

    // Zero flag is NOT set!
    // Pop the return value off of the stack and jump to it!
    if((flags & CARRY_FLAG) == 0)
    {
        uint16_t addr = gameboy->mmu.read16(SP.word);
        SP.word += 2; // The stack grows downwards so we INCREMENT the stack pointer!
        PC.word = addr;

        gameboy->cpu.clock.t += 20;
        gameboy->cpu.clock.m += 5;
    }
    else
    {
        PC.word++;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
    }
}

//0xD1
void pop_de()
{
    DE.word = gameboy->mmu.read16(SP.word);
    SP.word += 2;
    PC.word++;
}

//0xD2
void jp_nc_a16()
{
    uint8_t flags = AF.lo;

    if((flags & CARRY_FLAG) == 0)
    {
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 16;
        gameboy->cpu.clock.m += 4;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xD3

//0xD4
void call_nc_a16()
{
    uint8_t flags = AF.lo;

    if((flags & CARRY_FLAG) == 0)
    {
        SP.word -= 2;
        gameboy->mmu.write16(SP.word, PC.word);
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 24;
        gameboy->cpu.clock.m += 6;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xD5
void push_de()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, DE.word);
    PC.word++;
}

//0xD6
void sub_d8()
{
    sub(PC.word + 1);
    PC.word += 2;
}

//0xD7
void rst_10()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x10;
}

//0xD8
void ret_c()
{
    uint8_t flags = AF.lo;

    // Zero flag is NOT set!
    // Pop the return value off of the stack and jump to it!
    if((flags & CARRY_FLAG) == CARRY_FLAG)
    {
        uint16_t addr = gameboy->mmu.read16(SP.word);
        SP.word += 2; // The stack grows downwards so we INCREMENT the stack pointer!
        PC.word = addr;

        gameboy->cpu.clock.t += 20;
        gameboy->cpu.clock.m += 5;
    }
    else
    {
        PC.word++;

        gameboy->cpu.clock.t += 8;
        gameboy->cpu.clock.m += 2;
    }
}

//0xD9
void reti()
{
    PC.word = gameboy->mmu.read16(SP.word);
    SP.word += 2;
    ime = true;
}

//0xDA
void jp_c_a16()
{
    uint8_t flags = AF.lo;

    if((flags & CARRY_FLAG) == CARRY_FLAG)
    {
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 16;
        gameboy->cpu.clock.m += 4;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xDB // NOP

//0xDC
void call_c_a16()
{
    uint8_t flags = AF.lo;

    if((flags & CARRY_FLAG) == CARRY_FLAG)
    {
        SP.word -= 2;
        gameboy->mmu.write16(SP.word, PC.word);
        PC.word = gameboy->mmu.read16(PC.word + 1);

        gameboy->cpu.clock.t += 24;
        gameboy->cpu.clock.m += 6;
    }
    else
    {
        PC.word += 3;

        gameboy->cpu.clock.t += 12;
        gameboy->cpu.clock.m += 3;
    }
}

//0xDD // NOP

//0xDE
void sbc_a_d8()
{
    sbc(PC.word + 1);
    PC.word += 2;
}

//0xDF
void rst_18()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x18;
}

//0xE0
void ldh_a8p_a()
{
    uint8_t a8 = gameboy->mmu.read8(PC.word + 1);
    gameboy->mmu.write8(0xFF00 + a8, AF.hi);
    PC.word += 2;
}

//0xE1
void pop_hl()
{
    HL.word = gameboy->mmu.read16(SP.word);
    SP.word += 2;
    PC.word++;
}

//0xE2
void ld_cp_a()
{
    gameboy->mmu.write8(0xFF00 + BC.lo, AF.hi);
    PC.word++;
}

//0xE5
void push_hl()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, HL.word);
    PC.word++;
}

//0xE6
void and_d8()
{
    and(PC.word + 1);
    PC.word += 2;
}

//0xE7
void rst_20()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x20;
}

//0xE8
void add_sp_r8()
{
    SP.word += (signed)(PC.word + 1);
    PC.word += 2;
}

//0xE9
void jp_hlp()
{
    PC.word = gameboy->mmu.read16(HL.word);
}

//0xEA, CHALLENGE EVERYTHING
void ld_a16_a()
{
    gameboy->mmu.write8(PC.word + 1, AF.hi);
    PC.word += 3;
}

//0xEE
void xor_d8()
{
    xor(PC.word + 1);
    PC.word++;
}

//0xEF
void rst_28()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x28;
}

//0xF0
void ldh_a_a8p()
{
    uint8_t address = gameboy->mmu.read8(PC.word + 1);
    AF.hi = gameboy->mmu.read8(0xFF00 + address);
    PC.word += 2;
}

//0xF1
void pop_af()
{
    AF.word = gameboy->mmu.read16(SP.word);
    SP.word += 2;
    PC.word++;
}

//0xF2
void ld_a_cp()
{
    AF.hi = gameboy->mmu.read8(0xFF00 + BC.lo);
    PC.word++;
}

//0xF3
void di()
{
    ime = false;
    PC.word++;
}

//0xF5
void push_af()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, AF.word);
    PC.word++;
}

//0xF6
void or_d8()
{
    or(PC.word + 1);
    PC.word += 2;
}

//0xF7
void rst_30()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x30;
}

//0xF8
void ld_hl_spr8() // Hahaha what the fuck!
{

}

//0xF9
void ld_sp_hl()
{
    SP.word = HL.word;
    PC.word++;
}

//0xFA
void ld_a_a16p()
{
    AF.hi = gameboy->mmu.read8(PC.word + 1);
    PC.word += 3;
}

//0xFB
void ei()
{
    ime = true;
    PC.word++;
}

//0xFE
void cp_d8()
{
    uint8_t d8 = gameboy->mmu.read8(PC.word + 1);
    cp(d8);
    PC.word += 2;
}

//0xFF
void rst_38()
{
    SP.word -= 2;
    gameboy->mmu.write16(SP.word, PC.word);
    PC.word = 0x0000 + 0x38;
}

////////////////////////CB OPERATIONS////////////////////////

//CB 00
void rlc_b()
{
    rlc(&BC.hi);
    PC.word += 2;
}

//CB 01
void rlc_c()
{
    rlc(&BC.lo);
    PC.word += 2;
}

//CB 02
void rlc_d()
{
    rlc(&DE.hi);
    PC.word += 2;
}

//CB 03
void rlc_e()
{
    rlc(&DE.lo);
    PC.word += 2;
}

//CB 04
void rlc_h()
{
    rlc(&HL.hi);
    PC.word += 2;
}

//CB 05
void rlc_l()
{
    rlc(&HL.lo);
    PC.word += 2;
}

//CB 06
void rlc_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    rlc(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 07
void rlc_a()
{
    rlc(&AF.hi);
    PC.word += 2;
}

//CB 08
void rrc_b()
{
    rrc(&BC.hi);
    PC.word += 2;
}

//CB 09
void rrc_c()
{
    rrc(&BC.lo);
    PC.word += 2;
}

//CB 0A
void rrc_d()
{
    rrc(&DE.hi);
    PC.word += 2;
}

//CB 0B
void rrc_e()
{
    rrc(&DE.lo);
    PC.word += 2;
}

//CB 0C
void rrc_h()
{
    rrc(&HL.hi);
    PC.word += 2;
}

//CB 0D
void rrc_l()
{
    rrc(&HL.lo);
    PC.word += 2;
}

//CB 0E
void rrc_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    rrc(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 0F
void rrc_a()
{
    rrc(&AF.hi);
    PC.word += 2;
}

//CB 10
void rl_b()
{
    rl(&BC.hi);
    PC.word += 2;
}

//CB 11
void rl_c()
{
    rl(&BC.lo);
    PC.word += 2;
}

//CB 12
void rl_d()
{
    rl(&DE.hi);
    PC.word++;
}

//CB 13
void rl_e()
{
    rl(&DE.lo);
    PC.word += 2;
}

//CB 14
void rl_h()
{
    rl(&HL.hi);
    PC.word += 2;
}

//CB 15
void rl_l()
{
    rl(&HL.lo);
    PC.word += 2;
}

//CB 16
void rl_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    rl(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 17
void rl_a()
{
    rl(&AF.hi);
    PC.word += 2;
}

//CB 18
void rr_b()
{
    rr(&BC.hi);
    PC.word += 2;
}

//CB 19
void rr_c()
{
    rr(&BC.lo);
    PC.word += 2;
}

//CB 1A
void rr_d()
{
    rr(&DE.hi);
    PC.word += 2;
}

//CB 1B
void rr_e()
{
    rr(&DE.lo);
    PC.word += 2;
}

//CB 1C
void rr_h()
{
    rr(&HL.hi);
    PC.word += 2;
}

//CB 1D
void rr_l()
{
    rr(&HL.lo);
    PC.word += 2;
}

//CB 1E
void rr_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    rr(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 1F
void rr_a()
{
    rr(&AF.hi);
    PC.word += 2;
}

//CB 20
void sla_b()
{
    sla(&BC.hi);
    PC.word += 2;
}

//CB 21
void sla_c()
{
    sla(&BC.lo);
    PC.word += 2;
}

//CB 22
void sla_d()
{
    sla(&DE.hi);
    PC.word += 2;
}

//CB 23
void sla_e()
{
    sla(&DE.lo);
    PC.word += 2;
}

//CB 24
void sla_h()
{
    sla(&HL.hi);
    PC.word += 2;
}

//CB 25
void sla_l()
{
    sla(&HL.lo);
    PC.word += 2;
}

//CB 26
void sla_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    sla(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 27
void sla_a()
{
    sla(&AF.hi);
    PC.word += 2;
}

//CB 28
void sra_b()
{
    sra(&BC.hi);
    PC.word += 2;
}

//CB 29
void sra_c()
{
    sra(&BC.lo);
    PC.word += 2;
}

//CB 2A
void sra_d()
{
    sra(&DE.hi);
    PC.word += 2;
}

//CB 2B
void sra_e()
{
    sra(&DE.lo);
    PC.word += 2;
}

//CB 2C
void sra_h()
{
    sra(&HL.hi);
    PC.word += 2;
}

//CB 2D
void sra_l()
{
    sra(&HL.lo);
    PC.word += 2;
}

//CB 2E
void sra_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    sra(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 2F
void sra_a()
{
    sra(&AF.hi);
    PC.word += 2;
}

//CB 30
void swap_b()
{
    swap(&BC.hi);
    PC.word += 2;
}

//CB 31
void swap_c()
{
    swap(&BC.lo);
    PC.word += 2;
}

//CB 32
void swap_d()
{
    swap(&DE.hi);
    PC.word += 2;
}

//CB 33
void swap_e()
{
    swap(&DE.lo);
    PC.word += 2;
}

//CB 34
void swap_h()
{
    swap(&HL.hi);
    PC.word += 2;
}

//CB 35
void swap_l()
{
    swap(&HL.lo);
    PC.word += 2;
}

//CB 36
void swap_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    swap(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 37
void swap_a()
{
    swap(&AF.hi);
    PC.word += 2;
}

//CB 38
void srl_b()
{
    srl(&BC.hi);
    PC.word += 2;
}

//CB 39
void srl_c()
{
    srl(&BC.lo);
    PC.word += 2;
}

//CB 3A
void srl_d()
{
    srl(&DE.hi);
    PC.word += 2;
}

//CB 3B
void srl_e()
{
    srl(&DE.lo);
    PC.word += 2;
}

//CB 3C
void srl_h()
{
    srl(&HL.hi);
    PC.word += 2;
}

//CB 3D
void srl_l()
{
    srl(&HL.lo);
    PC.word += 2;
}

//CB 3E
void srl_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    srl(&val);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 3F
void srl_a()
{
    srl(&AF.hi);
    PC.word += 2;
}

//CB 40
void bit_0_b()
{
    test_bit(&BC.hi, 0);
    PC.word += 2;
}

//CB 41
void bit_0_c()
{
    test_bit(&BC.lo, 0);
    PC.word += 2;
}

//CB 42
void bit_0_d()
{
    test_bit(&DE.hi, 0);
    PC.word += 2;
}

//CB 43
void bit_0_e()
{
    test_bit(&DE.lo, 0);
    PC.word += 2;
}

//CB 44
void bit_0_h()
{
    test_bit(&HL.hi, 0);
    PC.word += 2;
}

//CB 45
void bit_0_l()
{
    test_bit(&HL.lo, 0);
    PC.word += 2;
}

//CB 46
void bit_0_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 0);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 47
void bit_0_a()
{
    test_bit(&AF.hi, 0);
    PC.word += 2;
}

//CB 48
void bit_1_b()
{
    test_bit(&BC.hi, 1);
    PC.word += 2;
}

//CB 49
void bit_1_c()
{
    test_bit(&BC.lo, 1);
    PC.word += 2;
}

//CB 4A
void bit_1_d()
{
    test_bit(&DE.hi, 1);
    PC.word += 2;
}

//CB 4B
void bit_1_e()
{
    test_bit(&DE.lo, 1);
    PC.word += 2;
}

//CB 4C
void bit_1_h()
{
    test_bit(&HL.hi, 1);
    PC.word += 2;
}

//CB 4D
void bit_1_l()
{
    test_bit(&HL.lo, 1);
    PC.word += 2;
}

//CB 4E
void bit_1_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 1);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 4F
void bit_1_a()
{
    test_bit(&AF.hi, 1);
    PC.word += 2;
}

//CB 50
void bit_2_b()
{
    test_bit(&BC.hi, 2);
    PC.word += 2;
}

//CB 51
void bit_2_c()
{
    test_bit(&BC.lo, 2);
    PC.word += 2;
}

//CB 52
void bit_2_d()
{
    test_bit(&DE.hi, 2);
    PC.word += 2;
}

//CB 53
void bit_2_e()
{
    test_bit(&DE.lo, 2);
    PC.word += 2;
}

//CB 54
void bit_2_h()
{
    test_bit(&HL.hi, 2);
    PC.word += 2;
}

//CB 55
void bit_2_l()
{
    test_bit(&HL.lo, 2);
    PC.word += 2;
}

//CB 56
void bit_2_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 2);
    gameboy->mmu.write8(val, HL.word);
    PC.word += 2;
}

//CB 57
void bit_2_a()
{
    test_bit(&AF.hi, 2);
    PC.word += 2;
}

//CB 58
void bit_3_b()
{
    test_bit(&BC.hi, 3);
    PC.word += 2;
}

//CB 59
void bit_3_c()
{
    test_bit(&BC.lo, 3);
    PC.word += 2;
}

//CB 5A
void bit_3_d()
{
    test_bit(&DE.hi, 3);
    PC.word += 2;
}

//CB 5B
void bit_3_e()
{
    test_bit(&DE.lo, 3);
    PC.word += 2;
}

//CB 5C
void bit_3_h()
{
    test_bit(&HL.hi, 3);
    PC.word += 2;
}

//CB 5D
void bit_3_l()
{
    test_bit(&HL.lo, 3);
    PC.word += 2;
}

//CB 5E
void bit_3_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 3);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 5F
void bit_3_a()
{
    test_bit(&AF.hi, 3);
    PC.word += 2;
}

//CB 60
void bit_4_b()
{
    test_bit(&BC.hi, 4);
    PC.word += 2;
}

//CB 61
void bit_4_c()
{
    test_bit(&BC.lo, 4);
    PC.word += 2;
}

//CB 62
void bit_4_d()
{
    test_bit(&DE.hi, 4);
    PC.word += 2;
}

//CB 63
void bit_4_e()
{
    test_bit(&DE.lo, 4);
    PC.word += 2;
}

//CB 64
void bit_4_h()
{
    test_bit(&HL.hi, 4);
    PC.word += 2;
}

//CB 65
void bit_4_l()
{
    test_bit(&HL.lo, 4);
    PC.word += 2;
}

//CB 66
void bit_4_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 4);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 67
void bit_4_a()
{
    test_bit(&AF.hi, 4);
    PC.word += 2;
}

//CB 68
void bit_5_b()
{
    test_bit(&BC.hi, 5);
    PC.word += 2;
}

//CB 69
void bit_5_c()
{
    test_bit(&BC.lo, 5);
    PC.word += 2;
}

//CB 6A
void bit_5_d()
{
    test_bit(&DE.hi, 5);
    PC.word += 2;
}

//CB 6B
void bit_5_e()
{
    test_bit(&DE.lo, 5);
    PC.word += 2;
}

//CB 6C
void bit_5_h()
{
    test_bit(&HL.hi, 5);
    PC.word += 2;
}

//CB 6D
void bit_5_l()
{
    test_bit(&HL.lo, 5);
    PC.word += 2;
}

//CB 6E
void bit_5_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 5);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 6F
void bit_5_a()
{
    test_bit(&AF.hi, 5);
    PC.word += 2;
}

//CB 70
void bit_6_b()
{
    test_bit(&BC.hi, 6);
    PC.word += 2;
}

//CB 71
void bit_6_c()
{
    test_bit(&BC.lo, 6);
    PC.word += 2;
}

//CB 72
void bit_6_d()
{
    test_bit(&DE.hi, 6);
    PC.word += 2;
}

//CB 73
void bit_6_e()
{
    test_bit(&DE.lo, 6);
    PC.word += 2;
}

//CB 74
void bit_6_h()
{
    test_bit(&HL.hi, 6);
    PC.word += 2;
}

//CB 75
void bit_6_l()
{
    test_bit(&HL.lo, 6);
    PC.word += 2;
}

//CB 76
void bit_6_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 6);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 77
void bit_6_a()
{
    test_bit(&AF.hi, 6);
    PC.word += 2;
}

//CB 78
void bit_7_b()
{
    test_bit(&BC.hi, 7);
    PC.word += 2;
}

//CB 79
void bit_7_c()
{
    test_bit(&BC.lo, 7);
    PC.word += 2;
}

//CB 7A
void bit_7_d()
{
    test_bit(&DE.hi, 7);
    PC.word += 2;
}

//CB 7B
void bit_7_e()
{
    test_bit(&DE.lo, 7);
    PC.word += 2;
}

//CB 7C
void bit_7_h()
{
    test_bit(&HL.hi, 7);
    PC.word += 2;
}

//CB 7D
void bit_7_l()
{
    test_bit(&HL.lo, 7);
    PC.word += 2;
}

//CB 7E
void bit_7_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    test_bit(&val, 7);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 7F
void bit_7_a()
{
    test_bit(&AF.hi, 7);
    PC.word += 2;
}

//CB 80
void res_0_b()
{
    unset_bit(&BC.hi, 0);
    PC.word += 2;
}

//CB 81
void res_0_c()
{
    unset_bit(&BC.lo, 0);
    PC.word += 2;
}

//CB 82
void res_0_d()
{
    unset_bit(&DE.hi, 0);
    PC.word += 2;
}

//CB 83
void res_0_e()
{
    unset_bit(&DE.lo, 0);
    PC.word += 2;
}

//CB 84
void res_0_h()
{
    unset_bit(&HL.hi, 0);
    PC.word += 2;
}

//CB 85
void res_0_l()
{
    unset_bit(&HL.lo, 0);
    PC.word += 2;
}

//CB 86
void res_0_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 0);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 87
void res_0_a()
{
    unset_bit(&AF.hi, 0);
    PC.word += 2;
}

//CB 88
void res_1_b()
{
    unset_bit(&BC.hi, 1);
    PC.word += 2;
}

//CB 89
void res_1_c()
{
    unset_bit(&BC.lo, 1);
    PC.word += 2;
}

//CB 8A
void res_1_d()
{
    unset_bit(&DE.hi, 1);
    PC.word += 2;
}

//CB 8B
void res_1_e()
{
    unset_bit(&DE.lo, 1);
    PC.word += 2;
}

//CB 8C
void res_1_h()
{
    unset_bit(&HL.hi, 1);
    PC.word += 2;
}

//CB 8D
void res_1_l()
{
    unset_bit(&HL.lo, 1);
    PC.word += 2;
}

//CB 8E
void res_1_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 1);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 8F
void res_1_a()
{
    unset_bit(&AF.hi, 1);
    PC.word += 2;
}

//CB 90
void res_2_b()
{
    unset_bit(&BC.hi, 2);
    PC.word += 2;
}

//CB 91
void res_2_c()
{
    unset_bit(&BC.lo, 2);
    PC.word += 2;
}

//CB 92
void res_2_d()
{
    unset_bit(&DE.hi, 2);
    PC.word += 2;
}

//CB 93
void res_2_e()
{
    unset_bit(&DE.lo, 2);
    PC.word += 2;
}

//CB 94
void res_2_h()
{
    unset_bit(&HL.hi, 2);
    PC.word += 2;
}

//CB 95
void res_2_l()
{
    unset_bit(&HL.lo, 2);
    PC.word += 2;
}

//CB 96
void res_2_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 2);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 97
void res_2_a()
{
    unset_bit(&AF.hi, 2);
    PC.word += 2;
}

//CB 98
void res_3_b()
{
    unset_bit(&BC.hi, 3);
    PC.word += 2;
}

//CB 99
void res_3_c()
{
    unset_bit(&BC.lo, 3);
    PC.word += 2;
}

//CB 9A
void res_3_d()
{
    unset_bit(&DE.hi, 3);
    PC.word += 2;
}

//CB 9B
void res_3_e()
{
    unset_bit(&DE.lo, 3);
    PC.word += 2;
}

//CB 9C
void res_3_h()
{
    unset_bit(&HL.hi, 3);
    PC.word += 2;
}

//CB 9D
void res_3_l()
{
    unset_bit(&HL.lo, 3);
    PC.word += 2;
}

//CB 9E
void res_3_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 3);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 9F
void res_3_a()
{
    unset_bit(&AF.hi, 3);
    PC.word += 2;
}

//CB A0
void res_4_b()
{
    unset_bit(&BC.hi, 4);
    PC.word += 2;
}

//CB A1
void res_4_c()
{
    unset_bit(&BC.lo, 4);
    PC.word += 2;
}

//CB A2
void res_4_d()
{
    unset_bit(&DE.hi, 4);
    PC.word += 2;
}

//CB A3
void res_4_e()
{
    unset_bit(&DE.lo, 4);
    PC.word += 2;
}

//CB A4
void res_4_h()
{
    unset_bit(&HL.hi, 4);
    PC.word += 2;
}

//CB A5
void res_4_l()
{
    unset_bit(&HL.lo, 4);
    PC.word += 2;
}

//CB A6
void res_4_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 4);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB A7
void res_4_a()
{
    unset_bit(&AF.hi, 4);
    PC.word += 2;
}

//CB A8
void res_5_b()
{
    unset_bit(&BC.hi, 5);
    PC.word += 2;
}

//CB A9
void res_5_c()
{
    unset_bit(&BC.lo, 5);
    PC.word += 2;
}

//CB AA
void res_5_d()
{
    unset_bit(&DE.hi, 5);
    PC.word += 2;
}

//CB AB
void res_5_e()
{
    unset_bit(&DE.lo, 5);
    PC.word += 2;
}

//CB AC
void res_5_h()
{
    unset_bit(&HL.hi, 5);
    PC.word += 2;
}

//CB AD
void res_5_l()
{
    unset_bit(&HL.lo, 5);
    PC.word += 2;
}

//CB AE
void res_5_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 5);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB AF
void res_5_a()
{
    unset_bit(&AF.hi, 5);
    PC.word += 2;
}

//CB B0
void res_6_b()
{
    unset_bit(&BC.hi, 6);
    PC.word += 2;
}

//CB B1
void res_6_c()
{
    unset_bit(&BC.lo, 6);
    PC.word += 2;
}

//CB B2
void res_6_d()
{
    unset_bit(&DE.hi, 6);
    PC.word += 2;
}

//CB B3
void res_6_e()
{
    unset_bit(&DE.lo, 6);
    PC.word += 2;
}

//CB B4
void res_6_h()
{
    unset_bit(&HL.hi, 6);
    PC.word += 2;
}

//CB B5
void res_6_l()
{
    unset_bit(&HL.lo, 6);
    PC.word += 2;
}

//CB B6
void res_6_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 6);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB B7
void res_6_a()
{
    unset_bit(&AF.hi, 6);
    PC.word += 2;
}

//CB B8
void res_7_b()
{
    unset_bit(&BC.hi, 7);
    PC.word += 2;
}

//CB B9
void res_7_c()
{
    unset_bit(&BC.lo, 7);
    PC.word += 2;
}

//CB BA
void res_7_d()
{
    unset_bit(&DE.hi, 7);
    PC.word += 2;
}

//CB BB
void res_7_e()
{
    unset_bit(&DE.lo, 7);
    PC.word += 2;
}

//CB BC
void res_7_h()
{
    unset_bit(&HL.hi, 7);
    PC.word += 2;
}

//CB BD
void res_7_l()
{
    unset_bit(&HL.lo, 7);
    PC.word += 2;
}

//CB BE
void res_7_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    unset_bit(&val, 7);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB BF
void res_7_a()
{
    unset_bit(&AF.hi, 7);
    PC.word += 2;
}

//CB 80
void set_0_b()
{
    set_bit(&BC.hi, 0);
    PC.word += 2;
}

//CB 81
void set_0_c()
{
    set_bit(&BC.lo, 0);
    PC.word += 2;
}

//CB 82
void set_0_d()
{
    set_bit(&DE.hi, 0);
    PC.word += 2;
}

//CB 83
void set_0_e()
{
    set_bit(&DE.lo, 0);
    PC.word += 2;
}

//CB 84
void set_0_h()
{
    set_bit(&HL.hi, 0);
    PC.word += 2;
}

//CB 85
void set_0_l()
{
    set_bit(&HL.lo, 0);
    PC.word += 2;
}

//CB 86
void set_0_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 0);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 87
void set_0_a()
{
    set_bit(&AF.hi, 0);
    PC.word += 2;
}

//CB 88
void set_1_b()
{
    set_bit(&BC.hi, 1);
    PC.word += 2;
}

//CB 89
void set_1_c()
{
    set_bit(&BC.lo, 1);
    PC.word += 2;
}

//CB 8A
void set_1_d()
{
    set_bit(&DE.hi, 1);
    PC.word += 2;
}

//CB 8B
void set_1_e()
{
    set_bit(&DE.lo, 1);
    PC.word += 2;
}

//CB 8C
void set_1_h()
{
    set_bit(&HL.hi, 1);
    PC.word += 2;
}

//CB 8D
void set_1_l()
{
    set_bit(&HL.lo, 1);
    PC.word += 2;
}

//CB 8E
void set_1_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 1);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 8F
void set_1_a()
{
    set_bit(&AF.hi, 1);
    PC.word += 2;
}

//CB 90
void set_2_b()
{
    set_bit(&BC.hi, 2);
    PC.word += 2;
}

//CB 91
void set_2_c()
{
    set_bit(&BC.lo, 2);
    PC.word += 2;
}

//CB 92
void set_2_d()
{
    set_bit(&DE.hi, 2);
    PC.word += 2;
}

//CB 93
void set_2_e()
{
    set_bit(&DE.lo, 2);
    PC.word += 2;
}

//CB 94
void set_2_h()
{
    set_bit(&HL.hi, 2);
    PC.word += 2;
}

//CB 95
void set_2_l()
{
    set_bit(&HL.lo, 2);
    PC.word += 2;
}

//CB 96
void set_2_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 2);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 97
void set_2_a()
{
    set_bit(&AF.hi, 2);
    PC.word += 2;
}

//CB 98
void set_3_b()
{
    set_bit(&BC.hi, 3);
    PC.word += 2;
}

//CB 99
void set_3_c()
{
    set_bit(&BC.lo, 3);
    PC.word += 2;
}

//CB 9A
void set_3_d()
{
    set_bit(&DE.hi, 3);
    PC.word += 2;
}

//CB 9B
void set_3_e()
{
    set_bit(&DE.lo, 3);
    PC.word += 2;
}

//CB 9C
void set_3_h()
{
    set_bit(&HL.hi, 3);
    PC.word += 2;
}

//CB 9D
void set_3_l()
{
    set_bit(&HL.lo, 3);
    PC.word += 2;
}

//CB 9E
void set_3_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 3);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB 9F
void set_3_a()
{
    set_bit(&AF.hi, 3);
    PC.word += 2;
}

//CB A0
void set_4_b()
{
    set_bit(&BC.hi, 4);
    PC.word += 2;
}

//CB A1
void set_4_c()
{
    set_bit(&BC.lo, 4);
    PC.word += 2;
}

//CB A2
void set_4_d()
{
    set_bit(&DE.hi, 4);
    PC.word += 2;
}

//CB A3
void set_4_e()
{
    set_bit(&DE.lo, 4);
    PC.word += 2;
}

//CB A4
void set_4_h()
{
    set_bit(&HL.hi, 4);
    PC.word += 2;
}

//CB A5
void set_4_l()
{
    set_bit(&HL.lo, 4);
    PC.word += 2;
}

//CB A6
void set_4_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 4);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB A7
void set_4_a()
{
    set_bit(&AF.hi, 4);
    PC.word += 2;
}

//CB A8
void set_5_b()
{
    set_bit(&BC.hi, 5);
    PC.word += 2;
}

//CB A9
void set_5_c()
{
    set_bit(&BC.lo, 5);
    PC.word += 2;
}

//CB AA
void set_5_d()
{
    set_bit(&DE.hi, 5);
    PC.word += 2;
}

//CB AB
void set_5_e()
{
    set_bit(&DE.lo, 5);
    PC.word += 2;
}

//CB AC
void set_5_h()
{
    set_bit(&HL.hi, 5);
    PC.word += 2;
}

//CB AD
void set_5_l()
{
    set_bit(&HL.lo, 5);
    PC.word += 2;
}

//CB AE
void set_5_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 5);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB AF
void set_5_a()
{
    set_bit(&AF.hi, 5);
    PC.word += 2;
}

//CB B0
void set_6_b()
{
    set_bit(&BC.hi, 6);
    PC.word += 2;
}

//CB B1
void set_6_c()
{
    set_bit(&BC.lo, 6);
    PC.word += 2;
}

//CB B2
void set_6_d()
{
    set_bit(&DE.hi, 6);
    PC.word += 2;
}

//CB B3
void set_6_e()
{
    set_bit(&DE.lo, 6);
    PC.word += 2;
}

//CB B4
void set_6_h()
{
    set_bit(&HL.hi, 6);
    PC.word += 2;
}

//CB B5
void set_6_l()
{
    set_bit(&HL.lo, 6);
    PC.word += 2;
}

//CB B6
void set_6_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 6);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB B7
void set_6_a()
{
    set_bit(&AF.hi, 6);
    PC.word += 2;
}

//CB B8
void set_7_b()
{
    set_bit(&BC.hi, 7);
    PC.word += 2;
}

//CB B9
void set_7_c()
{
    set_bit(&BC.lo, 7);
    PC.word += 2;
}

//CB BA
void set_7_d()
{
    set_bit(&DE.hi, 7);
    PC.word += 2;
}

//CB BB
void set_7_e()
{
    set_bit(&DE.lo, 7);
    PC.word += 2;
}

//CB BC
void set_7_h()
{
    set_bit(&HL.hi, 7);
    PC.word += 2;
}

//CB BD
void set_7_l()
{
    set_bit(&HL.lo, 7);
    PC.word += 2;
}

//CB BE
void set_7_hlp()
{
    uint8_t val = gameboy->mmu.read8(HL.word);

    set_bit(&val, 7);
    gameboy->mmu.write8(HL.word, val);
    PC.word += 2;
}

//CB BF
void set_7_a()
{
    set_bit(&AF.hi, 7);
    PC.word += 2;
}




/////////////////////////////////////////////////////////////////////////////////

instruction_t instructions[0xFF] =
{
    {"NOP",                 &nop,       1, 4},
    {"LD BC,d16",           &ld_bc_d16, 3, 12},
    {"LD (BC),A",           &ld_bcp_a,  2, 8},
    {"INC BC",              &bc_inc,    2, 8},
    {"INC B",               &b_inc,     1, 4},
    {"DEC B",               &b_dec,     1, 4},
    {"LD B,d8",             &ld_b_d8,   2, 4},
    {"RLCA",                &rlca,      1, 4},
    {"LD (a16),SP",         &ld_a16_sp, 5, 20},
    {"ADD HL,BC",           &add_hl_bc, 4, 8},
    {"LD A,(BC)",           &ld_a_bcp,  2, 8},
    {"DEC BC",              &bc_dec,    2, 8},
    {"INC C",               &c_inc,     1, 4},
    {"DEC C",               &c_dec,     1, 4},
    {"LD C,d8",             &ld_c_d8,   2, 8},
    {"RRCA",                &rrca,      1, 4},
    {"STOP 0",              &stop,      1, 4},
    {"LD DE,d16",           &ld_de_d16, 3, 12},
    {"LD (DE),A",           &ld_dep_a,  2, 8},
    {"INC DE",              &de_inc,    2, 8},
    {"INC D",               &d_inc,     1, 4},
    {"DEC D",               &d_dec,     1, 4},
    {"LD D,d8",             &ld_d_d8,   2, 8},
    {"RLA",                 &rla,       1, 4},
    {"JR r8",               &jr_r8,     3, 12},
    {"ADD HL,DE",           &add_hl_de, 2, 8},
    {"LD A,(DE)",           &ld_a_dep,  2, 8},
    {"DEC DE",              &de_dec,    2, 8},
    {"INC E",               &e_inc,     1, 4},
    {"DEC E",               &e_dec,     1, 4},
    {"LD E,d8",             &ld_e_d8,   2, 8},
    {"RRA",                 &rra,       1, 4},
    {"JR NZ,r8",            &jr_nz_r8,  0, 0}, // VARIABLE TICK CYCLES ARE ADDED IN THE FUNCTION ITSELF!
    {"LD HL,d16",           &ld_hl_d16, 3, 12},
    {"LD (HL+),A",          &ldi_hlp_a, 2, 8},
    {"INC HL",              &hl_inc,    2, 8},
    {"INC H",               &h_inc,     1, 4},
    {"DEC H",               &h_dec,     1, 4},
    {"LD H,d8",             &ld_h_d8,   2, 8},
    {"DAA",                 &daa,       1, 4},
    {"JR Z,r8",             &jr_z_r8,   0, 0},
    {"ADD HL,HL",           &add_hl_hl, 2, 8},
    {"LD A,(HL+)",          &ldi_a_hl,  2, 8},
    {"DEC HL",              &hl_dec,    2, 8},
    {"INC L",               &l_inc,     1, 4},
    {"DEC L",               &l_dec,     1, 4},
    {"LD L,d8",             &ld_l_d8,   2, 8},
    {"CPL",                 &cpl,       1, 4},
    {"JR NC,r8",            &jr_nc_r8,  0, 0},
    {"LD SP,d16",           &ld_sp_d16, 3, 12},
    {"LD (HL-),A",          &ldd_hlp_a, 2, 8},
    {"INC SP",              &sp_inc,    2, 8},
    {"INC (HL)",            &hlp_inc,   3, 12},
    {"DEC (HL)",            &hlp_dec,   3, 12},
    {"LD (HL),d8",          &ld_hlp_d8, 3, 12},
    {"SCF",                 &scf,       1, 4},
    {"JR C,r8",             &jr_c_r8,   0, 0},
    {"ADD HL,SP",           &add_hl_sp, 2, 8},
    {"LD A,(HL-)",          &ldd_a_hlp, 2, 8},
    {"DEC SP",              &sp_dec,    2, 8},
    {"INC A",               &a_inc,     1, 4},
    {"DEC A",               &a_dec,     1, 4},
    {"LD A,d8",             &ld_a_d8,   2, 8},
    {"CCF",                 &ccf,       1, 4},
    {"LD B,B",              &ld_b_b,    1, 4},
    {"LD B,C",              &ld_b_c,    1, 4},
    {"LD B,D",              &ld_b_d,    1, 4},
    {"LD B,E",              &ld_b_e,    1, 4},
    {"LD B,H",              &ld_b_h,    1, 4},
    {"LD B,L",              &ld_b_l,    1, 4},
    {"LD B,(HL)",           &ld_b_hlp,  2, 8},
    {"LD B,A",              &ld_b_a,    1, 4},
    {"LD C,B",              &ld_c_b,    1, 4},
    {"LD C,C",              &ld_c_c,    1, 4},
    {"LD C,D",              &ld_c_d,    1, 4},
    {"LD C,E",              &ld_c_e,    1, 4},
    {"LD C,H",              &ld_c_h,    1, 4},
    {"LD C,L",              &ld_c_l,    1, 4},
    {"LD C,(HL)",           &ld_c_hlp,  2, 8},
    {"LD C,A",              &ld_c_a,    1, 4},
    {"LD D,B",              &ld_d_b,    1, 4},
    {"LD D,C",              &ld_d_c,    1, 4},
    {"LD D,D",              &ld_d_d,    1, 4},
    {"LD D,E",              &ld_d_e,    1, 4},
    {"LD D,H",              &ld_d_h,    1, 4},
    {"LD D,L",              &ld_d_l,    1, 4},
    {"LD D,(HL)",           &ld_d_hlp,  2, 8},
    {"LD D,A",              &ld_d_a,    1, 4},
    {"LD E,B",              &ld_e_b,    1, 4},
    {"LD E,C",              &ld_e_c,    1, 4},
    {"LD E,D",              &ld_e_d,    1, 4},
    {"LD E,E",              &ld_e_e,    1, 4},
    {"LD E,H",              &ld_e_h,    1, 4},
    {"LD E,L",              &ld_e_l,    1, 4},
    {"LD E,HLP",            &ld_e_hlp,  2, 8},
    {"LD E,A",              &ld_e_a,    1, 4},
    {"LD H,B",              &ld_h_b,    1, 4},
    {"LD H,C",              &ld_h_c,    1, 4},
    {"LD H,D",              &ld_h_d,    1, 4},
    {"LD H,E",              &ld_h_e,    1, 4},
    {"LD H,H",              &ld_h_h,    1, 4},
    {"LD H,L",              &ld_h_l,    1, 4},
    {"LD H,(HL)",           &ld_h_hlp,  2, 8},
    {"LD H,A",              &ld_h_a,    1, 4},
    {"LD L,B",              &ld_l_b,    1, 4},
    {"LD L,C",              &ld_l_c,    1, 4},
    {"LD L,D",              &ld_l_d,    1, 4},
    {"LD L,E",              &ld_l_e,    1, 4},
    {"LD L,H",              &ld_l_h,    1, 4},
    {"LD L,L",              &ld_l_l,    1, 4},
    {"LD L,(HL)",           &ld_l_hlp,  2, 8},
    {"LD L,A",              &ld_l_a,    1, 4},
    {"LD (HL),B",           &ld_hlp_b,  2, 8},
    {"LD (HL),C",           &ld_hlp_c,  2, 8},
    {"LD (HL),D",           &ld_hlp_d,  2, 8},
    {"LD (HL),E",           &ld_hlp_e,  2, 8},
    {"LD (HL),H",           &ld_hlp_h,  2, 8},
    {"LD (HL),L",           &ld_hlp_l,  2, 8},
    {"HALT",                &halt,      1, 4},
    {"LD (HL),A",           &ld_hlp_a,  2, 8},
    {"LD A,B",              &ld_a_b,    1, 4},
    {"LD A,C",              &ld_a_c,    1, 4},
    {"LD A,D",              &ld_a_d,    1, 4},
    {"LD A,E",              &ld_a_e,    1, 4},
    {"LD A,H",              &ld_a_h,    1, 4},
    {"LD A,L",              &ld_a_l,    1, 4},
    {"LD A,(HL)",           &ld_a_hlp,  2, 8},
    {"LD A,A",              &ld_a_a,    1, 4},
    {"ADD A,B",             &add_a_b,   1, 4},
    {"ADD A,C",             &add_a_c,   1, 4},
    {"ADD A,D",             &add_a_d,   1, 4},
    {"ADD A,E",             &add_a_e,   1, 4},
    {"ADD A,H",             &add_a_h,   1, 4},
    {"ADD A,L",             &add_a_l,   1, 4},
    {"ADD A,(HL)",          &add_a_hlp, 2, 8},
    {"ADD A,A",             &add_a_a,   1, 4},
    {"ADC A,B",             &adc_a_b,   1, 4},
    {"ADC A,C",             &adc_a_c,   1, 4},
    {"ADC A,D",             &adc_a_d,   1, 4},
    {"ADC A,E",             &adc_a_e,   1, 4},
    {"ADC A,H",             &adc_a_h,   1, 4},
    {"ADC A,L",             &adc_a_l,   1, 4},
    {"ADC A,(HL)",          &adc_a_hlp, 2, 8},
    {"ADC A,A",             &adc_a_a,   1, 4},
    {"SUB B",               &sub_b,     1, 4},
    {"SUB C",               &sub_c,     1, 4},
    {"SUB D",               &sub_d,     1, 4},
    {"SUB E",               &sub_e,     1, 4},
    {"SUB H",               &sub_h,     1, 4},
    {"SUB L",               &sub_l,     1, 4},
    {"SUB (HL)",            &sub_hlp,   2, 8},
    {"SUB A",               &sub_a,     1, 4},
    {"SBC A,B",             &sbc_a_b,   1, 4},
    {"SBC A,C",             &sbc_a_c,   1, 4},
    {"SBC A,D",             &sbc_a_d,   1, 4},
    {"SBC A,E",             &sbc_a_e,   1, 4},
    {"SBC A,H",             &sbc_a_h,   1, 4},
    {"SBC A,L",             &sbc_a_l,   1, 4},
    {"SBC A,(HL)",          &sbc_a_hlp, 2, 8},
    {"SBC A,A",             &sbc_a_a,   1, 4},
    {"AND B",               &and_b,     1, 4},
    {"AND C",               &and_c,     1, 4},
    {"AND D",               &and_d,     1, 4},
    {"AND E",               &and_e,     1, 4},
    {"AND H",               &and_h,     1, 4},
    {"AND L",               &and_l,     1, 4},
    {"AND (HL)",            &and_hlp,   2, 8},
    {"AND A",               &and_a,     1, 4},
    {"XOR B",               &xor_b,     1, 4},
    {"XOR C",               &xor_c,     1, 4},
    {"XOR D",               &xor_d,     1, 4},
    {"XOR E",               &xor_e,     1, 4},
    {"XOR H",               &xor_h,     1, 4},
    {"XOR L",               &xor_l,     1, 4},
    {"XOR (HL)",            &xor_hlp,   2, 8},
    {"XOR A",               &xor_a,     1, 4},
    {"OR B",                &or_b,      1, 4},
    {"OR C",                &or_c,      1, 4},
    {"OR D",                &or_d,      1, 4},
    {"OR E",                &or_e,      1, 4},
    {"OR H",                &or_h,      1, 4},
    {"OR L",                &or_l,      1, 4},
    {"OR (HL)",             &or_hlp,    2, 8},
    {"OR A",                &or_a,      1, 4},
    {"CP B",                &cp_b,      1, 4},
    {"CP C",                &cp_c,      1, 4},
    {"CP D",                &cp_d,      1, 4},
    {"CP E",                &cp_e,      1, 4},
    {"CP H",                &cp_h,      1, 4},
    {"CP L",                &cp_l,      1, 4},
    {"CP (HL)",             &cp_hlp,    2, 8},
    {"CP A",                &cp_a,      1, 4},
    {"RET NZ",              &ret_nz,    0, 0},
    {"POP BC",              &pop_bc,    3, 12},
    {"JP NZ,A16",           &jp_nz_a16, 0, 0},
    {"JP A16",              &jp_a16,    4, 16},
    {"CALL NZ,A16",         &call_nz_a16, 0, 0},
    {"PUSH BC",             &push_bc,   4, 16},
    {"ADD A,d8",            &add_a_d8,  2, 8},
    {"RST 00h",             &rst_00,    4, 16},
    {"RET Z",               &ret_z,     0, 0},
    {"RET",                 &ret,       4, 16},
    {"JP Z,A16",            &jp_z_a16,  0, 0},
    {"CB PREFIX",           NULL,       0, 0}, // THIS SHOULD NEVER EVER EVER EVER BE EXECUTED EVER!!!!!!!!! D:
    {"CALL Z,A16",          &call_z_a16, 0, 0},
    {"CALL A16",            &call_a16,  6, 24},
    {"ADC A,d8",            &adc_a_d8,  2, 8},
    {"RST 08h",             &rst_08,    4, 16},
    {"RET NC",              &ret_nc,    0, 0},
    {"POP DE",              &pop_de,    3, 12},
    {"JP NC,A16",           &jp_nc_a16, 0, 0},
    {"NOP",                 &nop,       1, 4},
    {"CALL NC,A16",         &call_nc_a16,   0, 0},
    {"PUSH DE",             &push_de,   4, 16},
    {"SUB d8",              &sub_d8,    2, 8},
    {"RST 10h",             &rst_10,    4, 16},
    {"RET C",               &ret_c,     0, 0},
    {"RETI",                &reti,      4, 16},
    {"JP C,A16",            &jp_c_a16,  0, 0},
    {"NOP",                 &nop,       1, 4},
    {"CALL C,A16",          &call_c_a16, 0, 0},
    {"NOP",                 &nop,       1, 4},
    {"SBC A,d8",            &sbc_a_d8,  2, 8},
    {"RST 18h",             &rst_18,    4, 16},
    {"LDH (a8),A",          &ldh_a8p_a, 3, 12},
    {"POP HL",              &pop_hl,    3, 12},
    {"LD (C),A",            &ld_cp_a,   2, 8},
    {"NOP",                 &nop,       1, 4},
    {"NOP",                 &nop,       1, 4},
    {"PUSH HL",             &push_hl,   4, 16},
    {"AND d8",              &and_d8,    2, 8},
    {"RST 20h",             &rst_20,    4, 16},
    {"ADD SP,r8",           &add_sp_r8, 4, 16},
    {"JP (HL)",             &jp_hlp,    1, 4},
    {"LD (a16),A",          &ld_a16_a,  4, 16},
    {"NOP",                 &nop,       1, 4},
    {"NOP",                 &nop,       1, 4},
    {"NOP",                 &nop,       1, 4},
    {"XOR d8",              &xor_d8,    2, 8},
    {"RST 28h",             &rst_28,    4, 16},
    {"LDH A,(a8)",          &ldh_a_a8p, 3, 12},
    {"POP AF",              &pop_af,    3, 12},
    {"LD A,(C)",            &ld_a_cp,   2, 8},
    {"DI",                  &di,        1, 4},
    {"NOP",                 &nop,       1, 4},
    {"PUSH AF",             &push_af,   4, 16},
    {"OR d8",               &or_d8,     2, 8},
    {"RST 30h",             &rst_30,    4, 16},
    {"LD HL,SP+r8",         &ld_hl_spr8, 3, 4},
    {"LD SP,HL",            &ld_sp_hl,  2, 8},
    {"LD A,(a16)",          &ld_a_a16p, 4, 16},
    {"EI",                  &ei,        1, 4},
    {"NOP",                 &nop,       1, 4},
    {"NOP",                 &nop,       1, 4},
    {"CP d8",               &cp_d8,     2, 8},
    {"RST 38h",             &rst_38,    4, 16}
};

instruction_t instructionscb[0xFF] =
{
    {"RLC B",               &rlc_b,     2, 8},
    {"RLC C",               &rlc_c,     2, 8},
    {"RLC D",               &rlc_d,     2, 8},
    {"RLC E",               &rlc_e,     2, 8},
    {"RLC H",               &rlc_h,     2, 8},
    {"RLC L",               &rlc_l,     2, 8},
    {"RLC (HL)",            &rlc_hlp,   4, 16},
    {"RLC A",               &rlc_a,     2, 8},
    {"RRC B",               &rrc_b,     2, 8},
    {"RRC C",               &rrc_c,     2, 8},
    {"RRC D",               &rrc_d,     2, 8},
    {"RRC E",               &rrc_e,     2, 8},
    {"RRC H",               &rrc_h,     2, 8},
    {"RRC L",               &rrc_l,     2, 8},
    {"RRC (HL)",            &rrc_hlp,   4, 16},
    {"RRC A",               &rrc_a,     2, 8},
    {"RL  B",               &rl_b,      2, 8},
    {"RL  C",               &rl_c,      2, 8},
    {"RL  D",               &rl_d,      2, 8},
    {"RL  E",               &rl_e,      2, 8},
    {"RL  H",               &rl_h,      2, 8},
    {"RL  L",               &rl_l,      2, 8},
    {"RL  (HL)",            &rl_hlp,    4, 16},
    {"RL A",                &rl_a,      2, 8},
    {"RR B",                &rr_b,      2, 8},
    {"RR C",                &rr_c,      2, 8},
    {"RR D",                &rr_d,      2, 8},
    {"RR E",                &rr_e,      2, 8},
    {"RR H",                &rr_h,      2, 8},
    {"RR L",                &rr_l,      2, 8},
    {"RR (HL)",             &rr_hlp,    4, 16},
    {"RR A",                &rr_b,      2, 8},
    {"SLA B",               &sla_b,     2, 8},
    {"SLA C",               &sla_c,     2, 8},
    {"SLA D",               &sla_d,     2, 8},
    {"SLA E",               &sla_e,     2, 8},
    {"SLA H",               &sla_h,     2, 8},
    {"SLA L",               &sla_l,     2, 8},
    {"SLA (HL)",            &sla_hlp,   4, 16},
    {"SLA A",               &sla_a,     2, 8},
    {"SRA B",               &sra_b,     2, 8},
    {"SRA C",               &sra_c,     2, 8},
    {"SRA D",               &sra_d,     2, 8},
    {"SRA E",               &sra_e,     2, 8},
    {"SRA H",               &sra_h,     2, 8},
    {"SRA L",               &sra_l,     2, 8},
    {"SRA (HL)",            &sra_hlp,   4, 16},
    {"SRA A",               &sra_a,     2, 8},
    {"SWAP B",              &swap_b,    2, 8},
    {"SWAP C",              &swap_c,    2, 8},
    {"SWAP D",              &swap_d,    2, 8},
    {"SWAP E",              &swap_e,    2, 8},
    {"SWAP H",              &swap_h,    2, 8},
    {"SWAP L",              &swap_l,    2, 8},
    {"SWAP (HL)",           &swap_hlp,  4, 16},
    {"SWAP A",              &swap_a,    2, 8},
    {"SRL B",               &srl_b,     2, 8},
    {"SRL C",               &srl_c,     2, 8},
    {"SRL D",               &srl_d,     2, 8},
    {"SRL E",               &srl_e,     2, 8},
    {"SRL H",               &srl_h,     2, 8},
    {"SRL L",               &srl_l,     2, 8},
    {"SRL (HL)",            &srl_hlp,   4, 16},
    {"SRL A",               &srl_a,     2, 8},
    {"BIT 0,B",             &bit_0_b,   2, 8},
    {"BIT 0,C",             &bit_0_c,   2, 8},
    {"BIT 0,D",             &bit_0_d,   2, 8},
    {"BIT 0,E",             &bit_0_e,   2, 8},
    {"BIT 0,H",             &bit_0_h,   2, 8},
    {"BIT 0,L",             &bit_0_l,   2, 8},
    {"BIT 0,(HL)",          &bit_0_hlp, 4, 16},
    {"BIT 0,A",             &bit_0_a,   2, 8},
    {"BIT 1,B",             &bit_1_b,   2, 8},
    {"BIT 1,C",             &bit_1_c,   2, 8},
    {"BIT 1,D",             &bit_1_d,   2, 8},
    {"BIT 1,E",             &bit_1_e,   2, 8},
    {"BIT 1,H",             &bit_1_h,   2, 8},
    {"BIT 1,L",             &bit_1_l,   2, 8},
    {"BIT 1,(HL)",          &bit_1_hlp, 4, 16},
    {"BIT 1,A",             &bit_1_a,   2, 8},
    {"BIT 2,B",             &bit_2_b,   2, 8},
    {"BIT 2,C",             &bit_2_c,   2, 8},
    {"BIT 2,D",             &bit_2_d,   2, 8},
    {"BIT 2,E",             &bit_2_e,   2, 8},
    {"BIT 2,H",             &bit_2_h,   2, 8},
    {"BIT 2,L",             &bit_2_l,   2, 8},
    {"BIT 2,(HL)",          &bit_2_hlp, 4, 16},
    {"BIT 2,A",             &bit_2_a,   2, 8},
    {"BIT 3,B",             &bit_3_b,   2, 8},
    {"BIT 3,C",             &bit_3_c,   2, 8},
    {"BIT 3,D",             &bit_3_d,   2, 8},
    {"BIT 3,E",             &bit_3_e,   2, 8},
    {"BIT 3,H",             &bit_3_h,   2, 8},
    {"BIT 3,L",             &bit_3_l,   2, 8},
    {"BIT 3,(HL)",          &bit_3_hlp, 4, 16},
    {"BIT 3,A",             &bit_3_a,   2, 8},
    {"BIT 4,B",             &bit_4_b,   2, 8},
    {"BIT 4,C",             &bit_4_c,   2, 8},
    {"BIT 4,D",             &bit_4_d,   2, 8},
    {"BIT 4,E",             &bit_4_e,   2, 8},
    {"BIT 4,H",             &bit_4_h,   2, 8},
    {"BIT 4,L",             &bit_4_l,   2, 8},
    {"BIT 4,(HL)",          &bit_4_hlp, 4, 16},
    {"BIT 4,A",             &bit_4_a,   2, 8},
    {"BIT 5,B",             &bit_5_b,   2, 8},
    {"BIT 5,C",             &bit_5_c,   2, 8},
    {"BIT 5,D",             &bit_5_d,   2, 8},
    {"BIT 5,E",             &bit_5_e,   2, 8},
    {"BIT 5,H",             &bit_5_h,   2, 8},
    {"BIT 5,L",             &bit_5_l,   2, 8},
    {"BIT 5,(HL)",          &bit_5_hlp, 4, 16},
    {"BIT 5,A",             &bit_5_a,   2, 8},
    {"BIT 6,B",             &bit_6_b,   2, 8},
    {"BIT 6,C",             &bit_6_c,   2, 8},
    {"BIT 6,D",             &bit_6_d,   2, 8},
    {"BIT 6,E",             &bit_6_e,   2, 8},
    {"BIT 6,H",             &bit_6_h,   2, 8},
    {"BIT 6,L",             &bit_6_l,   2, 8},
    {"BIT 6,(HL)",          &bit_6_hlp, 4, 16},
    {"BIT 6,A",             &bit_6_a,   2, 8},
    {"BIT 7,B",             &bit_7_b,   2, 8},
    {"BIT 7,C",             &bit_7_c,   2, 8},
    {"BIT 7,D",             &bit_7_d,   2, 8},
    {"BIT 7,E",             &bit_7_e,   2, 8},
    {"BIT 7,H",             &bit_7_h,   2, 8},
    {"BIT 7,L",             &bit_7_l,   2, 8},
    {"BIT 7,(HL)",          &bit_7_hlp, 4, 16},
    {"BIT 7,A",             &bit_7_a,   2, 8},

    {"RES 0,B",             &res_0_b,   2, 8},
    {"RES 0,C",             &res_0_c,   2, 8},
    {"RES 0,D",             &res_0_d,   2, 8},
    {"RES 0,E",             &res_0_e,   2, 8},
    {"RES 0,H",             &res_0_h,   2, 8},
    {"RES 0,L",             &res_0_l,   2, 8},
    {"RES 0,(HL)",          &res_0_hlp, 4, 16},
    {"RES 0,A",             &res_0_a,   2, 8},
    {"RES 1,B",             &res_1_b,   2, 8},
    {"RES 1,C",             &res_1_c,   2, 8},
    {"RES 1,D",             &res_1_d,   2, 8},
    {"RES 1,E",             &res_1_e,   2, 8},
    {"RES 1,H",             &res_1_h,   2, 8},
    {"RES 1,L",             &res_1_l,   2, 8},
    {"RES 1,(HL)",          &res_1_hlp, 4, 16},
    {"RES 1,A",             &res_1_a,   2, 8},
    {"RES 2,B",             &res_2_b,   2, 8},
    {"RES 2,C",             &res_2_c,   2, 8},
    {"RES 2,D",             &res_2_d,   2, 8},
    {"RES 2,E",             &res_2_e,   2, 8},
    {"RES 2,H",             &res_2_h,   2, 8},
    {"RES 2,L",             &res_2_l,   2, 8},
    {"RES 2,(HL)",          &res_2_hlp, 4, 16},
    {"RES 2,A",             &res_2_a,   2, 8},
    {"RES 3,B",             &res_3_b,   2, 8},
    {"RES 3,C",             &res_3_c,   2, 8},
    {"RES 3,D",             &res_3_d,   2, 8},
    {"RES 3,E",             &res_3_e,   2, 8},
    {"RES 3,H",             &res_3_h,   2, 8},
    {"RES 3,L",             &res_3_l,   2, 8},
    {"RES 3,(HL)",          &res_3_hlp, 4, 16},
    {"RES 3,A",             &res_3_a,   2, 8},
    {"RES 4,B",             &res_4_b,   2, 8},
    {"RES 4,C",             &res_4_c,   2, 8},
    {"RES 4,D",             &res_4_d,   2, 8},
    {"RES 4,E",             &res_4_e,   2, 8},
    {"RES 4,H",             &res_4_h,   2, 8},
    {"RES 4,L",             &res_4_l,   2, 8},
    {"RES 4,(HL)",          &res_4_hlp, 4, 16},
    {"RES 4,A",             &res_4_a,   2, 8},
    {"RES 5,B",             &res_5_b,   2, 8},
    {"RES 5,C",             &res_5_c,   2, 8},
    {"RES 5,D",             &res_5_d,   2, 8},
    {"RES 5,E",             &res_5_e,   2, 8},
    {"RES 5,H",             &res_5_h,   2, 8},
    {"RES 5,L",             &res_5_l,   2, 8},
    {"RES 5,(HL)",          &res_5_hlp, 4, 16},
    {"RES 5,A",             &res_5_a,   2, 8},
    {"RES 6,B",             &res_6_b,   2, 8},
    {"RES 6,C",             &res_6_c,   2, 8},
    {"RES 6,D",             &res_6_d,   2, 8},
    {"RES 6,E",             &res_6_e,   2, 8},
    {"RES 6,H",             &res_6_h,   2, 8},
    {"RES 6,L",             &res_6_l,   2, 8},
    {"RES 6,(HL)",          &res_6_hlp, 4, 16},
    {"RES 6,A",             &res_6_a,   2, 8},
    {"RES 7,B",             &res_7_b,   2, 8},
    {"RES 7,C",             &res_7_c,   2, 8},
    {"RES 7,D",             &res_7_d,   2, 8},
    {"RES 7,E",             &res_7_e,   2, 8},
    {"RES 7,H",             &res_7_h,   2, 8},
    {"RES 7,L",             &res_7_l,   2, 8},
    {"RES 7,(HL)",          &res_7_hlp, 4, 16},
    {"RES 7,A",             &res_7_a,   2, 8},
    {"SET 0,B",             &set_0_b,   2, 8},
    {"SET 0,C",             &set_0_c,   2, 8},
    {"SET 0,D",             &set_0_d,   2, 8},
    {"SET 0,E",             &set_0_e,   2, 8},
    {"SET 0,H",             &set_0_h,   2, 8},
    {"SET 0,L",             &set_0_l,   2, 8},
    {"SET 0,(HL)",          &set_0_hlp, 4, 16},
    {"SET 0,A",             &set_0_a,   2, 8},
    {"SET 1,B",             &set_1_b,   2, 8},
    {"SET 1,C",             &set_1_c,   2, 8},
    {"SET 1,D",             &set_1_d,   2, 8},
    {"SET 1,E",             &set_1_e,   2, 8},
    {"SET 1,H",             &set_1_h,   2, 8},
    {"SET 1,L",             &set_1_l,   2, 8},
    {"SET 1,(HL)",          &set_1_hlp, 4, 16},
    {"SET 1,A",             &set_1_a,   2, 8},
    {"SET 2,B",             &set_2_b,   2, 8},
    {"SET 2,C",             &set_2_c,   2, 8},
    {"SET 2,D",             &set_2_d,   2, 8},
    {"SET 2,E",             &set_2_e,   2, 8},
    {"SET 2,H",             &set_2_h,   2, 8},
    {"SET 2,L",             &set_2_l,   2, 8},
    {"SET 2,(HL)",          &set_2_hlp, 4, 16},
    {"SET 2,A",             &set_2_a,   2, 8},
    {"SET 3,B",             &set_3_b,   2, 8},
    {"SET 3,C",             &set_3_c,   2, 8},
    {"SET 3,D",             &set_3_d,   2, 8},
    {"SET 3,E",             &set_3_e,   2, 8},
    {"SET 3,H",             &set_3_h,   2, 8},
    {"SET 3,L",             &set_3_l,   2, 8},
    {"SET 3,(HL)",          &set_3_hlp, 4, 16},
    {"SET 3,A",             &set_3_a,   2, 8},
    {"SET 4,B",             &set_4_b,   2, 8},
    {"SET 4,C",             &set_4_c,   2, 8},
    {"SET 4,D",             &set_4_d,   2, 8},
    {"SET 4,E",             &set_4_e,   2, 8},
    {"SET 4,H",             &set_4_h,   2, 8},
    {"SET 4,L",             &set_4_l,   2, 8},
    {"SET 4,(HL)",          &set_4_hlp, 4, 16},
    {"SET 4,A",             &set_4_a,   2, 8},
    {"SET 5,B",             &set_5_b,   2, 8},
    {"SET 5,C",             &set_5_c,   2, 8},
    {"SET 5,D",             &set_5_d,   2, 8},
    {"SET 5,E",             &set_5_e,   2, 8},
    {"SET 5,H",             &set_5_h,   2, 8},
    {"SET 5,L",             &set_5_l,   2, 8},
    {"SET 5,(HL)",          &set_5_hlp, 4, 16},
    {"SET 5,A",             &set_5_a,   2, 8},
    {"SET 6,B",             &set_6_b,   2, 8},
    {"SET 6,C",             &set_6_c,   2, 8},
    {"SET 6,D",             &set_6_d,   2, 8},
    {"SET 6,E",             &set_6_e,   2, 8},
    {"SET 6,H",             &set_6_h,   2, 8},
    {"SET 6,L",             &set_6_l,   2, 8},
    {"SET 6,(HL)",          &set_6_hlp, 4, 16},
    {"SET 6,A",             &set_6_a,   2, 8},
    {"SET 7,B",             &set_7_b,   2, 8},
    {"SET 7,C",             &set_7_c,   2, 8},
    {"SET 7,D",             &set_7_d,   2, 8},
    {"SET 7,E",             &set_7_e,   2, 8},
    {"SET 7,H",             &set_7_h,   2, 8},
    {"SET 7,L",             &set_7_l,   2, 8},
    {"SET 7,(HL)",          &set_7_hlp, 4, 16},
    {"SET 7,A",             &set_7_a,   2, 8},
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

    gameboy->cpu.clock.m = 0; // Make sure the clock struct isn't filled with crap
    gameboy->cpu.clock.t = 0;

    #ifdef DEBUG_ENABLE
        trace = fopen("trace.txt", "w");
    #endif
    printf("CPU Initialised Successfully!\n");
}

uint8_t instruction = 0x00;

// Main Processor loop
void cpu_cycle()
{
    instruction = gameboy->mmu.read8(PC.word);

    switch(instruction)
    {
        case 0xCB:
            instruction = gameboy->mmu.read8(PC.word + 1);

            fprintf(trace, "%-20s", instructionscb[instruction].name);
            fprintf(trace, "instruction:0x%02x ", instruction);
            fprintf(trace, "AF:0x%04x BC:0x%04x DE:0x%04x HL:0x%04x PC:0x%04x SP:0x%04x\n", AF.word, BC.word, DE.word, HL.word, PC.word, SP.word);

            ((void (*)(void))instructionscb[instruction].operation)();

            gameboy->cpu.clock.t += instructionscb[instruction].t_cycles;
            gameboy->cpu.clock.m += instructionscb[instruction].m_cycles;
            break;
        default:
            fprintf(trace, "%-20s", instructions[instruction].name);
            fprintf(trace, "instruction:0x%02x ", instruction);
            fprintf(trace, "AF:0x%04x BC:0x%04x DE:0x%04x HL:0x%04x PC:0x%04x SP:0x%04x\n", AF.word, BC.word, DE.word, HL.word, PC.word, SP.word);

            ((void (*)(void))instructions[instruction].operation)();

            gameboy->cpu.clock.t += instructions[instruction].t_cycles;
            gameboy->cpu.clock.m += instructions[instruction].m_cycles;
            break;
    }
}

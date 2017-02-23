#ifndef CPU_H
#define CPU_H

#include<iostream>
#include<memory>
#include<vector>
#include<bitset>
#include<unordered_map>
#include<iterator>
#include<fstream>
#include<string>
#include<algorithm>

#include "ppu.h"

#define CPU_INT_MEMORY_SIZE 0xFFFF

class PPU;

class CPU
{
public:
    enum Flag {Carry=0, Zero, Int_Disable, Dec_Mode, Break, Blank, Overflow, Negative};
    unsigned char A;
    unsigned char X ;
    unsigned char Y;
    unsigned short S;
    unsigned short PC;
    bool IRQ;
    bool NMI;
    bool oam_write_pending;
    PPU *ppu;
    std::bitset<8> flags;
    unsigned char int_memory[CPU_INT_MEMORY_SIZE];
    int cycle;
    int clocks_remain;
    CPU();
    void do_cycle();
    void push(unsigned char val);
    unsigned char pull();
    unsigned char read_memory(unsigned short address);
    void read_memory_chunk(unsigned short addr, unsigned short length, unsigned char *buffer);
    void write_memory(unsigned short address, unsigned char value);
    void dump_memory(unsigned char *buffer);
private:
    void dump_registers();
    int controller_read_count;
    bool controller_strobe;
    bool buttons_pressed;
    void update_adc_flags(unsigned char arg, unsigned int result);
    void update_and_flags();
    void update_asl_flags(unsigned char orig, unsigned char result);
    void update_cmp_flags(unsigned char mem);
    void update_cpx_flags(unsigned char mem);
    void update_cpy_flags(unsigned char mem);
    void update_dec_flags(unsigned char result);
    void update_eor_flags();
    void update_inc_flags(unsigned char result);
    void update_ldx_flags();
    void update_ldy_flags();
    void update_lda_flags();
    void update_lsr_flags(unsigned char orig, unsigned char result);
    void update_ora_flags();
    void update_rol_flags(unsigned char orig, unsigned char result);
    void update_ror_flags(unsigned char orig, unsigned char result);

    unsigned short get_indirect_x_address();
    unsigned short get_address_from_zp();
    unsigned short get_absolute_address();
};
#endif
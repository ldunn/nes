#ifndef PPU_H
#define PPU_H

#include "cpu.h"
#include <SFML/Graphics.hpp>
#include<queue>

class CPU;

class PPU
{
public:
    unsigned char PPUCTRL;
    unsigned char PPUMASK;
    unsigned char PPUSTATUS;
    unsigned char OAMADDR;
    unsigned char OAMDATA;
    unsigned char PPUSCROLL;
    unsigned char PPUADDR;
    unsigned char PPUDATA;
    unsigned char OAMDMA;  
    unsigned short vram_addr;
    bool vblank;
    bool NMI_occurred;
    bool NMI_output;
    unsigned char OAM[256];
    unsigned char OAM_secondary[8];
    unsigned char sprite_dots[256]; //index into palette
    unsigned char pattern_tables[2][4096];
    unsigned char name_tables[4][1024];
    unsigned char palette[32];
    CPU *cpu;
    void do_cycle();
    unsigned char read_memory(unsigned short address);
    void write_memory(unsigned short address, unsigned char val);
    void render();
    unsigned char read_data();
    void write_data(unsigned char val);
    void write_oam(unsigned char val);
    void update_addr(unsigned short byte);
    void increment_addr();
    int scanline;
    int dot;
    int frame;
    bool odd_frame;
    sf::Uint8 *buffer;
    bool vram_addr_high_byte; // 0 = update low byte, 1 = update high byte
    void dump_memory(unsigned char *buffer);
    PPU();
};
#endif
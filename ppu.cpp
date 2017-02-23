#include "ppu.h"

unsigned char palette_colors[192] = {124,124,124,
0,0,252,
0,0,188,
68,40,188,
148,0,132,
168,0,32,
168,16,0,
136,20,0,
80,48,0,
0,120,0,
0,104,0,
0,88,0,
0,64,88,
0,0,0,
0,0,0,
0,0,0,
188,188,188,
0,120,248,
0,88,248,
104,68,252,
216,0,204,
228,0,88,
248,56,0,
228,92,16,
172,124,0,
0,184,0,
0,168,0,
0,168,68,
0,136,136,
0,0,0,
0,0,0,
0,0,0,
248,248,248,
60,188,252,
104,136,252,
152,120,248,
248,120,248,
248,88,152,
248,120,88,
252,160,68,
248,184,0,
184,248,24,
88,216,84,
88,248,152,
0,232,216,
120,120,120,
0,0,0,
0,0,0,
252,252,252,
164,228,252,
184,184,248,
216,184,248,
248,184,248,
248,164,192,
240,208,176,
252,224,168,
248,216,120,
216,248,120,
184,248,184,
184,248,216,
0,252,252,
248,216,248,
0,0,0,
0,0,0};

void PPU::do_cycle()
{
    switch(scanline)
    {
        case 0 ... 239:
            if(dot < 256)
            {
                //std::cout << "[PPU] DOING VISIBLE SCANLINE " << scanline << " AT DOT " << dot << std::endl;
                unsigned char name_table = PPUCTRL & 0x3; // Low 2 bits of PPUCTRL select name table
                unsigned char pattern_table = std::bitset<8>(PPUCTRL)[4];
                int x = dot/8;
                int i = dot % 8;
                int y = scanline;
                unsigned short tile = name_tables[name_table][x + (y/8)*32];
                unsigned char low_byte = pattern_tables[pattern_table][(tile<<4)+y%8];
                unsigned char high_byte = pattern_tables[pattern_table][(tile<<4)+(y%8)+8];
                unsigned char attr_index = 8*y/32 + dot/32;
                unsigned char attr_byte = name_tables[name_table][0x3C0+attr_index];
                unsigned char attr_data;
                if((dot/16) % 2 == 0 && (y/16) % 2 == 0) // Upper left quad
                    attr_data = (attr_byte >> 6) & 0x3;
                else if((dot/16) % 2 == 1 && (y/16) % 2 == 0) // Upper right quad
                    attr_data = (attr_byte >> 4) & 0x3;
                else if((dot/16) % 2 == 0 && (y/16) % 2 == 1) // Lower left quad
                    attr_data = (attr_byte >> 2) & 0x3;
                else
                    attr_data = attr_byte & 0x3;


                /*unsigned char low_byte = pattern_tables[pattern_table][0x500+y%8];
                unsigned char high_byte = pattern_tables[pattern_table][0x508+y%8];*/
                unsigned char pixel_on = ((low_byte >> i) & 0x1) + ((high_byte >> i) & 0x1);
                if(pixel_on != 0)
                {
                    buffer[(y*32*8 + x*8 + (7-i))*4] = palette_colors[palette[((attr_data<<2) | pixel_on)]*3];
                    buffer[(y*32*8 + x*8 + (7-i))*4 + 1] = palette_colors[palette[((attr_data<<2) | pixel_on)]*3+1];
                    buffer[(y*32*8 + x*8 + (7-i))*4 + 2] = palette_colors[palette[((attr_data<<2) | pixel_on)]*3+2];
                    buffer[(y*32*8 + x*8 + (7-i))*4 + 3] = 255;
                }
                else
                {
                    buffer[(y*32*8 + x*8 + (7-i))*4] = palette_colors[palette[0]*3];
                    buffer[(y*32*8 + x*8 + (7-i))*4 + 1] = palette_colors[palette[0]*3+1];
                    buffer[(y*32*8 + x*8 + (7-i))*4 + 2] = palette_colors[palette[0]*3+2];
                    buffer[(y*32*8 + x*8 + (7-i))*4 + 3] = 255;
                }
            }
            break;
        case 241:
            if(dot == 0)
            {
                NMI_occurred = true;
                std::cout << "[PPU] SETTING VBLANK BIT OF PPUSTATUS" << std::endl;
                PPUSTATUS |= 0x80;
            }
            break;
        case 260:
            NMI_occurred = false;
            scanline = 0;
            break;
    }
    if(dot++ > 339)
    {
        scanline++;
        dot = 0;
    }
    if(NMI_occurred && NMI_output)
        cpu->NMI = true;

}

PPU::PPU()
{
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 1024; j++)
            name_tables[i][j] = 0x24;
    }
    return;
    vram_addr_high_byte = true;
    vram_addr = 0;
}

/*void PPU::render(sf::Uint8 *buffer)
{
    unsigned char name_table = PPUCTRL & 0x3; // Low 2 bits of PPUCTRL select name table
    unsigned char pattern_table = PPUCTRL & 0x8;
    for(int y = 0; y< 30*8; y++)
    {
        for(int x = 0; x < 32; x++)
        {
            unsigned short tile = name_tables[name_table][x + (y/8)*32];
            unsigned char low_byte = pattern_tables[pattern_table][tile+y%8];
            unsigned char high_byte = pattern_tables[pattern_table][tile+(y%8)+8];
            for(int i = 0; i < 8; i++)
            {
                bool pixel_on = ((low_byte >> i) & 0x1) | ((high_byte >> i) & 0x1);
                buffer[(y*32*8 + x*8 + i)*4] = 0;
                buffer[(y*32*8 + x*8 + i)*4 + 1] = 0;
                buffer[(y*32*8 + x*8 + i)*4 + 2] = pixel_on*255;
                buffer[(y*32*8 + x*8 + i)*4 + 3] = 255;
            }
        }
    }
    NMI_occurred = true;
    std::cout << "SETTING VBLANK BIT OF PPUSTATUS" << std::endl;
    PPUSTATUS |= 0x80;
}*/

unsigned char PPU::read_memory(unsigned short address)
{
    if(address <= 0xFFF)
        return pattern_tables[0][address];
    else if(address <= 0x1FFF)
        return pattern_tables[1][address - 0x1000];
    else if(address <= 0x23FF)
        return name_tables[0][address - 0x2000];
    else if(address <= 0x27FF)
        return name_tables[1][address - 0x2400];
    else if(address <= 0x2BFF)
        return name_tables[2][address - 0x2800];
    else if(address <= 0x2FFF)
        return name_tables[3][address - 0x2C00];
    else if(address >= 0x3F00)
        return palette[address - 0x3F00];
    else
        return 0;
}

void PPU::dump_memory(unsigned char *buffer)
{
    for(int i = 0; i < 0x4000; i++)
        buffer[i]  = read_memory(i);
}

void PPU::write_memory(unsigned short address, unsigned char val)
{
    if(address <= 0xFFF)
        pattern_tables[0][address] = val;
    else if(address <= 0x1FFF)
        pattern_tables[1][address - 0x1000] = val;
    else if(address <= 0x23FF)
        name_tables[0][address - 0x2000] = val;
    else if(address <= 0x27FF)
        name_tables[1][address - 0x2400] = val;
    else if(address <= 0x2BFF)
        name_tables[2][address - 0x2800] = val;
    else if(address <= 0x2FFF)
        name_tables[3][address - 0x2C00] = val;
    else if(address >= 0x3F00)
        palette[address - 0x3F00] = val;

}

unsigned char PPU::read_data()
{
    unsigned char value = read_memory(vram_addr);
    increment_addr();
    return value;
}

void PPU::write_data(unsigned char val)
{
    write_memory(vram_addr, val);
    increment_addr();
}

void PPU::increment_addr()
{
    std::bitset<8> ppuctrl_bits(PPUCTRL);
    if(ppuctrl_bits[2])
        vram_addr += 32;
    else
        vram_addr += 1;
}

void PPU::update_addr(unsigned short byte)
{
    if(vram_addr_high_byte)
    {
        std::cout << "[PPU] SETTING HIGH BYTE OF VRAM ADDR AS 0x" << std::hex << byte << std::dec << std::endl;
        vram_addr &= 0x00ff;
        vram_addr |= (byte << 8);
    }
    else
    {
        std::cout << "[PPU] SETTING LOW BYTE OF VRAM ADDR AS 0x" << std::hex << byte << std::dec << std::endl;
        vram_addr &= 0xff00;
        vram_addr |= byte & 0x00FF;
    }
    vram_addr_high_byte = !vram_addr_high_byte;
}

void PPU::write_oam(unsigned char val)
{
    OAM[OAMADDR] = val;
    OAMADDR++;
}
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

int mod(int a, int b) {
    return a >= 0 ? a % b : ( b - abs( a%b ) ) % b;
}

void PPU::do_cycle()
{
    if(dot > 340 || (scanline == 0 && dot == 339))
    {
        scanline++;
        dot = 0;
        if(scanline == 261)
        {
            scanline = -1;
            if(odd_frame)
            {
                odd_frame = !odd_frame;
                dot = 0;
            }
            frame++;
            //std::cout << "FRAME: " << frame << std::endl;
        }
    }
    if(dot == 300)
    {
        attr_data = std::queue<unsigned char>();
        tile_low_byte = std::queue<unsigned char>();
        tile_high_byte = std::queue<unsigned char>();
        nametable_byte = std::queue<unsigned char>();
    }
    if(dot == 257 && ((PPUMASK & 0x8) || (PPUMASK & 0x10)) && scanline < 240)
    {
        vram_addr &= ~0x41F;
        vram_addr |= vram_addr_temp & 0x41F;
    }
    if(((mod(dot - fine_x, 8) == 0 && dot >= 8 && (dot-fine_x < 256)) || dot == 328 || dot == 336) && ((PPUMASK & 0x8) || (PPUMASK & 0x10)) && scanline < 240)
    {
        if((vram_addr & 0x001F) == 31) // coarse X == 31
        {
            vram_addr &= ~0x001F; // coarse X = 0
            vram_addr ^= 0x400; // Switch nametable;
            //std::cout << "SWTICHING HORIZ NAMETABLE" << std::endl;
        }
        else
        {
            //std::cout << "INCREMENTING VRAM ADDR AT DOT " << dot << std::endl;
            vram_addr += 1;
        }
    }
    if(dot == 256 && ((PPUMASK & 0x8) || (PPUMASK & 0x10)) && scanline < 240 && scanline > -1)
    {
        // From https://wiki.nesdev.com/w/index.php/PPU_scrollinghttps://wiki.nesdev.com/w/index.php/PPU_scrolling
        if ((vram_addr & 0x7000) != 0x7000)        // if fine Y < 7
            vram_addr+= 0x1000 ;                    // increment fine Y
        else
        {
            vram_addr &= ~0x7000 ;                   // fine Y = 0
            int y = (vram_addr & 0x03E0) >> 5;        // let y = coarse Y
            if (y == 29)
            {
                y = 0;                          // coarse Y = 0
                //std::cout << "SWITCHING VERT NAMETABLE" << std::endl;
                vram_addr ^= 0x0800;                    // switch vertical nametable
            }
            else if (y == 31)
                y = 0;                          // coarse Y = 0, nametable not switched
            else
                y += 1;                         // increment coarse Y
            vram_addr = (vram_addr & ~0x03E0) | (y << 5)  ;   // put coarse Y back into v 
        }
    }
    switch(scanline)
    {
        case -1:
            if(dot == 1)
            {
                PPUSTATUS &= ~(0x40);
                PPUSTATUS &= ~(0x80);
                PPUSTATUS &= ~(0x20);
                NMI_occurred = false;
            }
            else if(dot >= 280 && dot <= 304 && ((PPUMASK & 0x8) && (PPUMASK & 0x10)))
            {
                vram_addr &= ~0x7BE0;
                vram_addr |= vram_addr_temp & 0x7BE0;
            }
            if(dot == 321 || dot == 329)
                fetch_tile_data();
            break;
        case 0 ... 239:
            if(((mod(dot-fine_x - 1, 8) == 0) && (dot-fine_x <= 249) && dot > 0) || (dot == 321) || (dot == 329))
            {
               // std::cout << "FETCHING TILE AT DOT " << dot << std::endl;
                fetch_tile_data();
            }
            if((mod(dot + fine_x, 8) == 0 && (dot+fine_x) < 256) || dot == 0)
            {
                if(attr_data.size() == 0)
                    std::cout << "EMPTY QUEUE" << std::endl;
                //std::cout << "UPDATING TILE AT DOT " << dot << std::endl;
                curr_attr_data = attr_data.front();
                curr_nametable_byte = nametable_byte.front();
                curr_tile_low_byte = tile_low_byte.front();
                curr_tile_high_byte = tile_high_byte.front();
                attr_data.pop();
                nametable_byte.pop();
                tile_low_byte.pop();
                tile_high_byte.pop();

            }
            if(dot == 260)
            {
                std::fill(sprite_dots, sprite_dots+256, 0);
                std::fill(sprite_zero_pixels, sprite_zero_pixels+8, -1);
                
                for(int i = 0; i < 64; i++)
                {
                    unsigned char *oam_data = &(OAM[i*4]);
                    unsigned char x = oam_data[3];
                    unsigned char y = oam_data[0];
                    unsigned char pixel_on;
                    if(scanline+1 > y && scanline+1 <= y + 8)
                    {
                        unsigned char palette_sel = (oam_data[2] & 0x3) + 4;
                        unsigned char tile = oam_data[1];
                        unsigned char pattern_table_spr = std::bitset<8>(PPUCTRL)[3];
                        unsigned char low_byte = pattern_tables[pattern_table_spr][(tile<<4)+(scanline - oam_data[0])];
                        unsigned char high_byte = pattern_tables[pattern_table_spr][(tile<<4)+((scanline - oam_data[0]) + 8)];  
                        if(oam_data[2] & 0x40) // Horizontal flip
                        {
                            for(int j = 7; j >= 0; j--)
                            {
                                pixel_on = ((low_byte >> (7-j)) & 0x1) + ((high_byte >> (7-j)) & 0x1);
                                if(pixel_on)
                                {
                                    sprite_dots[7-j+oam_data[3]] = palette_sel*4+pixel_on;
                                    if(i == 0)
                                    {
                                        sprite_zero_pixels[j] = 7-j+oam_data[3];
                                    }
                                }
                                else
                                    sprite_dots[7-j+oam_data[3]] = 0;
                            }
                        }
                        else
                        {
                            for(int j = 0; j < 8; j++)
                            {
                                pixel_on = ((low_byte >> (7-j)) & 0x1) + ((high_byte >> (7-j)) & 0x1);
                                if(pixel_on)
                                {
                                    sprite_dots[j+oam_data[3]] = palette_sel*4+pixel_on;
                                    if(i == 0)
                                    {
                                        sprite_zero_pixels[j] = j+oam_data[3];
                                    }
                                }
                                else
                                    sprite_dots[j+oam_data[3]] = 0;                  
                            }
                        }
                    }
                }
            }
            if(dot == 2 && sprite_zero_pending)
            {
                PPUSTATUS |= 0x40;
                sprite_zero_pending = false;
            }

            if(dot < 256)
            {
                int x = dot/8;
                int i = dot % 8;
                int y = scanline;
                if(PPUMASK & 0x8)
                {
                    int scrolled_x = (vram_addr & 0x1F);
                    int scrolled_y = ((vram_addr >> 5) & 0x1F)*8 + ((vram_addr >> 12) & 0x7);
                    int scrolled_i = mod(dot + fine_x, 8);
                    //std::cout << "TILE AT (" << x << "," << y << "): GOT " << (int)curr_nametable_byte << std::endl;
                    //std::cout << "ATTR_INDEX FOR (" << dot << "," << y <<") :" << (int)attr_index << std::endl;

                    unsigned char pixel_on = ((curr_tile_low_byte >> (7-scrolled_i)) & 0x1) + ((curr_tile_high_byte >> (7-scrolled_i)) & 0x1);
                    if(pixel_on != 0)
                    {
                        buffer[(y*32*8 + dot)*4] = palette_colors[palette[(curr_attr_data*4 + pixel_on)]*3];
                        buffer[(y*32*8 + dot)*4 + 1] = palette_colors[palette[(curr_attr_data*4 + pixel_on)]*3+1];
                        buffer[(y*32*8 + dot)*4 + 2] = palette_colors[palette[(curr_attr_data*4 + pixel_on)]*3+2];
                        buffer[(y*32*8 + dot)*4 + 3] = 255;
                        bg_opaque[dot] = true;
                    }
                    else
                    {
                        buffer[(y*32*8 + dot)*4] = palette_colors[palette[0]*3];
                        buffer[(y*32*8 + dot)*4 + 1] = palette_colors[palette[0]*3+1];
                        buffer[(y*32*8 + dot)*4 + 2] = palette_colors[palette[0]*3+2];
                        buffer[(y*32*8 + dot)*4 + 3] = 255;
                        bg_opaque[dot] = false;
                    }
                }
                if(PPUMASK & 0x10)
                {
                    if(sprite_dots[dot] != 0)
                    {
                        unsigned char palette_sel = sprite_dots[dot];
                        buffer[(y*32*8 + dot)*4] = palette_colors[palette[palette_sel]*3];
                        buffer[(y*32*8 + dot)*4 + 1] = palette_colors[palette[palette_sel]*3+1];
                        buffer[(y*32*8 + dot)*4 + 2] = palette_colors[palette[palette_sel]*3+2];
                        buffer[(y*32*8 + dot)*4 + 3] = 255;
                    }
                }
                if(((PPUMASK & 0x8) && (PPUMASK & 0x10)) && !(PPUSTATUS & 0x40) && bg_opaque[dot] && dot != 255)
                {
                    if((!(PPUMASK & 0x4) || !(PPUMASK & 0x2)) && dot <= 7)
                        break;
                    int *hit = std::find(std::begin(sprite_zero_pixels), std::end(sprite_zero_pixels), dot);
                    if(hit != std::end(sprite_zero_pixels))
                    {   
                        if(dot < 2)
                            sprite_zero_pending = true;
                        else
                            PPUSTATUS |= 0x40;
                    }
                }
            }
            break;
        case 241:
            if(dot == 1)
            {
                //std::cout << "[PPU] FINISHED VISIBLE RENDER" << std::endl;
                NMI_occurred = true;
                PPUSTATUS |= 0x80;
            }
            break;
    }
    dot++;
    if(NMI_occurred && NMI_output)
        cpu->NMI = true;
}

void PPU::fetch_tile_data()
{
    unsigned char pattern_table_bg = std::bitset<8>(PPUCTRL)[4];
    int scrolled_x = (vram_addr & 0x1F);
    int scrolled_y = ((vram_addr >> 5) & 0x1F)*8 + ((vram_addr >> 12) & 0x7);
    unsigned char tile = read_memory(0x2000 | (vram_addr & 0xFFF));
    //std::cout << "READING TILE AT " << (int) (vram_addr & 0xFFF) << " GOT " << (int) tile <<std::endl;
    nametable_byte.push(tile);
    unsigned char low_byte = pattern_tables[pattern_table_bg][(tile<<4)+scrolled_y%8];
    tile_low_byte.push(low_byte);
    unsigned char high_byte = pattern_tables[pattern_table_bg][(tile<<4)+(scrolled_y%8)+8];
    tile_high_byte.push(high_byte);
    unsigned char attr_byte = read_memory(0x23C0 | (vram_addr & 0x0C00) | ((vram_addr >> 4) & 0x38) | ((vram_addr >> 2) & 0x07));
    unsigned char attr;
    if(((scrolled_x)/2) % 2 == 0 && (scrolled_y/16) % 2 == 0) // Upper left quad
        attr = attr_byte & 0x3;
    else if(((scrolled_x)/2) % 2 == 1 && (scrolled_y/16) % 2 == 0) // Upper right quad
        attr = (attr_byte >> 2) & 0x3;
    else if(((scrolled_x)/2) % 2 == 0 && (scrolled_y/16) % 2 == 1) // Lower left quad
        attr = (attr_byte >> 4) & 0x3;
    else
        attr = (attr_byte >> 6) & 0x3;
    attr_data.push(attr);
}

PPU::PPU()
{
    vram_addr_high_byte = true;
    vram_addr = 0;
    addr_scroll_latch = false;
    fine_x = 0;
    scanline = -1;
    attr_data = std::queue<unsigned char>();
    tile_low_byte = std::queue<unsigned char>();
    tile_high_byte = std::queue<unsigned char>();
    nametable_byte = std::queue<unsigned char>();
}

unsigned char PPU::read_memory(unsigned short address)
{
    if(address >= 0x4000)
    {
        std::cout << "BAD PPU READ" << std::endl;
        while(1);
    }
    if(address <= 0xFFF)
        return pattern_tables[0][address];
    else if(address <= 0x1FFF)
        return pattern_tables[1][address - 0x1000];
    else if(address <= 0x2FFF)
        return name_tables[get_nametable_address(address)-0x2000];
    else if(address >= 0x3F00)
    {
        if(address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C)
            return palette[address - 0x3F10];
        else
            return palette[address - 0x3F00];
    }
    else
        return 0;
}

unsigned short PPU::get_nametable_address(unsigned short address)
{
    switch(mirroring)
    {
        case 0: // Horizontal (vertical arrangement)
            return address & ~0x400;
        case 1:
            return address & ~0x800;
    }
    return 0;
}

void PPU::dump_memory(unsigned char *buffer)
{
    for(int i = 0; i < 0x4000; i++)
        buffer[i]  = read_memory(i);
}

void PPU::write_memory(unsigned short address, unsigned char val)
{
    if(address >= 0x4000)
    {
        std::cout << "BAD PPU WRITE" << std::endl;
        while(1);
    }
    if(address <= 0xFFF)
        pattern_tables[0][address] = val;
    else if(address <= 0x1FFF)
        pattern_tables[1][address - 0x1000] = val;
    else if(address <= 0x2FFF)
        name_tables[get_nametable_address(address)-0x2000] = val;
    else if(address >= 0x3F00)
    {
        if(address == 0x3F10 || address == 0x3F14 || address == 0x3F18 || address == 0x3F1C)
            palette[address - 0x3F10] = val;
        else
            palette[address - 0x3F00] = val;
    }

}

void PPU::write_ppuscroll(unsigned char val)
{
    if(!addr_scroll_latch)
    {
        vram_addr_temp &= ~0x1F;
        vram_addr_temp |= (val >> 3) & 0x1F;
        fine_x = val & 0x7;
        //fine_x = 4;
    }
    else
    {
        vram_addr_temp &= ~0x73E0;
        vram_addr_temp |= ((val & 0x7) << 12) | (((val >> 3) & 0x1F) << 5);
    }
    addr_scroll_latch = !addr_scroll_latch;
}

unsigned char PPU::read_data()
{
    unsigned char value = read_memory(vram_addr);
    unsigned char ret;
    if(vram_addr > 0x3EFF) // Reading from palette
    {
        ret = value;
        read_buffer = value;
    }
    else
    {
        ret = read_buffer;
        read_buffer = value;
    }
    increment_addr();
    return ret;
}

void PPU::write_data(unsigned char val)
{
    write_memory(vram_addr, val);
    increment_addr();
}

void PPU::increment_addr()
{
    if(((PPUMASK & 0x8) && (PPUMASK & 0x10)) && scanline >= -1 && scanline <= 239)
    {
        vram_addr &= ~(1 << 9);
        vram_addr |= (vram_addr_temp & (1 << 9));
        vram_addr &= ~0x1F;
        vram_addr |= (vram_addr_temp & 0x1F);
        std::cout << "[PPU] INC ADDR DURING RENDER" << std::endl;
        if ((vram_addr & 0x7000) != 0x7000)        // if fine Y < 7
            vram_addr += 0x1000 ;                    // increment fine Y
        else
        {
            vram_addr &= ~0x7000 ;                   // fine Y = 0
            int y = (vram_addr & 0x03E0) >> 5;        // let y = coarse Y
            if (y == 29)
            {
                y = 0;                          // coarse Y = 0
                vram_addr^= 0x0800;                    // switch vertical nametable
            }
            else if (y == 31)
                y = 0;                          // coarse Y = 0, nametable not switched
            else
                y += 1;                         // increment coarse Y
            vram_addr = (vram_addr & ~0x03E0) | (y << 5)  ;   // put coarse Y back into v 
        }
    }
    else
    {
        std::bitset<8> ppuctrl_bits(PPUCTRL);
        if(ppuctrl_bits[2])
            vram_addr += 32;
        else
            vram_addr += 1;
    }
}

void PPU::update_addr(unsigned short byte)
{
    if(!addr_scroll_latch)
    {
       // std::cout << "[PPU] SETTING HIGH BYTE OF VRAM ADDR AS 0x" << std::hex << byte << std::dec << std::endl;
        vram_addr_temp &= ~(0xFF00);
        vram_addr_temp |= (byte << 8) & 0xFf00;
        vram_addr_temp &= ~(1 << 15);
     //   std::cout << "[PPU] TEMP VRAM IS 0x" << std::hex << vram_addr_temp << std::dec << std::endl;
    }
    else
    {
       // std::cout << "[PPU] SETTING LOW BYTE OF VRAM ADDR AS 0x" << std::hex << byte << std::dec << std::endl;
        vram_addr_temp &= ~0xFF;
        vram_addr_temp |= byte;
        vram_addr = vram_addr_temp;
      //  std::cout << "[PPU] VRAM ADDR IS 0x" << std::hex << vram_addr << std::dec << std::endl;
    }
    addr_scroll_latch = !addr_scroll_latch;
}

void PPU::write_oam(unsigned char val)
{
    OAM[OAMADDR] = val;
    OAMADDR++;
}
#include<iostream>
#include<memory>
#include<vector>
#include<bitset>
#include<unordered_map>
#include<iterator>
#include<fstream>
#include<string>
#include<algorithm>

#include<SFML/Graphics.hpp>
#include<SFML/Graphics/Image.hpp>

#include "ppu.h"
#include "cpu.h"

class NESFile
{
public:
    std::vector<char> prg_rom;
    std::vector<char> chr_rom;
    NESFile(std::vector<char> &buf)
    {
        std::vector<char> header(buf.begin(), buf.begin()+16);
        std::cout << "Header size: " << header.size() << std::endl;
        std::string magic(header.data(), 4);
        std::cout << magic << std::endl;
        if(magic != "NES\x1a")
        {
            std::cout << "INVALID MAGIC" << std::endl;
            return;
        }
        unsigned char prg_rom_size = header[4]; // In 16kb units
        unsigned char chr_rom_size = header[5]; // In 8kb units
        unsigned char flags_six = header[6];
        unsigned char flags_seven = header[7];
        unsigned char prg_ram_size = header[8]; // In 8kb units
        unsigned char flags_nine = header[9];
        prg_rom = std::vector<char>(buf.begin()+16, buf.begin()+16+16384*prg_rom_size);
        chr_rom = std::vector<char>(buf.begin()+16+16384*prg_rom_size, buf.begin()+16+16384*prg_rom_size+8192*chr_rom_size);
        std::cout << prg_rom.size() << std::endl;
    }
};




int main(int argc, char *argv[])
{
    std::ifstream nes_in(argv[1], std::ios::binary);
    std::vector<char> nes_buffer((std::istreambuf_iterator<char>(nes_in)), 
        std::istreambuf_iterator<char>());
    NESFile nes{nes_buffer};
    CPU cpu = CPU();
    PPU ppu = PPU();
    cpu.ppu = &ppu;
    ppu.cpu = &cpu;
    std::cout << "PRG ROM SIZE: " << nes.prg_rom.size() << std::endl;
    std::cout << "CHR ROM SIZE: " << nes.chr_rom.size() << std::endl;
    std::copy(nes.prg_rom.begin(), nes.prg_rom.end(), &(cpu.int_memory[0x10000-nes.prg_rom.size()]));
    std::copy(nes.chr_rom.begin(), nes.chr_rom.begin() + 0x1000, &(ppu.pattern_tables[0][0]));
    std::copy(nes.chr_rom.begin() + 0x1000, nes.chr_rom.end(), &(ppu.pattern_tables[1][0]));
    unsigned short reset_addr = (cpu.int_memory[0xfffd] << 8) + cpu.int_memory[0xfffc];
    cpu.PC = reset_addr;
    //cpu.PC = 0xC000;
    std::cout << "RESETTING TO 0x" << std::hex << reset_addr << std::dec  << std::endl;
    ppu.vram_addr_high_byte = true;

    ppu.buffer = new sf::Uint8[61440*4];
    sf::RenderWindow window(sf::VideoMode(256, 240), "NES");
    sf::Texture text;
    text.create(256, 240);
    sf::Sprite sprite;
    sprite.setTexture(text);
    sprite.setPosition(0, 0);
    while(window.isOpen())
    {
        ppu.do_cycle();
        ppu.do_cycle();
        ppu.do_cycle();
        cpu.do_cycle();   
        
        if(cpu.cycle % 40000 == 0)
        {
            window.setTitle(std::to_string(ppu.frame));    
            std::ofstream out("dump", std::ios::out | std::ios::binary);
            unsigned char ppu_buffer[0x4000];
            ppu.dump_memory(ppu_buffer);
            out.write((char *)ppu_buffer, 0x4000);
            out.close();
            std::ofstream test_out("test_out", std::ios::out);
            test_out.write((char *)&(cpu.int_memory[0x6004]),0x2000);
            test_out.close();
            if(cpu.S > 0x1ff || cpu.S < 0x100)
            {
                std::cout << "STACK BLOWN" << std::endl;
                while(1);
            }
                
            window.clear(sf::Color(255, 255, 255));
            //ppu.render(buffer);

            text.update(ppu.buffer);

            window.draw(sprite);

            window.display();
            sf::Event event;
            while(window.pollEvent(event))
            {
                if(event.type == sf::Event::Closed)
                    window.close();
            }
        }


    }
}

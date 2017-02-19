#include<iostream>
#include<memory>
#include<vector>
#include<bitset>
#include<unordered_map>

#define INT_MEMORY_SIZE 16384

class CPU
{
    public:
        enum Reg {A, X, Y, PC, S};
        enum Flag {Carry, Zero, Int_Disable, Dec_Mode, Break, Overflow, Negative};
        std::unordered_map<Reg, int> regs;
        std::bitset<30> pins;
        std::bitset<8> flags;
        unsigned char int_memory[INT_MEMORY_SIZE];
        int cycle;
        int clocks_remain;
        CPU()
        {
            // Initial state from https://wiki.nesdev.com/w/index.php/CPU_power_up_state
            regs[Reg::A] = 0;
            regs[Reg::X] = 0;
            regs[Reg::Y] = 0;
            regs[Reg::S] = 0xFD;
            regs[Reg::PC] = 0x0;
            cycle = 0;
            clocks_remain = 0;
        }

        void update_lda_flags()
        {
            flags[Flag::Zero] = regs[Reg::A] == 0;
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7] == 1;
            
        }

        void update_adc_flags(unsigned int result)
        {
            flags[Flag::Carry] = result > 0xFF;
            flags[Flag::Zero] = regs[Reg::A]== 0;
            flags[Flag::Overflow] = ~(regs[Reg::A] ^ read_memory(regs[Reg::PC] + 1)) & (regs[Reg::A] ^ result) & 0x80; // ???
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7] == 1;            
        }

        void update_and_flags()
        {
            flags[Flag::Zero] = regs[Reg::A] == 0;
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7] == 1;
        }

        void update_asl_flags(unsigned int result)
        {
            flags[Flag::Carry] = std::bitset<32>(regs[Reg::A])[8];
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7] == 1;
            flags[Flag::Zero] = regs[Reg::A] == 0;
        }

        void do_cycle()
        {
            clocks_remain--;
            std::cout << "Doing cycle: " << cycle << std::endl;
            std::cout << "Clocks remaining: " << clocks_remain << std::endl;
            auto opcode = read_memory(regs[Reg::PC]);
            switch(opcode)
            {
                case 0xA9: // LDA IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = read_memory(regs[Reg::PC] + 1);
                        regs[Reg::PC] += 2;
                        update_lda_flags();
                    }
                    break;
                case 0xA5: // LDA ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = read_memory(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                        update_lda_flags();
                    }
                    
                    break;
                case 0xB5: // LDA ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 255);
                        regs[Reg::PC] += 2;
                        update_lda_flags();
                    }
                    break;
                case 0xAD: // LDA ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address();
                        regs[Reg::A] = read_memory(address);
                        regs[Reg::PC] += 3;
                        update_lda_flags();
                    }
                    break;
                case 0xBD: // LDA ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if((absolute/256) < (absolute+regs[Reg::X])/256) // Crossed a page boundary
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X];
                        regs[Reg::A] = read_memory(address);
                        regs[Reg::PC] += 3;
                        update_lda_flags();
                    }
                    break;
                case 0xB9: // LDA ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if((absolute/256) < (absolute+regs[Reg::Y])/256) // Crossed a page boundary
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::Y];
                        regs[Reg::A] = read_memory(address);
                        regs[Reg::PC] += 3;
                        update_lda_flags();
                    }
                    break;
                case 0xA1: // LDA (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_indirect_x_address();
                        regs[Reg::A] = read_memory(address);
                        regs[Reg::PC] += 2;
                        update_lda_flags();
                    }
                    break;
                case 0xB1: // LDA (INDIRECT),Y
                    if(clocks_remain < 0)
                    {
                        unsigned short address = get_address_from_zp();
                        if(address/256 < (address + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 5; 
                        else
                            clocks_remain = 4;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short zp_address = read_memory(regs[Reg::PC] + 1);
                        unsigned short address = (read_memory(zp_address + 1) << 8) + (read_memory(zp_address)); // Address _before_ Y is added
                        regs[Reg::A] = read_memory(address + regs[Reg::Y]);
                        regs[Reg::PC] += 2;
                        update_lda_flags();
                    }
                    break;
                case 0x69: // ADC IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        // From http://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
                        unsigned int result = regs[Reg::A] + read_memory(regs[Reg::PC] + 1) + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x65: // ADC ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        unsigned char arg = read_memory(read_memory(regs[Reg::PC] + 1));
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x75: // ADC ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char arg = read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 255);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x6D: // ADC ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address();
                        unsigned char arg = read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 3;
                        update_adc_flags(result);
                    }
                    break;
                case 0x7D: // ADC ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if((absolute/256) < (absolute+regs[Reg::X])/256) // Crossed a page boundary
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X];
                        unsigned char arg = read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 3;
                        update_adc_flags(result);
                    }
                    break;
                case 0x79: // ADC ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if((absolute/256) < (absolute+regs[Reg::Y])/256) // Crossed a page boundary
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::Y];
                        unsigned char arg = read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 3;
                        update_adc_flags(result);
                    }
                    break;
                case 0x61: // ADC (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_indirect_x_address();
                        unsigned char arg = read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x71: // ADC (INDIRECT),Y
                    if(clocks_remain < 0)
                    {
                        unsigned short address = get_address_from_zp(); // _before_ Y added
                        if(address/256 < (address + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 5; 
                        else
                            clocks_remain = 4;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_address_from_zp() + regs[Reg::Y];
                        unsigned char arg = read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x29: // AND IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] & read_memory(regs[Reg::PC] + 1);
                        regs[Reg::PC] += 2;
                        update_and_flags();
                    }
                    break;
                case 0x25: // AND ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] & read_memory(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                        update_and_flags();
                    }
                    break;
                case 0x35: // AND ZERO PAGE, 0x
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] & read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 255);
                        regs[Reg::PC] += 2;
                        update_and_flags();
                    }
                    break;
                case 0x2D: // AND ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address();
                        regs[Reg::A] = regs[Reg::A] & read_memory(address);
                        regs[Reg::PC] += 3;
                        update_and_flags();
                    }
                    break;
                case 0x3D: // AND ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute+regs[Reg::X])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X];
                        regs[Reg::A]  = regs[Reg::A] & read_memory(address);
                        regs[Reg::PC] += 3;
                        update_and_flags();
                    }
                    break;
                case 0x39: // AND ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute+regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::Y];
                        regs[Reg::A] = regs[Reg::A] & read_memory(address);
                        regs[Reg::PC] += 3;
                        update_and_flags();
                    }
                    break;
                case 0x21: // AND (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_indirect_x_address();
                        regs[Reg::A] = regs[Reg::A] & read_memory(address);
                        regs[Reg::PC] += 2;
                        update_and_flags();
                    }
                    break;
                case 0x31: // AND (INDIRECT),Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_address_from_zp();
                        if(absolute/256 < (absolute+regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 5;
                        else
                            clocks_remain = 4;
                    }
                    else if(clocks_remain==0)
                    {
                        unsigned short address = get_address_from_zp() + regs[Reg::Y];
                        regs[Reg::A] = regs[Reg::A] & read_memory(address);
                        regs[Reg::PC] += 2;
                        update_and_flags();                        
                    }
                    break;
                case 0x0A: // ASL ACCUMULATOR
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        unsigned int result = regs[Reg::A] << 1;
                        regs[Reg::A] = result % 255;
                        regs[Reg::PC] += 1;
                        update_asl_flags(result);
                    }
                case 0x06: // ASL ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = read_memory(regs[Reg::PC]+1);
                        unsigned int result = read_memory(address) << 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 2;
                        update_asl_flags(result);
                    }
                    break;
                case 0x16: // ASL ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 255;
                        unsigned int result = read_memory(address) << 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 2;
                        update_asl_flags(result);
                    }
                    break;
                case 0x0E: // ASL ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address();
                        unsigned int result = read_memory(address) << 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 3;
                        update_asl_flags(result);
                    }
                    break;
                case 0x1E: // ASL ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 6;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X];
                        unsigned int result = read_memory(address) << 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 3;
                        update_asl_flags(result);
                    }
                    break;
                
            }
            cycle++;
        }

        unsigned short get_indirect_x_address()
        {
            unsigned short zp_address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 255; // Address stored in zero page to be read from
            return (read_memory(zp_address + 1) << 8) + (read_memory(zp_address));              
        }

        unsigned short get_address_from_zp() // For (INDIRECT),Y addressing
        {
            unsigned short zp_address = read_memory(regs[Reg::PC] + 1);
            return (read_memory(zp_address + 1) << 8) + (read_memory(zp_address)); 
        }

        unsigned short get_absolute_address()
        {
            return (read_memory(regs[Reg::PC] + 2) << 8) + (read_memory(regs[Reg::PC] + 1));
        }

        unsigned char read_memory(int address)
        {
            if(address < INT_MEMORY_SIZE)
            {
                std::cout << "Reading memory from 0x" << std::hex << address << ": got 0x" << (int)int_memory[address] << std::dec << std::endl;
                return int_memory[address];
            }
            else
                return 0;
        }

        void write_memory(int address, unsigned char value)
        {
            if(address < INT_MEMORY_SIZE)
            {
                std::cout << "Writing at address 0x" << std::hex << address << ": 0x" << (int)value << std::dec << std::endl;
                int_memory[address] = value;
            }
        }
};

int main()
{
    CPU cpu = CPU();
    cpu.regs[CPU::Reg::A] = 0x1;
    cpu.regs[CPU::Reg::Y] = 0x4;
    cpu.int_memory[0] = 0x0A;
    cpu.int_memory[1] = 0x0A;
    cpu.int_memory[2] = 0x0A;
    cpu.int_memory[0x105] = 0xBD;
    while(cpu.cycle < 10)
    {
        cpu.do_cycle();
    }
}

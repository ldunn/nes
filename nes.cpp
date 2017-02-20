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
        std::unordered_map<Reg, unsigned int> regs;
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

        void update_cmp_flags(unsigned char mem)
        {
            unsigned char result = regs[Reg::A] - mem;
            flags[Flag::Carry] = regs[Reg::A] >= mem;
            flags[Flag::Zero] = regs[Reg::A] == mem;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_cpx_flags(unsigned char mem)
        {
            unsigned char result = regs[Reg::X] - mem;
            flags[Flag::Carry] = regs[Reg::X] >= mem;
            flags[Flag::Zero] = regs[Reg::X] == mem;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_cpy_flags(unsigned char mem)
        {
            unsigned char result = regs[Reg::Y] - mem;
            flags[Flag::Carry] = regs[Reg::Y] >= mem;
            flags[Flag::Zero] = regs[Reg::Y] == mem;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_dec_flags(unsigned char result)
        {
            flags[Flag::Zero] = result == 0;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_eor_flags()
        {
            flags[Flag::Zero] = regs[Reg::A] == 0;
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7];
        }

        void update_inc_flags(unsigned char result)
        {
            flags[Flag::Zero] = result == 0;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_ldx_flags()
        {
            flags[Flag::Zero] = regs[Reg::X] == 0;
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::X])[7];
        }

        void update_ldy_flags()
        {
            flags[Flag::Zero] = regs[Reg::Y] == 0;
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::Y])[7];
        }

        void update_lsr_flags(unsigned char orig, unsigned char result)
        {
            flags[Flag::Carry] = std::bitset<8>(orig)[0];
            flags[Flag::Zero] = result == 0;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_ora_flags()
        {
            flags[Flag::Zero] = regs[Reg::A] == 0;
            flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7];
        }

        void update_rol_flags(unsigned char orig, unsigned char result)
        {
            flags[Flag::Carry] = std::bitset<8>(orig)[7];
            flags[Flag::Zero] = result == 0;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
        }

        void update_ror_flags(unsigned char orig, unsigned char result)
        {
            flags[Flag::Carry] = std::bitset<8>(orig)[0];
            flags[Flag::Zero] = result == 0;
            flags[Flag::Negative] = std::bitset<8>(result)[7];
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
                        regs[Reg::A] = read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 256);
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
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x75: // ADC ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char arg = read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 256);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = result % 256;
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
                        regs[Reg::A] = regs[Reg::A] & read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 256);
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
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 1;
                        update_asl_flags(result);
                    }
                    break;
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
                        unsigned short address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
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
                case 0x90: // BCC
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Carry] == 0;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Carry] == 0)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0xB0: // BCS
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Carry] == 1;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Carry] == 1)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0xF0: // BEQ
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Zero] == 1;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Zero] == 1)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0x24: // BIT ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain==0)
                    {
                        unsigned short val = read_memory(read_memory(regs[Reg::PC]+1));
                        unsigned char result = regs[Reg::A] & val;
                        flags[Flag::Zero] = result == 0;
                        flags[Flag::Overflow] = std::bitset<8>(val)[6];
                        flags[Flag::Negative] = std::bitset<8>(val)[7];
                    }
                    break;
                case 0x2C: // BIT ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain ==0)
                    {
                        unsigned short val = read_memory(get_absolute_address());
                        unsigned char result = regs[Reg::A] & val;
                        flags[Flag::Zero] = result == 0;
                        flags[Flag::Overflow] = std::bitset<8>(val)[6];
                        flags[Flag::Negative] = std::bitset<8>(val)[7];
                    }
                    break;
                case 0x30: // BMI
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Negative] == 1;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Negative] == 1)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0xD0: // BNE
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Zero] == 0;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Zero] == 0)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0x10: // BPL
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Negative] == 0;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Negative] == 0)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0x00: // BRK
                    if(clocks_remain < 0)
                        clocks_remain = 6;
                    else if(clocks_remain == 0)
                    {
                        push(regs[Reg::PC]);
                        std::bitset<8> status{};
                        status[0] = flags[Flag::Carry];
                        status[1] = flags[Flag::Zero];
                        status[2] = flags[Flag::Int_Disable];
                        status[3] = flags[Flag::Dec_Mode];
                        status[6] = flags[Flag::Overflow];
                        status[7] = flags[Flag::Negative];
                        push((unsigned char)status.to_ulong());
                        flags[Flag::Break] = 1;
                        regs[Reg::PC] = (read_memory(0xffff) << 8) + read_memory(0xfffe);
                    }
                    break;
                case 0x50: // BVC
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Overflow] == 0;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Overflow] == 0)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0x70: // BVS
                    if(clocks_remain < 0)
                    {
                        // Not positive on the timing here
                        clocks_remain = 1;
                        unsigned short branch_addr = regs[Reg::PC] + (signed char)read_memory(regs[Reg::PC] + 1);
                        bool branch_succeed = flags[Flag::Overflow] == 1;
                        if(branch_succeed)
                            clocks_remain += 1;
                        if(regs[Reg::PC]/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                            clocks_remain += 2;
                    }
                    else if(clocks_remain == 0)
                    {
                        if(flags[Flag::Overflow] == 1)
                            regs[Reg::PC] += (signed char)read_memory(regs[Reg::PC] + 1);
                    }
                    break;
                case 0x18: // CLC
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        flags[Flag::Carry] = 0;
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x58: // CLI
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        flags[Flag::Int_Disable] = 0;
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0xB8: // CLI
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        flags[Flag::Overflow] = 0;
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0xC9: // CMP IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;    
                    }
                    break;
                case 0xC5: // CMP ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(read_memory(read_memory(regs[Reg::PC] + 1)));
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0xD5: // CMP ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        update_cmp_flags(read_memory(address));
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0xCD: // CMP ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(get_absolute_address());
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0xDD: // CMP ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::X])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(get_absolute_address() + regs[Reg::X]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0xD9: // CMP ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(get_absolute_address() + regs[Reg::Y]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0xC1: // CMP (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(get_indirect_x_address());
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0xD1: // CMP (INDIRECT),Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_address_from_zp();
                        if(absolute/256 < (absolute + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 5;
                        else
                            clocks_remain = 4;
                    }
                    else if(clocks_remain == 0)
                    {
                        update_cmp_flags(get_address_from_zp() + regs[Reg::Y]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0xE0: // CPX IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        update_cpx_flags(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0xE4: // CPX ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        update_cpx_flags(read_memory(read_memory(regs[Reg::PC] + 1)));
                        regs[Reg::PC] += 2;

                    }
                    break;
                case 0xEC: // CPX ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        update_cpx_flags(get_absolute_address());
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0xC0: // CPY IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        update_cpy_flags(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0xC4: // CPY ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        update_cpy_flags(read_memory(read_memory(regs[Reg::PC] + 1)));
                        regs[Reg::PC] += 2;

                    }
                    break;
                case 0xCC: // CPY ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        update_cpy_flags(get_absolute_address());
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0xC6: // DEC ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = read_memory(regs[Reg::PC] + 1);
                        unsigned char result = read_memory(address) - 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 2;
                        update_dec_flags(result);
                        
                    }
                    break;
                case 0xD6: // DEC ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        unsigned char result = read_memory(address) - 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 2;
                        update_dec_flags(result);
                    }
                    break;
                case 0xCE: // DEC ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address();
                        unsigned char result = read_memory(address) - 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 3;
                        update_dec_flags(result);
                    }
                    break;
                case 0xDE: // DEC ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 6;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X]; 
                        unsigned char result = read_memory(address) - 1;
                        write_memory(address, result);
                        regs[Reg::PC] += 3;
                        update_dec_flags(result);
                    }
                    break;
                case 0xCA: // DEX
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        unsigned char result = regs[Reg::X] - 1;
                        regs[Reg::X] = result;
                        regs[Reg::PC] += 1;
                        flags[Flag::Zero] = result == 0;
                        flags[Flag::Negative] = std::bitset<8>(result)[7];
                    }
                    break;
                case 0x88: // DEY
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        unsigned char result = regs[Reg::Y] - 1;
                        regs[Reg::Y] = result;
                        regs[Reg::PC] += 1;
                        flags[Flag::Zero] = result == 0;
                        flags[Flag::Negative] = std::bitset<8>(result)[7];
                    }
                    break;
                case 0x49: // EOR IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(regs[Reg::PC] + 1);
                        regs[Reg::PC] += 2;
                        update_eor_flags();
                    }
                    break;
                case 0x45: // EOR ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                        update_eor_flags();
                    }
                    break;
                case 0x55: // EOR ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(address);
                        regs[Reg::PC] += 2;
                        update_eor_flags();
                    }
                    break;
                case 0x4D: // EOR ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(get_absolute_address());
                        regs[Reg::PC] += 3;
                        update_eor_flags();
                    }
                    break;
                case 0x5D: // EOR ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::X])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X];
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(address);
                        regs[Reg::PC] += 3;
                        update_eor_flags();
                    }
                    break;
                case 0x59: // EOR ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::Y];
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(address);
                        regs[Reg::PC] += 3;
                        update_eor_flags();
                    }
                    break;
                case 0x41: //EOR (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(get_indirect_x_address());
                        regs[Reg::PC] += 3;
                        update_eor_flags();
                    }
                    break;
                case 0x51: // EOR (INDIRECT),Y
                    if(clocks_remain < 0)
                    {
                        unsigned short zp_addr = get_address_from_zp();
                        if(zp_addr/256 < (zp_addr + regs[Reg::Y])/256)
                            clocks_remain = 5;
                        else
                            clocks_remain = 4;
                    }
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] ^ read_memory(get_address_from_zp() + regs[Reg::Y]);
                        regs[Reg::PC] += 3;
                        update_eor_flags();
                    }
                    break;
                case 0xE6: // INC ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        unsigned short addr = read_memory(regs[Reg::PC]+1);
                        unsigned char val = read_memory(addr) + 1;
                        write_memory(addr, val);
                        regs[Reg::PC] += 2;
                        update_inc_flags(val);
                    }
                    break;
                case 0xF6: // INC ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short addr = (read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 256;
                        unsigned char val = read_memory(addr) + 1;
                        write_memory(addr, val);
                        regs[Reg::PC] += 2;
                        update_inc_flags(val);
                    }
                    break;
                case 0xEE: // INC ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short addr = get_absolute_address();
                        unsigned char val = read_memory(addr) + 1;
                        write_memory(addr, val);
                        regs[Reg::PC] += 3;
                        update_inc_flags(val);
                    }
                    break;
                case 0xFE: // INC ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short addr = get_absolute_address() + regs[Reg::X];
                        unsigned char val = read_memory(addr) + 1;
                        write_memory(addr, val);
                        regs[Reg::PC] += 3;
                        update_inc_flags(val);
                    }
                    break;
                case 0xE8: // INX
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] += 1;
                        regs[Reg::PC] += 1;
                        flags[Flag::Zero] = regs[Reg::X] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::X])[7];
                    }
                    break;
                case 0xC8: // INY
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::Y] += 1;
                        regs[Reg::PC] += 1;
                        flags[Flag::Zero] = regs[Reg::Y] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::Y])[7];
                    }
                    break;
                case 0x4C: // JMP ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::PC] = get_absolute_address();
                    }
                    break;
                case 0x6C: // JMP INDIRECT 
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        // TODO POSSIBLY BREAK THIS AS DESCRIBED AT http://obelisk.me.uk/6502/reference.html#JMP
                        unsigned short indir_addr = get_absolute_address();
                        regs[Reg::PC] = (read_memory(indir_addr+1) << 8) + (read_memory(indir_addr));
                    }
                    break;
                case 0x20: // JSR ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short return_addr = regs[Reg::PC] + 2;
                        push((return_addr >> 8) & 0xff); // Push high byte of return address
                        push(return_addr & 0xff); // Push low byte of return address
                        regs[Reg::PC] = get_absolute_address();
                    }
                    break;
                case 0xA2: // LDX IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] = read_memory(regs[Reg::PC] + 1);
                        regs[Reg::PC] += 2;
                        update_ldx_flags();
                    }
                    break;
                case 0xA6: // LDX ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] = read_memory(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                        update_ldx_flags();
                    }
                    break;
                case 0xB6: // LDX ZERO PAGE, Y
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::Y]) % 256;
                        regs[Reg::X] = read_memory(addr);
                        regs[Reg::PC] += 2;
                        update_ldx_flags();
                    }
                    break;
                case 0xAE: // LDX ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] = read_memory(get_absolute_address());
                        regs[Reg::PC] += 3;
                        update_ldx_flags();
                    }
                    break;
                case 0xBE: // LDX ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] = read_memory(get_absolute_address() + regs[Reg::Y]);
                        regs[Reg::PC] += 3;
                        update_ldx_flags();
                    }
                    break;
                case 0xA0: // LDY IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::Y] = read_memory(regs[Reg::PC] + 1);
                        regs[Reg::PC] += 2;
                        update_ldy_flags();
                    }
                    break;
                case 0xA4: // LDY ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::Y] = read_memory(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                        update_ldy_flags();
                    }
                    break;
                case 0xB4: // LDY ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        regs[Reg::Y] = read_memory(addr);
                        regs[Reg::PC] += 2;
                        update_ldy_flags();
                    }
                    break;
                case 0xAC: // LDY ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::Y] = read_memory(get_absolute_address());
                        regs[Reg::PC] += 3;
                        update_ldy_flags();
                    }
                    break;
                case 0xBC: // LDY ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::X])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::Y] = read_memory(get_absolute_address() + regs[Reg::X]);
                        regs[Reg::PC] += 3;
                        update_ldy_flags();
                    }
                    break;
                case 0x4A: // LSR ACCUMULATOR
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        unsigned char result = regs[Reg::A] >> 2;
                        unsigned char orig = regs[Reg::A];
                        regs[Reg::A] = result;
                        regs[Reg::PC] += 1;
                        update_lsr_flags(orig, result);
                    }
                    break;
                case 0x46: // LSR ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = read_memory(regs[Reg::PC] + 1);
                        unsigned char orig = read_memory(addr);
                        unsigned char result = orig >> 2;
                        write_memory(addr, result);
                        regs[Reg::PC] += 2;
                        update_lsr_flags(orig, result);
                    }
                    break;
                case 0x56: // LSR ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        unsigned char orig = read_memory(addr);
                        unsigned char result = orig >> 2;
                        write_memory(addr, result);
                        regs[Reg::PC] += 2;
                        update_lsr_flags(orig, result);
                    }
                    break;
                case 0x4E: // LSR ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = get_absolute_address();
                        unsigned char orig = read_memory(addr);
                        unsigned char result = orig >> 2;
                        write_memory(addr, result);
                        regs[Reg::PC] += 2;
                        update_lsr_flags(orig, result);
                    }
                    break;
                case 0x5E: // LSR ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 6;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = get_absolute_address() + regs[Reg::X];
                        unsigned char orig = read_memory(addr);
                        unsigned char result = orig >> 2;
                        write_memory(addr, result);
                        regs[Reg::PC] += 2;
                        update_lsr_flags(orig, result);
                    }
                    break;
                case 0xEA: // NOP
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                        regs[Reg::PC] += 1;
                    break;
                case 0x09: // ORA IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] | read_memory(regs[Reg::PC] + 1);
                        regs[Reg::PC] += 2;
                        update_ora_flags();
                    }
                    break;
                case 0x05: // ORA ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] | read_memory(read_memory(regs[Reg::PC] + 1));
                        regs[Reg::PC] += 2;
                        update_ora_flags();
                    }
                    break;
                case 0x15: // ORA ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        regs[Reg::A] = regs[Reg::A] | read_memory(address);
                        regs[Reg::PC] += 2;
                        update_ora_flags();
                    }
                    break;
                case 0x0D: // ORA ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] | read_memory(get_absolute_address());
                        regs[Reg::PC] += 3;
                        update_ora_flags();
                    }
                    break;
                case 0x1D: // ORA ABSOLUTE,X
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::X])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::X];
                        regs[Reg::A] = regs[Reg::A] | read_memory(address);
                        regs[Reg::PC] += 3;
                        update_ora_flags();
                    }
                    break;
                case 0x19: // ORA ABSOLUTE,Y
                    if(clocks_remain < 0)
                    {
                        unsigned short absolute = get_absolute_address();
                        if(absolute/256 < (absolute + regs[Reg::Y])/256) // Page boundary crossed
                            clocks_remain = 4;
                        else
                            clocks_remain = 3;
                    }
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address() + regs[Reg::Y];
                        regs[Reg::A] = regs[Reg::A] | read_memory(address);
                        regs[Reg::PC] += 3;
                        update_ora_flags();
                    }
                    break;
                case 0x01: //ORA (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] | read_memory(get_indirect_x_address());
                        regs[Reg::PC] += 3;
                        update_ora_flags();
                    }
                    break;
                case 0x11: // ORA (INDIRECT),Y
                    if(clocks_remain < 0)
                    {
                        unsigned short zp_addr = get_address_from_zp();
                        if(zp_addr/256 < (zp_addr + regs[Reg::Y])/256)
                            clocks_remain = 5;
                        else
                            clocks_remain = 4;
                    }
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::A] | read_memory(get_address_from_zp() + regs[Reg::Y]);
                        regs[Reg::PC] += 3;
                        update_ora_flags();
                    }
                    break;
                case 0x48: // PHA
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        push(regs[Reg::A]);
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x08: // PHP
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        std::bitset<8> status{};
                        status[0] = flags[Flag::Carry];
                        status[1] = flags[Flag::Zero];
                        status[2] = flags[Flag::Int_Disable];
                        status[3] = flags[Flag::Dec_Mode];
                        status[6] = flags[Flag::Overflow];
                        status[7] = flags[Flag::Negative];
                        push((unsigned char)status.to_ulong());
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x68: // PLA
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = pull();
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x28: // PLP
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        std::bitset<8> status(pull());
                        flags[Flag::Carry] = status[0];
                        flags[Flag::Zero] = status[1];
                        flags[Flag::Int_Disable] = status[2];
                        flags[Flag::Dec_Mode] = status[3];
                        flags[Flag::Overflow] = status[6];
                        flags[Flag::Negative] = status[7];
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x2A: // ROL ACCUMULATOR
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        unsigned char orig = regs[Reg::A];
                        unsigned char result = (orig << 1) | flags[Flag::Carry];
                        regs[Reg::A] = result;
                        regs[Reg::PC] += 1;
                        update_rol_flags(orig, result);
                    }
                    break;
                case 0x26: // ROL ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = read_memory(regs[Reg::PC] + 1);
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig << 1) | flags[Flag::Carry];
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_rol_flags(orig, result);
                    }
                    break;
                case 0x36: // ROL ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig << 1) | flags[Flag::Carry];
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_rol_flags(orig, result);
                    }
                    break;
                case 0x2E: // ROL ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = get_absolute_address();
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig << 1) | flags[Flag::Carry];
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_rol_flags(orig, result);
                    }
                    break;
                case 0x3E: // ROL ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = get_absolute_address() + regs[Reg::X];
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig << 1) | flags[Flag::Carry];
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_rol_flags(orig, result);
                    }
                    break;
                case 0x6A: // ROR ACCUMULATOR
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        unsigned char orig = regs[Reg::A];
                        unsigned char result = (orig >> 1) | ((flags[Flag::Carry] << 8) & 0x80);
                        regs[Reg::A] = result;
                        regs[Reg::PC] += 1;
                        update_ror_flags(orig, result);
                    }
                    break;
                case 0x66: // ROR ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = read_memory(regs[Reg::PC] + 1);
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig >> 1) | ((flags[Flag::Carry] << 8) & 0x80);
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_ror_flags(orig, result);
                    }
                    break;
                case 0x76: // ROR ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig >> 1) | ((flags[Flag::Carry] << 8) & 0x80);
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_ror_flags(orig, result);
                    }
                    break;
                case 0x6E: // ROR ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = get_absolute_address();
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig >> 1) | ((flags[Flag::Carry] << 8) & 0x80);
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_ror_flags(orig, result);
                    }
                    break;
                case 0x7E: // ROR ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = get_absolute_address() + regs[Reg::X];
                        unsigned char orig = read_memory(addr);
                        unsigned char result = (orig >> 1) | ((flags[Flag::Carry] << 8) & 0x80);
                        write_memory(addr, result);
                        regs[Reg::PC] += 1;
                        update_ror_flags(orig, result);
                    }
                    break;
                case 0x40: // RTI
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        std::bitset<8> status(pull());
                        unsigned char return_low = pull();
                        unsigned char return_high = pull();
                        regs[Reg::PC] = ((unsigned short)return_high << 8) + return_low;
                        flags[Flag::Carry] = status[0];
                        flags[Flag::Zero] = status[1];
                        flags[Flag::Int_Disable] = status[2];
                        flags[Flag::Dec_Mode] = status[3];
                        flags[Flag::Overflow] = status[6];
                        flags[Flag::Negative] = status[7];
                    }
                    break;
                case 0x60: // RTS
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned char return_low = pull();
                        unsigned char return_high = pull();
                        regs[Reg::PC] = ((unsigned short)return_high << 8) + return_low;
                    }
                    break;
                case 0xE9: // SBC IMMEDIATE
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        // From http://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
                        unsigned int result = regs[Reg::A] + ~read_memory(regs[Reg::PC] + 1) + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0xE5: // SBC ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        unsigned char arg = ~read_memory(read_memory(regs[Reg::PC] + 1));
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0xF5: // SBC ZERO PAGE, X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char arg = ~read_memory((read_memory(regs[Reg::PC]+1) + regs[Reg::X]) % 256);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0xED: // SBC ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_absolute_address();
                        unsigned char arg = ~read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 3;
                        update_adc_flags(result);
                    }
                    break;
                case 0xFD: // SBC ABSOLUTE,X
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
                        unsigned char arg = ~read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 3;
                        update_adc_flags(result);
                    }
                    break;
                case 0xF9: // SBC ABSOLUTE,Y
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
                        unsigned char arg = ~read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 3;
                        update_adc_flags(result);
                    }
                    break;
                case 0xE1: // SBC (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        unsigned short address = get_indirect_x_address();
                        unsigned char arg = ~read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0xF1: // SBC (INDIRECT),Y
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
                        unsigned char arg = ~read_memory(address);
                        unsigned int result = regs[Reg::A] + arg + flags[Flag::Carry];
                        regs[Reg::A] = result % 256;
                        regs[Reg::PC] += 2;
                        update_adc_flags(result);
                    }
                    break;
                case 0x38: // SEC
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        flags[Flag::Carry] = 1;
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x78: // SEI
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        flags[Flag::Int_Disable] = 1;
                        regs[Reg::PC] += 1;
                    }
                    break;
                case 0x85: // STA ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = read_memory(regs[Reg::PC] + 1);
                        write_memory(addr, regs[Reg::A]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x95: // STA ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        write_memory(addr, regs[Reg::A]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x8D: // STA ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_absolute_address(), regs[Reg::A]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0x9D: // STA ABSOLUTE,X
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_absolute_address() + regs[Reg::X], regs[Reg::A]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0x99: // STA ABSOLUTE,Y
                    if(clocks_remain < 0)
                        clocks_remain = 4;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_absolute_address() + regs[Reg::Y], regs[Reg::A]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0x81: // STA (INDIRECT,X)
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_indirect_x_address(), regs[Reg::A]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x91: // STA (INDIRECT),Y
                    if(clocks_remain < 0)
                        clocks_remain = 5;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_address_from_zp() + regs[Reg::Y], regs[Reg::A]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x86: // STX, ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                       unsigned char addr = read_memory(regs[Reg::PC] + 1);
                        write_memory(addr, regs[Reg::X]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x96: // STX, ZERO PAGE,Y
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                       unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::Y]) % 256;
                        write_memory(addr, regs[Reg::X]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x8E: // STX ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_absolute_address(), regs[Reg::X]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0x84: // STY ZERO PAGE
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                       unsigned char addr = read_memory(regs[Reg::PC] + 1);
                        write_memory(addr, regs[Reg::Y]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x94: // STY ZERO PAGE,X
                    if(clocks_remain < 0)
                        clocks_remain = 2;
                    else if(clocks_remain == 0)
                    {
                       unsigned char addr = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256;
                        write_memory(addr, regs[Reg::Y]);
                        regs[Reg::PC] += 2;
                    }
                    break;
                case 0x8C: // STY ABSOLUTE
                    if(clocks_remain < 0)
                        clocks_remain = 3;
                    else if(clocks_remain == 0)
                    {
                        write_memory(get_absolute_address(), regs[Reg::Y]);
                        regs[Reg::PC] += 3;
                    }
                    break;
                case 0xAA: // TAX
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] = regs[Reg::A];
                        flags[Flag::Zero] = regs[Reg::X] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::X])[7];
                    }
                    break;
                case 0xA8: // TAY
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::Y] = regs[Reg::A];
                        flags[Flag::Zero] = regs[Reg::Y] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::Y])[7];
                    }
                    break;
                case 0xBA: // TSX
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::X] = regs[Reg::S];
                        flags[Flag::Zero] = regs[Reg::X] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::X])[7];
                    }
                    break;
                case 0x8A: // TXA
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::X];
                        flags[Flag::Zero] = regs[Reg::A] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7];
                    }
                    break;
                case 0x9A: // TXS
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::S] = regs[Reg::X];
                        flags[Flag::Zero] = regs[Reg::S] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::S])[7];
                    }
                    break;
                case 0x98: // TYA
                    if(clocks_remain < 0)
                        clocks_remain = 1;
                    else if(clocks_remain == 0)
                    {
                        regs[Reg::A] = regs[Reg::Y];
                        flags[Flag::Zero] = regs[Reg::A] == 0;
                        flags[Flag::Negative] = std::bitset<8>(regs[Reg::A])[7];
                    }
                    break;
            }
            cycle++;
        }

        void push(unsigned char val)
        {
            int_memory[regs[Reg::S]] = val;
            regs[Reg::S]--;
            return;
        }

        unsigned char pull()
        {
            unsigned char val = int_memory[regs[Reg::S]];
            regs[Reg::S]++;
            return val;
        }

        unsigned short get_indirect_x_address()
        {
            unsigned short zp_address = (read_memory(regs[Reg::PC] + 1) + regs[Reg::X]) % 256; // Address stored in zero page to be read from
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
    cpu.int_memory[0] = 0x0A;
    cpu.int_memory[1] = 0x90;
    cpu.int_memory[2] = -1;
    cpu.int_memory[0x105] = 0xBD;
    while(cpu.cycle < 100)
    {
        cpu.do_cycle();
    }
    return 0;
}

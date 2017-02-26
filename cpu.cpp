#include "cpu.h"
CPU::CPU()
{
    // Initial state from https://wiki.nesdev.com/w/index.php/CPU_power_up_state
    A = 0;
    X = 0;
    Y = 0;
    S = 0x1FF;
    PC = 0x0;
    cycle = 0;
    flags_zero = 1;
    flags_carry = 1;
    flags_int_disable = 1;
    flags_dec_mode = 0;
    flags_break = 0;
    flags_overflow = 0;
    flags_negative = 0;
    clocks_remain = 0;
    oam_write_pending = false;
    controller_read_count = 0;
}

void CPU::dump_memory(unsigned char *buffer)
{
    for(int i = 0; i < CPU_INT_MEMORY_SIZE; i++)
    {
        buffer[i] = read_memory(i);
    }
}

void CPU::dump_registers()
{
    std::cout << std::hex << " A: " << (unsigned short)A;
    std::cout << std::hex << " X: " << (unsigned short)X ;
    std::cout << std::hex << " Y: " << (unsigned short)Y;
    std::cout << std::hex << " S: " << (unsigned short)S << std::endl;
}

void CPU::update_lda_flags()
{
    flags_zero = A == 0;
    flags_negative = A & 0x80;
    
}

void CPU::update_adc_flags(unsigned char arg, unsigned int result)
{
    //std::cout << "[ADC] RESULT: 0x" << std::hex << result << std::dec << std::endl;
    flags_carry = (result > 0xFF);
    flags_zero = (result & 0xff) == 0;
    flags_overflow = ((A ^ result) & (arg ^ result) & 0x80);
    flags_negative = ((result & 0xff) & 0x80);            
}

void CPU::update_and_flags()
{
    flags_zero = A == 0;
    flags_negative = (A & 0x80);
}

void CPU::update_asl_flags(unsigned char orig, unsigned char result)
{
    flags_carry = orig & 0x80;
    flags_negative = result & 0x80;
    flags_zero = A == 0;
}

void CPU::update_cmp_flags(unsigned char mem)
{
    unsigned char result = A - mem;
    flags_carry = A >= mem;
    flags_zero = A == mem;
    flags_negative = result & 0x80;
}

void CPU::update_cpx_flags(unsigned char mem)
{
    unsigned short result = (signed char)X - (signed char)mem;
    flags_carry = X >= mem;
    flags_zero = X == mem;
    flags_negative = result & 0x80;
}

void CPU::update_cpy_flags(unsigned char mem)
{
    unsigned char result = Y - mem;
    flags_carry = Y >= mem;
    flags_zero = Y == mem;
    flags_negative = result & 0x80;
}

void CPU::update_dec_flags(unsigned char result)
{
    flags_zero = result == 0;
    flags_negative = result & 0x80;
}

void CPU::update_eor_flags()
{
    flags_zero = A == 0;
    flags_negative = A & 0x80;
}

void CPU::update_inc_flags(unsigned char result)
{
    flags_zero = result == 0;
    flags_negative = result & 0x80;
}

void CPU::update_ldx_flags()
{
    flags_zero = X == 0;
    flags_negative = X & 0x80;
}

void CPU::update_ldy_flags()
{
    flags_zero = Y == 0;
    flags_negative = Y & 0x80;
}

void CPU::update_lsr_flags(unsigned char orig, unsigned char result)
{
    flags_carry = orig & 0x1;
    flags_zero = result == 0;
    flags_negative = result & 0x80;
}

void CPU::update_ora_flags()
{
    flags_zero = A == 0;
    flags_negative = A & 0x80;
}

void CPU::update_rol_flags(unsigned char orig, unsigned char result)
{
    flags_carry = orig & 0x80;
    flags_zero = A == 0;
    flags_negative = result & 0x80;
}

void CPU::update_ror_flags(unsigned char orig, unsigned char result)
{
    flags_carry = orig & 0x1;
    flags_zero = A == 0;
    flags_negative = result & 0x80;
}

void CPU::do_cycle()
{
    clocks_remain--;
    cycle++;

    if(NMI && clocks_remain < 0)
    {
        push(PC >> 8);
        push(PC & 0xff);
        std::bitset<8> status{};
        status[0] = flags_carry;
        status[1] = flags_zero;
        status[2] = flags_int_disable;
        status[3] = flags_dec_mode;
        status[4] = 0;
        status[5] = 1;
        status[6] = flags_overflow;
        status[7] = flags_negative;
        push((unsigned char)status.to_ulong());
        flags_int_disable = 1;
        PC = (read_memory(0xfffb) << 8) + read_memory(0xfffa);
        NMI = false;
        ppu->NMI_occurred = false;
    }
    if(oam_write_pending && clocks_remain > 0)
        return;
    else if(oam_write_pending && clocks_remain == 0)
    {
        unsigned char buffer[256];
        read_memory_chunk((ppu->OAMDMA << 8) & 0xff00, 256, buffer);
        std::copy(std::begin(buffer), std::end(buffer), std::begin(ppu->OAM));
        oam_write_pending = false;
        return;
    }
    auto opcode = read_memory(PC);
    //dump_registers();
    switch(opcode)
    {
        case 0xA9: // LDA IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                A = read_memory(PC + 1);
                PC += 2;
                update_lda_flags();
            }
            break;
        case 0xA5: // LDA ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                A = read_memory(read_memory(PC + 1));
                PC += 2;
                update_lda_flags();
            }
            
            break;
        case 0xB5: // LDA ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                A = read_memory((read_memory(PC+1) + X) % 256);
                PC += 2;
                update_lda_flags();
            }
            break;
        case 0xAD: // LDA ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address();
                A = read_memory(address);
                PC += 3;
                update_lda_flags();
            }
            break;
        case 0xBD: // LDA ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if((absolute/256) < (absolute+X)/256) // Crossed a page boundary
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                A = read_memory(address);
                PC += 3;
                update_lda_flags();
            }
            break;
        case 0xB9: // LDA ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if((absolute/256) < (absolute+Y)/256) // Crossed a page boundary
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + Y;
                A = read_memory(address);
                PC += 3;
                update_lda_flags();
            }
            break;
        case 0xA1: // LDA (INDIRECT,X)
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_indirect_x_address();
                A = read_memory(address);
                PC += 2;
                update_lda_flags();
            }
            break;
        case 0xB1: // LDA (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short address = get_address_from_zp();
                if(address/256 < (address + Y)/256) // Page boundary crossed
                    clocks_remain = 5; 
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain == 0)
            {
                unsigned short zp_address = get_address_from_zp() + Y;
                //unsigned short address = (read_memory(zp_address + 1) << 8) + (read_memory(zp_address)); // Address _before_ Y is added
                A = read_memory(zp_address);
                PC += 2;
                update_lda_flags();
            }
            break;
        case 0x69: // ADC IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                // From http://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
                unsigned int result = A + read_memory(PC + 1) + flags_carry;
                unsigned char arg = read_memory(PC + 1);                    
                update_adc_flags(arg, result);
                A = result & 0xff;     
                PC += 2;
            }
            break;
        case 0x65: // ADC ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char arg = read_memory(read_memory(PC + 1));
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);
                A = result % 256;   
                PC += 2;
                
            }
            break;
        case 0x75: // ADC ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char arg = read_memory((read_memory(PC+1) + X) % 256);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);
                A = result % 256;
                PC += 2;

            }
            break;
        case 0x6D: // ADC ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address();
                unsigned char arg = read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);
                A = result % 256;               
                PC += 3;
            }
            break;
        case 0x7D: // ADC ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if((absolute/256) < (absolute+X)/256) // Crossed a page boundary
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                unsigned char arg = read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);
                A = result % 256;
                PC += 3;
            }
            break;
        case 0x79: // ADC ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if((absolute/256) < (absolute+Y)/256) // Crossed a page boundary
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + Y;
                unsigned char arg = read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);
                A = result % 256;              
                PC += 3;
            }
            break;
        case 0x61: // ADC (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_indirect_x_address();
                unsigned char arg = read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);   
                A = result % 256;             
                PC += 2;
            }
            break;
        case 0x71: // ADC (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short address = get_address_from_zp(); // _before_ Y added
                if(address/256 < (address + Y)/256) // Page boundary crossed
                    clocks_remain = 5; 
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_address_from_zp() + Y;
                unsigned char arg = read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);   
                A = result % 256;      
                PC += 2;
            }
            break;
        case 0x29: // AND IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                A = A & read_memory(PC + 1);
                PC += 2;
                update_and_flags();
            }
            break;
        case 0x25: // AND ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                A = A & read_memory(read_memory(PC + 1));
                PC += 2;
                update_and_flags();
            }
            break;
        case 0x35: // AND ZERO PAGE, 0x
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                A = A & read_memory((read_memory(PC+1) + X) % 256);
                PC += 2;
                update_and_flags();
            }
            break;
        case 0x2D: // AND ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address();
                A = A & read_memory(address);
                PC += 3;
                update_and_flags();
            }
            break;
        case 0x3D: // AND ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute+X)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                A  = A & read_memory(address);
                PC += 3;
                update_and_flags();
            }
            break;
        case 0x39: // AND ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute+Y)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + Y;
                A = A & read_memory(address);
                PC += 3;
                update_and_flags();
            }
            break;
        case 0x21: // AND (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_indirect_x_address();
                A = A & read_memory(address);
                PC += 2;
                update_and_flags();
            }
            break;
        case 0x31: // AND (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_address_from_zp();
                if(absolute/256 < (absolute+Y)/256) // Page boundary crossed
                    clocks_remain = 5;
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain==0)
            {
                unsigned short address = get_address_from_zp() + Y;
                A = A & read_memory(address);
                PC += 2;
                update_and_flags();                        
            }
            break;
        case 0x0A: // ASL ACCUMULATOR
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                unsigned int result = A << 1;   
                unsigned char orig = A;
                A = result % 256;         
                update_asl_flags(orig, result);    
                PC += 1;

            }
            break;
        case 0x06: // ASL ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned short address = read_memory(PC+1);
                unsigned char result = read_memory(address) << 1;
                update_asl_flags(read_memory(address), result);                
                write_memory(address, result);
                PC += 2;

            }
            break;
        case 0x16: // ASL ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = (read_memory(PC + 1) + X) % 256;
                unsigned int result = read_memory(address) << 1;
                update_asl_flags(read_memory(address), result);                
                write_memory(address, result);
                PC += 2;
            }
            break;
        case 0x0E: // ASL ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address();
                unsigned int result = read_memory(address) << 1;
                update_asl_flags(read_memory(address), result);                
                write_memory(address, result);
                PC += 3;
            }
            break;
        case 0x1E: // ASL ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 6;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                unsigned int result = read_memory(address) << 1;
                update_asl_flags(read_memory(address), result);                
                write_memory(address, result);
                PC += 3;
            }
            break;
        case 0x90: // BCC
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_carry == 0;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_carry == 0)
                    PC += (signed char)read_memory(PC + 1)+2;
                else
                    PC += 2;
            }
            break;
        case 0xB0: // BCS
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_carry == 1;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_carry == 1)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0xF0: // BEQ
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_zero == 1;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_zero == 1)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0x24: // BIT ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain==0)
            {
                unsigned short val = read_memory(read_memory(PC+1));
                unsigned char result = A & val;
                flags_zero = result == 0;
                flags_overflow = val & 0x40;
                flags_negative = val & 0x80;
                PC += 2;
            }
            break;
        case 0x2C: // BIT ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain ==0)
            {
                unsigned short val = read_memory(get_absolute_address());
                unsigned char result = A & val;
                flags_zero = result == 0;
                flags_overflow = val & 0x40;
                flags_negative = val & 0x80;
                PC += 3;
            }
            break;
        case 0x30: // BMI
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_negative == 1;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_negative == 1)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0xD0: // BNE
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_zero == 0;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_zero == 0)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0x10: // BPL
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_negative == 0;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_negative == 0)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0x00: // BRK
            if(clocks_remain < 0)
                clocks_remain = 6;
            else if(clocks_remain == 0)
            {
                push((PC+2) >> 8);
                push((PC+2) & 0xff);
                std::bitset<8> status{};
                status[0] = flags_carry;
                status[1] = flags_zero;
                status[2] = flags_int_disable;
                status[3] = flags_dec_mode;
                status[4] = 1;
                status[5] = 1;
                status[6] = flags_overflow;
                status[7] = flags_negative;
                push((unsigned char)status.to_ulong());
                flags_int_disable = 1;
                PC = (read_memory(0xffff) << 8) + read_memory(0xfffe);
            }
            break;
        case 0x50: // BVC
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_overflow == 0;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;
            }
            else if(clocks_remain == 0)
            {
                if(flags_overflow == 0)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0x70: // BVS
            if(clocks_remain < 0)
            {
                // Not positive on the timing here
                clocks_remain = 1;
                unsigned short branch_addr = PC + (signed char)read_memory(PC + 1);
                bool branch_succeed = flags_overflow == 1;
                if(branch_succeed)
                    clocks_remain += 1;
                if(PC/256 < branch_addr/256 && branch_succeed) // Page boundary crossed
                    clocks_remain += 2;

            }
            else if(clocks_remain == 0)
            {
                if(flags_overflow == 1)
                    PC += (signed char)read_memory(PC + 1) + 2;
                else
                    PC += 2;
            }
            break;
        case 0x18: // CLC
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_carry = 0;
                PC += 1;
            }
            break;
        case 0xD8:
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_dec_mode = 0;
                PC += 1;
            }
            break;
        case 0x58: // CLI
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_int_disable = 0;
                PC += 1;
//                std::cout << "CLI" << std::endl;
            }
            break;
        case 0xB8: // CLI
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_overflow = 0;
                PC += 1;
            }
            break;
        case 0xC9: // CMP IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(PC + 1));
                PC += 2;    
            }
            break;
        case 0xC5: // CMP ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(read_memory(PC + 1)));
                PC += 2;
            }
            break;
        case 0xD5: // CMP ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned short address = (read_memory(PC + 1) + X) % 256;
                update_cmp_flags(read_memory(address));
                PC += 2;
            }
            break;
        case 0xCD: // CMP ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(get_absolute_address()));
                PC += 3;
            }
            break;
        case 0xDD: // CMP ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + X)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(get_absolute_address() + X));
                PC += 3;
            }
            break;
        case 0xD9: // CMP ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + Y)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(get_absolute_address() + Y));
                PC += 3;
            }
            break;
        case 0xC1: // CMP (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(get_indirect_x_address()));
                PC += 2;
            }
            break;
        case 0xD1: // CMP (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_address_from_zp();
                if(absolute/256 < (absolute + Y)/256) // Page boundary crossed
                    clocks_remain = 5;
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain == 0)
            {
                update_cmp_flags(read_memory(get_address_from_zp() + Y));
                PC += 2;
            }
            break;
        case 0xE0: // CPX  IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                update_cpx_flags(read_memory(PC + 1));
                PC += 2;
            }
            break;
        case 0xE4: // CPX  ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                update_cpx_flags(read_memory(read_memory(PC + 1)));
                PC += 2;

            }
            break;
        case 0xEC: // CPX  ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                update_cpx_flags(read_memory(get_absolute_address()));
                PC += 3;
            }
            break;
        case 0xC0: // CPY IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                update_cpy_flags(read_memory(PC + 1));
                PC += 2;
            }
            break;
        case 0xC4: // CPY ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                update_cpy_flags(read_memory(read_memory(PC + 1)));
                PC += 2;

            }
            break;
        case 0xCC: // CPY ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                update_cpy_flags(read_memory(get_absolute_address()));
                PC += 3;
            }
            break;
        case 0xC6: // DEC ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned short address = read_memory(PC + 1);
                unsigned char result = read_memory(address) - 1;
                write_memory(address, result);
                PC += 2;
                update_dec_flags(result);
                
            }
            break;
        case 0xD6: // DEC ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = (read_memory(PC + 1) + X) % 256;
                unsigned char result = read_memory(address) - 1;
                write_memory(address, result);
                PC += 2;
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
                PC += 3;
                update_dec_flags(result);
            }
            break;
        case 0xDE: // DEC ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 6;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X; 
                unsigned char result = read_memory(address) - 1;
                write_memory(address, result);
                PC += 3;
                update_dec_flags(result);
            }
            break;
        case 0xCA: // DEX 
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                unsigned char result = X - 1;
                X = result;
                PC += 1;
                flags_zero = result == 0;
                flags_negative = result & 0x80;
            }
            break;
        case 0x88: // DEY
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                unsigned char result = Y - 1;
                Y = result;
                PC += 1;
                flags_zero = result == 0;
                flags_negative = result & 0x80;
            }
            break;
        case 0x49: // EOR IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                A = A ^ read_memory(PC + 1);
                PC += 2;
                update_eor_flags();
            }
            break;
        case 0x45: // EOR ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                A = A ^ read_memory(read_memory(PC + 1));
                PC += 2;
                update_eor_flags();
            }
            break;
        case 0x55: // EOR ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char address = (read_memory(PC + 1) + X) % 256;
                A = A ^ read_memory(address);
                PC += 2;
                update_eor_flags();
            }
            break;
        case 0x4D: // EOR ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                A = A ^ read_memory(get_absolute_address());
                PC += 3;
                update_eor_flags();
            }
            break;
        case 0x5D: // EOR ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + X)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                A = A ^ read_memory(address);
                PC += 3;
                update_eor_flags();
            }
            break;
        case 0x59: // EOR ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + Y)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + Y;
                A = A ^ read_memory(address);
                PC += 3;
                update_eor_flags();
            }
            break;
        case 0x41: //EOR (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                A = A ^ read_memory(get_indirect_x_address());
                PC += 2;
                update_eor_flags();
            }
            break;
        case 0x51: // EOR (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short zp_addr = get_address_from_zp();
                if(zp_addr/256 < (zp_addr + Y)/256)
                    clocks_remain = 5;
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain == 0)
            {
                A = A ^ read_memory(get_address_from_zp() + Y);
                PC += 2;
                update_eor_flags();
            }
            break;
        case 0xE6: // INC ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned short addr = read_memory(PC+1);
                unsigned char val = read_memory(addr) + 1;
                write_memory(addr, val);
                PC += 2;
                update_inc_flags(val);
            }
            break;
        case 0xF6: // INC ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = (read_memory(PC+1) + X) % 256;
                unsigned char val = read_memory(addr) + 1;
                write_memory(addr, val);
                PC += 2;
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
                PC += 3;
                update_inc_flags(val);
            }
            break;
        case 0xFE: // INC ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address() + X;
                unsigned char val = read_memory(addr) + 1;
                write_memory(addr, val);
                PC += 3;
                update_inc_flags(val);
            }
            break;
        case 0xE8: // INX 
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                X += 1;
                PC += 1;
                flags_zero = X == 0;
                flags_negative = X & 0x80;
            }
            break;
        case 0xC8: // INY
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                Y += 1;
                PC += 1;
                flags_zero = Y == 0;
                flags_negative = Y & 0x80;
            }
            break;
        case 0x4C: // JMP ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                PC = get_absolute_address();
            }
            break;
        case 0x6C: // JMP INDIRECT 
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address();
                unsigned short indir_addr_lsb = addr;
                unsigned short indir_addr_msb;
                // BREAK THIS AS DESCRIBED AT http://obelisk.me.uk/6502/reference.html#JMP
                if((indir_addr_lsb & 0xFF) == 0xFF)
                    indir_addr_msb = (indir_addr_lsb & 0xFF00);
                else
                    indir_addr_msb = addr+1;
                PC = (read_memory(indir_addr_msb) << 8) + (read_memory(indir_addr_lsb));
            }
            break;
        case 0x20: // JSR ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short return_addr = PC + 2;
                push((return_addr >> 8) & 0xff); // Push high byte of return address
                push(return_addr & 0xff); // Push low byte of return address
                PC = get_absolute_address();
            }
            break;
        case 0xA2: // LDX  IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                X = read_memory(PC + 1);
                PC += 2;
                update_ldx_flags();
            }
            break;
        case 0xA6: // LDX  ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                X = read_memory(read_memory(PC + 1));
                PC += 2;
                update_ldx_flags();
            }
            break;
        case 0xB6: // LDX  ZERO PAGE, Y
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + Y) % 256;
                X = read_memory(addr);
                PC += 2;
                update_ldx_flags();
            }
            break;
        case 0xAE: // LDX  ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                X = read_memory(get_absolute_address());
                PC += 3;
                update_ldx_flags();
            }
            break;
        case 0xBE: // LDX  ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + Y)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                X = read_memory(get_absolute_address() + Y);
                PC += 3;
                update_ldx_flags();
            }
            break;
        case 0xA0: // LDY IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                Y = read_memory(PC + 1);
                PC += 2;
                update_ldy_flags();
            }
            break;
        case 0xA4: // LDY ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                Y = read_memory(read_memory(PC + 1));
                PC += 2;
                update_ldy_flags();
            }
            break;
        case 0xB4: // LDY ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + X) % 256;
                Y = read_memory(addr);
                PC += 2;
                update_ldy_flags();
            }
            break;
        case 0xAC: // LDY ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                Y = read_memory(get_absolute_address());
                PC += 3;
                update_ldy_flags();
            }
            break;
        case 0xBC: // LDY ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + X)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                Y = read_memory(get_absolute_address() + X);
                PC += 3;
                update_ldy_flags();
            }
            break;
        case 0x4A: // LSR ACCUMULATOR
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                unsigned char result = A >> 1;
                unsigned char orig = A;
                A = result;
                PC += 1;
                update_lsr_flags(orig, result);
            }
            break;
        case 0x46: // LSR ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned char addr = read_memory(PC + 1);
                unsigned char orig = read_memory(addr);
                unsigned char result = orig >> 1;
                write_memory(addr, result);
                PC += 2;
                update_lsr_flags(orig, result);
            }
            break;
        case 0x56: // LSR ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + X) % 256;
                unsigned char orig = read_memory(addr);
                unsigned char result = orig >> 1;
                write_memory(addr, result);
                PC += 2;
                update_lsr_flags(orig, result);
            }
            break;
        case 0x4E: // LSR ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address();
                unsigned char orig = read_memory(addr);
                unsigned char result = orig >> 1;
                write_memory(addr, result);
                PC += 3;
                update_lsr_flags(orig, result);
            }
            break;
        case 0x5E: // LSR ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 6;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address() + X;
                unsigned char orig = read_memory(addr);
                unsigned char result = orig >> 1;
                write_memory(addr, result);
                PC += 3;
                update_lsr_flags(orig, result);
            }
            break;
        case 0xEA: // NOP
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
                PC += 1;
            break;
        case 0x09: // ORA IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                A = A | read_memory(PC + 1);
                PC += 2;
                update_ora_flags();
            }
            break;
        case 0x05: // ORA ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                A = A | read_memory(read_memory(PC + 1));
                PC += 2;
                update_ora_flags();
            }
            break;
        case 0x15: // ORA ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char address = (read_memory(PC + 1) + X) % 256;
                A = A | read_memory(address);
                PC += 2;
                update_ora_flags();
            }
            break;
        case 0x0D: // ORA ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                A = A | read_memory(get_absolute_address());
                PC += 3;
                update_ora_flags();
            }
            break;
        case 0x1D: // ORA ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + X)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                A = A | read_memory(address);
                PC += 3;
                update_ora_flags();
            }
            break;
        case 0x19: // ORA ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if(absolute/256 < (absolute + Y)/256) // Page boundary crossed
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + Y;
                A = A | read_memory(address);
                PC += 3;
                update_ora_flags();
            }
            break;
        case 0x01: //ORA (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                A = A | read_memory(get_indirect_x_address());
                PC += 2;
                update_ora_flags();
            }
            break;
        case 0x11: // ORA (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short zp_addr = get_address_from_zp();
                if(zp_addr/256 < (zp_addr + Y)/256)
                    clocks_remain = 5;
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain == 0)
            {
                A = A | read_memory(get_address_from_zp() + Y);
                PC += 2;
                update_ora_flags();
            }
            break;
        case 0x48: // PHA
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                push(A);
                PC += 1;
            }
            break;
        case 0x08: // PHP
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                std::bitset<8> status{};
                status[0] = flags_carry;
                status[1] = flags_zero;
                status[2] = flags_int_disable;
                status[3] = flags_dec_mode;
                status[4] = 1;
                status[5] = 1;
                status[6] = flags_overflow;
                status[7] = flags_negative;
                push((unsigned char)status.to_ulong());
                PC += 1;
            }
            break;
        case 0x68: // PLA
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                A = pull();
                flags_zero = A == 0;
                flags_negative = (A & 0x80);
                PC += 1;
            }
            break;
        case 0x28: // PLP
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                std::bitset<8> status(pull());
                flags_carry = status[0];
                flags_zero = status[1];
                flags_int_disable = status[2];
                flags_dec_mode = status[3];
                flags_break = status[4];
                flags_overflow = status[6];
                flags_negative = status[7];
                PC += 1;
            }
            break;
        case 0x2A: // ROL ACCUMULATOR
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                unsigned char orig = A;
                unsigned char result = ((orig << 1) | flags_carry) & 0xff;
                A = result;
                PC += 1;
                update_rol_flags(orig, result);
            }
            break;
        case 0x26: // ROL ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned char addr = read_memory(PC + 1);
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig << 1) | flags_carry;
                write_memory(addr, result);
                PC += 2;
                update_rol_flags(orig, result);
            }
            break;
        case 0x36: // ROL ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + X) % 256;
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig << 1) | flags_carry;
                write_memory(addr, result);
                PC += 2;
                update_rol_flags(orig, result);
            }
            break;
        case 0x2E: // ROL ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address();
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig << 1) | flags_carry;
                write_memory(addr, result);
                PC += 3;
                update_rol_flags(orig, result);
            }
            break;
        case 0x3E: // ROL ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address() + X;
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig << 1) | flags_carry;
                write_memory(addr, result);
                PC += 3;
                update_rol_flags(orig, result);
            }
            break;
        case 0x6A: // ROR ACCUMULATOR
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                unsigned char orig = A;
                unsigned char result = (orig >> 1) | ((flags_carry << 7) & 0x80);
                A = result;
                PC += 1;
                update_ror_flags(orig, result);
            }
            break;
        case 0x66: // ROR ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                unsigned char addr = read_memory(PC + 1);
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig >> 1) | ((flags_carry << 7) & 0x80);
                write_memory(addr, result);
                PC += 2;
                update_ror_flags(orig, result);
            }
            break;
        case 0x76: // ROR ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + X) % 256;
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig >> 1) | ((flags_carry << 7) & 0x80);
                write_memory(addr, result);
                PC += 2;
                update_ror_flags(orig, result);
            }
            break;
        case 0x6E: // ROR ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address();
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig >> 1) | ((flags_carry << 7) & 0x80);
                //std::cout << "[ROR] READING FROM 0x" << std::hex << addr << ": " << orig << " BECOMES " << result << std::dec << std::endl;
                write_memory(addr, result);
                PC += 3;
                update_ror_flags(orig, result);
            }
            break;
        case 0x7E: // ROR ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short addr = get_absolute_address() + X;
                unsigned char orig = read_memory(addr);
                unsigned char result = (orig >> 1) | ((flags_carry << 7) & 0x80);
                write_memory(addr, result);
                PC += 3;
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
                PC = ((unsigned short)return_high << 8) + return_low;
                flags_carry = status[0];
                flags_zero = status[1];
                flags_int_disable = status[2];
                flags_dec_mode = status[3];
                flags_overflow = status[6];
                flags_negative = status[7];
                //std::cout << "[CPU] RETURNING FROM INTERRUPT: JUMPING TO 0x" << std::hex << PC << std::dec << std::endl;
            }
            break;
        case 0x60: // RTS
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned char return_low = pull();
                unsigned char return_high = pull();
                PC = ((unsigned short)return_high << 8) + return_low + 1;
            }
            break;
        case 0xE9: // SBC IMMEDIATE
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                // From http://stackoverflow.com/questions/29193303/6502-emulation-proper-way-to-implement-adc-and-sbc
                unsigned char arg = ~read_memory(PC + 1);                   
                unsigned short result = A + arg + (unsigned short)flags_carry;
                update_adc_flags(arg, result);                  
                A = result % 256;
                PC += 2;
            }
            break;
        case 0xE5: // SBC ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char arg = ~read_memory(read_memory(PC + 1));
                
                unsigned int result = (unsigned short)A + (unsigned short)arg + (unsigned short)flags_carry;
                update_adc_flags(arg, result);   
                A = result % 256;               
                PC += 2;
            }
            break;
        case 0xF5: // SBC ZERO PAGE, X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char arg = ~read_memory((read_memory(PC+1) + X) % 256);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);   
                A = result % 256;            
                PC += 2;
            }
            break;
        case 0xED: // SBC ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address();
                unsigned char arg = ~read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);   
                A = result % 256;             
                PC += 3;
            }
            break;
        case 0xFD: // SBC ABSOLUTE,X 
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if((absolute/256) < (absolute+X)/256) // Crossed a page boundary
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + X;
                unsigned char arg = ~read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);  
                A = result % 256;              
                PC += 3;
            }
            break;
        case 0xF9: // SBC ABSOLUTE,Y
            if(clocks_remain < 0)
            {
                unsigned short absolute = get_absolute_address();
                if((absolute/256) < (absolute+Y)/256) // Crossed a page boundary
                    clocks_remain = 4;
                else
                    clocks_remain = 3;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_absolute_address() + Y;
                unsigned char arg = ~read_memory(address);
                unsigned int result = A + arg + flags_carry;
                //std::cout << "[SBC] READING FROM " << std::hex << address << ": GOT " << ~arg << std::dec << std::endl;
                update_adc_flags(arg, result);                             
                A = result % 256;
                PC += 3;
            }
            break;
        case 0xE1: // SBC (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                unsigned short address = get_indirect_x_address();
                unsigned char arg = ~read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);                  
                A = result % 256;               
                PC += 2;
            }
            break;
        case 0xF1: // SBC (INDIRECT),Y
            if(clocks_remain < 0)
            {
                unsigned short address = get_address_from_zp(); // _before_ Y added
                if(address/256 < (address + Y)/256) // Page boundary crossed
                    clocks_remain = 5; 
                else
                    clocks_remain = 4;
            }
            else if(clocks_remain == 0)
            {
                unsigned short address = get_address_from_zp() + Y;
                unsigned char arg = ~read_memory(address);
                unsigned int result = A + arg + flags_carry;
                update_adc_flags(arg, result);                  
                A = result % 256;              
                PC += 2;
            }
            break;
        case 0x38: // SEC
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_carry = 1;
                PC += 1;
            }
            break;
        case 0xF8: // SED
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_dec_mode = 1;
                PC += 1;
            }
            break;
        case 0x78: // SEI
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                flags_int_disable = 1;
                PC += 1;
            }
            break;
        case 0x85: // STA ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char addr = read_memory(PC + 1);
                write_memory(addr, A);
                PC += 2;
            }
            break;
        case 0x95: // STA ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + X) % 256;
                write_memory(addr, A);
                PC += 2;
            }
            break;
        case 0x8D: // STA ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                write_memory(get_absolute_address(), A);
                PC += 3;
            }
            break;
        case 0x9D: // STA ABSOLUTE,X 
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                write_memory(get_absolute_address() + X, A);
                PC += 3;
            }
            break;
        case 0x99: // STA ABSOLUTE,Y
            if(clocks_remain < 0)
                clocks_remain = 4;
            else if(clocks_remain == 0)
            {
                write_memory(get_absolute_address() + Y, A);
                PC += 3;
            }
            break;
        case 0x81: // STA (INDIRECT,X )
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                write_memory(get_indirect_x_address(), A);
                PC += 2;
            }
            break;
        case 0x91: // STA (INDIRECT),Y
            if(clocks_remain < 0)
                clocks_remain = 5;
            else if(clocks_remain == 0)
            {
                write_memory(get_address_from_zp() + Y, A);
                PC += 2;
            }
            break;
        case 0x86: // STX , ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char addr = read_memory(PC + 1);
                write_memory(addr, X);
                PC += 2;
            }
            break;
        case 0x96: // STX , ZERO PAGE,Y
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + Y) % 256;
                write_memory(addr, X);
                PC += 2;
            }
            break;
        case 0x8E: // STX  ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                write_memory(get_absolute_address(), X);
                PC += 3;
            }
            break;
        case 0x84: // STY ZERO PAGE
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char addr = read_memory(PC + 1);
                write_memory(addr, Y);
                PC += 2;
            }
            break;
        case 0x94: // STY ZERO PAGE,X 
            if(clocks_remain < 0)
                clocks_remain = 2;
            else if(clocks_remain == 0)
            {
                unsigned char addr = (read_memory(PC + 1) + X) % 256;
                write_memory(addr, Y);
                PC += 2;
            }
            break;
        case 0x8C: // STY ABSOLUTE
            if(clocks_remain < 0)
                clocks_remain = 3;
            else if(clocks_remain == 0)
            {
                write_memory(get_absolute_address(), Y);
                PC += 3;
            }
            break;
        case 0xAA: // TAX 
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                X = A;
                flags_zero = X == 0;
                flags_negative = X & 0x80;
                PC += 1;
            }
            break;
        case 0xA8: // TAY
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                Y = A;
                flags_zero = Y == 0;
                flags_negative = Y & 0x80;
                PC += 1;
            }
            break;
        case 0xBA: // TSX 
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                X = S;
                flags_zero = X == 0;
                flags_negative = X & 0x80;
                PC += 1;
            }
            break;
        case 0x8A: // TXA
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                A = X;
                flags_zero = A == 0;
                flags_negative = A & 0x80;
                PC += 1;
            }
            break;
        case 0x9A: // TXS
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                S = 0x100 + (unsigned char)X;
                PC += 1;
            }
            break;
        case 0x98: // TYA
            if(clocks_remain < 0)
                clocks_remain = 1;
            else if(clocks_remain == 0)
            {
                A = Y;
                flags_zero = A == 0;
                flags_negative = A & 0x80;
                PC += 1;
            }
            break;
        default:
            PC += 1;
    }
}

void CPU::push(unsigned char val)
{
    write_memory(S, val);
    S--;
    return;
}

unsigned char CPU::pull()
{
    S++;
    unsigned char val = read_memory(S);
    return val;
}

unsigned short CPU::get_indirect_x_address()
{
    unsigned short zp_address = (read_memory(PC + 1) + X) % 256; // Address stored in zero page to be read from
    return (read_memory((zp_address + 1) % 256) << 8) + (read_memory(zp_address));              
}

unsigned short CPU::get_address_from_zp() // For (INDIRECT),Y addressing
{
    unsigned short zp_address = read_memory(PC + 1);
    return (read_memory((zp_address + 1) % 256) << 8) + (read_memory(zp_address)); 
}

unsigned short CPU::get_absolute_address()
{
    return (read_memory(PC + 2) << 8) + (read_memory(PC + 1));
}

void CPU::read_memory_chunk(unsigned short addr, unsigned short length, unsigned char *buffer)
{
    for(int i = 0; i < length; i++)
        buffer[i] = read_memory(addr + i);
}

unsigned char CPU::read_memory(unsigned short address)
{
    unsigned char ret;
    switch(address)
    {
        case 0x2002: // PPUSTATUS
            ret = ppu->PPUSTATUS;
            //std::cout << "READING PPUSTATUS: 0x" << std::hex << (short)ret << std::dec << std::endl;
            ppu->PPUSTATUS &= ~(1 << 7); // Clear vblank on read
            ppu->NMI_occurred = false;
            ppu->addr_scroll_latch = false;
            break;
        case 0x2004: // OAMDATA
            ret = ppu->OAMDATA; // For now this has no side-effects
            //std::cout << "READING OAMDATA 0x" << std::hex << (short)ret << std::dec << std::endl;
            break;
        case 0x2007: // PPUDATA
            ret = ppu->read_data();
            //std::cout << "READING PPUDATA 0x" << std::hex << (short)ret << std::dec << std::endl;
            break;
        case 0x4016: // JOYPAD1
            if(!controller_strobe)
            {
                switch(controller_read_count)
                {
                    case 0:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Z);
                        break;
                    case 1:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::X);
                        break;
                    case 2:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
                        break;
                    case 3:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Return);
                        break;
                    case 4:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
                        break;
                    case 5:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
                        break;
                    case 6:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
                        break;
                    case 7:
                        ret = 0x40 | sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
                        break;
                }
                controller_read_count += 1;
                controller_read_count = controller_read_count % 8;
            }
            else
                controller_read_count = 0;
            break;
        default: 
            ret = int_memory[address];
    }
    return ret;
}

void CPU::write_memory(unsigned short address, unsigned char value)
{
    switch(address)
    {
        case 0x2000: // PPUCTRL
            //std::cout << "WRITING TO PPUCTRL: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            ppu->PPUCTRL = value;
            ppu->NMI_output = std::bitset<8>(ppu->PPUCTRL)[7];
            ppu->vram_addr_temp &= ~0xC00;
            ppu->vram_addr_temp |= (value & 0x3) << 10;
            break;
        case 0x2001: // PPUMASK
            //std::cout << "WRITING TO PPUMASK: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            ppu->PPUMASK = value;
            break;
        case 0x2003: // OAMADDR
            //std::cout << "WRITING TO OAMADDR: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            ppu->OAMADDR = value;
            break;
        case 0x2004: // OAMDATA
            //std::cout << "WRITING TO OAMDATA: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            ppu->write_oam(value);
            break;
        case 0x2005: // PPUSCROLL
            //std::cout << "WRITING TO PPUSCROLL: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            // TODO IMPLEMENT PROPERLY
            ppu->write_ppuscroll(value);
            break;
        case 0x2006: // PPUADDR
            //std::cout << "WRITING TO PPUADDR: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            ppu->update_addr(value);
            break;
        case 0x2007: // PPUDATA
            //std::cout << "WRITING TO PPUDATA: 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            //std::cout << "CURRENT PPU VRAM ADDR: 0x" << std::hex << ppu->vram_addr << std::dec << std::endl;
            ppu->write_data(value);
            break;
        case 0x4014: // OAMDMA
            //std::cout << "WRITING TO OAMDMA 0x" << std::hex << (unsigned short)value << std::dec << std::endl;
            oam_write_pending = true;
            ppu->OAMDMA = value;
            clocks_remain = 513;
            break;
        case 0x4016: // JOYPAD1
            //std::cout << "WROTE JOYPAD1: 0x" << std::hex << (unsigned short) value << std::dec << std::endl;
            controller_strobe = value % 2 == 1;
            if(controller_strobe && cycle % 10 == 0)
                buttons_pressed = !buttons_pressed;
            break;
        default:
            //std::cout << "Writing at address 0x" << std::hex << address << ": 0x" << (int)value << std::dec << std::endl;
            int_memory[address] = value;       
    }

}

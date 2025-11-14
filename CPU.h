// File  : CPU.h
// Author: Cole Schwandt
//
// CPU fetches, decodes and executes machine code

#ifndef CPU_H
#define CPU_H

#include "RegisterFile.h"

class CPU
{
public:
    void execute(uint32_t word)
    {
        uint8_t opcode = (word >> 26) & mask_bits(6);
        
        switch (opcode)
        {
            case OP_RTYPE: // R-format
            {
                uint8_t rs = (word >> 21) & mask_bits(5);
                uint8_t rt = (word >> 16) & mask_bits(5);
                uint8_t rd = (word >> 11) & mask_bits(5);
                uint8_t shamt = (word >> 6) & mask_bits(5);
                uint8_t funct = word & mask_bits(6);

                switch (funct)
                {
                    case FUNCT_ADD:
                    {
                        // regs[rd] = regs[rs] + regs[rt];
                        int64_t t = regs.readS(rs) + regs.readS(rt);

                        if (t < INT32_MIN || t > INT32_MAX)
                        {
                            throw std::runtime_error("MIPS Integer Overflow on add");
                        }
                        else
                        {
                            regs.writeS(rd, t);
                        }

                        break;
                    }
                
                    case FUNCT_SUB:
                    {
                        // regs[rd] = regs[rs] - regs[rt];
                        int64_t t = regs.readS(rs) - regs.readS(rt);

                        if (t < INT32_MIN || t > INT32_MAX)
                        {
                            throw std::runtime_error("MIPS Integer Overflow on add");
                        }
                        else
                        {
                            regs.writeS(rd, t);
                        }
            
                        break;
                    }

                    case FUNCT_AND:
                        // regs[rd] = regs[rs] & regs[rt];
                        break;

                    case FUNCT_OR:
                        // regs[rd] = regs[rs] | regs[rt];
                        break;

                    case FUNCT_SLT:
                        // regs[rd] = (int32_t)regs[rs] < (int32_t)regs[rt];
                        break;

                    case FUNCT_SLL:
                        // regs[rd] = regs[rt] << shamt;
                        break;

                    case FUNCT_SRL:
                        // regs[rd] = (uint32_t)regs[rt] >> shamt;
                        break;

                    case FUNCT_SRA:
                        // regs[rd] = (int32_t)regs[rt] >> shamt;
                        break;
                }
            }
            break;
        }
    }
    
    // member variables
    //=======================
    RegisterFile regs;
    // Memory mem -> std::map<unsigned int, unsigned char>
    uint32_t pc;
};

#endif

// File  : CPU.h
// Author: Cole Schwandt
//
// CPU fetches, decodes and executes machine code

#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <stdexcept>
#include <limits>

#include "mybitlib.h"
#include "RegisterFile.h"
#include "Memory.h"
#include "Constants.h"

//==============================================================
// CPU
//==============================================================
class CPU
{
public:
    // CPU holds a reference to the machine's memory
    CPU(Memory & m)
        : regs(), mem(m), pc(TEXT_BASE)
    {}

    void reset()
    {
        regs.reset();
        pc = TEXT_BASE;
    }

    // one step of execution: fetch, bump PC, then execute
    void step()
    {
        uint32_t word = mem.load32(pc);
        pc += 4; // default PC increment
        execute(word);
    }

    void execute(uint32_t word)
    {
        uint8_t opcode = (word >> 26) & mask_bits(6);

        switch (opcode)
        {
            case OP_RTYPE: // R-format
            {
                uint8_t rs    = (word >> 21) & mask_bits(5);
                uint8_t rt    = (word >> 16) & mask_bits(5);
                uint8_t rd    = (word >> 11) & mask_bits(5);
                uint8_t shamt = (word >>  6) & mask_bits(5);
                uint8_t funct =  word        & mask_bits(6);

                switch (funct)
                {
                    case FUNCT_ADD:
                    {
                        int64_t a = static_cast<int64_t>(regs.readS(rs));
                        int64_t b = static_cast<int64_t>(regs.readS(rt));
                        int64_t t = a + b;

                        if (t < std::numeric_limits<int32_t>::min() ||
                            t > std::numeric_limits<int32_t>::max())
                        {
                            throw std::runtime_error("MIPS integer overflow on add");
                        }
                        regs.writeS(rd, static_cast<int32_t>(t));
                        break;
                    }

                    case FUNCT_SUB:
                    {
                        int64_t a = static_cast<int64_t>(regs.readS(rs));
                        int64_t b = static_cast<int64_t>(regs.readS(rt));
                        int64_t t = a - b;

                        if (t < std::numeric_limits<int32_t>::min() ||
                            t > std::numeric_limits<int32_t>::max())
                        {
                            throw std::runtime_error("MIPS integer overflow on sub");
                        }
                        regs.writeS(rd, static_cast<int32_t>(t));
                        break;
                    }

                    case FUNCT_AND:
                    {
                        uint32_t r = regs.readU(rs) & regs.readU(rt);
                        regs.writeU(rd, r);
                        break;
                    }

                    case FUNCT_OR:
                    {
                        uint32_t r = regs.readU(rs) | regs.readU(rt);
                        regs.writeU(rd, r);
                        break;
                    }

                    case FUNCT_SLT:
                    {
                        int32_t a = regs.readS(rs);
                        int32_t b = regs.readS(rt);
                        uint32_t r = (a < b) ? 1u : 0u;
                        regs.writeU(rd, r);
                        break;
                    }

                    case FUNCT_SLL:
                    {
                        // rd = rt << shamt (logical)
                        uint32_t r = regs.readU(rt) << shamt;
                        regs.writeU(rd, r);
                        break;
                    }

                    case FUNCT_SRL:
                    {
                        // rd = rt >> shamt (logical)
                        uint32_t r = regs.readU(rt) >> shamt;
                        regs.writeU(rd, r);
                        break;
                    }

                    case FUNCT_SRA:
                    {
                        // rd = rt >> shamt (arithmetic)
                        int32_t v = regs.readS(rt);
                        int32_t r = v >> shamt;
                        regs.writeS(rd, r);
                        break;
                    }

                    default:
                        throw std::runtime_error("Unknown R-type funct");
                }
                break;
            }

            // TODO: add I-format and J-format opcodes (addi, lw, sw, beq, j, etc.)

            default:
                throw std::runtime_error("Unknown opcode");
        }
    }

    // member variables
    RegisterFile regs;
    Memory &     mem;
    uint32_t     pc;
};

#endif // CPU_H


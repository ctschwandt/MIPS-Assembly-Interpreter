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
                //===========================
                // R3 arithmetic / logical
                //===========================

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

                case FUNCT_ADDU:
                {
                    uint64_t a = regs.readU(rs);
                    uint64_t b = regs.readU(rt);
                    uint64_t t = a + b;

                    if (t < std::numeric_limits<uint32_t>::min() ||
                        t > std::numeric_limits<uint32_t>::max())
                    {
                        throw std::runtime_error("MIPS integer overflow on addu");
                    }
                    regs.writeU(rd, static_cast<uint32_t>(t));
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

                case FUNCT_SUBU:
                {
                    uint64_t a = regs.readU(rs);
                    uint64_t b = regs.readU(rt);
                    uint64_t t = a - b;

                    if (t < std::numeric_limits<uint32_t>::min() ||
                        t > std::numeric_limits<uint32_t>::max())
                    {
                        throw std::runtime_error("MIPS integer overflow on subu");
                    }
                    regs.writeU(rd, static_cast<uint32_t>(t));
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

                case FUNCT_XOR:
                {
                    uint32_t r = regs.readU(rs) ^ regs.readU(rt);
                    regs.writeU(rd, r);
                    break;
                }

                case FUNCT_NOR:
                {
                    uint32_t r = ~(regs.readU(rs) | regs.readU(rt));
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

                case FUNCT_SLTU:
                {
                    uint32_t a = regs.readU(rs);
                    uint32_t b = regs.readU(rt);
                    uint32_t r = (a < b) ? 1u : 0u;
                    regs.writeU(rd, r);
                    break;
                }

                // seq: set equal (non-standard, but you have a funct for it)
                case FUNCT_SEQ:
                {
                    uint32_t a = regs.readU(rs);
                    uint32_t b = regs.readU(rt);
                    uint32_t r = (a == b) ? 1u : 0u;
                    regs.writeU(rd, r);
                    break;
                }

                //===========================
                // RSHIFT (rd, rt, shamt)
                //===========================

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

                //===========================
                // variable shifts (R3)
                //===========================

                // sllv: rd = rt << (rs & 0x1F)
                case FUNCT_SLLV:
                {
                    uint32_t sh = regs.readU(rs) & 0x1F;
                    uint32_t r  = regs.readU(rt) << sh;
                    regs.writeU(rd, r);
                    break;
                }

                // srlv: rd = rt >> (rs & 0x1F) (logical)
                case FUNCT_SRLV:
                {
                    uint32_t sh = regs.readU(rs) & 0x1F;
                    uint32_t r  = regs.readU(rt) >> sh;
                    regs.writeU(rd, r);
                    break;
                }

                // srav: rd = rt >> (rs & 0x1F) (arithmetic)
                case FUNCT_SRAV:
                {
                    uint32_t sh = regs.readU(rs) & 0x1F;
                    int32_t  v  = regs.readS(rt);
                    int32_t  r  = v >> sh;
                    regs.writeS(rd, r);
                    break;
                }

                //===========================
                // other R-type: mult/div, HI/LO moves
                //===========================

                // multiply/divide & hi/lo moves
                case FUNCT_MULT:
                {
                    // signed 32×32 → 64, hi = upper, lo = lower
                    int64_t a = static_cast<int64_t>(regs.readS(rs));
                    int64_t b = static_cast<int64_t>(regs.readS(rt));
                    int64_t p = a * b;

                    uint64_t up = static_cast<uint64_t>(p);
                    regs.write_hiU(static_cast<uint32_t>(up >> 32));
                    regs.write_loU(static_cast<uint32_t>(up & 0xFFFFFFFFu));
                    break;
                }

                case FUNCT_MULTU:
                {
                    uint64_t a = static_cast<uint64_t>(regs.readU(rs));
                    uint64_t b = static_cast<uint64_t>(regs.readU(rt));
                    uint64_t p = a * b;

                    regs.write_hiU(static_cast<uint32_t>(p >> 32));
                    regs.write_loU(static_cast<uint32_t>(p & 0xFFFFFFFFu));
                    break;
                }

                case FUNCT_DIV:
                {
                    int32_t a = regs.readS(rs);
                    int32_t b = regs.readS(rt);
                    if (b == 0)
                    {
                        throw std::runtime_error("MIPS divide by zero (div)");
                    }
                    // signed quotient / remainder in 2’s complement
                    regs.write_loS(a / b); // quotient
                    regs.write_hiS(a % b); // remainder
                    break;
                }

                case FUNCT_DIVU:
                {
                    uint32_t a = regs.readU(rs);
                    uint32_t b = regs.readU(rt);
                    if (b == 0)
                    {
                        throw std::runtime_error("MIPS divide by zero (divu)");
                    }
                    regs.write_loU(a / b); // quotient
                    regs.write_hiU(a % b); // remainder
                    break;
                }

                case FUNCT_MFHI:
                {
                    // rd = hi
                    regs.writeU(rd, regs.hiU());
                    break;
                }

                case FUNCT_MFLO:
                {
                    // rd = lo
                    regs.writeU(rd, regs.loU());
                    break;
                }

                case FUNCT_MTHI:
                {
                    // hi = rs
                    regs.write_hiU(regs.readU(rs));
                    break;
                }

                case FUNCT_MTLO:
                {
                    // lo = rs
                    regs.write_loU(regs.readU(rs));
                    break;
                }

                default:
                    throw std::runtime_error("Unknown R-type funct");
            }
            break;
        }

        case OP_ADDI: // addi rt, rs, imm  (signed, traps on overflow)
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

            int64_t a = static_cast<int64_t>(regs.readS(rs));
            int64_t t = a + imm;

            if (t < std::numeric_limits<int32_t>::min() ||
                t > std::numeric_limits<int32_t>::max())
            {
                throw std::runtime_error("MIPS integer overflow on addi");
            }

            regs.writeS(rt, static_cast<int32_t>(t));
            break;
        }

        case OP_ADDIU: // addiu rt, rs, imm  (non-trapping add)
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

            int32_t a = regs.readS(rs);
            int32_t r = a + imm;  // wraparound in 2's complement

            regs.writeS(rt, r);
            break;
        }

        case OP_ANDI: // andi rt, rs, imm  (zero-extended imm)
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            uint16_t imm = static_cast<uint16_t>(word & mask_bits(16)); // zero-extend

            uint32_t r = regs.readU(rs) & imm;
            regs.writeU(rt, r);
            break;
        }

        case OP_ORI: // ori rt, rs, imm
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            uint16_t imm = static_cast<uint16_t>(word & mask_bits(16));

            uint32_t r = regs.readU(rs) | imm;
            regs.writeU(rt, r);
            break;
        }

        case OP_XORI: // xori rt, rs, imm
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            uint16_t imm = static_cast<uint16_t>(word & mask_bits(16));

            uint32_t r = regs.readU(rs) ^ imm;
            regs.writeU(rt, r);
            break;
        }

        case OP_SLTI: // slti rt, rs, imm  (signed compare)
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

            int32_t a = regs.readS(rs);
            int32_t b = static_cast<int32_t>(imm);
            uint32_t r = (a < b) ? 1u : 0u;
            regs.writeU(rt, r);
            break;
        }

        case OP_SLTIU: // sltiu rt, rs, imm  (unsigned compare, sign-extended imm)
        {
            uint8_t rs = (word >> 21) & mask_bits(5);
            uint8_t rt = (word >> 16) & mask_bits(5);
            int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

            uint32_t a = regs.readU(rs);
            uint32_t b = static_cast<uint32_t>(static_cast<int32_t>(imm)); // same bit pattern
            uint32_t r = (a < b) ? 1u : 0u;
            regs.writeU(rt, r);
            break;
        }

        case OP_LUI: // lui rt, imm  (load upper immediate)
        {
            uint8_t rt = (word >> 16) & mask_bits(5);
            uint16_t imm = static_cast<uint16_t>(word & mask_bits(16));

            uint32_t r = static_cast<uint32_t>(imm) << 16;
            regs.writeU(rt, r);
            break;
        }

        default:
            throw std::runtime_error("Unknown opcode");
    }
}

    // member variables
    RegisterFile regs;
    Memory & mem;
    uint32_t pc;
};

#endif // CPU_H


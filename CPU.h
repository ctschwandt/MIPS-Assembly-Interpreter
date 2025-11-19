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
        : regs(), mem(m), pc(TEXT_BASE), halted(false)
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
                        // signed 32*32 -> 64, hi = upper, lo = lower
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

                    //===========================
                    // Jumps via register
                    //===========================
                    case FUNCT_JR: // jr rs
                    {
                        // set pc to rs
                        uint8_t rs  = (word >> 21) & mask_bits(5);
                        pc = regs.readU(rs);
                        break;
                    }

                    case FUNCT_JALR: // jalr rs
                    {
                        // set ra to pc
                        uint32_t ra = pc;
                        regs.writeU(31, ra);

                        uint8_t rs  = (word >> 21) & mask_bits(5);
                        pc = regs.readU(rs);
                        break;
                    }

                    case FUNCT_SYSCALL:
                    {
                        uint32_t service_code = regs.readU(2);
                        
                        switch(service_code)
                        {
                            //--------------------------------------
                            // 1: print integer ($a0)
                            //--------------------------------------
                            case 1:
                            {
                                std::cout << "executing sys1" << std::endl;
                                int32_t value = regs.readS(4); // $a0
                                std::cout << value;
                                break;
                            }

                            //--------------------------------------
                            // 4: print string ($a0 = addr)
                            //--------------------------------------
                            case 4:
                            {
                                uint32_t addr = regs.readU(4); // $a0

                                while (true)
                                {
                                    uint8_t byte = mem.load8(addr);
                                    if (byte == 0)    // null terminator
                                        break;
                                    std::cout << static_cast<char>(byte);
                                    ++addr;
                                }
                                break;
                            }

                            //--------------------------------------
                            // 5: read integer -> $v0
                            //--------------------------------------
                            case 5:
                            {
                                int32_t value;
                                std::cout << "CONSOLE INTEGER INPUT> ";
                                std::cin >> value;

                                if (std::cin.peek() == '\n')
                                    std::cin.get();
                                
                                regs.writeU(2, static_cast<uint32_t>(value)); // store into $v0
                                break;
                            }

                            //--------------------------------------
                            // 8: read string
                            //     $a0 = buffer address
                            //     $a1 = max chars
                            //--------------------------------------
                            case 8:
                            {
                                uint32_t buf_addr = regs.readU(4); // $a0
                                uint32_t max_len  = regs.readU(5); // $a1

                                std::cout << "CONSOLE STRING INPUT> ";

                                // Read a line from stdin.
                                // First flush leftover newline if needed.
                                if (std::cin.peek() == '\n')
                                    std::cin.get();

                                std::string line;
                                std::getline(std::cin, line);

                                // SPIM behavior: store at most max_len-1 chars,
                                // and null-terminate. (Optionally include '\n' if you want.)
                                if (max_len == 0)
                                    break; // nothing we can store

                                if (line.size() >= max_len)
                                    line.resize(max_len - 1);

                                // Copy into memory
                                uint32_t addr = buf_addr;
                                for (std::size_t i = 0; i < line.size(); ++i)
                                {
                                    mem.store8(addr++, static_cast<uint8_t>(line[i]));
                                }
                                // null terminator
                                mem.store8(addr, 0);
                                break;
                            }

                            //--------------------------------------
                            // 10: exit
                            //--------------------------------------
                            case 10:
                            {
                                halted = true;
                                break;
                            }

                            //--------------------------------------
                            // 11: print character ($a0)
                            //--------------------------------------
                            case 11:
                            {
                                uint32_t v = regs.readU(4); // $a0
                                char c = static_cast<char>(v & 0xFF);
                                std::cout << c;
                                break;
                            }

                            //--------------------------------------
                            // 12: read character -> $v0
                            //--------------------------------------
                            case 12:
                            {
                                char c;
                                std::cout << "CONSOLE INTEGER INPUT> ";
                                std::cin.get(c);
                                regs.writeU(2, static_cast<uint32_t>(
                                                static_cast<unsigned char>(c)));
                                break;
                            }

                            //--------------------------------------
                            // Not implemented / unknown syscall
                            //--------------------------------------
                            default:
                                throw std::runtime_error("Unknown or unimplemented syscall code in $v0");
                        }
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

            case OP_LB: // lb rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                uint8_t raw = mem.load8(addr);
                int8_t  sval = static_cast<int8_t>(raw); // sign-extend from 8 bits
                regs.writeS(rt, sval);
                break;
            }

            case OP_LBU: // lbu rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                uint8_t raw = mem.load8(addr);
                regs.writeU(rt, static_cast<uint32_t>(raw)); // zero-extend
                break;
            }

            case OP_LH: // lh rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                if (addr & 0x1)
                {
                    throw std::runtime_error("MIPS lh: unaligned address");
                }

                uint8_t b0 = mem.load8(addr + 0); // big-endian
                uint8_t b1 = mem.load8(addr + 1);
                uint16_t half = static_cast<uint16_t>((b0 << 8) | b1);

                int16_t sval = static_cast<int16_t>(half); // sign-extend from 16 bits
                regs.writeS(rt, sval);
                break;
            }

            case OP_LHU: // lhu rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                if (addr & 0x1)
                {
                    throw std::runtime_error("MIPS lhu: unaligned address");
                }

                uint8_t b0 = mem.load8(addr + 0); // big-endian
                uint8_t b1 = mem.load8(addr + 1);
                uint16_t half = static_cast<uint16_t>((b0 << 8) | b1);

                regs.writeU(rt, static_cast<uint32_t>(half)); // zero-extend
                break;
            }

            case OP_LW: // lw rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                uint32_t val = mem.load32(addr); // does its own alignment + bounds checks
                regs.writeU(rt, val);
                break;
            }

            case OP_SB: // sb rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                uint32_t val = regs.readU(rt);
                uint8_t  b   = static_cast<uint8_t>(val & 0xFF);
                mem.store8(addr, b);
                break;
            }

            case OP_SH: // sh rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                if (addr & 0x1)
                {
                    throw std::runtime_error("MIPS sh: unaligned address");
                }

                uint32_t val = regs.readU(rt);
                uint8_t  b0  = static_cast<uint8_t>((val >> 8) & 0xFF); // high byte
                uint8_t  b1  = static_cast<uint8_t>( val       & 0xFF); // low byte

                mem.store8(addr, b0); // big-endian
                mem.store8(addr + 1, b1);
                break;
            }

            case OP_SW: // sw rt, offset(rs)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                int16_t imm = static_cast<int16_t>(word & mask_bits(16)); // sign-extend

                uint32_t base = regs.readU(rs);
                int32_t  off  = static_cast<int32_t>(imm);
                uint32_t addr = base + static_cast<uint32_t>(off);

                uint32_t val = regs.readU(rt);
                mem.store32(addr, val); // does alignment + bounds checks
                break;
            }

            //==================================================
            // Branches (I-format)
            //==================================================
            case OP_BEQ: // beq rs, rt, label
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                uint32_t imm = word & mask_bits(16);

                // sign-extend immediate, then shift left 2 for word offset
                int32_t  simm   = static_cast<int16_t>(imm);
                uint32_t offset = static_cast<uint32_t>(simm) << 2;

                if (regs.readU(rs) == regs.readU(rt))
                {
                    // pc currently points to the *next* instruction
                    pc = pc + offset;
                }
                break;
            }

            case OP_BNE: // bne rs, rt, label
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint8_t rt  = (word >> 16) & mask_bits(5);
                uint32_t imm = word & mask_bits(16);

                int32_t  simm   = static_cast<int16_t>(imm);
                uint32_t offset = static_cast<uint32_t>(simm) << 2;

                if (regs.readU(rs) != regs.readU(rt))
                {
                    pc = pc + offset;
                }
                break;
            }

            // the int, identifier format currently is not
            // identified as a valid pattern.
            // needs to be handled
            case OP_BGTZ: // bgtz rs, label   (rs > 0, signed)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint32_t imm = word & mask_bits(16);

                int32_t  simm   = static_cast<int16_t>(imm);
                uint32_t offset = static_cast<uint32_t>(simm) << 2;

                int32_t v = regs.readS(rs);
                if (v > 0)
                {
                    pc = pc + offset;
                }
                break;
            }

            case OP_BLEZ: // blez rs, label   (rs <= 0, signed)
            {
                uint8_t rs  = (word >> 21) & mask_bits(5);
                uint32_t imm = word & mask_bits(16);

                int32_t  simm   = static_cast<int16_t>(imm);
                uint32_t offset = static_cast<uint32_t>(simm) << 2;

                int32_t v = regs.readS(rs);
                if (v <= 0)
                {
                    pc = pc + offset;
                }
                break;
            }

            //==================================================
            // Jumps (J-format)
            //==================================================
            case OP_J: // j label
            {
                uint32_t target = word & mask_bits(26); // low 26 bits
                // pc currently = "next PC"; keep high 4 bits, plug in target<<2
                uint32_t new_pc = (pc & 0xF0000000u) | (target << 2);
                pc = new_pc;
                break;
            }

            case OP_JAL: // jal label
            {
                uint32_t target = word & mask_bits(26);

                // return address = address of next sequential instruction
                // (since pc has already been advanced in step()).
                uint32_t ra = pc;
                regs.writeU(31, ra);

                uint32_t new_pc = (pc & 0xF0000000u) | (target << 2);
                pc = new_pc;
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
    bool halted;
};

#endif // CPU_H


// File  : Constants.h
// Author: Cole Schwandt

#ifndef CONSTANTS_H
#define CONSTANTS_H

// 6-bit opcodes (bits 31..26)
enum Opcode : uint8_t
{
    OP_RTYPE = 0x00,  // SPECIAL (R-type)

    OP_J     = 0x02,  // j
    OP_JAL   = 0x03,  // jal

    OP_BEQ   = 0x04,  // beq
    OP_BNE   = 0x05,  // bne

    OP_ADDI  = 0x08,  // addi
    OP_SLTI  = 0x0A,  // slti
    OP_ANDI  = 0x0C,  // andi
    OP_ORI   = 0x0D,  // ori

    OP_LB    = 0x20,  // lb
    OP_LW    = 0x23,  // lw
    OP_SB    = 0x28,  // sb
    OP_SW    = 0x2B   // sw
};

// 6-bit funct codes (bits 5..0) for R-type (opcode = 0)
enum Funct : uint8_t
{
    FUNCT_NONE = 0x00, // for non-r-type
    FUNCT_SLL = 0x00, // 000000b
    FUNCT_SRL = 0x02, // 000010b
    FUNCT_SRA = 0x03, // 000011b

    FUNCT_ADD = 0x20, // 100000b
    FUNCT_SUB = 0x22, // 100010b
    FUNCT_AND = 0x24, // 100100b
    FUNCT_OR  = 0x25, // 100101b
    FUNCT_SLT = 0x2A  // 101010b
};

// text segment bounds
static const uint32_t TEXT_BASE  = 0x00400000;
static const uint32_t TEXT_LIMIT = 0x10000000; // just before data

// data segment bounds
static const uint32_t DATA_BASE  = 0x10000000;
static const uint32_t DATA_LIMIT = 0x10040000; // from your notes

// stack
static const uint32_t STACK_BASE = 0x7fffeffc; // initial $sp
static const uint32_t STACK_LIMIT= 0x80000000; // top of stack region (exclusive)

#endif


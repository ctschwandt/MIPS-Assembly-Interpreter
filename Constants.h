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
    OP_BLEZ  = 0x06,  // blez
    OP_BGTZ  = 0x07,  // bgtz

    OP_ADDI  = 0x08,  // addi
    OP_ADDIU = 0x09,  // addiu
    OP_SLTI  = 0x0A,  // slti
    OP_SLTIU = 0x0B,  // sltiu
    OP_ANDI  = 0x0C,  // andi
    OP_ORI   = 0x0D,  // ori
    OP_XORI  = 0x0E,  // xori
    OP_LUI   = 0x0F,  // lui

    OP_LB    = 0x20,  // lb
    OP_LH    = 0x21,  // lh
    OP_LW    = 0x23,  // lw
    OP_LBU   = 0x24,  // lbu
    OP_LHU   = 0x25,  // lhu

    OP_SB    = 0x28,  // sb
    OP_SH    = 0x29,  // sh
    OP_SW    = 0x2B,  // sw
    OP_SC    = 0x38   // sc (store conditional; MIPS II)
};

// 6-bit funct codes (bits 5..0) for R-type (opcode = 0)
enum Funct : uint8_t
{
    FUNCT_NONE  = 0x00, // for non-r-type

    // shifts with shamt
    FUNCT_SLL   = 0x00, // 000000b
    FUNCT_SRL   = 0x02, // 000010b
    FUNCT_SRA   = 0x03, // 000011b

    // variable shifts
    FUNCT_SLLV  = 0x04,
    FUNCT_SRLV  = 0x06,
    FUNCT_SRAV  = 0x07,

    // jumps via register
    FUNCT_JR    = 0x08,
    FUNCT_JALR  = 0x09,

    // syscall / break
    FUNCT_SYSCALL = 0x0C,

    // hi/lo moves
    FUNCT_MFHI  = 0x10,
    FUNCT_MTHI  = 0x11,
    FUNCT_MFLO  = 0x12,
    FUNCT_MTLO  = 0x13,

    // multiply/divide
    FUNCT_MULT  = 0x18,
    FUNCT_MULTU = 0x19,
    FUNCT_DIV   = 0x1A,
    FUNCT_DIVU  = 0x1B,

    // basic ALU
    FUNCT_ADD   = 0x20,        // 100000b
    FUNCT_ADDU  = 0x21,        // 100001b
    FUNCT_SUB   = 0x22,        // 100010b
    FUNCT_SUBU  = 0x23,        // 100011b
    FUNCT_AND   = 0x24,        // 100100b
    FUNCT_OR    = 0x25,        // 100101b
    FUNCT_XOR   = 0x26,        // 100110b
    FUNCT_NOR   = 0x27,        // 100111b
    FUNCT_SEQ   = 0x28,        // set-if-equal (non-standard, SPIM-style)
    FUNCT_SLT   = 0x2A,        // 101010b
    FUNCT_SLTU  = 0x2B         // 101011b
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


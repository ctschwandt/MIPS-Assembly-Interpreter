// File  : Constants.h
// Author: Cole Schwandt

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Token.h"

enum Opcode : uint8_t
{
    OP_RTYPE = 0x00,  // SPECIAL (R-type)
    OP_REGIMM = 0x01, // bgez, bltz
    
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
    FUNCT_JR    = 0b001000,
    FUNCT_JALR  = 0b001001,

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

enum RegimmCode : uint8_t
{
    RT_BLTZ = 0x00,
    RT_BGEZ = 0x01,
};

//==============================================================
// Instruction metadata
//==============================================================
enum InstrType
{
    R3,        // R-format: rd, rs, rt        (add, sub, and, or, slt, ...)
    RSHIFT,    // R-format: rd, rt, shamt     (sll, srl, sra)
    I_ARITH,   // I-format: rt, rs, imm       (addi, andi, ori, slti, ...)
    I_LS,      // I-format: rt, offset(rs)    (lw, sw, lb, sb, ...)
    I_BRANCH,  // I-format: rs, rt, label     (beq, bne)
    I_BRANCH1, // I-format: rs, label         (bgtz, blez, bltz, bgez)
    JUMP,      // J-format: label             (j, jal)
    SYSCALL,   // R-format: syscall
    JR_JALR,   // R3: jr rs, jalr rs
    R_HILO1,   // rd     (mfhi, mflo)
    R_HILO2,   // rs, rt (mult, multu, div, divu)
    NUM_INSTRTYPE, 
};

struct InstrInfo
{
    InstrType type;
    Opcode    opcode;  // 6-bit opcode field
    Funct     funct;   // 6-bit funct field (R-type); 0 / FUNCT_NONE for non-R
};

enum PseudoType
{
    // Arithmetic / logical
    ABS,    // abs   rd, rs
    NEG,    // neg   rd, rs
    NEGU,   // negu  rd, rs
    NOT,    // not   rd, rs
    MUL,    // mul   rd, rs, rt
    DIV3,    // div   rd, rs, rt

    // Set-on-compare pseudos
    SGE,    // sge   rd, rs, rt
    SGT,    // sgt   rd, rs, rt

    // Branch pseudo-ops
    BLT,    // blt   rs, rt, label
    BLE,    // ble   rs, rt, label
    BGT,    // bgt   rs, rt, label
    BGE,    // bge   rs, rt, label
    B,      // b label

    // Load/Move pseudos
    LI,         // li    rt, imm32
    LA,         // la    rt, label
    MOVE,       // move  rd, rs
    LW_LABEL    // lw    rt, label   (i.e., lw with label instead of offset(base))
};

static const std::vector< TokenType > PATTERNS[NUM_INSTRTYPE] = {
    // R3: rd, rs, rt          e.g. add $t0, $t1, $t2
    std::vector<TokenType>{ REGISTER, COMMA, REGISTER, COMMA, REGISTER, EOL },

    // RSHIFT: rd, rt, shamt   e.g. sll $t0, $t1, 4
    std::vector<TokenType>{ REGISTER, COMMA, REGISTER, COMMA, INT, EOL },

    // I_ARITH: rt, rs, imm    e.g. addi $t0, $t1, 42
    std::vector<TokenType>{ REGISTER, COMMA, REGISTER, COMMA, INT, EOL },

    // I_LS: rt, offset(rs)    e.g. lw $t0, 4($t1)
    std::vector<TokenType>{ REGISTER, COMMA, INT, LPAREN, REGISTER, RPAREN, EOL },

    // I_BRANCH: rs, rt, label e.g. beq $t0, $t1, LOOP
    std::vector<TokenType>{ REGISTER, COMMA, REGISTER, COMMA, IDENTIFIER, EOL },

    // I_BRANCH1: rs, label
    std::vector<TokenType>{ REGISTER, COMMA, IDENTIFIER, EOL },

    // JUMP: label             e.g. j LOOP
    std::vector<TokenType>{ IDENTIFIER, EOL },

    // SYSCALL: syscall
    std::vector< TokenType > { EOL },

    // JR_JALR: jr $ra
    std::vector< TokenType > { REGISTER, EOL },

    // R_HILO1: rd
    std::vector< TokenType > { REGISTER, EOL },

    // R_HILO2: rs, rt
    std::vector< TokenType > { REGISTER, COMMA, REGISTER, EOL },
};

const std::unordered_map<std::string, uint8_t> REG_TABLE = {
    {"$zero", 0}, {"$0", 0},

    {"$at",   1}, {"$1",  1},

    {"$v0",   2}, {"$2",  2},
    {"$v1",   3}, {"$3",  3},

    {"$a0",   4}, {"$4",  4},
    {"$a1",   5}, {"$5",  5},
    {"$a2",   6}, {"$6",  6},
    {"$a3",   7}, {"$7",  7},

    {"$t0",   8}, {"$8",  8},
    {"$t1",   9}, {"$9",  9},
    {"$t2",  10}, {"$10",10},
    {"$t3",  11}, {"$11",11},
    {"$t4",  12}, {"$12",12},
    {"$t5",  13}, {"$13",13},
    {"$t6",  14}, {"$14",14},
    {"$t7",  15}, {"$15",15},

    {"$s0",  16}, {"$16",16},
    {"$s1",  17}, {"$17",17},
    {"$s2",  18}, {"$18",18},
    {"$s3",  19}, {"$19",19},
    {"$s4",  20}, {"$20",20},
    {"$s5",  21}, {"$21",21},
    {"$s6",  22}, {"$22",22},
    {"$s7",  23}, {"$23",23},

    {"$t8",  24}, {"$24",24},
    {"$t9",  25}, {"$25",25},

    {"$k0",  26}, {"$26",26},
    {"$k1",  27}, {"$27",27},

    {"$gp",  28}, {"$28",28},
    {"$sp",  29}, {"$29",29},

    {"$fp",  30}, {"$s8",30}, {"$30",30},

    {"$ra",  31}, {"$31",31},
};

const char * REGISTER_NAMES[32] = {
    "$zero", // 0
    "$at",   // 1
    "$v0",   // 2
    "$v1",   // 3
    "$a0",   // 4
    "$a1",   // 5
    "$a2",   // 6
    "$a3",   // 7
    "$t0",   // 8
    "$t1",   // 9
    "$t2",   // 10
    "$t3",   // 11
    "$t4",   // 12
    "$t5",   // 13
    "$t6",   // 14
    "$t7",   // 15
    "$s0",   // 16
    "$s1",   // 17
    "$s2",   // 18
    "$s3",   // 19
    "$s4",   // 20
    "$s5",   // 21
    "$s6",   // 22
    "$s7",   // 23
    "$t8",   // 24
    "$t9",   // 25
    "$k0",   // 26
    "$k1",   // 27
    "$gp",   // 28
    "$sp",   // 29
    "$fp",   // 30 (aka $s8)
    "$ra"    // 31
};

inline const std::unordered_map<std::string, InstrInfo> INSTR_TABLE = {
    //==========================================================
    // R-type arithmetic / logical: rd, rs, rt   (R3)
    //==========================================================
    { "add",   { R3,      OP_RTYPE, FUNCT_ADD   } },
    { "addu",  { R3,      OP_RTYPE, FUNCT_ADDU  } },
    { "sub",   { R3,      OP_RTYPE, FUNCT_SUB   } },
    { "subu",  { R3,      OP_RTYPE, FUNCT_SUBU  } },
    { "and",   { R3,      OP_RTYPE, FUNCT_AND   } },
    { "or",    { R3,      OP_RTYPE, FUNCT_OR    } },
    { "xor",   { R3,      OP_RTYPE, FUNCT_XOR   } },
    { "nor",   { R3,      OP_RTYPE, FUNCT_NOR   } },
    { "slt",   { R3,      OP_RTYPE, FUNCT_SLT   } },
    { "sltu",  { R3,      OP_RTYPE, FUNCT_SLTU  } },
    { "seq",   { R3,      OP_RTYPE, FUNCT_SEQ   } }, // pseudo-ish set-equal

    // Multiply / divide to hi/lo
    { "mult",  { R_HILO2, OP_RTYPE, FUNCT_MULT  } },
    { "multu", { R_HILO2, OP_RTYPE, FUNCT_MULTU } },
    { "div",   { R_HILO2, OP_RTYPE, FUNCT_DIV   } },
    { "divu",  { R_HILO2, OP_RTYPE, FUNCT_DIVU  } },

    // Moves to/from hi/lo (one register operand)
    { "mfhi",  { R_HILO1,      OP_RTYPE, FUNCT_MFHI  } },
    { "mflo",  { R_HILO1,      OP_RTYPE, FUNCT_MFLO  } },
    { "mthi",  { R_HILO1,      OP_RTYPE, FUNCT_MTHI  } },
    { "mtlo",  { R_HILO1,      OP_RTYPE, FUNCT_MTLO  } },

    //==========================================================
    // R-type shifts with shamt: rd, rt, shamt   (RSHIFT)
    //==========================================================
    { "sll",   { RSHIFT,  OP_RTYPE, FUNCT_SLL   } },
    { "srl",   { RSHIFT,  OP_RTYPE, FUNCT_SRL   } },
    { "sra",   { RSHIFT,  OP_RTYPE, FUNCT_SRA   } },

    // Variable shifts: rd, rs, rt   (you may later give them their own type)
    { "sllv",  { R3,      OP_RTYPE, FUNCT_SLLV  } },
    { "srlv",  { R3,      OP_RTYPE, FUNCT_SRLV  } },
    { "srav",  { R3,      OP_RTYPE, FUNCT_SRAV  } },

    //==========================================================
    // Specials: SYSCALL, JR_JALR
    //==========================================================
    { "jr",     { JR_JALR,    OP_RTYPE,     FUNCT_JR  } },
    { "jalr",   { JR_JALR,    OP_RTYPE,     FUNCT_JALR  } },
    { "syscall", { SYSCALL,     OP_RTYPE, FUNCT_SYSCALL } }, // no explicit operands

    //==========================================================
    // I-type arithmetic / logical: rt, rs, imm   (I_ARITH)
    //==========================================================
    { "addi",  { I_ARITH, OP_ADDI,  FUNCT_NONE  } },
    { "addiu", { I_ARITH, OP_ADDIU, FUNCT_NONE  } },
    { "andi",  { I_ARITH, OP_ANDI,  FUNCT_NONE  } },
    { "ori",   { I_ARITH, OP_ORI,   FUNCT_NONE  } },
    { "xori",  { I_ARITH, OP_XORI,  FUNCT_NONE  } },
    { "slti",  { I_ARITH, OP_SLTI,  FUNCT_NONE  } },
    { "sltiu", { I_ARITH, OP_SLTIU, FUNCT_NONE  } },
    { "lui",   { I_ARITH, OP_LUI,   FUNCT_NONE  } },   // rt, imm (rs = $zero)

    //==========================================================
    // I-type load/store: rt, offset(rs)         (I_LS)
    //==========================================================
    { "lw",    { I_LS,    OP_LW,    FUNCT_NONE  } },
    { "sw",    { I_LS,    OP_SW,    FUNCT_NONE  } },
    { "lb",    { I_LS,    OP_LB,    FUNCT_NONE  } },
    { "lbu",   { I_LS,    OP_LBU,   FUNCT_NONE  } },
    { "lh",    { I_LS,    OP_LH,    FUNCT_NONE  } },
    { "lhu",   { I_LS,    OP_LHU,   FUNCT_NONE  } },
    { "sb",    { I_LS,    OP_SB,    FUNCT_NONE  } },
    { "sh",    { I_LS,    OP_SH,    FUNCT_NONE  } },
    
    //==========================================================
    // I-type branches
    //   - two-register: rs, rt, label          (I_BRANCH)
    //   - one-register: rs, label              (still I_BRANCH for now)
    //==========================================================
    // two reg
    { "beq",   { I_BRANCH,OP_BEQ,   FUNCT_NONE  } },
    { "bne",   { I_BRANCH,OP_BNE,   FUNCT_NONE  } },

    // one-reg, normal opcodes
    { "bgtz",  { I_BRANCH1,OP_BGTZ,  FUNCT_NONE  } },   // rs, label
    { "blez",  { I_BRANCH1,OP_BLEZ,  FUNCT_NONE  } },   // rs, label

    // one-reg, REGIMM (opcode = OP_REGIMM, rt = subopcode)
    { "bltz",  { I_BRANCH1, OP_REGIMM, static_cast<Funct>(RT_BLTZ) } },
    { "bgez",  { I_BRANCH1, OP_REGIMM, static_cast<Funct>(RT_BGEZ) } },

    //==========================================================
    // Jumps (J-format): label                  (JUMP)
    //==========================================================
    { "j",     { JUMP,    OP_J,     FUNCT_NONE  } },
    { "jal",   { JUMP,    OP_JAL,   FUNCT_NONE  } },
};

inline const std::unordered_map<std::string, PseudoType> PSEUDO_TABLE = {
    { "abs",  ABS  },
    { "neg",  NEG  },
    { "negu", NEGU },
    { "not",  NOT  },
    { "mul",  MUL  },

    { "sge",  SGE  },
    { "sgt",  SGT  },

    { "blt",  BLT  },
    { "ble",  BLE  },
    { "bgt",  BGT  },
    { "bge",  BGE  },
    { "b",    B    },

    { "li",   LI   },
    { "la",   LA   },
    { "move", MOVE }
    // LW_LABEL is based off pattern, not instruction name
};


// text segment bounds
static const uint32_t TEXT_BASE  = 0x00400000;
static const uint32_t TEXT_LIMIT = 0x10000000; // just before data

// data segment bounds
static const uint32_t DATA_BASE  = 0x10000000;
static const uint32_t DATA_LIMIT = 0x10040000;

// stack
static const uint32_t STACK_BASE = DATA_LIMIT;  // bottom of stack region (just above data)
static const uint32_t STACK_LIMIT = 0x80000000;  // top (exclusive)
static const uint32_t STACK_INIT = 0x7fffeffc;  // initial $sp

#endif


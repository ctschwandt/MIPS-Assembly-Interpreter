// File  : Constants.h
// Author: Cole Schwandt

#ifndef CONSTANTS_H
#define CONSTANTS_H

//==============================================================
// Token Types
//==============================================================
enum TokenType
{
    INS,    // instruction mnemonic (add, lw, j, ...)
    REG,    // register ($t0, $sp, ...)
    INT,    // immediate / integer literal (42, -7, 0x10010000, ...)
    PUN,    // punctuation (',', '(', ')', ':', '.')
    LAB,    // label identifiers (foo:)
    DIR,    // assembler directives (.data, .word, etc)
    STR,    // string literal (for .ascii/.asciiz)
    EOL     // end-of-line marker
};

//==============================================================
// Instructions
//==============================================================
enum Instruction
{
    ADD, ADDU, SUB, SUBU,
    MULT, MULTU, DIV, DIVU,
    AND, OR, XOR, NOR,
    SLT, SLTU,

    ADDI, ADDIU, ANDI, ORI, XORI,
    LUI, LW, SW, LB, LBU, SB, LH, LHU, SH,

    BEQ, BNE, BGTZ, BLEZ, BLTZ, BGEZ,
    J, JAL, JR, JALR,

    MFHI, MFLO, MTHI, MTLO,
    SLL, SRL, SRA, SLLV, SRLV, SRAV,

    SYSCALL,

    // pseudoinstructions
    MOVE, LA, LI
};

//==============================================================
// Registers
//==============================================================
enum Register
{
    ZERO,        // $zero
    AT   = 1,    // assembler temporary
    V0   = 2, V1,        // function results
    A0   = 4, A1, A2, A3,  // arguments
    T0   = 8, T1, T2, T3, T4, T5, T6, T7, // temporaries
    S0   = 16, S1, S2, S3, S4, S5, S6, S7, // saved
    T8   = 24, T9,       // more temporaries
    K0   = 26, K1,       // reserved for OS
    GP   = 28,             // global pointer
    SP   = 29,             // stack pointer
    FP   = 30,             // frame pointer
    RA   = 31              // return address
};

//==============================================================
// Punctuation
//==============================================================
enum Punctuation
{
    COMMA,    // ,
    LPAREN,   // (
    RPAREN,   // )
    COLON,    // :
    DOT,      // .
    UNKNOWN
};
// shld $ also be added in here?

//==============================================================
// Directives
//==============================================================
enum Directive
{
    TEXT,     // .text
    DATA,     // .data
    WORD,     // .word
    ASCIIZ,   // .asciiz
    ASCII     // .ascii
};

#endif


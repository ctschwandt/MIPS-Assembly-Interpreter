// File  : Token.h
// Author: Cole Schwandt

#ifndef TOKEN_H
#define TOKEN_H

#include <cstdint>
#include <vector>
#include <string>

#include "Constants.h"

struct StrPos
{
    uint32_t start;
    uint32_t end;
};

struct TokenReference
{
    TokenType type; // which side table to index
    uint32_t idx;   // index into the side table
    uint16_t line;  // line number in program
    uint16_t col;   // column number in program
};

struct TokenTables
{
    // token sequence
    std::vector< TokenReference > stream;

    // side tables
    std::vector< Instruction > instrs;   // Instruction enum values (fits in uint8_t)
    std::vector< Register > regs;     // Register enum values  (0..31)
    std::vector< int32_t > ints;     // parsed immediates (sign-extended)
    std::vector< Punctuation > puncts;   // Punctuation enum values
    std::vector< Directive > dirs;     // Directive enum values
    std::vector< StrPos > labels;   // label names, symbols (slices into source)
    std::vector< StrPos > strings;  // .ascii/.asciiz slices (without surrounding quotes)
};

//==============================================================
// Emit helpers
//==============================================================
void emit_ins(TokenTables &, Instruction, uint16_t ln, uint16_t col);
void emit_reg(TokenTables &, Register, uint16_t ln, uint16_t col);
void emit_int(TokenTables &, int32_t, uint16_t ln, uint16_t col);
void emit_punct(TokenTables &, Punctuation, uint16_t ln, uint16_t col);
void emit_dir(TokenTables &, Directive, uint16_t ln, uint16_t col);
void emit_label(TokenTables &, StrPos, uint16_t ln, uint16_t col);
void emit_string(TokenTables &, StrPos, uint16_t ln, uint16_t col);
void emit_eol(TokenTables &, uint16_t ln, uint16_t col);

#endif

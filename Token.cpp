// File  : Token.cpp
// Author: Cole Schwandt

#include "Token.h"

static inline void push_ref(TokenTables & toks,
                            TokenType type,
                            uint32_t idx,
                            uint16_t ln,
                            uint16_t col)
{
    TokenReference ref;
    ref.type = type;
    ref.idx  = idx;
    ref.line = ln;
    ref.col  = col;
    toks.stream.push_back(ref);
}

//==============================================================
// Emits
//==============================================================
void emit_ins(TokenTables & toks, Instruction ins, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.instrs.size());
    toks.instrs.push_back(ins);
    push_ref(toks, TokenType::INS, idx, ln, col);
}

void emit_reg(TokenTables & toks, Register reg, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.regs.size());
    toks.regs.push_back(reg);
    push_ref(toks, TokenType::REG, idx, ln, col);
}

void emit_int(TokenTables & toks, int32_t value, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.ints.size());
    toks.ints.push_back(value);
    push_ref(toks, TokenType::INT, idx, ln, col);
}

void emit_punct(TokenTables & toks, Punctuation p, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.puncts.size());
    toks.puncts.push_back(p);
    push_ref(toks, TokenType::PUN, idx, ln, col);
}

void emit_dir(TokenTables & toks, Directive dir, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.dirs.size());
    toks.dirs.push_back(dir);
    push_ref(toks, TokenType::DIR, idx, ln, col);
}

void emit_label(TokenTables & toks, StrPos sp, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.labels.size());
    toks.labels.push_back(sp);
    push_ref(toks, TokenType::LAB, idx, ln, col);
}

void emit_string(TokenTables & toks, StrPos sp, uint16_t ln, uint16_t col)
{
    const uint32_t idx = static_cast<uint32_t>(toks.strings.size());
    toks.strings.push_back(sp);
    push_ref(toks, TokenType::STR, idx, ln, col);
}

void emit_eol(TokenTables & toks, uint16_t ln, uint16_t col)
{
    // EOL has no side-table payload; idx=0 is fine.
    push_ref(toks, EOL, /*idx=*/0, ln, col);
}

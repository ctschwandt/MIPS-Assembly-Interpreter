// File  : Lexer.cpp
// Author: Cole Schwandt

#include "Lexer.h"

//==============================================================
// Static member definitions
//==============================================================
const std::unordered_map< std::string, Instruction > Lexer::instr_map = {
    {"li", Instruction::LI},
    {"add", Instruction::ADD},
    {"sub", Instruction::SUB},
    // ...
};

const std::unordered_map< std::string, Register > Lexer::reg_map = {
    {"zero", Register::ZERO},
    {"at",   Register::AT},
    {"v0",   Register::V0}, {"v1", Register::V1},
    {"a0",   Register::A0}, {"a1", Register::A1}, {"a2", Register::A2}, {"a3", Register::A3},
    {"t0",   Register::T0}, {"t1", Register::T1}, {"t2", Register::T2}, {"t3", Register::T3},
    {"t4",   Register::T4}, {"t5", Register::T5}, {"t6", Register::T6}, {"t7", Register::T7},
    {"s0",   Register::S0}, {"s1", Register::S1}, {"s2", Register::S2}, {"s3", Register::S3},
    {"s4",   Register::S4}, {"s5", Register::S5}, {"s6", Register::S6}, {"s7", Register::S7},
    {"t8",   Register::T8}, {"t9", Register::T9},
    {"k0",   Register::K0}, {"k1", Register::K1},
    {"gp",   Register::GP}, {"sp", Register::SP},
    {"fp",   Register::FP}, {"ra", Register::RA}
};

const std::unordered_map< char, Punctuation > Lexer::punct_map = {
    {',', Punctuation::COMMA},
    {'(', Punctuation::LPAREN},
    {')', Punctuation::RPAREN},
    {':', Punctuation::COLON},
    {'.', Punctuation::DOT}
};

const std::unordered_map< std::string, Directive > Lexer::dir_map = {
    {"data",   Directive::DATA},
    {"text",   Directive::TEXT},
    {"word",   Directive::WORD},
    {"asciiz", Directive::ASCIIZ},
    {"ascii",  Directive::ASCII},
    // ...
};

//==============================================================
// Getters and setters
//==============================================================
Lexer::State & Lexer::state()       { return state_; }
Lexer::State   Lexer::state() const { return state_; }

//==============================================================
// Small helpers
//==============================================================
char Lexer::peek(const std::string & src, uint32_t i, uint32_t n, uint32_t k) const
{
    return (i + k < n) ? src[i + k] : '\0';
}

void Lexer::advance(char c, uint32_t & i, uint16_t & line, uint16_t & col) const
{
    ((c == '\n') ? (++line, col = 1) : (++col));
    ++i;
}

bool Lexer::is_whitespace(char c) const
{
    return (c == ' ' || c == '\t' || c == '\r');
}

bool Lexer::is_ident_start(char c) const
{
    return (std::isalpha(static_cast<unsigned char>(c)) || c == '_');
}

bool Lexer::is_ident_char(char c) const
{
    return (std::isalnum(static_cast<unsigned char>(c)) || c == '_');
}

bool Lexer::is_digit(char c) const
{
    return (std::isdigit(static_cast<unsigned char>(c)));
}

void Lexer::start_token(uint16_t line, uint16_t col,
                        uint16_t & tok_line, uint16_t & tok_col,
                        std::string & lexeme) const
{
    tok_line = line;
    tok_col  = col;
    lexeme.clear();
}

bool Lexer::is_punctuation(char c) const
{
    return punct_map.find(c) != punct_map.end();
}

Instruction Lexer::get_instr(const std::string & name, uint16_t line, uint16_t col) const
{
    auto it = instr_map.find(name);
    if (it == instr_map.end())
        throw UnknownInstruction(name, line, col);
    return it->second;
}

Register Lexer::get_reg(const std::string & name, uint16_t line, uint16_t col) const
{
    auto it = reg_map.find(name);
    if (it == reg_map.end())
        throw UnknownRegister(name, line, col);
    return it->second;
}

Directive Lexer::get_dir(const std::string & name, uint16_t line, uint16_t col) const
{
    auto it = dir_map.find(name);
    if (it == dir_map.end())
        throw UnknownDirective(name, line, col);
    return it->second;
}

Punctuation Lexer::get_punct(char c, uint16_t line, uint16_t col) const
{
    auto it = punct_map.find(c);
    if (it == punct_map.end())
        throw UnexpectedChar(c, line, col);
    return it->second;
}

//==============================================================
// State handlers
//==============================================================
void Lexer::handle_default(const std::string & src, uint32_t n,
                           uint32_t & i, uint16_t & line, uint16_t & col,
                           std::string & lexeme,
                           uint16_t & tok_line, uint16_t & tok_col,
                           TokenTables & toks)
{
    char c = peek(src, i, n);

    // whitespace
    if (is_whitespace(c)) { advance(c, i, line, col); return; }

    // newline
    if (c == '\n')
    {
        emit_eol(toks, line, col);
        advance(c, i, line, col);
        return;
    }

    // comment (consume until EOL in COMMENT state)
    if (c == '#')
    {
        state() = COMMENT;
        return;
    }

    // register
    if (c == '$')
    {
        start_token(line, col, tok_line, tok_col, lexeme);
        advance(c, i, line, col); // past '$'
        state() = REGISTER;
        return;
    }

    // directive
    if (c == '.')
    {
        start_token(line, col, tok_line, tok_col, lexeme);
        advance(c, i, line, col); // past '.'
        state() = DIRECTIVE;
        return;
    }

    // identifier / instruction / label
    if (is_ident_start(c))
    {
        start_token(line, col, tok_line, tok_col, lexeme);
        state() = LEXEME;
        return;
    }

    // number
    if (is_digit(c))
    {
        start_token(line, col, tok_line, tok_col, lexeme);
        state() = IMMEDIATE;
        return;
    }

    // punctuation (single char)
    if (is_punctuation(c))
    {
        Punctuation p = get_punct(c);
        emit_punct(toks, p, line, col);
        advance(c, i, line, col);
        return;
    }

    // otherwise, just advance
    advance(c, i, line, col);
}

void Lexer::handle_comment(const std::string & src, uint32_t n,
                           uint32_t & i, uint16_t & line, uint16_t & col)
{
    // Consume everything until (but not including) newline.
    while (i < n)
    {
        char c = peek(src, i, n);
        if (c == '\n') break;         // stop before newline; DEFAULT will emit EOL
        advance(c, i, line, col);
    }
    state() = DEFAULT;
}

void Lexer::handle_register(const std::string & src, uint32_t n,
                            uint32_t & i, uint16_t & line, uint16_t & col,
                            std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                            TokenTables & toks)
{
    // Accumulate identifier characters after '$'
    while (i < n)
    {
        char c = peek(src, i, n);
        if (!is_ident_char(c)) break;
        lexeme.push_back(c);
        advance(c, i, line, col);
    }

    // Finalize register (assumes valid name present in reg_map)
    Register r = get_reg(lexeme);
    emit_reg(toks, r, tok_line, tok_col);

    // Do not consume the stopping character here; let DEFAULT handle it.
    state() = DEFAULT;
}

void Lexer::handle_directive(const std::string & src, uint32_t n,
                             uint32_t & i, uint16_t & line, uint16_t & col,
                             std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                             TokenTables & toks)
{
    // get directive name after '.'
    while (i < n)
    {
        char c = peek(src, i, n);
        if (!is_ident_char(c))
            break;
        lexeme.push_back(c);
        advance(c, i, line, col);
    }

    // finalize directive (assumes valid name present in dir_map)
    Directive d = get_dir(lexeme);
    emit_dir(toks, d, tok_line, tok_col);

    // leave the current (non-ident) char for DEFAULT.
    state() = DEFAULT;
}

void Lexer::handle_lexeme(const std::string & src, uint32_t n,
                          uint32_t & i, uint16_t & line, uint16_t & col,
                          std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                          TokenTables & toks)
{
    // get identifier body
    while (i < n)
    {
        char c = peek(src, i, n);
        if (!is_ident_char(c))
            break;
        lexeme.push_back(c);
        advance(c, i, line, col);
    }

    // label definition if next char is ':'
    if (i < n && peek(src, i, n) == ':')
    {
        // store label text slice
        StrPos sp;
        sp.start = 0u;
        sp.end   = static_cast<uint32_t>(lexeme.size());
        emit_label(toks, sp, tok_line, tok_col);

        // emit ':' punctuation and consume it
        Punctuation p = get_punct(':');
        emit_punct(toks, p, line, col);

        char c = peek(src, i, n);
        advance(c, i, line, col);

        state() = DEFAULT;
        return;
    }

    // otherwise treat as instruction
    Instruction ins = get_instr(lexeme);
    emit_ins(toks, ins, tok_line, tok_col);

    state() = DEFAULT;
}

void Lexer::handle_immediate(const std::string & src, uint32_t n,
                             uint32_t & i, uint16_t & line, uint16_t & col,
                             std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                             TokenTables & toks)
{
    // only gets decimal digits
    // (needs to be able to handle hex and maybe binary)
    while (i < n)
    {
        char c = peek(src, i, n);
        if (!is_digit(c))
            break;
        lexeme.push_back(c);
        advance(c, i, line, col);
    }

    int32_t value = static_cast<int32_t>(std::stoi(lexeme));
    emit_int(toks, value, tok_line, tok_col);

    // leave the current non-digit for DEFAULT.
    state() = DEFAULT;
}

//==============================================================
// Public entry points
//==============================================================
TokenTables Lexer::lex_file(const std::string & source)
{
    uint16_t start_line = 1;
    return lex_core(source, start_line, /*reset_state_at_end=*/true);
}

TokenTables Lexer::lex_repl(const std::string & chunk, uint16_t & line_counter)
{
    // start at caller's current line; do not reset DFA state at end.
    return lex_core(chunk, line_counter, /*reset_state_at_end=*/false);
}

//==============================================================
// Core shared by file+repl
//==============================================================
TokenTables Lexer::lex_core(const std::string & source,
                            uint16_t & line_ref,
                            bool reset_state_at_end)
{
    TokenTables toks;

    uint32_t i = 0;
    const uint32_t n = source.size();

    uint16_t line = line_ref;
    uint16_t col  = 1;

    std::string lexeme;
    uint16_t tok_line = 0;
    uint16_t tok_col  = 0;

    while (i < n)
    {
        switch (state())
        {
            case DEFAULT:
                handle_default(source, n, i, line, col, lexeme, tok_line, tok_col, toks);
                break;
            case COMMENT:
                handle_comment(source, n, i, line, col);
                break;
            case REGISTER:
                handle_register(source, n, i, line, col, lexeme, tok_line, tok_col, toks);
                break;
            case DIRECTIVE:
                handle_directive(source, n, i, line, col, lexeme, tok_line, tok_col, toks);
                break;
            case LEXEME:
                handle_lexeme(source, n, i, line, col, lexeme, tok_line, tok_col, toks);
                break;
            case IMMEDIATE:
                handle_immediate(source, n, i, line, col, lexeme, tok_line, tok_col, toks);
                break;
        }
    }

    // handle unfinished token (for example .s with only "add")

    // always emit a trailing EOL so downstream can rely on it
    if (toks.stream.empty() || toks.stream.back().type != EOL)
        emit_eol(toks, line, col);

    // return final line position to caller (for REPL continuity)
    line_ref = line;

    if (reset_state_at_end)
        state() = DEFAULT;

    return toks;
}

// File  : Lexer.h
// Author: Cole Schwandt

#ifndef LEXER_H
#define LEXER_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Token.h"
#include "Constants.h"

//==============================================================
// Exceptions (inline "structors" in header)
//==============================================================
class LexerError
{
public:
    LexerError(std::string msg, uint16_t line, uint16_t col)
        : msg_(msg), line_(line), col_(col) {}

    virtual ~LexerError() {}

    const std::string & message() const { return msg_; }
    uint16_t line() const { return line_; }
    uint16_t col()  const { return col_; }

private:
    std::string msg_;
    uint16_t    line_;
    uint16_t    col_;
};

class UnknownRegister : public LexerError
{
public:
    UnknownRegister(const std::string & name, uint16_t line, uint16_t col)
        : LexerError(std::string("unknown register: ") + name, line, col) {}
};

class UnknownDirective : public LexerError
{
public:
    UnknownDirective(const std::string & name, uint16_t line, uint16_t col)
        : LexerError(std::string("unknown directive: ") + name, line, col) {}
};

class UnknownInstruction : public LexerError
{
public:
    UnknownInstruction(const std::string & name, uint16_t line, uint16_t col)
        : LexerError(std::string("unknown instruction: ") + name, line, col) {}
};

class InvalidNumber : public LexerError
{
public:
    InvalidNumber(const std::string & lit, uint16_t line, uint16_t col)
        : LexerError(std::string("invalid number: ") + lit, line, col) {}
};

class MissingName : public LexerError
{
public:
    MissingName(const char * what, uint16_t line, uint16_t col)
        : LexerError(std::string("missing ") + what, line, col) {}
};

class UnexpectedChar : public LexerError
{
public:
    UnexpectedChar(char c, uint16_t line, uint16_t col)
        : LexerError(std::string("unexpected char: '") + c + "'", line, col) {}
};

//==============================================================
// Class Lexer
//==============================================================
class Lexer
{
public:
    Lexer()
        : state_(DEFAULT)
    {}
    
    enum State {
        DEFAULT,
        REGISTER,
        DIRECTIVE,
        LEXEME,
        IMMEDIATE,
        COMMENT
    };

    //===== getters and setters =====//
    State & state();
    State state() const;

    //====== member functions =======//
    TokenTables lex_file(const std::string & source);
    TokenTables lex_repl(const std::string & chunk,
                         uint16_t & line_counter);

private:
    // persistent DFA state
    State state_;

    // static lookup maps
    static const std::unordered_map< std::string, Instruction > instr_map;
    static const std::unordered_map< std::string, Register >    reg_map;
    static const std::unordered_map< char, Punctuation >        punct_map;
    static const std::unordered_map< std::string, Directive >   dir_map;

    // helpers
    char  peek(const std::string & src, uint32_t i, uint32_t n, uint32_t k = 0) const;
    void  advance(char c, uint32_t & i, uint16_t & line, uint16_t & col) const;
    bool  is_whitespace(char c) const;
    bool  is_ident_start(char c) const;
    bool  is_ident_char(char c) const;
    bool  is_digit(char c) const;
    void  start_token(uint16_t line, uint16_t col,
                      uint16_t & tok_line, uint16_t & tok_col,
                      std::string & lexeme) const;

    bool         is_punctuation(char c) const;

    // map getters
    Instruction  get_instr(const std::string & name) const;
    Register     get_reg  (const std::string & name) const;
    Directive    get_dir  (const std::string & name) const;
    Punctuation  get_punct(char c) const;

    // state handlers
    void handle_default(const std::string & src, uint32_t n,
                        uint32_t & i, uint16_t & line, uint16_t & col,
                        std::string & lexeme,
                        uint16_t & tok_line, uint16_t & tok_col,
                        TokenTables & toks);

    void handle_comment (const std::string & src, uint32_t n,
                         uint32_t & i, uint16_t & line, uint16_t & col);
    void handle_register(const std::string & src, uint32_t n,
                         uint32_t & i, uint16_t & line, uint16_t & col,
                         std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                         TokenTables & toks);
    void handle_directive(const std::string & src, uint32_t n,
                          uint32_t & i, uint16_t & line, uint16_t & col,
                          std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                          TokenTables & toks);
    void handle_lexeme   (const std::string & src, uint32_t n,
                          uint32_t & i, uint16_t & line, uint16_t & col,
                          std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                          TokenTables & toks);
    void handle_immediate(const std::string & src, uint32_t n,
                          uint32_t & i, uint16_t & line, uint16_t & col,
                          std::string & lexeme, uint16_t tok_line, uint16_t tok_col,
                          TokenTables & toks);

    // shared core (used by file+repl)
    TokenTables lex_core(const std::string & source,
                         uint16_t & line_ref,
                         bool reset_state_at_end);
};

#endif

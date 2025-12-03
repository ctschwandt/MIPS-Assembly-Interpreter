// File  : Lexer.h
// Author: Cole Schwandt

#ifndef LEXER_H
#define LEXER_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Token.h"

//============================c==================================
// Helpers
//==============================================================
inline
bool is_alpha(char c)
{
    return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
}

inline
bool is_num(char c)
{
    return ('0' <= c && c <= '9');
}

inline
bool is_alphanum(char c)
{
    return (is_alpha(c) || is_num(c));
}

inline
bool is_ident_start(char c)
{
    return (is_alpha(c) || c == '_' || c == '.');
}

inline
bool is_ident_char(char c)
{
    return (is_ident_start(c) || is_num(c));
}

inline
bool is_register_start(char c)
{
    return (c == '$');
}

inline
bool is_comment_start(char c)
{
    return (c == '#');
}

inline
bool is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\r');
}

inline
bool is_lowercase(char c)
{
    return ('a' <= c && c <= 'z');
}

inline
char lowercase(char c)
{
    return (is_lowercase(c) ? c : c - 'A' + 'a'); 
}

inline
bool is_hex_digit(char c)
{
    return (is_num(c) ||
            ('a' <= c && c <= 'f') ||
            ('A' <= c && c <= 'F'));
}

inline
bool is_oct_digit(char c)
{
    return ('0' <= c && c <= '7');
}

//==============================================================
// Class Lexer
// Finite state machine that tokenizes line of assembly to be
// parsed
//==============================================================
class Lexer
{
public:
    Lexer()
        : state_(DEFAULT)
    {}

    enum State
    {
        DEFAULT,
        IN_IDENT,
        IN_REGISTER,
        IN_INT,
        IN_STRING,
        IN_CHAR
    };

    //===== getters and setters =====//
    State & state() { return state_; }
    State state() const { return state_; }

    // lex a single line into tokens
    void lex_core(std::vector< Token > & toks, const std::string & s, uint32_t line_n)
    {
        size_t i = 0;
        size_t n = s.size();

        auto push_tok = [&toks, line_n](TokenType ty, size_t start,
                                size_t end, int ival=0)
        {
            toks.push_back(Token(ty, line_n, start, end - start));
        };

        state() = DEFAULT;
        size_t start = 0; // start index of current lexeme

        while (i < n)
        {
            char c = s[i];
            switch (state())
            {
                case DEFAULT:
                    if (is_whitespace(c))
                    {
                        ++i;
                        break;
                    }

                    if (is_comment_start(c))
                    {
                        // skip rest of line
                        i = n;
                        state() = DEFAULT;
                        break;
                    }
                    
                    // single char punctation
                    if (c == ',') { push_tok(COMMA, i, i+1); ++i; break; }
                    if (c == '(') { push_tok(LPAREN, i, i+1); ++i; break; }
                    if (c == ')') { push_tok(RPAREN, i, i+1); ++i; break; }
                    if (c == ':') { push_tok(COLON, i, i+1); ++i; break; }

                    // string literal
                    if (c == '"')
                    {
                        start = i++;
                        state() = IN_STRING;
                        break;
                    }

                    // char literal (tokenized as int)
                    if (c == '\'')
                    {
                        start = i++;
                        state() = IN_CHAR;
                        break;
                    }

                    if (is_register_start(c))
                    {
                        start = i++;
                        state() = IN_REGISTER;
                        break;
                    }

                    if (is_ident_start(c))
                    {
                        start = i++;
                        state() = IN_IDENT;
                        break;
                    }

                    // integer (decimal/hex/octal) with potential leading '-'
                    if (is_num(c) || (c == '-' && i + 1 < n && is_num(s[i+1])))
                    {
                        start = i++;
                        state() = IN_INT;
                        break;
                    }

                    // invalid (ERROR) (invalids are allowed after comments though)
                    push_tok(ERROR, i, i+1);
                    ++i;
                    break;

                case IN_REGISTER:
                    if (is_alphanum(s[i]))
                    {
                        ++i;
                    }
                    else
                    {
                        push_tok(REGISTER, start, i);
                        state() = DEFAULT;
                    }
                    
                    break;

                case IN_IDENT:
                    if (is_ident_char(s[i]))
                    {
                        ++i;
                    }
                    else
                    {
                        push_tok(IDENTIFIER, start, i);
                        state() = DEFAULT;
                    }
                  
                    break;
                    
                case IN_INT:
                {
                    size_t p = (s[start] == '-' ? start + 1 : start);
               
                    // hex: 0x0923f
                    if (p + 1 < n && s[p] == '0' && (lowercase(s[p+1]) == 'x'))
                    {
                        i = p + 2;
                        while (i < n && is_hex_digit(s[i]))
                            ++i;
                        push_tok(INT, start, i);
                        state() = DEFAULT;
                        break;
                    }

                    // octal: 0127 (note: 0 alone is fine)
                    if (p < n && s[p] == '0')
                    {
                        i = p + 1;
                        if (i < n && is_oct_digit(s[i]))
                        {
                            while (i < n && is_oct_digit(s[i]))
                                ++i;
                            push_tok(INT, start, i);
                            state() = DEFAULT;
                            break;
                        }

                        // else it's just 0
                        push_tok(INT, start, i);
                        state() = DEFAULT;
                        break;
                    }

                    // decimal:
                    while (i < n && is_num(s[i]))
                        ++i;
                    push_tok(INT, start, i);
                    state() = DEFAULT;

                    break;
                }

                case IN_CHAR:
                {
                    // enter IN_CHAR with:
                    //   start = index of opening '
                    //   i     = index of first char after opening '
                    if (i >= n)
                    {
                        // nothing after ', unterminated char
                        push_tok(ERROR, start, i);
                        state() = DEFAULT;
                        break;
                    }

                    // consume body: either a single char or an escape sequence
                    if (s[i] == '\\')
                    {
                        // escape sequence: backslash + one more char (if present)
                        i += (i + 1 < n ? 2 : 1);
                    }
                    else
                    {
                        // normal single char
                        ++i;
                    }

                    // now expect closing '
                    if (i < n && s[i] == '\'')
                    {
                        ++i; // consume closing '
                        // token text will be something like:  "'+'", "'\\n'", etc.
                        push_tok(INT, start, i);
                    }
                    else
                    {
                        // no closing quote -> error token
                        push_tok(ERROR, start, i);
                    }

                    state() = DEFAULT;
                    break;
                }

                case IN_STRING:
                    if (s[i] == '\\') // ex: \n
                    {
                        i += (i + 1 < n ? 2 : 1); // skip past special char
                    }
                    else if (s[i] != '"')
                    {
                        ++i;
                    }
                    else
                    {
                        ++i; // consume closing "
                        push_tok(STRING, start, i); // [start, i) includes the quotes
                        state() = DEFAULT;
                    }
                    break;
                default:
                    break;
            }
        }

        switch (state())
        {
            case IN_IDENT:
                push_tok(IDENTIFIER, start, i);
                break;
            case IN_REGISTER:
                push_tok(REGISTER, start, i);
                break;
            case IN_INT:
                push_tok(INT, start, i);
                break;
            case IN_STRING:
                // unterminated string: decide if I want to push or error
                push_tok(ERROR, start, i);
                break;
            case IN_CHAR:
                // unterminated char: likewise
                push_tok(ERROR, start, i);
                break;
            default:
                break;
        }
        
        push_tok(EOL, n, n, 0);
    }
    
private:
    State state_;
};

#endif

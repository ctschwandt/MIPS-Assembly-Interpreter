// File  : Token.h
// Author: Cole Schwandt

#ifndef TOKEN_H
#define TOKEN_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <iomanip>

enum TokenType
{
    IDENTIFIER,  // mnemonics, labels, directives (add, LOOP, .word)
    REGISTER,    // register ($t0, $sp, $0)
    INT,         // immediates (42, -7, 0x10010000)
    STRING,      // string literal for .ascii/.asciiz ("Hello, world!")
    COMMA,       // ,
    LPAREN,      // (
    RPAREN,      // )
    COLON,       // :
    ERROR,       // invalid char (&&&&, **^*)
    EOL          // end of line marker
};

class Token
{
public:
    Token(TokenType t, uint32_t lin, uint32_t p, uint32_t le)
        : type(t), line(lin), pos(p), len(le)
    {}

    std::string get_string(const std::string & s) const
    {
        return s.substr(pos, len);
    }
    
    TokenType type;
    uint32_t line; // line number (might be better to do std::vector< std::vector< Token > >)
    uint32_t pos; // byte offset in the line
    uint32_t len; // length in bytes
};

//==============================================================
// Printing
//==============================================================
inline
const char * token_type_cstr(TokenType t)
{
    switch (t)
    {
        case IDENTIFIER: return "IDENTIFIER";
        case REGISTER:   return "REGISTER";
        case INT:        return "INT";
        case STRING:     return "STRING";
        case COMMA:      return "COMMA";
        case LPAREN:     return "LPAREN";
        case RPAREN:     return "RPAREN";
        case COLON:      return "COLON";
        case ERROR:      return "ERROR";
        case EOL:        return "EOL";
    }
    return "UNKNOWN";
}

inline
void println_toks(const std::vector< Token> & toks)
{
    std::cout << '{';
    std::string delim = "";
    for (const auto & tok : toks)
    {
        std::cout << delim << token_type_cstr(tok.type);
        delim = ", ";
    }
    std::cout << '}';
    std::cout << std::endl;
}

inline
void println_toks_detail(const std::vector<Token> & toks,
                         const std::string & line)
{
    std::cout << "Tokens (" << toks.size() << "):\n";

    if (toks.empty())
    {
        std::cout << "(no tokens)\n\n";
        return;
    }

    //--------------------------------------------------
    // first pass: compute column widths
    //--------------------------------------------------
    std::size_t idx_width  = 0;
    std::size_t type_width = 0;
    std::size_t line_width = 0;
    std::size_t pos_width  = 0;
    std::size_t len_width  = 0;

    auto digits = [](std::size_t x) -> std::size_t
    {
        std::size_t d = 1;
        while (x >= 10) { x /= 10; ++d; }
        return d;
    };

    idx_width = digits(toks.size() - 1);

    for (const auto & tok : toks)
    {
        // token type string length
        const char * tstr = token_type_cstr(tok.type);
        type_width = std::max(type_width, std::strlen(tstr));

        // numeric fields
        line_width = std::max< std::size_t >(line_width, digits(tok.line));
        pos_width  = std::max< std::size_t >(pos_width,  digits(tok.pos));
        len_width  = std::max< std::size_t >(len_width,  digits(tok.len));
    }

    //--------------------------------------------------
    // second pass: print aligned
    //--------------------------------------------------
    for (std::size_t i = 0; i < toks.size(); ++i)
    {
        const Token & tok = toks[i];

        std::string lexeme;
        if (tok.pos + tok.len <= line.size())
        {
            lexeme = line.substr(tok.pos, tok.len);
        }
        else
        {
            lexeme = "<out-of-range>";
        }

        std::cout << "  ["
                  << std::setw(idx_width) << i
                  << "]  ";

        // type: left-aligned
        std::cout << std::left  << std::setw(type_width) << token_type_cstr(tok.type)
                  << "  ";

        // restore right alignment for numbers
        std::cout << std::right
                  << "line=" << std::setw(line_width) << tok.line << "  "
                  << "pos="  << std::setw(pos_width)  << tok.pos  << "  "
                  << "len="  << std::setw(len_width)  << tok.len  << "  "
                  << "text=\"" << lexeme << "\"\n";
    }

    std::cout << '\n';
}

#endif

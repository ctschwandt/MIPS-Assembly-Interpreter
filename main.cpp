// File  : main.cpp
// Author: Cole Schwandt

#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdint>
#include <string>

#include "Token.h"
#include "RegisterFile.h"
#include "Lexer.h"
#include "Parser.h"
#include "Executor.h"

void debug_print(const std::string & line)
{
    std::cout << "[";
    for (unsigned char c : line)
    {
        if (std::isprint(c))
        {
            std::cout << c; // print normal characters
        }
        else
        {
            // show invisible/control chars as \xNN
            std::cout << "\\x" 
                      << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(c) 
                      << std::dec;
        }
    }
    std::cout << ']' << std::endl;
}

void print_tokens(const TokenTables & toks)
{
    for (const TokenReference & ref : toks.stream)
    {
        std::cout << "Token(";

        switch (ref.type)
        {
            case INS:
                std::cout << "INS," << toks.instrs[ref.idx];
                break;
            case REG:
                std::cout << "REG," << toks.regs[ref.idx];
                break;
            case INT:
                std::cout << "INT," << toks.ints[ref.idx];
                break;
            case PUN:
                std::cout << "PUN," << toks.puncts[ref.idx];
                break;
            case LAB:
            {
                StrPos sp = toks.labels[ref.idx];
                std::cout << "LAB,[" << sp.start << ":" << sp.end << "]";
                break;
            }
            case DIR:
                std::cout << "DIR," << toks.dirs[ref.idx];
                break;
            case STR:
            {
                StrPos sp = toks.strings[ref.idx];
                std::cout << "STR,[" << sp.start << ":" << sp.end << "]";
                break;
            }
            case EOL:
                std::cout << "EOL";
                break;
        }

        std::cout << " @ line=" << ref.line
                  << ", col=" << ref.col
                  << ')' << std::endl;
    }
}

int main()
{
    RegisterFile reg_file;
    Lexer lexer;

    std::string line;
    uint16_t line_counter = 1;
    
    while (1)
    {
        if (!std::getline(std::cin, line))
        {
            break; // handle EOF or error cleanly
        }

        // show raw input for debugging
        debug_print(line);
        
        // lex into tokens
        TokenTables toks = lexer.lex_repl(line + '\n', line_counter);

        // print tokens
        print_tokens(toks);
     }

    return 0;
}

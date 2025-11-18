// File  : main.cpp
// Author: Cole Schwandt

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <string>

#include "mybitlib.h"
#include "Token.h"
#include "RegisterFile.h"
#include "Lexer.h"
#include "Parser.h"
#include "CPU.h"
//#include "Executor.h"
#include "Interpreter.h"

int main()
{
    // test_lex_core();
    // test_parser();

    Interpreter interpreter;
    try
    {
        Interpreter interpreter;
        interpreter.repl(std::cin, std::cout);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

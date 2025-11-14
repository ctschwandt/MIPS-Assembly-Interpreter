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

void test_lex_core_(const std::string & s)
{
    std::cout << "========================================\n";
    std::cout << "Input: \"" << s << "\"\n";

    Lexer lexer;
    std::vector< Token > toks;
    // line number doesn't really matter for these tests
    lexer.lex_core(toks, s, 1);
    println_toks_detail(toks, s);
}

void test_lex_core()
{
    // ----------------------------------------------------
    // Simple R-format instructions
    // ----------------------------------------------------
    test_lex_core_("add $t0, $t1, $t2");
    test_lex_core_("sub $s0, $s1, $s2");
    test_lex_core_("and $a0, $a1, $a2");
    test_lex_core_("or $v0, $v1, $zero");
    test_lex_core_("slt $t0, $t1, $t2");

    // ----------------------------------------------------
    // I-format with immediates
    // ----------------------------------------------------
    test_lex_core_("addi $t0, $t1, 42");
    test_lex_core_("addi $t0, $t1, -7");
    test_lex_core_("addi $t0, $t1, 0x10010000");

    // ----------------------------------------------------
    // Load/store with offset(base)
    // ----------------------------------------------------
    test_lex_core_("lw $t0, 4($sp)");
    test_lex_core_("sw $t1, -8($fp)");
    test_lex_core_("lw $v0, 0($gp)");

    // ----------------------------------------------------
    // Branches and jumps with labels
    // ----------------------------------------------------
    test_lex_core_("beq $t0, $t1, LOOP");
    test_lex_core_("bne $s0, $s1, done");
    test_lex_core_("j TARGET");
    test_lex_core_("jal func");

    // ----------------------------------------------------
    // Labels and colons
    // ----------------------------------------------------
    test_lex_core_("LOOP: add $t0, $t1, $t2");
    test_lex_core_("END:");
    test_lex_core_("main:      # program entry point");

    // ----------------------------------------------------
    // Directives and integers
    // ----------------------------------------------------
    test_lex_core_(".word 1 2 3 4");
    test_lex_core_(".word 0x10 0xff -3");
    test_lex_core_(".space 32");
    test_lex_core_(".byte 1 2 3");

    // ----------------------------------------------------
    // String directives
    // ----------------------------------------------------
    test_lex_core_(".asciiz \"Hello, world!\"");
    test_lex_core_(".ascii \"abc\\n\"");
    test_lex_core_(".asciiz \"\"");         // empty string
    test_lex_core_(".asciiz \" spaced out \"");

    // ----------------------------------------------------
    // Comments and whitespace
    // ----------------------------------------------------
    test_lex_core_("# full line comment");
    test_lex_core_("   # comment with leading spaces");
    test_lex_core_("add $t0, $t1, $t2   # trailing comment");
    test_lex_core_("\t lw   $t0 ,  16 ( $sp )   # tabs and spaces");

    // ----------------------------------------------------
    // Weird identifiers / edge cases
    // ----------------------------------------------------
    test_lex_core_("label.with.dots: add $t0, $t1, $t2");
    test_lex_core_("_start:  jal main");
    test_lex_core_(".data");
    test_lex_core_(".text");

    // ----------------------------------------------------
    // Invalid / intentionally sketchy things
    // ----------------------------------------------------
    test_lex_core_("add $t00, $t1, $t2");        // weird register
    test_lex_core_("add $t0, $t1, $t223r2sd");   // your original junk
    test_lex_core_("addi $t0, $t1, 12abc");      // malformed number
    test_lex_core_("addi $t0 $t1 5");            // missing commas
    test_lex_core_("lw t0, 4($sp)");             // missing '$'
    test_lex_core_("just_garbage &&&& 123");
}

void test_parse_line_(const std::string & s)
{
    std::cout << "========================================\n";
    std::cout << "Parse input: \"" << s << "\"\n";

    Lexer lexer;
    std::vector<Token> toks;
    lexer.lex_core(toks, s, 1);

    println_toks_detail(toks, s);

    try
    {
        uint32_t word = Parser::parse_line_core(toks, s);
        CPU cpu;
        cpu.execute(word);
        std::cout << "Encoded word: ";
        if (word == 0)
        {
            std::cout << "(0)  -- branch/jump/label-only/empty or parse failed\n";
        }
        else
        {
            std::string hex_str = to_hex32(word);
            std::string bin_str = to_binary32(word);

            std::cout << "0x" << hex_str
                      << " (" << word << ")\n";

            // binary split into fields (R-format layout)
            // opcode (31..26), rs (25..21), rt (20..16),
            // rd (15..11), shamt (10..6), funct (5..0)
            std::cout << "Binary: "
                      << bin_str.substr(0,  6) << ' '  // opcode
                      << bin_str.substr(6,  5) << ' '  // rs
                      << bin_str.substr(11, 5) << ' '  // rt
                      << bin_str.substr(16, 5) << ' '  // rd
                      << bin_str.substr(21, 5) << ' '  // shamt
                      << bin_str.substr(26, 6)         // funct / low 6 bits
                      << "\n";

            // also show decimal values of the fields
            uint32_t opcode = (word >> 26) & 0x3Fu;
            uint32_t rs     = (word >> 21) & 0x1Fu;
            uint32_t rt     = (word >> 16) & 0x1Fu;
            uint32_t rd     = (word >> 11) & 0x1Fu;
            uint32_t shamt  = (word >>  6) & 0x1Fu;
            uint32_t funct  = (word >>  0) & 0x3Fu;

            std::cout << "Fields (dec): "
                      << "op="    << opcode
                      << " rs="   << rs
                      << " rt="   << rt
                      << " rd="   << rd
                      << " shamt="<< shamt
                      << " funct="<< funct
                      << "\n";
        }
    }
    catch (const std::exception & e)
    {
        std::cout << "Parser error: " << e.what() << "\n";
    }
}

void test_parser()
{
    // R-format
    test_parse_line_("add $t0, $s1, $s2");
    // test_parse_line_("sub $s0, $s1, $s2");
    // test_parse_line_("and $a0, $a1, $a2");
    // test_parse_line_("or $v0, $v1, $zero");
    // test_parse_line_("slt $t0, $t1, $t2");

    // // Shifts
    // test_parse_line_("sll $t0, $t1, 4");
    // test_parse_line_("srl $t2, $t3, 1");
    // test_parse_line_("sra $s0, $s1, 2");

    // // I-format arithmetic
    // test_parse_line_("addi $t0, $t1, 42");
    // test_parse_line_("addi $t0, $t1, -7");
    // test_parse_line_("andi $t0, $t1, 0xff");
    // test_parse_line_("ori  $t0, $t1, 0x1234");
    // test_parse_line_("slti $s0, $s1, 10");

    // // Load/store
    // test_parse_line_("lw $t0, 4($sp)");
    // test_parse_line_("sw $t1, -8($fp)");
    // test_parse_line_("lb $v0, 0($gp)");
    // test_parse_line_("sb $t2, 16($s0)");

    // // With labels on same line
    // test_parse_line_("LOOP: add $t0, $t1, $t2");
    // test_parse_line_("main:  lw $t0, 0($sp)");

    // // Branch/jump: should return 0 but not throw (labels unresolved)
    // test_parse_line_("beq $t0, $t1, LOOP");
    // test_parse_line_("bne $s0, $s1, done");
    // test_parse_line_("j TARGET");
    // test_parse_line_("jal func");

    // // some invalid ones to make sure errors are caught
    //test_parse_line_("add $t00, $t1, $t2");        // invalid register name
    //test_parse_line_("add $t0, $t1, $t223r2sd");   // invalid register name
    // test_parse_line_("addi $t0, $t1, 12abc");      // malformed immediate
    // test_parse_line_("addi $t0 $t1 5");            // bad pattern (missing commas)
    // test_parse_line_("lw t0, 4($sp)");             // missing '$'
}

int main()
{
    //test_lex_core();
    test_parser();
    
    return 0;
}

// File  : Interpreter.h
// Author: Cole Schwandt
//
// Top-level interactive interpreter / REPL for the MIPS machine.
// Owns a Machine and uses Lexer/Parser to assemble lines entered
// by the user into text/data segments, and can run code interactively
// or in batch mode.

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <iostream>
#include <string>
#include <cctype>
#include <stdexcept>

#include "Machine.h"
#include "Lexer.h"
#include "Parser.h"

/*
  todo:
  - r format instructions
    - other R-type: jr, jalr, syscall

  - i format instructions
    - I_LS:    lw, sw, lb, lbu, lh, lhu,
               sb, sh, sc
    - I_BRANCH: beq, bne, bgtz, blez, bgez, bltz

  - j format instructions
    - JUMP: j, jal

  - handle pseudoinstructions
    - move, li, la, lw (with label),
      blt, ble, bgt, bge

  - labels display
  - labels parsed and stored in label table (store text address)
  - data segment display
  - add things to data segment (.word, .asciiz, etc)
    -- need some function that handles assembly directives. the parser
       does not look at those lines

  dr liow feature list:
  - The user can enter SPIM instruction and data at the prompt.
  - The user can load a SPIM program.
  - The simulator executes instruction and data input whenever possible. If an invalid instruction
    was entered the program displays a warning that should help in debugging the instruction.
  - The user can view the state of the MIPS including the text, data segment and labels.
  - The user can re-execute the program that was interactively entered from the first instruction.
  - The user can save the program entered into a file.
  - [OPTIONAL] The user inserts breakpoints and singlestep/multi-step through the program.
  - [OPTIONAL] The user can set environment variables such as setting the verbosity of the
    simulator.
  
  bugs:
 */

//==============================================================
// Small helpers
//==============================================================
inline
std::string trim_copy(const std::string& s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    return s.substr(start, end - start);
}

// case-sensitive compare to a command word
inline
bool is_cmd(const std::string & line, const char * cmd)
{
    return (line == cmd);
}

inline
std::string char_value_str(uint32_t val)
{
    unsigned char c = static_cast<unsigned char>(val & 0xFF);
    if (std::isprint(c))
    {
        return std::string("'") + static_cast<char>(c) + "'";
    }
    else
    {
        return "'.'";
    }
}

inline
const char * uint_to_reg(unsigned i)
{
    static const char * names[32] = {
        "$zero", // 0
        "$at",   // 1
        "$v0",   // 2
        "$v1",   // 3
        "$a0",   // 4
        "$a1",   // 5
        "$a2",   // 6
        "$a3",   // 7
        "$t0",   // 8
        "$t1",   // 9
        "$t2",   // 10
        "$t3",   // 11
        "$t4",   // 12
        "$t5",   // 13
        "$t6",   // 14
        "$t7",   // 15
        "$s0",   // 16
        "$s1",   // 17
        "$s2",   // 18
        "$s3",   // 19
        "$s4",   // 20
        "$s5",   // 21
        "$s6",   // 22
        "$s7",   // 23
        "$t8",   // 24
        "$t9",   // 25
        "$k0",   // 26
        "$k1",   // 27
        "$gp",   // 28
        "$sp",   // 29
        "$fp",   // 30 (aka $s8)
        "$ra"    // 31
    };
    return (i < 32) ? names[i] : "??";
}

//==============================================================
// Interpreter
//==============================================================
class Interpreter
{
public:
    Interpreter()
        : machine(), lexer(), parser(), line_number(1)
    {}

    void reset()
    {
        line_number = 1;
    }

    // interactive REPL: read lines from 'in', write output to 'out'.
    void repl(std::istream & in, std::ostream & out)
    {
        bool quit = false;

        while (!quit)
        {
            // prompt depends on current segment mode
            if (machine.in_text_mode)
                out << "TEXT:0x" << std::hex << machine.text_cursor << " > " << std::dec;
            else
                out << "DATA:0x" << std::hex << machine.data_cursor << " > " << std::dec;

            std::string line;
            if (!std::getline(in, line))
                break; // EOF

            line = trim_copy(line);
            if (line.empty())
                continue;

            // segment-switching directives first
            if (is_cmd(line, ".text"))
            {
                machine.in_text_mode = true;
                continue;
            }
            if (is_cmd(line, ".data"))
            {
                machine.in_text_mode = false;
                continue;
            }

            // global commands (work in any segment)
            if (is_command_line(line))
            {
                handle_command(line, out, quit);
                continue;
            }

            // otherwise: treat as assembly in current segment
            // Remember old cursor so we can roll back on error
            uint32_t old_text_cursor = machine.text_cursor;

            try
            {
                if (machine.in_text_mode)
                {
                    // assemble instruction(s) into text segment
                    assemble_text_line(line);

                    // interactive execution: run newly emitted instructions
                    // from old_text_cursor up to new text_cursor.
                    while (machine.cpu.pc < machine.text_cursor)
                    {
                        machine.cpu.step();
                    }
                }
                else
                {
                    //assemble_data_line(line);
                }
            }
            catch (const std::exception & e)
            {
                // roll back the text cursor
                machine.text_cursor = old_text_cursor;

                out << "Error: " << e.what() << "\n";
            }
        }
    }

    // load a file and assemble it in batch (non-interactive) mode
    void load_file(const std::string & path);

private:
    Machine machine;
    Lexer   lexer;
    Parser  parser;
    int line_number;

    //==========================================================
    // Command handling
    //==========================================================
    bool is_command_line(const std::string & line) const
    {
        return  is_cmd(line, "?")      ||
                is_cmd(line, "help")   ||
                is_cmd(line, "regs")   ||
                is_cmd(line, "run")    ||
                is_cmd(line, "reset")  ||
                is_cmd(line, "exit")   ||
                is_cmd(line, "quit");
    }

    void handle_command(const std::string & line, std::ostream & out, bool & quit)
    {
        if (is_cmd(line, "?") || is_cmd(line, "help"))
        {
            print_help(out);
        }
        else if (is_cmd(line, "regs"))
        {
            print_registers(out);
        }
        else if (is_cmd(line, "run"))
        {
            run_program(out);
        }
        else if (is_cmd(line, "reset"))
        {
            machine.reset();
            out << "Machine reset.\n";
        }
        else if (is_cmd(line, "exit") || is_cmd(line, "quit"))
        {
            std::cout << "exiting...\n";
            quit = true;
        }
        else
        {
            out << "Unknown command: " << line << "\n";
        }
    }

    void print_help(std::ostream & out) const
    {
        out << "Commands:\n"
            << "  ?/help     - show this help\n"
            << "  .text      - switch to text segment\n"
            << "  .data      - switch to data segment\n"
            << "  regs       - show register file\n"
            << "  run        - run program from TEXT_BASE to current text_cursor\n"
            << "  reset      - reset machine (regs, pc, cursors, memory)\n"
            << "  exit/quit  - quit interpreter\n";
    }

    void print_registers(std::ostream & out) const
    {
        out << std::setfill('=') << std::setw(65) << '\n';
        out << "REGISTERS\n";
        out << std::setw(65) << '\n';
        out << std::setfill(' ')
            << std::setw(12) << "reg number"   << '|'
            << std::setw(12) << "reg name"     << '|'
            << std::setw(12) << "value (int)"  << '|'
            << std::setw(12) << "value (hex)"  << '|'
            << std::setw(12) << "value (char)" << '\n';
        out << std::setfill('-')
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '\n';
        out << std::setfill(' ');

        for (unsigned int i = 0; i < 32; ++i)
        {
            uint32_t u = machine.cpu.regs.readU(i);
            int32_t  s = machine.cpu.regs.readS(i);

            unsigned int reg_w = (i < 10 ? 11 : 10);

            out << std::setw(reg_w) << '$' << i << '|';
            out << std::setw(12) << uint_to_reg(i) << '|';
            out << std::setw(12) << s << '|';
            out << "  0x" << std::setw(8) << std::hex << std::setfill('0')
                << u << std::setfill(' ') << std::dec << '|';
            out << std::setw(12) << char_value_str(u) << '\n';
        }

        // Hi register
        {
            uint32_t u = machine.cpu.regs.hiU();
            int32_t  s = machine.cpu.regs.hiS();

            out << std::setw(12) << "N/A" << '|';
            out << std::setw(12) << "$hi" << '|';
            out << std::setw(12) << s << '|';
            out << "  0x" << std::setw(8) << std::hex << std::setfill('0')
                << u << std::setfill(' ') << std::dec << '|';
            out << std::setw(12) << char_value_str(u) << '\n';
        }

        // Lo register
        {
            uint32_t u = machine.cpu.regs.loU();
            int32_t  s = machine.cpu.regs.loS();

            out << std::setw(12) << "N/A" << '|';
            out << std::setw(12) << "$lo" << '|';
            out << std::setw(12) << s << '|';
            out << "  0x" << std::setw(8) << std::hex << std::setfill('0')
                << u << std::setfill(' ') << std::dec << '|';
            out << std::setw(12) << char_value_str(u) << '\n';
        }

        out << std::setfill('-')
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '\n';
        out << std::setfill(' ');
    }

    void run_program(std::ostream & out)
    {
        // - start PC at TEXT_BASE
        // - run until PC reaches text_cursor or some safety limit.
        machine.cpu.pc = TEXT_BASE;

        const uint32_t max_steps = 1000000; // safety cap to avoid infinite loops
        uint32_t steps = 0;

        try
        {
            while (machine.cpu.pc < machine.text_cursor && steps < max_steps)
            {
                machine.cpu.step();
                ++steps;
            }

            if (steps >= max_steps)
            {
                out << "run: stopped after " << steps << " steps (possible infinite loop)\n";
            }
        }
        catch (const std::exception & e)
        {
            out << "Runtime error: " << e.what() << "\n";
        }
    }

    //==========================================================
    // Assembly helpers
    //==========================================================
    void assemble_text_line(const std::string & line)
    {
        // need to put toks object in lexer
        // instead of recreating vector everytime
        // (do I even need token vector?)
        std::vector< Token > toks;
        lexer.lex_core(toks, line, line_number);
        println_toks_detail(toks, line);
        
        uint32_t word = Parser::parse_line_core(toks, line);
        machine.emit_text_word(word);
        //machine.cpu.execute(word);
        //parser.assemble_text_line(line, machine, lexer);
    }
};

#endif // INTERPRETER_H

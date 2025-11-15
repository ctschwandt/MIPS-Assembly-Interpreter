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

//==============================================================
// Small helpers
//==============================================================
inline std::string trim_copy(const std::string& s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    return s.substr(start, end - start);
}

// Case-sensitive compare to a command word
inline bool is_cmd(const std::string& line, const char * cmd)
{
    return line == cmd;
}

//==============================================================
// Interpreter
//==============================================================
class Interpreter
{
public:
    Interpreter()
        : machine(), lexer(), parser()
    {}

    // Interactive REPL: read lines from 'in', write output to 'out'.
    void repl(std::istream & in, std::ostream & out)
    {
        bool quit = false;

        while (!quit)
        {
            // prompt depends on current segment mode
            if (machine.text_mode)
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
                machine.text_mode = true;
                continue;
            }
            if (is_cmd(line, ".data"))
            {
                machine.text_mode = false;
                continue;
            }

            // global commands (work in any segment)
            if (is_command_line(line))
            {
                handle_command(line, out, quit);
                continue;
            }

            // otherwise: treat as assembly in current segment
            try
            {
                if (machine.text_mode)
                {
                    // assemble instruction(s) into text segment
                    uint32_t old_cursor = machine.text_cursor;
                    assemble_text_line(line);

                    // interactive execution: run newly emitted instructions
                    // from old_cursor up to new text_cursor.
                    while (machine.cpu.pc < machine.text_cursor)
                    {
                        machine.cpu.step();
                    }
                }
                else
                {
                    assemble_data_line(line);
                }
            }
            catch (const std::exception & e)
            {
                out << "Error: " << e.what() << "\n";
            }
        }
    }

    // load a file and assemble it in batch (non-interactive) mode
    void load_file(const std::string & path);

private:
    Machine machine;
    Lexer   lexer;   // you already have these classes
    Parser  parser;

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
                is_cmd(line, "mode");
                // add more as needed
    }

    void handle_command(const std::string & line, std::ostream & out, bool & quit)
    {
        if (is_cmd(line, "?") || is_cmd(line, "help"))
        {
            print_help(out);
        }
        else if (is_cmd(line, "regs"))
        {
            dump_registers(out);
        }
        else if (is_cmd(line, "run"))
        {
            run_program(out);
        }
        else if (is_cmd(line, "reset"))
        {
            machine.reset(true);
            out << "Machine reset.\n";
        }
        else if (is_cmd(line, "mode"))
        {
            out << "Current segment mode: "
                << (machine.text_mode ? ".text" : ".data") << "\n";
        }
        else if (is_cmd(line, "exit") || is_cmd(line, "quit"))
        {
            quit = true;
        }
        else
        {
            out << "Unknown command: " << line << "\n";
        }
    }

    void print_help(std::ostream& out) const
    {
        out << "Commands:\n"
            << "  ? / help   - show this help\n"
            << "  .text      - switch to text segment\n"
            << "  .data      - switch to data segment\n"
            << "  regs       - show register file\n"
            << "  run        - run program from TEXT_BASE to current text_cursor\n"
            << "  reset      - reset machine (regs, pc, cursors, memory)\n"
            << "  mode       - show current segment mode\n"
            << "  exit       - quit interpreter\n";
    }

    void dump_registers(std::ostream& out) const
    {
        out << "Registers:\n";
        for (uint8_t i = 0; i < 32; ++i)
        {
            uint32_t u = machine.cpu.regs.readU(i);
            int32_t  s = machine.cpu.regs.readS(i);
            out << "  $" << static_cast<int>(i)
                << "  0x" << std::hex << u
                << "  (" << std::dec << s << ")\n";
        }
        out << "  hi: 0x" << std::hex << machine.cpu.regs.hiU()
            << " (" << std::dec << machine.cpu.regs.hiS() << ")\n";
        out << "  lo: 0x" << std::hex << machine.cpu.regs.loU()
            << " (" << std::dec << machine.cpu.regs.loS() << ")\n";
        out << "  pc: 0x" << std::hex << machine.cpu.pc
            << std::dec << "\n";
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
        catch (const std::exception& e)
        {
            out << "Runtime error: " << e.what() << "\n";
        }
    }

    //==========================================================
    // Assembly helpers
    //==========================================================
    void assemble_text_line(const std::string & line)
    {
        // You need to implement this using your Lexer + Parser.
        //
        // Typical approach:
        //  1) lexer.tokenize_line(line)
        //  2) parser.parse_instruction(tokens, machine) which
        //     calls machine.emit_text_word() for each real instruction
        //
        // For now, we assume Parser has a helper like this:
        parser.assemble_text_line(line, machine, lexer);
    }

    void assemble_data_line(const std::string & line)
    {
        // Same idea as assemble_text_line, but for directives.
        //
        //  e.g., parse ".word 42" and call machine.emit_data_word(42);
        //        parse ".asciiz \"Hello\"" and call machine.emit_data_asciiz("Hello");
        //
        parser.assemble_data_line(line, machine, lexer);
    }
};

#endif // INTERPRETER_H

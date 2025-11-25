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
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstring>

#include "Machine.h"
#include "Lexer.h"
#include "Parser.h"

/*
  todo:
  - bug fixing on tic-tac-toe within sim:
Enter row: CONSOLE INTEGER INPUT> 0
Enter col: CONSOLE INTEGER INPUT> 0
+-+-+-+
|X| | |
+-+-+-+
| |O| |
+-+-+-+
| | | |
+-+-+-+
Runtime error: MIPS integer overflow on addu


  
  Features to support (Dr. Liow list):
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
*/

//==============================================================
// Small helpers
//==============================================================
inline
std::string trim_copy(const std::string & s)
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
bool starts_with(const std::string & s, const char * prefix)
{
    return s.rfind(prefix, 0) == 0; // prefix at position 0
}

inline
std::string char_value_str(uint32_t u)
{
    auto show_char = [](uint8_t c) -> std::string
    {
        switch (c)
        {
            case '\n': return "\\n";
            case '\t': return "\\t";
            case '\r': return "\\r";
            case '\0': return "\\0";
            case '\"': return "\\\"";
            case '\\': return "\\\\";
            default:
                if (c >= 32 && c < 127)
                {
                    // printable ASCII
                    return std::string(1, static_cast<char>(c));
                }
                else
                {
                    // non-printable; you could also use hex here
                    return ".";
                }
        }
    };

    auto pad2 = [](const std::string & s) -> std::string
    {
        if (s.size() >= 2)
            return s.substr(0, 2);
        return s + std::string(2 - s.size(), ' ');
    };

    uint8_t b0 = static_cast<uint8_t>((u >> 24) & 0xFF);
    uint8_t b1 = static_cast<uint8_t>((u >> 16) & 0xFF);
    uint8_t b2 = static_cast<uint8_t>((u >> 8)  & 0xFF);
    uint8_t b3 = static_cast<uint8_t>( u        & 0xFF);

    std::string s;
    s.reserve(11); // "xx xx xx xx"

    s += pad2(show_char(b0)); s += ' ';
    s += pad2(show_char(b1)); s += ' ';
    s += pad2(show_char(b2)); s += ' ';
    s += pad2(show_char(b3));

    return s;
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
    return (0 <= i && i < 32) ? names[i] : "??";
}

// One assembled source line in the interactive program
struct SourceLine
{
    std::string text;      // original line ("li $s0, 1")
    bool        in_text;   // true if assembled in text mode
    uint32_t    pc_before; // text/data cursor before assembling this line
    uint32_t    pc_after;  // text/data cursor after assembling this line
};

//==============================================================
// Interpreter
//==============================================================
class Interpreter
{
public:
    Interpreter()
        : machine(), lexer(), parser(machine), line_number(1)
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

            // remember old cursors so we can roll back on error
            uint32_t old_text_cursor = machine.text_cursor;
            uint32_t old_data_cursor = machine.data_cursor;

            try
            {
                if (machine.in_text_mode)
                {
                    // assemble instruction(s) into text segment
                    assemble_text_line(line);

                    // record successful text line in program history
                    record_source_line_text(line, old_text_cursor);

                    if (!machine.has_unresolved_fixups())
                    {
                        // run newly emitted instructions
                        // from old_text_cursor up to new text_cursor.
                        while (machine.cpu.pc < machine.text_cursor)
                        {
                            machine.cpu.step();
                        }
                    }
                    else
                    {
                        out << "Execution paused: unresolved labels remain.\n";
                    }
                }
                else // in data mode
                {
                    // data inserts into the data segment only
                    assemble_data_line(line);
                    record_source_line_data(line, old_data_cursor);
                }
            }
            catch (const std::exception & e)
            {
                // roll back the cursor on error
                if (machine.in_text_mode)
                {
                    machine.text_cursor = old_text_cursor;
                }
                else
                {
                    machine.data_cursor = old_data_cursor;
                }

                out << "Error: " << e.what() << "\n";
            }

            quit = machine.cpu.halted;
        }
        std::cout << "exiting..." << std::endl;
    }

    // load a file and assemble it (non-interactive loader)
    void load_file(const std::string & path)
    {
        std::ifstream in(path);
        if (!in)
        {
            throw std::runtime_error("Could not open file: " + path);
        }

        std::string line;
        while (std::getline(in, line))
        {
            std::string trimmed = trim_copy(line);
            if (trimmed.empty())
                continue;

            // segment directives
            if (is_cmd(trimmed, ".text"))
            {
                machine.in_text_mode = true;
                continue;
            }
            if (is_cmd(trimmed, ".data"))
            {
                machine.in_text_mode = false;
                continue;
            }

            // treat as assembly line
            uint32_t old_text_cursor = machine.text_cursor;
            uint32_t old_data_cursor = machine.data_cursor;

            try
            {
                if (machine.in_text_mode)
                {
                    assemble_text_line(trimmed);
                    record_source_line_text(trimmed, old_text_cursor);
                }
                else
                {
                    assemble_data_line(trimmed);
                    record_source_line_data(trimmed, old_data_cursor);
                }
            }
            catch (const std::exception &)
            {
                if (machine.in_text_mode)
                    machine.text_cursor = old_text_cursor;
                else
                    machine.data_cursor = old_data_cursor;

                throw; // let caller report the error
            }
        }
    }

private:
    Machine machine;
    Lexer   lexer;
    Parser  parser;
    int     line_number;

    // all successfully assembled source lines (in order)
    std::vector< SourceLine > program_;

    //----------------------------------------------------------
    // Program history helpers
    //----------------------------------------------------------
    void record_source_line_text(const std::string & line,
                                 uint32_t pc_before)
    {
        SourceLine src;
        src.text      = line;
        src.in_text   = true;
        src.pc_before = pc_before;
        src.pc_after  = machine.text_cursor;
        program_.push_back(src);
    }

    void record_source_line_data(const std::string & line,
                                 uint32_t pc_before)
    {
        SourceLine src;
        src.text      = line;
        src.in_text   = false;
        src.pc_before = pc_before;
        src.pc_after  = machine.data_cursor;
        program_.push_back(src);
    }

    void rebuild_from_program()
    {
        // Start from a clean machine state
        machine.reset();

        // Re-assemble every stored source line, without executing
        for (const SourceLine & src : program_)
        {
            machine.in_text_mode = src.in_text;

            if (src.in_text)
            {
                assemble_text_line(src.text);
            }
            else
            {
                assemble_data_line(src.text);
            }
        }
    }

    void save_program(const std::string & path)
    {
        std::ofstream out(path);
        if (!out)
        {
            throw std::runtime_error("Could not open file for writing: " + path);
        }

        for (const SourceLine & src : program_)
        {
            out << src.text << '\n';
        }
    }

    //----------------------------------------------------------
    // Command handling
    //----------------------------------------------------------
    bool is_command_line(const std::string & line) const
    {
        return  is_cmd(line, "?")      ||
                is_cmd(line, "help")   ||
                is_cmd(line, "regs")   ||
                is_cmd(line, "run")    ||
                is_cmd(line, "reset")  ||
                is_cmd(line, "data")   ||
                is_cmd(line, "stack")  ||
                is_cmd(line, "labels") ||
                is_cmd(line, "save")   ||
                starts_with(line, "read ") ||
                starts_with(line, "load ") ||
                is_cmd(line, "exit")   ||
                is_cmd(line, "quit");
    }

    // parse filename after a command like:  read "file.s"
    // supports:
    //   read "foo.s"
    //   read foo.s
    std::string parse_filename_after_command(const std::string & line,
                                             const char * cmd) const
    {
        std::size_t cmd_len = std::strlen(cmd);
        if (line.size() < cmd_len)
            return "";

        std::size_t pos = cmd_len;
        // skip spaces after command
        while (pos < line.size() &&
               std::isspace(static_cast<unsigned char>(line[pos])))
        {
            ++pos;
        }

        if (pos >= line.size())
            return ""; // no filename

        if (line[pos] == '"')
        {
            std::size_t end = line.find('"', pos + 1);
            if (end == std::string::npos)
                return ""; // no closing quote
            return line.substr(pos + 1, end - (pos + 1));
        }
        else
        {
            std::string fname = line.substr(pos);
            return trim_copy(fname);
        }
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
        else if (is_cmd(line, "labels"))
        {
            print_labels(out);
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
        else if (is_cmd(line, "data"))
        {
            print_data_segment(out);
        }
        else if (is_cmd(line, "stack"))
        {
            print_stack(out);
        }
        else if (is_cmd(line, "save"))
        {
            try
            {
                // simple version: always save to "program.s"
                const std::string filename = "program.s";
                save_program(filename);
                out << "Program saved to '" << filename << "'.\n";
            }
            catch (const std::exception & e)
            {
                out << "Save error: " << e.what() << "\n";
            }
        }
        else if (starts_with(line, "read "))
        {
            std::string filename = parse_filename_after_command(line, "read");
            if (filename.empty())
            {
                out << "Usage: read \"FILE\"\n";
            }
            else
            {
                try
                {
                    load_file(filename);
                    out << "Read \"" << filename << "\".\n";
                }
                catch (const std::exception & e)
                {
                    out << "Read error: " << e.what() << "\n";
                }
            }
        }
        else if (starts_with(line, "load "))
        {
            std::string filename = parse_filename_after_command(line, "load");
            if (filename.empty())
            {
                out << "Usage: load \"FILE\"\n";
            }
            else
            {
                try
                {
                    load_file(filename);
                    out << "Loaded \"" << filename << "\".\n";
                }
                catch (const std::exception & e)
                {
                    out << "Load error: " << e.what() << "\n";
                }
            }
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

    void print_help(std::ostream & out) const
    {
        out << "Commands:\n"
            << "  ?/help       - show this help\n"
            << "  .text        - switch to text segment\n"
            << "  .data        - switch to data segment\n"
            << "  read \"FILE\" - read assembly file into memory\n"
            << "  load \"FILE\" - same as read\n"
            << "  regs         - show register file\n"
            << "  data         - show data segment in use\n"
            << "  stack        - show stack segment in use\n"
            << "  labels       - show all currently defined labels\n"
            << "  run          - rebuild from history and run from TEXT_BASE\n"
            << "  save         - save interactive program to program.s\n"
            << "  reset        - reset machine (regs, pc, cursors, memory)\n"
            << "  exit/quit    - quit interpreter\n";
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

    void print_stack(std::ostream & out) const
    {
        machine.mem.print_region(out, STACK_BASE, STACK_LIMIT, "STACK SEGMENT");
    }

    void print_data_segment(std::ostream & out) const
    {
        machine.mem.print_region(out, DATA_BASE, DATA_LIMIT, "DATA SEGMENT");
    }

    void print_labels(std::ostream & out) const
    {
        machine.print_labels(out);
    }

    void run_program(std::ostream & out)
    {
        // Rebuild machine state from stored program history,
        // then run from TEXT_BASE to current text_cursor.
        rebuild_from_program();
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
                out << "run: stopped after " << steps
                    << " steps (possible infinite loop)\n";
            }
        }
        catch (const std::exception & e)
        {
            out << "Runtime error: " << e.what() << "\n";
        }
    }

    //----------------------------------------------------------
    // Assembly helpers
    //----------------------------------------------------------
    void assemble_text_line(const std::string & line)
    {
        std::vector< Token > toks;
        lexer.lex_core(toks, line, line_number);
        println_toks_detail(toks, line);

        uint32_t line_pc = machine.text_cursor;

        std::vector< uint32_t > words = parser.assemble_text_line(toks, line, line_pc);

        for (uint32_t word : words)
        {
            machine.emit_text_word(word);
        }
    }

    void assemble_data_line(const std::string & line)
    {
        std::vector< Token > toks;
        lexer.lex_core(toks, line, line_number);
        println_toks_detail(toks, line);
        uint32_t line_pc = machine.data_cursor;
        parser.assemble_data_line(toks, line, line_pc);
    }
};

#endif // INTERPRETER_H

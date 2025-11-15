// File  : Machine.h
// Author: Cole Schwandt
//
// Represents a full MIPS machine: CPU + Memory + segment cursors.
// Used by the interpreter/REPL to assemble code and data into the
// appropriate segments and to run the CPU.

#ifndef MACHINE_H
#define MACHINE_H

#include <cstdint>
#include <stdexcept>

#include "Constants.h"
#include "Memory.h"
#include "CPU.h"

class Machine
{
public:
    Machine()
        : mem(),
          cpu(mem),
          text_cursor(TEXT_BASE),
          data_cursor(DATA_BASE),
          in_text_mode(true)
    {
        reset();
    }

    // reset machine state: registers, PC, cursors, memory
    void reset()
    {
        mem.reset();
        
        cpu.regs.reset();
        cpu.pc = TEXT_BASE;
        
        text_cursor = TEXT_BASE;
        data_cursor = DATA_BASE;
        in_text_mode = true;

        // init stack pointer
        cpu.regs.writeU(29, STACK_BASE);
    }

    // append a 32-bit instruction word to the text segment at the
    // current text_cursor. throws if text segment overflows.
    void emit_text_word(uint32_t instr)
    {
        // 4-byte aligned text, within [TEXT_BASE, TEXT_LIMIT)
        if (text_cursor & 0x3)
            throw std::runtime_error("emit_text_word: text_cursor not aligned");

        if (text_cursor + 4 > TEXT_LIMIT)
            throw std::runtime_error("emit_text_word: text segment overflow");

        mem.store32(text_cursor, instr);
        text_cursor += 4;
    }

    // append a sequence of bytes to the data segment at data_cursor.
    void emit_data_bytes(const uint8_t * bytes, std::size_t n)
    {
        if (data_cursor + n > DATA_LIMIT)
            throw std::runtime_error("emit_data_bytes: data segment overflow");

        for (std::size_t i = 0; i < n; ++i)
        {
            mem.store8(data_cursor + static_cast<uint32_t>(i), bytes[i]);
        }
        data_cursor += static_cast<uint32_t>(n);
    }

    // .word (32-bit, big-endian)
    void emit_data_word(uint32_t value)
    {
        // ensure 4-byte alignment
        if (data_cursor & 0x3)
            throw std::runtime_error("emit_data_word: data_cursor not aligned");

        if (data_cursor + 4 > DATA_LIMIT)
            throw std::runtime_error("emit_data_word: data segment overflow");

        mem.store32(data_cursor, value);
        data_cursor += 4;
    }

    // .byte (single byte)
    void emit_data_byte(uint8_t value)
    {
        emit_data_bytes(&value, 1);
    }

    // .asciiz (null-terminated string)
    void emit_data_asciiz(const char* s)
    {
        // write characters including terminating '\0'
        const char * p = s;
        while (*p)
        {
            uint8_t c = static_cast<uint8_t>(*p);
            emit_data_byte(c);
            ++p;
        }
        emit_data_byte(0); // terminating null
    }

    //==============================================================
    // Members
    //==============================================================
    Memory      mem;
    CPU         cpu;
    uint32_t    text_cursor;   // next free address in text segment
    uint32_t    data_cursor;   // next free address in data segment
    bool        in_text_mode;  // current assembly target (.text / .data)
};

#endif // MACHINE_H

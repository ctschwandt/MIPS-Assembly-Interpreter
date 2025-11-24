// File  : Memory.h
// Author: Cole Schwandt
//
// Sparse memory model for MIPS simulator.
// Uses a single 32-bit address space with text, data, and stack
// regions defined in Constants.h.

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <map>
#include <stdexcept>

#include "Constants.h"

class Memory
{
public:
    // clear all memory contents
    void reset()
    {
        mem_.clear();
    }

    //==============================================================
    // Byte access
    //==============================================================
    // load a single byte from memory.
    // if the address is unmapped but in a valid segment, returns 0.
    uint8_t load8(uint32_t addr) const
    {
        if (!is_valid_address(addr))
            throw std::runtime_error("Memory load8: address out of bounds");

        auto it = mem_.find(addr);
        if (it == mem_.end())
            return 0; // default value for unmapped memory

        return it->second;
    }

    // store a single byte into memory
    void store8(uint32_t addr, uint8_t val)
    {
        if (!is_valid_address(addr))
            throw std::runtime_error("Memory store8: address out of bounds");

        mem_[addr] = val;
    }

    //==============================================================
    // 32-bit word access
    //==============================================================
    // load a 32-bit word from memory (big-endian)
    uint32_t load32(uint32_t addr) const
    {
        if (addr & 0x3)
            throw std::runtime_error("Memory load32: unaligned address");

        // check all 4 bytes are in a valid region
        if (!is_valid_address(addr) ||
            !is_valid_address(addr + 1) ||
            !is_valid_address(addr + 2) ||
            !is_valid_address(addr + 3))
        {
            throw std::runtime_error("Memory load32: address out of bounds");
        }

        uint32_t b0 = load8(addr + 0);
        uint32_t b1 = load8(addr + 1);
        uint32_t b2 = load8(addr + 2);
        uint32_t b3 = load8(addr + 3);

        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    // store a 32-bit word to memory
    void store32(uint32_t addr, uint32_t val)
    {
        if (addr & 0x3)
            throw std::runtime_error("Memory store32: unaligned address");

        // check all 4 bytes are in a valid region
        if (!is_valid_address(addr + 3))
        {
            throw std::runtime_error("Memory store32: address out of bounds");
        }

        uint8_t b0 = static_cast<uint8_t>((val >> 24) & 0xFF);
        uint8_t b1 = static_cast<uint8_t>((val >> 16) & 0xFF);
        uint8_t b2 = static_cast<uint8_t>((val >> 8)  & 0xFF);
        uint8_t b3 = static_cast<uint8_t>( val        & 0xFF);

        store8(addr + 0, b0);
        store8(addr + 1, b1);
        store8(addr + 2, b2);
        store8(addr + 3, b3);
    }

    //==============================================================
    // Segment classification helpers
    //==============================================================
    static bool is_text(uint32_t addr)
    {
        return addr >= TEXT_BASE && addr < TEXT_LIMIT;
    }

    static bool is_data(uint32_t addr)
    {
        return addr >= DATA_BASE && addr < DATA_LIMIT;
    }

    static bool is_stack(uint32_t addr)
    {
        return addr >= STACK_BASE && addr < STACK_LIMIT;
    }

    // print all mapped 32-bit words in [start, limit) in a table
    void print_region(std::ostream & out,
                      uint32_t start,
                      uint32_t limit,
                      const char * title) const
    {
        out << std::setfill('=') << std::setw(65) << '\n';
        out << title << '\n';
        out << std::setw(65) << '\n';

        out << std::setfill(' ')
            << std::setw(12) << "addr (int)"   << '|'
            << std::setw(12) << "addr (hex)"   << '|'
            << std::setw(12) << "value (int)"  << '|'
            << std::setw(12) << "value (hex)"  << '|'
            << std::setw(12) << "value (char)" << '\n';

        out << std::setfill('-')
            << std::setw(13) << '+'   // 12 chars + border
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '\n';
        out << std::setfill(' ');

        // map a byte to a string representation (including escapes)
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
                        // non-printable; you could also show hex here
                        return ".";
                    }
            }
        };

        // pad/truncate to exactly 2 characters for each "cell"
        auto pad2 = [](const std::string & s) -> std::string
        {
            if (s.size() >= 2)
                return s.substr(0, 2);
            return s + std::string(2 - s.size(), ' ');
        };

        // collect unique aligned word addresses in [start, limit)
        // that have at least one mapped byte.
        std::vector<uint32_t> word_addrs;
        uint32_t last_word = std::numeric_limits<uint32_t>::max();

        auto it  = mem_.lower_bound(start);
        auto end = mem_.lower_bound(limit);

        for (; it != end; ++it)
        {
            uint32_t addr = it->first;
            if (addr < start || addr >= limit)
                continue;

            uint32_t word_addr = addr & ~0x3u; // align down to 4-byte boundary
            if (word_addr == last_word)
                continue; // already recorded this word
            last_word = word_addr;

            if (word_addr + 3 >= limit)
                continue; // avoid crossing region limit

            word_addrs.push_back(word_addr);
        }

        if (word_addrs.empty())
        {
            out << "  (no mapped words in region)\n";
        }
        else
        {
            // Print in ascending address order
            for (uint32_t addr : word_addrs)
            {
                uint32_t w = load32(addr);

                uint8_t b0 = static_cast<uint8_t>((w >> 24) & 0xFF);
                uint8_t b1 = static_cast<uint8_t>((w >> 16) & 0xFF);
                uint8_t b2 = static_cast<uint8_t>((w >> 8)  & 0xFF);
                uint8_t b3 = static_cast<uint8_t>( w        & 0xFF);

                // addr (int)
                out << std::setw(12) << std::dec << addr << '|';

                // addr (hex) – 8 hex digits, no 0x
                out << std::setw(12) << std::hex << addr << std::dec << '|';

                // value (int) – signed
                out << std::setw(12) << static_cast<int32_t>(w) << '|';

                // value (hex) – each byte 2 hex digits (zero-padded)
                {
                    std::ostringstream hex_ss;
                    hex_ss << std::hex << std::nouppercase << std::setfill('0')
                           << std::setw(2) << static_cast<unsigned>(b0) << ' '
                           << std::setw(2) << static_cast<unsigned>(b1) << ' '
                           << std::setw(2) << static_cast<unsigned>(b2) << ' '
                           << std::setw(2) << static_cast<unsigned>(b3);
                    out << std::setw(12) << hex_ss.str() << '|';
                }

                // value (char) – 4 "cells" of width 2, separated by spaces
                {
                    std::string chars;
                    chars += pad2(show_char(b0)); chars += ' ';
                    chars += pad2(show_char(b1)); chars += ' ';
                    chars += pad2(show_char(b2)); chars += ' ';
                    chars += pad2(show_char(b3));

                    out << std::setw(12) << chars << '\n';
                }
            }
        }

        out << std::setfill('-')
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '+'
            << std::setw(13) << '\n';
        out << std::setfill(' ');
    }

private:
    bool is_valid_address(uint32_t addr) const
    {
        return is_text(addr) || is_data(addr) || is_stack(addr);
    }

    std::map< uint32_t, uint8_t > mem_;
};

#endif // MEMORY_H

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

private:
    bool is_valid_address(uint32_t addr) const
    {
        return is_text(addr) || is_data(addr) || is_stack(addr);
    }

    std::map< uint32_t, uint8_t > mem_;
};

#endif // MEMORY_H

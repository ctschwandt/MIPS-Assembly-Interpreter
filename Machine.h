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
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>

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

        labels.clear();
        branch_fixups.clear();
        jump_fixups.clear();
        la_fixups_.clear();

        // init stack pointer
        cpu.regs.writeU(29, STACK_INIT);
    }

    void define_label(const std::string & name, uint32_t addr)
    {
        auto it = labels.find(name);

        // record label if label DNE
        if (it == labels.end())
        {
            labels[name] = addr;
            resolve_fixups_for(name);
        } 
        // otherwise, throw error (label redefined)
        else
        {
            throw std::runtime_error("Label redefined: " + name);
        }
    }

    uint32_t lookup_label(const std::string & name) const
    {
        auto it = labels.find(name);
        if (it == labels.end())
        {
            throw std::runtime_error("Unknown label: " + name);
        }
        return it->second;
    }

    bool has_label(const std::string & name) const
    {
        return labels.find(name) != labels.end();
    }

    struct BranchFixup
    {
        uint32_t instr_addr;   // address of the branch instruction
        Opcode   opcode;       // OP_BEQ, OP_BNE, OP_BGTZ, ...
        uint32_t rs;
        uint32_t rt;
        std::string label;     // label to branch to
    };

    struct JumpFixup
    {
        uint32_t instr_addr;   // address of the j/jal instruction
        Opcode   opcode;       // OP_J or OP_JAL
        std::string label;     // label we jump to
    };

    struct LaFixup
    {
        uint32_t instr_addr;   // address of the LUI word (first word of la)
        uint8_t  rt;           // destination register of the ORI
        std::string label;     // label we’re waiting on
    };

    void add_branch_fixup(uint32_t instr_addr,
                          Opcode opcode,
                          uint32_t rs,
                          uint32_t rt,
                          const std::string & label)
    {
        branch_fixups.push_back(BranchFixup{instr_addr, opcode, rs, rt, label});
    }

    void add_jump_fixup(uint32_t instr_addr,
                        Opcode opcode,
                        const std::string & label)
    {
        jump_fixups.push_back(JumpFixup{instr_addr, opcode, label});
    }

    void add_la_fixup(uint32_t instr_addr,
                      uint32_t rt,
                      const std::string & label)
    {
        la_fixups_.push_back(LaFixup{instr_addr,
                                     static_cast<uint8_t>(rt),
                                     label});
    }

    bool has_unresolved_fixups() const
    {
        return !branch_fixups.empty() ||
               !jump_fixups.empty()   ||
               !la_fixups_.empty();
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
        // bounds check
        if (data_cursor + static_cast<uint32_t>(n) > DATA_LIMIT)
            throw std::runtime_error("emit_data_bytes: data segment overflow");

        // write n bytes starting at current data_cursor
        for (std::size_t i = 0; i < n; ++i)
        {
            mem.store8(data_cursor + static_cast<uint32_t>(i), bytes[i]);
        }

        data_cursor += static_cast<uint32_t>(n);
    }

    // .byte (single byte)
    void emit_data_byte(uint8_t value)
    {
        emit_data_bytes(&value, 1);
    }

    // .word (32-bit word, stored via Memory::store32)
    void emit_data_word(uint32_t value)
    {
        // enforce 4-byte alignment for .word
        if (data_cursor & 0x3u)
            throw std::runtime_error("emit_data_word: data_cursor not aligned");

        if (data_cursor + 4u > DATA_LIMIT)
            throw std::runtime_error("emit_data_word: data segment overflow");

        mem.store32(data_cursor, value);
        data_cursor += 4u;
    }

    // .half (16-bit, big-endian like SPIM)
    void emit_data_half(uint16_t value)
    {
        // enforce 2-byte alignment
        if (data_cursor & 0x1u)
            throw std::runtime_error("emit_data_half: data_cursor not aligned");

        if (data_cursor + 2u > DATA_LIMIT)
            throw std::runtime_error("emit_data_half: data segment overflow");

        // big-endian: high byte first
        uint8_t hi = static_cast<uint8_t>((value >> 8) & 0xFFu);
        uint8_t lo = static_cast<uint8_t>( value       & 0xFFu);

        mem.store8(data_cursor,     hi);
        mem.store8(data_cursor + 1, lo);

        data_cursor += 2u;
    }

    // .asciiz (null-terminated string)
    void emit_data_asciiz(const char * s)
    {
        // write characters including the final '\0'
        const char * p = s;
        while (*p)
        {
            emit_data_byte(static_cast<uint8_t>(*p));
            ++p;
        }
        emit_data_byte(0); // terminating null
    }

    void print_labels(std::ostream & out) const
    {
        out << std::setfill('=') << std::setw(65) << '\n';
        out << "Labels\n";
        out << std::setw(65) << '\n';
        out << std::setfill(' ');

        if (labels.empty())
        {
            out << " (no labels defined)\n";
            return;
        }

        // collect (addr, name) pairs so can sort by address
        std::vector< std::pair<uint32_t, std::string> > entries;
        entries.reserve(labels.size());
        for (const auto & kv : labels)
        {
            const std::string & name = kv.first;
            uint32_t addr = kv.second;
            entries.emplace_back(addr, name);
        }

        std::sort(entries.begin(), entries.end(),
                  [](const auto & a, const auto & b)
                  {
                      if (a.first != b.first)
                          return a.first < b.first;   // sort by address
                      return a.second < b.second;     // tie-break by name
                  });

        for (const auto & e : entries)
        {
            uint32_t addr        = e.first;
            const std::string & name = e.second;

            out << std::setw(12) << addr << '|'
                << ' ' << name << '\n';
        }
    }


    //==============================================================
    // Members
    //==============================================================
    Memory mem;
    CPU cpu;
    uint32_t text_cursor;   // next free address in text segment
    uint32_t data_cursor;   // next free address in data segment
    bool in_text_mode;      // current assembly target (.text / .data)
    std::unordered_map< std::string, uint32_t > labels; // addresses of labels

private:
    std::vector< BranchFixup > branch_fixups;
    std::vector< JumpFixup > jump_fixups;
    std::vector< LaFixup >    la_fixups_;

    void resolve_fixups_for(const std::string & label)
    {
        auto it_label = labels.find(label);
        if (it_label == labels.end())
            return;

        uint32_t target_addr = it_label->second;

        // --------- branches ---------
        for (std::size_t i = 0; i < branch_fixups.size(); )
        {
            BranchFixup & f = branch_fixups[i];
            if (f.label != label)
            {
                ++i;
                continue;
            }

            uint32_t instr_pc = f.instr_addr;

            // offset = (target - (PC_of_next_instr)) / 4
            int32_t diff = static_cast<int32_t>(target_addr)
                - static_cast<int32_t>(instr_pc + 4);

            if (diff & 0x3)
                throw std::runtime_error("Branch target not word-aligned");

            int32_t offset = diff >> 2;

            if (offset < std::numeric_limits<int16_t>::min() ||
                offset > std::numeric_limits<int16_t>::max())
            {
                throw std::runtime_error("Branch offset out of 16-bit range");
            }

            uint16_t imm = static_cast<uint16_t>(offset & 0xFFFF);
            uint32_t op  = static_cast<uint32_t>(f.opcode);

            uint32_t word =
                (op << 26) |
                (f.rs << 21) |
                (f.rt << 16) |
                imm;

            mem.store32(instr_pc, word); // patch placeholder

            // remove fixup (swap & pop)
            branch_fixups[i] = branch_fixups.back();
            branch_fixups.pop_back();
        }

        // --------- jumps ---------
        for (std::size_t i = 0; i < jump_fixups.size(); )
        {
            JumpFixup & f = jump_fixups[i];
            if (f.label != label)
            {
                ++i;
                continue;
            }

            if (target_addr & 0x3u)
                throw std::runtime_error("Jump target not word-aligned");

            uint32_t op           = static_cast<uint32_t>(f.opcode);
            uint32_t target_field = (target_addr >> 2) & 0x03FFFFFFu;

            uint32_t word = (op << 26) | target_field;

            mem.store32(f.instr_addr, word);

            jump_fixups[i] = jump_fixups.back();
            jump_fixups.pop_back();
        }

        // --------- la (lui/ori for la rt,label) ---------
        for (std::size_t i = 0; i < la_fixups_.size(); )
        {
            LaFixup & f = la_fixups_[i];
            if (f.label != label)
            {
                ++i;
                continue;
            }

            // split address into hi/lo
            uint16_t hi = static_cast<uint16_t>((target_addr >> 16) & 0xFFFFu);
            uint16_t lo = static_cast<uint16_t>( target_addr        & 0xFFFFu);

            // first word: LUI at instr_addr
            uint32_t lui_word = mem.load32(f.instr_addr);
            // clear low 16 bits, insert hi
            lui_word = (lui_word & 0xFFFF0000u) | hi;
            mem.store32(f.instr_addr, lui_word);

            // second word: ORI at instr_addr + 4
            uint32_t ori_addr = f.instr_addr + 4u;
            uint32_t ori_word = mem.load32(ori_addr);
            // clear low 16 bits, insert lo
            ori_word = (ori_word & 0xFFFF0000u) | lo;
            mem.store32(ori_addr, ori_word);

            // remove fixup (swap & pop)
            la_fixups_[i] = la_fixups_.back();
            la_fixups_.pop_back();
        }
    }
};

#endif // MACHINE_H

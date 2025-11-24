// File  : Parser.h
// Author: Cole Schwandt

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <limits>

#include "Constants.h"
#include "Token.h"
#include "Lexer.h"
#include "Machine.h"

//==============================================================
// Parser
//==============================================================
class Parser
{
public:
    Parser(Machine & m)
        : machine(m)
    {}
    
    // mnemonic string -> InstrInfo
    static InstrInfo get_instr_info(const std::string & mnemonic)
    {
        auto it = INSTR_TABLE.find(mnemonic);
        if (it == INSTR_TABLE.end())
        {
            // TODO: better error handling
            throw std::runtime_error("Unknown instruction: " + mnemonic);
        }

        return it->second;
    }

    // mnemonic token -> InstrInfo
    static InstrInfo get_instr_info(const Token & tok, const std::string & line)
    {
        std::string mnemonic = tok.get_string(line);
        return get_instr_info(mnemonic);
    }

    static InstrType get_instr_type(const std::string & mnemonic)
    {
        return get_instr_info(mnemonic).type;
    }

    static InstrType get_instr_type(const Token & tok, const std::string & line)
    {
        return get_instr_info(tok, line).type;
    }

    // InstrType -> expected token pattern AFTER mnemonic
    static const std::vector<TokenType> & get_pattern(InstrType type)
    {
        return PATTERNS[int(type)];
    }

    static bool is_pseudo(const std::string & mnem)
    {
        return PSEUDO_TABLE.find(mnem) != PSEUDO_TABLE.end();
    }

    static PseudoType get_pseudo(const std::string & mnem)
    {
        auto it = PSEUDO_TABLE.find(mnem);
        if (it == PSEUDO_TABLE.end())
            throw std::runtime_error("Unknown pseudo-instruction: " + mnem);
        return it->second;
    }

    static int parse_register(const Token & tok, const std::string & line)
    {
        if (tok.type != REGISTER)
            throw std::runtime_error("Expected register token");

        std::string name = tok.get_string(line);
        auto it = REG_TABLE.find(name);
        if (it == REG_TABLE.end())
            throw std::runtime_error("Invalid register name: " + name);

        return it->second;
    }

    static int64_t parse_int_token(const Token & tok, const std::string & line)
    {
        if (tok.type != INT)
            throw std::runtime_error("Expected integer token");

        std::string text = tok.get_string(line);
        const std::size_t n = text.size();
        if (n == 0)
            throw std::runtime_error("Empty integer token");

        std::size_t i = 0;
        bool negative = false;

        // Optional leading '-'
        if (text[i] == '-')
        {
            negative = true;
            ++i;
            if (i >= n)
                throw std::runtime_error("Invalid integer literal: " + text);
        }

        int base = 10;

        // Hex: 0x...
        if (i + 1 < n && text[i] == '0' &&
            (text[i+1] == 'x' || text[i+1] == 'X'))
        {
            base = 16;
            i += 2;
            if (i >= n)
                throw std::runtime_error("Invalid hex literal: " + text);
        }
        else if (i < n && text[i] == '0')
        {
            // octal or just 0
            base = 8;
            ++i;

            // if no more digits, it's just 0
            if (i == n)
                return 0;
        }
        else
        {
            base = 10;
        }

        uint64_t value = 0;

        for (; i < n; ++i)
        {
            char c = text[i];
            int digit = -1;

            if (base == 16)
            {
                if (!is_hex_digit(c))
                    throw std::runtime_error("Invalid hex digit in: " + text);

                if ('0' <= c && c <= '9')
                    digit = c - '0';
                else if ('a' <= lowercase(c) && lowercase(c) <= 'f')
                    digit = 10 + (lowercase(c) - 'a');
            }
            else if (base == 8)
            {
                if (!is_oct_digit(c))
                    throw std::runtime_error("Invalid octal digit in: " + text);

                digit = c - '0'; // '0'..'7'
            }
            else // base 10
            {
                if (!is_num(c))
                    throw std::runtime_error("Invalid decimal digit in: " + text);

                digit = c - '0'; // '0'..'9'
            }

            value = value * base + static_cast<uint64_t>(digit);
        }

        int64_t signed_val = negative ? -static_cast<int64_t>(value)
                                      : static_cast<int64_t>(value);
        return signed_val;
    }

    static int16_t parse_imm16_signed(const Token & tok, const std::string & line)
    {
        int64_t v = parse_int_token(tok, line);
        if (v < -32768 || v > 32767)
            throw std::runtime_error("Immediate out of 16-bit signed range");
        return static_cast<int16_t>(v);
    }

    static uint16_t parse_imm16_unsigned(const Token & tok, const std::string & line)
    {
        int64_t v = parse_int_token(tok, line);
        if (v < 0 || v > 0xFFFF)
            throw std::runtime_error("Immediate out of 16-bit unsigned range");
        return static_cast<uint16_t>(v);
    }

    static int32_t parse_imm32(const Token & tok, const std::string & line)
    {
        int64_t v = parse_int_token(tok, line);
        if (v < std::numeric_limits<int32_t>::min() ||
            v > std::numeric_limits<int32_t>::max())
        {
            throw std::runtime_error("Immediate out of 32-bit range");
        }
        return static_cast<int32_t>(v);
    }

    static uint8_t parse_shamt(const Token & tok, const std::string & line)
    {
        int64_t v = parse_int_token(tok, line);
        if (v < 0 || v > 31)
            throw std::runtime_error("Shift amount out of range 0..31");
        return static_cast<uint8_t>(v);
    }

    void
    expand_pseudo(const std::vector<Token> & toks,
                          int mnem_index,
                          const std::string & line,
                          uint32_t current_pc,
                          std::vector<uint32_t> & words)
    {
        std::string mnem = toks[mnem_index].get_string(line);
        PseudoType type = get_pseudo(mnem);

        const uint8_t AT   = REG_TABLE.at("$at"); // $at
        const uint8_t ZERO = REG_TABLE.at("$zero"); // $zero

        switch (type)
        {
            //--------------------------------------------------
            // move rd, rs  ==  addu rd, rs, $zero
            //--------------------------------------------------
            case MOVE:
            {
                // move rd, rs
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);
                uint32_t rt = ZERO;

                uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                uint32_t funct = static_cast<uint32_t>(FUNCT_ADDU);

                uint32_t word =
                    (op    << 26) |
                    (rs    << 21) |
                    (rt    << 16) |
                    (rd    << 11) |
                    (0u    <<  6) |
                    (funct <<  0);

                words.push_back(word);
                break;
            }

            //--------------------------------------------------
            // li rt, imm32
            //
            // if 16-bit signed:  addi rt, $zero, imm
            // else:              lui  $at, hi(imm)
            //                     ori  rt,  $at, lo(imm)
            //--------------------------------------------------
            case LI:
            {
                int j = mnem_index + 1;
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // COMMA
                int32_t imm = parse_imm32(toks[j++], line);

                // li does different things depending
                // on size of immediate
                if (imm >= -32768 && imm <= 32767)
                {
                    uint16_t imm16 = static_cast<uint16_t>(imm);

                    uint32_t op = static_cast<uint32_t>(OP_ADDI);
                    uint32_t word =
                        (op    << 26) |
                        (ZERO  << 21) |
                        (rt    << 16) |
                        imm16;

                    words.push_back(word);
                }
                else
                {
                    uint32_t uimm = static_cast<uint32_t>(imm);
                    uint16_t hi = static_cast<uint16_t>((uimm >> 16) & 0xFFFFu);
                    uint16_t lo = static_cast<uint16_t>(uimm & 0xFFFFu);

                    // lui $at, hi
                    {
                        uint32_t op = static_cast<uint32_t>(OP_LUI);
                        uint32_t word =
                            (op    << 26) |
                            (ZERO  << 21) |
                            (AT    << 16) |
                            hi;
                        words.push_back(word);
                    }

                    // ori rt, $at, lo
                    {
                        uint32_t op = static_cast<uint32_t>(OP_ORI);
                        uint32_t word =
                            (op    << 26) |
                            (AT    << 21) |
                            (rt    << 16) |
                            lo;
                        words.push_back(word);
                    }
                }
                break;
            }

            //--------------------------------------------------
            // la rt, label
            //
            // For now we require the label to already be defined.
            //   addr = machine.lookup_label(label)
            //   lui  $at, hi(addr)
            //   ori  rt,  $at, lo(addr)
            //--------------------------------------------------
            case LA:
            {
                int j = mnem_index + 1;
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // COMMA

                if (toks[j].type != IDENTIFIER)
                    throw std::runtime_error("Expected label in la");

                std::string label = toks[j++].get_string(line);

                if (!machine.has_label(label))
                    throw std::runtime_error("Label not defined yet for la: " + label);

                uint32_t addr = machine.lookup_label(label);

                uint16_t hi = static_cast<uint16_t>((addr >> 16) & 0xFFFFu);
                uint16_t lo = static_cast<uint16_t>(addr & 0xFFFFu);

                // lui $at, hi
                {
                    uint32_t op = static_cast<uint32_t>(OP_LUI);
                    uint32_t word =
                        (op    << 26) |
                        (ZERO  << 21) |
                        (AT    << 16) |
                        hi;
                    words.push_back(word);
                }

                // ori rt, $at, lo
                {
                    uint32_t op = static_cast<uint32_t>(OP_ORI);
                    uint32_t word =
                        (op    << 26) |
                        (AT    << 21) |
                        (rt    << 16) |
                        lo;
                    words.push_back(word);
                }

                break;
            }

            //--------------------------------------------------
            // Branch pseudos:
            //  blt rs, rt, label:
            //    slt $at, rs, rt
            //    bne $at, $zero, label
            //
            //  bgt rs, rt, label:
            //    slt $at, rt, rs
            //    bne $at, $zero, label
            //
            //  ble rs, rt, label:
            //    slt $at, rt, rs
            //    beq $at, $zero, label
            //
            //  bge rs, rt, label:
            //    slt $at, rs, rt
            //    beq $at, $zero, label
            //--------------------------------------------------
            case BLT:
            case BLE:
            case BGT:
            case BGE:
            {
                int j = mnem_index + 1;
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // COMMA

                if (toks[j].type != IDENTIFIER)
                    throw std::runtime_error("Expected label in branch pseudo");

                std::string label = toks[j++].get_string(line);

                // 1) slt $at, A, B
                uint32_t slt_rs, slt_rt;

                if (type == BLT || type == BGE)
                {
                    slt_rs = rs;
                    slt_rt = rt;
                }
                else
                {
                    slt_rs = rt;
                    slt_rt = rs;
                }

                {
                    uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                    uint32_t funct = static_cast<uint32_t>(FUNCT_SLT);

                    uint32_t word =
                        (op      << 26) |
                        (slt_rs  << 21) |
                        (slt_rt  << 16) |
                        (AT      << 11) |
                        (0u      <<  6) |
                        (funct   <<  0);

                    words.push_back(word);
                }

                // 2) branch instruction using existing fixup logic
                uint32_t opcode_branch =
                    (type == BLT || type == BGT)
                    ? static_cast<uint32_t>(OP_BNE)   // blt/bgt
                    : static_cast<uint32_t>(OP_BEQ);  // ble/bge

                uint32_t instr_pc =
                    current_pc + 4u * static_cast<uint32_t>(words.size());

                uint16_t imm = 0;

                if (machine.has_label(label))
                {
                    uint32_t target_addr = machine.lookup_label(label);

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

                    imm = static_cast<uint16_t>(offset & 0xFFFF);
                }

                uint32_t word_branch =
                    (opcode_branch << 26) |
                    (AT            << 21) |   // rs = $at
                    (ZERO          << 16) |   // rt = $zero
                    imm;

                words.push_back(word_branch);

                if (!machine.has_label(label))
                {
                    machine.add_branch_fixup(instr_pc,
                                             static_cast<Opcode>(opcode_branch),
                                             AT,
                                             ZERO,
                                             label);
                }

                break;
            }

            //--------------------------------------------------
            // b label
            //
            // Unconditional PC-relative branch:
            //   b label   ==   beq $zero, $zero, label
            //--------------------------------------------------
            case B:
            {
                int j = mnem_index + 1;

                if (toks[j].type != IDENTIFIER)
                    throw std::runtime_error("Expected label in b pseudo");

                std::string label = toks[j++].get_string(line);

                uint32_t op = static_cast<uint32_t>(OP_BEQ);

                // PC where this beq will be stored (first word of this line’s expansion)
                uint32_t instr_pc =
                    current_pc + 4u * static_cast<uint32_t>(words.size());

                uint16_t imm = 0; // placeholder

                // If label is already known, fully encode now
                if (machine.has_label(label))
                {
                    uint32_t target_addr = machine.lookup_label(label);

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

                    imm = static_cast<uint16_t>(offset & 0xFFFF);
                }

                uint32_t word =
                    (op            << 26) |
                    (ZERO          << 21) |  // rs = $zero
                    (ZERO          << 16) |  // rt = $zero
                    imm;

                words.push_back(word);

                // If label not yet known, record fixup
                if (!machine.has_label(label))
                {
                    machine.add_branch_fixup(instr_pc,
                                             OP_BEQ,
                                             ZERO,
                                             ZERO,
                                             label);
                }

                break;
            }

            //--------------------------------------------------
            // abs rd, rs
            //
            // Branchless trick:
            //   sra  $at, rs, 31        # at = 0x0000..0 or 0xFFFF..F depending on sign
            //   xor  rd,  rs, $at      # if negative: rd = rs ^ -1 = ~rs
            //   subu rd,  rd, $at      # if negative: rd = ~rs - (-1) = -rs; else rd = rs
            //--------------------------------------------------
            case ABS:
            {
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);

                // sra $at, rs, 31
                {
                    uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                    uint32_t funct = static_cast<uint32_t>(FUNCT_SRA);
                    uint32_t shamt = 31u;

                    uint32_t word =
                        (op    << 26) |
                        (0u    << 21) |       // rs = 0 for shifts
                        (rs    << 16) |       // rt = source
                        (AT    << 11) |       // rd = $at
                        (shamt <<  6) |
                        (funct <<  0);

                    words.push_back(word);
                }

                // xor rd, rs, $at
                {
                    uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                    uint32_t funct = static_cast<uint32_t>(FUNCT_XOR);

                    uint32_t word =
                        (op    << 26) |
                        (rs    << 21) |       // rs
                        (AT    << 16) |       // rt
                        (rd    << 11) |       // rd
                        (0u    <<  6) |
                        (funct <<  0);

                    words.push_back(word);
                }

                // subu rd, rd, $at
                {
                    uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                    uint32_t funct = static_cast<uint32_t>(FUNCT_SUBU);

                    uint32_t word =
                        (op    << 26) |
                        (rd    << 21) |       // rs = rd
                        (AT    << 16) |       // rt = $at
                        (rd    << 11) |       // rd
                        (0u    <<  6) |
                        (funct <<  0);

                    words.push_back(word);
                }

                break;
            }

            //--------------------------------------------------
            // neg rd, rs   == sub rd, $zero, rs
            //--------------------------------------------------
            case NEG:
            {
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);

                uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                uint32_t funct = static_cast<uint32_t>(FUNCT_SUB);

                uint32_t word =
                    (op    << 26) |
                    (ZERO  << 21) |       // rs = $zero
                    (rs    << 16) |       // rt = source
                    (rd    << 11) |       // rd = dest
                    (0u    <<  6) |
                    (funct <<  0);

                words.push_back(word);
                break;
            }

            //--------------------------------------------------
            // negu rd, rs  == subu rd, $zero, rs
            //--------------------------------------------------
            case NEGU:
            {
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);

                uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                uint32_t funct = static_cast<uint32_t>(FUNCT_SUBU);

                uint32_t word =
                    (op    << 26) |
                    (ZERO  << 21) |       // rs = $zero
                    (rs    << 16) |       // rt = source
                    (rd    << 11) |       // rd = dest
                    (0u    <<  6) |
                    (funct <<  0);

                words.push_back(word);
                break;
            }

            //--------------------------------------------------
            // not rd, rs   == nor rd, rs, $zero
            //--------------------------------------------------
            case NOT:
            {
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);

                uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                uint32_t funct = static_cast<uint32_t>(FUNCT_NOR);

                uint32_t word =
                    (op    << 26) |
                    (rs    << 21) |       // rs
                    (ZERO  << 16) |       // rt = $zero
                    (rd    << 11) |       // rd
                    (0u    <<  6) |
                    (funct <<  0);

                words.push_back(word);
                break;
            }

            //--------------------------------------------------
            // sgt rd, rs, rt   == slt rd, rt, rs
            //--------------------------------------------------
            case SGT:
            {
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rt = parse_register(toks[j++], line);

                // slt rd, rt, rs
                uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                uint32_t funct = static_cast<uint32_t>(FUNCT_SLT);

                uint32_t word =
                    (op    << 26) |
                    (rt    << 21) |       // rs = rt
                    (rs    << 16) |       // rt = rs
                    (rd    << 11) |       // rd
                    (0u    <<  6) |
                    (funct <<  0);

                words.push_back(word);
                break;
            }

            //--------------------------------------------------
            // sge rd, rs, rt
            //
            // rs >= rt  <=>  !(rs < rt)
            //   slt  rd, rs, rt
            //   xori rd, rd, 1
            //--------------------------------------------------
            case SGE:
            {
                int j = mnem_index + 1;
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rt = parse_register(toks[j++], line);

                // slt rd, rs, rt
                {
                    uint32_t op    = static_cast<uint32_t>(OP_RTYPE);
                    uint32_t funct = static_cast<uint32_t>(FUNCT_SLT);

                    uint32_t word =
                        (op    << 26) |
                        (rs    << 21) |       // rs
                        (rt    << 16) |       // rt
                        (rd    << 11) |       // rd
                        (0u    <<  6) |
                        (funct <<  0);

                    words.push_back(word);
                }

                // xori rd, rd, 1
                {
                    uint32_t op    = static_cast<uint32_t>(OP_XORI);
                    uint32_t imm16 = 1u;   // flip 0/1

                    uint32_t word =
                        (op    << 26) |
                        (rd    << 21) |       // rs = rd
                        (rd    << 16) |       // rt = rd
                        imm16;

                    words.push_back(word);
                }

                break;
            }

            // //--------------------------------------------------
            // // lw rt, label   (LW_LABEL)
            // //
            // //   la  $at, label
            // //   lw  rt, 0($at)
            // //--------------------------------------------------
            // case LW_LABEL:
            // {
            //     int j = mnem_index + 1;
            //     uint32_t rt = parse_register(toks[j++], line);
            //     ++j; // COMMA

            //     if (toks[j].type != IDENTIFIER)
            //         throw std::runtime_error("Expected label in lw pseudo");

            //     std::string label = toks[j++].get_string(line);

            //     if (!machine.has_label(label))
            //         throw std::runtime_error("Label not defined yet for lw: " + label);

            //     uint32_t addr = machine.lookup_label(label);
            //     uint16_t hi   = static_cast<uint16_t>((addr >> 16) & 0xFFFFu);
            //     uint16_t lo   = static_cast<uint16_t>(addr & 0xFFFFu);

            //     // la $at, label  -> lui / ori

            //     // lui $at, hi
            //     {
            //         uint32_t op = static_cast<uint32_t>(OP_LUI);
            //         uint32_t word =
            //             (op    << 26) |
            //             (ZERO  << 21) |
            //             (AT    << 16) |
            //             hi;
            //         words.push_back(word);
            //     }

            //     // ori $at, $at, lo
            //     {
            //         uint32_t op = static_cast<uint32_t>(OP_ORI);
            //         uint32_t word =
            //             (op    << 26) |
            //             (AT    << 21) |
            //             (AT    << 16) |
            //             lo;
            //         words.push_back(word);
            //     }

            //     // lw rt, 0($at)
            //     {
            //         uint32_t op     = static_cast<uint32_t>(OP_LW);
            //         uint16_t offset = 0u;

            //         uint32_t word =
            //             (op     << 26) |
            //             (AT     << 21) |   // base = $at
            //             (rt     << 16) |   // rt
            //             offset;

            //         words.push_back(word);
            //     }
            //     break;
            // }

            default:
                throw std::runtime_error("Unknown pseudo-instruction: " + mnem);
        }
    }

    std::vector<uint32_t>
    assemble_text_line(const std::vector<Token> & toks,
                       const std::string & line,
                       uint32_t current_pc)
    {
        std::vector<uint32_t> words;

        if (toks.empty())
        {
            std::cout << "No tokens to parse. Returning empty words";
            return words;
        }

        int i = 0;

        // LABEL:
        if (toks[i].type == IDENTIFIER &&
            i + 1 < toks.size() &&
            toks[i+1].type == COLON)
        {
            // defines labels when rest of line is garbage
            // (is that ok?)
            std::string label = toks[i].get_string(line);
            machine.define_label(label, current_pc);
            i += 2;
        }

        // empty line case should just be ignored in interpreter
        if (toks[i].type == EOL)
            return words; // empty or label-only line

        // instruction mnemonic must be IDENTIFIER here
        if (toks[i].type != IDENTIFIER)
        {
            throw std::runtime_error("Expected instruction mnemonic at start of line");
        }

        // get mnemonic string
        std::string mnemonic = toks[i].get_string(line);

        //==========================================================
        // PSEUDOINSTRUCTIONS
        //==========================================================
        if (is_pseudo(mnemonic))
        {
            // expand into 1 or more real instructions and return
            expand_pseudo(toks, i, line, current_pc, words);
            return words;
        }

        //==========================================================
        // REAL INSTRUCTIONS
        //==========================================================
        // get instruction info
        InstrInfo info = get_instr_info(mnemonic);
        const std::vector<TokenType> & pattern = get_pattern(info.type);

        int j = i + 1; // first token after mnemonic

        // check if pattern matches
        // the -1 is because checking the EOL is pointless
        for (std::size_t k = 0; k < pattern.size() - 1; ++k, ++j)
        {
            if (j >= (int)toks.size() || toks[j].type != pattern[k])
            {
                throw std::runtime_error("Unknown assembly pattern");
            }
        }

        // encode to 32-bit machine word and store to memory.
        // reset j to first token after mnemonic
        j = i + 1;

        uint32_t word = 0;

        switch (info.type)
        {
            case R3: // rd, rs, rt
            {
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // skip COMMA
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // skip COMMA
                uint32_t rt = parse_register(toks[j++], line);

                uint32_t op    = static_cast<uint32_t>(info.opcode);
                uint32_t funct = static_cast<uint32_t>(info.funct);

                word = (op    << 26) |
                    (rs    << 21) |
                    (rt    << 16) |
                    (rd    << 11) |
                    (0u    <<  6) |
                    (funct <<  0);
                words.push_back(word);
                break;
            }

            case RSHIFT: // rd, rt, shamt
            {
                uint32_t rd = parse_register(toks[j++], line);
                ++j; // skip COMMA
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // skip COMMA
                uint8_t shamt = parse_shamt(toks[j++], line);

                uint32_t op    = static_cast<uint32_t>(info.opcode);
                uint32_t funct = static_cast<uint32_t>(info.funct);

                word = (op           << 26) |
                    (0u           << 21) |  // rs = 0 for shifts
                    (rt           << 16) |
                    (rd           << 11) |
                    ((uint32_t)shamt <<  6) |
                    (funct        <<  0);
                words.push_back(word);
                break;
            }

            case I_ARITH: // rt, rs, imm
            {
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // COMMA

                int16_t imm16;
                if (info.opcode == OP_ANDI || info.opcode == OP_ORI)
                    imm16 = (int16_t)parse_imm16_unsigned(toks[j++], line);
                else
                    imm16 = parse_imm16_signed(toks[j++], line);

                uint32_t op = static_cast<uint32_t>(info.opcode);

                word = (op           << 26) |
                    (rs           << 21) |
                    (rt           << 16) |
                    ((uint16_t)imm16);
                words.push_back(word);
                break;
            }

            case I_LS: // rt, offset(rs)
            {
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // COMMA

                int16_t offset = parse_imm16_signed(toks[j++], line);

                ++j; // LPAREN
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // RPAREN

                uint32_t op = static_cast<uint32_t>(info.opcode);
        
                word = (op           << 26) |
                    (rs           << 21) |
                    (rt           << 16) |
                    ((uint16_t)offset);
                words.push_back(word);
                break;
            }

            case I_BRANCH: // rs, rt, label
            {
                uint32_t rs = parse_register(toks[j++], line);
                ++j; // COMMA
                uint32_t rt = parse_register(toks[j++], line);
                ++j; // COMMA

                if (toks[j].type != IDENTIFIER)
                    throw std::runtime_error("Expected label identifier in branch");

                std::string label = toks[j++].get_string(line);

                uint32_t op = static_cast<uint32_t>(info.opcode);

                // pc where this instruction will be stored
                uint32_t instr_pc = current_pc + 4u * static_cast<uint32_t>(words.size());

                uint16_t imm = 0; // placeholder

                // if label is already known, can fully encode
                if (machine.has_label(label))
                {
                    uint32_t target_addr = machine.lookup_label(label);

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

                    imm = static_cast<uint16_t>(offset & 0xFFFF);
                }

                word = (op << 26) |
                    (rs << 21) |
                    (rt << 16) |
                    imm;

                words.push_back(word);

                // if label was not yet known,
                // record a fixup so Machine can patch later
                if (!machine.has_label(label))
                {
                    machine.add_branch_fixup(instr_pc,
                                             info.opcode,
                                             rs,
                                             rt,
                                             label);
                }

                break;
            }
          
            case JUMP: // label
            {
                if (toks[j].type != IDENTIFIER)
                    throw std::runtime_error("Expected label identifier after jump");

                std::string label = toks[j++].get_string(line);

                uint32_t op = static_cast<uint32_t>(info.opcode);

                uint32_t instr_pc =
                    current_pc + 4u * static_cast<uint32_t>(words.size());

                uint32_t target_field = 0; // placeholder

                if (machine.has_label(label))
                {
                    uint32_t target_addr = machine.lookup_label(label);

                    if (target_addr & 0x3u)
                        throw std::runtime_error("Jump target not word-aligned");

                    target_field = (target_addr >> 2) & 0x03FFFFFFu;
                }

                word = (op << 26) | target_field;

                words.push_back(word);

                if (!machine.has_label(label))
                {
                    machine.add_jump_fixup(instr_pc,
                                           info.opcode,
                                           label);
                }

                break;
            }

            case SYSCALL:
            {
                uint32_t op    = static_cast<uint32_t>(info.opcode);
                uint32_t funct = static_cast<uint32_t>(info.funct);

                word = (op << 26) |
                    (0u << 21) |
                    (0u << 16) |
                    (0u << 11) |
                    (0u <<  6) |
                    (funct);

                words.push_back(word);
                break;
            }

            case JR_JALR: // jr rs   OR   jalr rs
            {
                // Pattern: REGISTER, EOL
                uint32_t rs = parse_register(toks[j++], line);

                uint32_t op    = static_cast<uint32_t>(info.opcode); // OP_RTYPE = 0
                uint32_t funct = static_cast<uint32_t>(info.funct);

                // jr:   rd = 0
                // jalr: rd = $ra (31), since you're using "jalr rs" syntax
                uint32_t rd = 0u;
                if (funct == static_cast<uint32_t>(FUNCT_JALR))
                {
                    rd = 31u; // $ra
                }

                word = (op    << 26) |
                    (rs    << 21) |  // jump target register
                    (0u    << 16) |  // rt = 0
                    (rd    << 11) |  // 0 for jr, 31 for jalr
                    (0u    <<  6) |  // shamt = 0
                    (funct <<  0);

                words.push_back(word);
                break;
            }
        
            default:
                throw std::runtime_error("Unknown instruction pattern");
        }

        return words;
    }

private:
    Machine & machine;
};

#endif // PARSER_H

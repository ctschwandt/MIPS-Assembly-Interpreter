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

    static uint8_t parse_shamt(const Token & tok, const std::string & line)
    {
        int64_t v = parse_int_token(tok, line);
        if (v < 0 || v > 31)
            throw std::runtime_error("Shift amount out of range 0..31");
        return static_cast<uint8_t>(v);
    }

    std::vector<uint32_t>
    assemble_text_line(const std::vector< Token > & toks,
                       const std::string & line,
                       uint32_t current_pc)
    {
        std::vector< uint32_t > words;

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

        // get instruction info
        InstrInfo info = get_instr_info(toks[i], line);
        const std::vector< TokenType > & pattern = get_pattern(info.type);

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

                word = (op              << 26) |
                    (0u              << 21) |  // rs = 0 for shifts
                    (rt              << 16) |
                    (rd              << 11) |
                    ((uint32_t)shamt <<  6) |
                    (funct           <<  0);
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

                word = (op              << 26) |
                    (rs              << 21) |
                    (rt              << 16) |
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
            
                word = (op              << 26) |
                    (rs              << 21) |
                    (rt              << 16) |
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
                //uint32_t instr_pc = current_pc;
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
            
            default:
                throw std::runtime_error("Unknown instruction pattern");
        }

        return words;
    }

private:
    Machine & machine;
};

#endif // PARSER_H

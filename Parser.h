// File  : Parser.h
// Author: Cole Schwandt

#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "Constants.h"
#include "Token.h"
#include "Lexer.h"

//==============================================================
// Instruction Types
//==============================================================
enum InstrType
{
    R3,        // R-format: rd, rs, rt        (add, sub, and, or, slt, ...)
    RSHIFT,    // R-format: rd, rt, shamt     (sll, srl, sra)
    I_ARITH,   // I-format: rt, rs, imm       (addi, andi, ori, slti, ...)
    I_LS,      // I-format: rt, offset(rs)    (lw, sw, lb, sb, ...)
    I_BRANCH,  // I-format: rs, rt, label     (beq, bne)
    JUMP,      // J-format: label             (j, jal)
    NUM_INSTRTYPE
};

//==============================================================
// Parser
//==============================================================
class Parser
{
public:
    // info about an instruction mnemonic
    struct InstrInfo
    {
        InstrType type;
        Opcode    opcode;  // 6-bit opcode field
        Funct     funct;   // 6-bit funct field (R-type); 0 / FUNCT_NONE for non-R
    };

    // mnemonic string -> InstrInfo
    static InstrInfo get_instr_info(const std::string & mnemonic)
    {
        static const std::unordered_map<std::string, InstrInfo> INSTR_TABLE = {
            // R-type arithmetic
            { "add",  { R3,      OP_RTYPE, FUNCT_ADD } },
            { "sub",  { R3,      OP_RTYPE, FUNCT_SUB } },
            { "and",  { R3,      OP_RTYPE, FUNCT_AND } },
            { "or",   { R3,      OP_RTYPE, FUNCT_OR  } },
            { "slt",  { R3,      OP_RTYPE, FUNCT_SLT } },

            // R-type shifts
            { "sll",  { RSHIFT,  OP_RTYPE, FUNCT_SLL } },
            { "srl",  { RSHIFT,  OP_RTYPE, FUNCT_SRL } },
            { "sra",  { RSHIFT,  OP_RTYPE, FUNCT_SRA } },

            // I-type arithmetic / logical
            { "addi", { I_ARITH, OP_ADDI,  FUNCT_NONE } },
            { "andi", { I_ARITH, OP_ANDI,  FUNCT_NONE } },
            { "ori",  { I_ARITH, OP_ORI,   FUNCT_NONE } },
            { "slti", { I_ARITH, OP_SLTI,  FUNCT_NONE } },

            // load/store
            { "lw",   { I_LS,    OP_LW,    FUNCT_NONE } },
            { "sw",   { I_LS,    OP_SW,    FUNCT_NONE } },
            { "lb",   { I_LS,    OP_LB,    FUNCT_NONE } },
            { "sb",   { I_LS,    OP_SB,    FUNCT_NONE } },

            // branches
            { "beq",  { I_BRANCH,OP_BEQ,   FUNCT_NONE } },
            { "bne",  { I_BRANCH,OP_BNE,   FUNCT_NONE } },

            // jumps
            { "j",    { JUMP,    OP_J,     FUNCT_NONE } },
            { "jal",  { JUMP,    OP_JAL,   FUNCT_NONE } },
        };

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
        static const std::vector<TokenType> PATTERNS[NUM_INSTRTYPE] = {
            // R3: rd, rs, rt          e.g. add $t0, $t1, $t2
            { REGISTER, COMMA, REGISTER, COMMA, REGISTER, EOL },

            // RSHIFT: rd, rt, shamt   e.g. sll $t0, $t1, 4
            { REGISTER, COMMA, REGISTER, COMMA, INT, EOL },

            // I_ARITH: rt, rs, imm    e.g. addi $t0, $t1, 42
            { REGISTER, COMMA, REGISTER, COMMA, INT, EOL },

            // I_LS: rt, offset(rs)    e.g. lw $t0, 4($t1)
            { REGISTER, COMMA, INT, LPAREN, REGISTER, RPAREN, EOL },

            // I_BRANCH: rs, rt, label e.g. beq $t0, $t1, LOOP
            { REGISTER, COMMA, REGISTER, COMMA, IDENTIFIER, EOL },

            // JUMP: label             e.g. j LOOP
            { IDENTIFIER, EOL },
        };

        return PATTERNS[int(type)];
    }

    static int parse_register(const Token & tok, const std::string & line)
    {
        if (tok.type != REGISTER)
            throw std::runtime_error("Expected register token");

        static const std::unordered_map<std::string, uint8_t> REG_TABLE =
            {
                {"$zero", 0}, {"$0", 0},

                {"$at",   1}, {"$1",  1},

                {"$v0",   2}, {"$2",  2},
                {"$v1",   3}, {"$3",  3},

                {"$a0",   4}, {"$4",  4},
                {"$a1",   5}, {"$5",  5},
                {"$a2",   6}, {"$6",  6},
                {"$a3",   7}, {"$7",  7},

                {"$t0",   8}, {"$8",  8},
                {"$t1",   9}, {"$9",  9},
                {"$t2",  10}, {"$10",10},
                {"$t3",  11}, {"$11",11},
                {"$t4",  12}, {"$12",12},
                {"$t5",  13}, {"$13",13},
                {"$t6",  14}, {"$14",14},
                {"$t7",  15}, {"$15",15},

                {"$s0",  16}, {"$16",16},
                {"$s1",  17}, {"$17",17},
                {"$s2",  18}, {"$18",18},
                {"$s3",  19}, {"$19",19},
                {"$s4",  20}, {"$20",20},
                {"$s5",  21}, {"$21",21},
                {"$s6",  22}, {"$22",22},
                {"$s7",  23}, {"$23",23},

                {"$t8",  24}, {"$24",24},
                {"$t9",  25}, {"$25",25},

                {"$k0",  26}, {"$26",26},
                {"$k1",  27}, {"$27",27},

                {"$gp",  28}, {"$28",28},
                {"$sp",  29}, {"$29",29},

                {"$fp",  30}, {"$s8",30}, {"$30",30},

                {"$ra",  31}, {"$31",31},
            };

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

    static uint32_t parse_line_core(const std::vector<Token> & toks, const std::string & s)
    {
        if (toks.empty())
            return 0;

        int i = 0;

        // LABEL:
        if (toks[i].type == IDENTIFIER &&
            i + 1 < (int)toks.size() &&
            toks[i+1].type == COLON)
        {
            // TODO: record label name: toks[i].get_string(s)
            i += 2;
        }

        if (i >= (int)toks.size() || toks[i].type == EOL)
            return 0; // empty or label-only line

        // instruction mnemonic must be IDENTIFIER here
        if (toks[i].type != IDENTIFIER)
        {
            throw std::runtime_error("Expected instruction mnemonic at start of line");
        }

        // get instruction info
        InstrInfo info = get_instr_info(toks[i], s);
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
            uint32_t rd = parse_register(toks[j++], s);
            ++j; // skip COMMA
            uint32_t rs = parse_register(toks[j++], s);
            ++j; // skip COMMA
            uint32_t rt = parse_register(toks[j++], s);

            uint32_t op    = static_cast<uint32_t>(info.opcode);
            uint32_t funct = static_cast<uint32_t>(info.funct);

            word = (op    << 26) |
                   (rs    << 21) |
                   (rt    << 16) |
                   (rd    << 11) |
                   (0u    <<  6) |
                   (funct <<  0);
            break;
        }

        case RSHIFT: // rd, rt, shamt
        {
            uint32_t rd = parse_register(toks[j++], s);
            ++j; // skip COMMA
            uint32_t rt = parse_register(toks[j++], s);
            ++j; // skip COMMA
            uint8_t shamt = parse_shamt(toks[j++], s);

            uint32_t op    = static_cast<uint32_t>(info.opcode);
            uint32_t funct = static_cast<uint32_t>(info.funct);

            word = (op              << 26) |
                   (0u              << 21) |  // rs = 0 for shifts
                   (rt              << 16) |
                   (rd              << 11) |
                   ((uint32_t)shamt <<  6) |
                   (funct           <<  0);
            break;
        }

        case I_ARITH: // rt, rs, imm
        {
            uint32_t rt = parse_register(toks[j++], s);
            ++j; // COMMA
            uint32_t rs = parse_register(toks[j++], s);
            ++j; // COMMA

            int16_t imm16;
            if (info.opcode == OP_ANDI || info.opcode == OP_ORI)
                imm16 = (int16_t)parse_imm16_unsigned(toks[j++], s);
            else
                imm16 = parse_imm16_signed(toks[j++], s);

            uint32_t op = static_cast<uint32_t>(info.opcode);

            word = (op              << 26) |
                   (rs              << 21) |
                   (rt              << 16) |
                   ((uint16_t)imm16);
            break;
        }

        case I_LS: // rt, offset(rs)
        {
            uint32_t rt = parse_register(toks[j++], s);
            ++j; // COMMA

            int16_t offset = parse_imm16_signed(toks[j++], s);

            ++j; // LPAREN
            uint32_t rs = parse_register(toks[j++], s);
            ++j; // RPAREN

            uint32_t op = static_cast<uint32_t>(info.opcode);

            word = (op              << 26) |
                   (rs              << 21) |
                   (rt              << 16) |
                   ((uint16_t)offset);
            break;
        }

        case I_BRANCH: // rs, rt, label
        {
            uint32_t rs = parse_register(toks[j++], s);
            ++j; // COMMA
            uint32_t rt = parse_register(toks[j++], s);
            ++j; // COMMA

            std::string label = toks[j++].get_string(s);

            // TODO: record (info.opcode, rs, rt, label, current_pc) for second pass
            return 0;
        }

        case JUMP: // label
        {
            std::string label = toks[j++].get_string(s);

            // TODO: record (info.opcode, label, current_pc) for second pass
            return 0;
        }
        }

        return word;
    }
};

#endif // PARSER_H

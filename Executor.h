// File  : Executor.h
// Author: Cole Schwandt

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "mybitlib.h"

// class Executor
// {
// public:
//     void execute(uint32_t word)
//     {
//         uint8_t opcode = (word >> 26) & mask_bits(6);
        
//         switch (opcode)
//         {
//             case OP_RTYPE: // R-format
//             {
//                 uint8_t rs = (word >> 21) & mask_bits(5);
//                 uint8_t rt = (word >> 16) & mask_bits(5);
//                 uint8_t rd = (word >> 11) & mask_bits(5);
//                 uint8_t shamt = (word >> 6) & mask_bits(5);
//                 uint8_t funct = word & mask_bits(6);

//                 switch (funct)
//                 {
//                     case FUNCT_ADD:
//                         // regs[rd] = regs[rs] + regs[rt];
//                         break;

//                     case FUNCT_SUB:
//                         // regs[rd] = regs[rs] - regs[rt];
//                         break;

//                     case FUNCT_AND:
//                         // regs[rd] = regs[rs] & regs[rt];
//                         break;

//                     case FUNCT_OR:
//                         // regs[rd] = regs[rs] | regs[rt];
//                         break;

//                     case FUNCT_SLT:
//                         // regs[rd] = (int32_t)regs[rs] < (int32_t)regs[rt];
//                         break;

//                     case FUNCT_SLL:
//                         // regs[rd] = regs[rt] << shamt;
//                         break;

//                     case FUNCT_SRL:
//                         // regs[rd] = (uint32_t)regs[rt] >> shamt;
//                         break;

//                     case FUNCT_SRA:
//                         // regs[rd] = (int32_t)regs[rt] >> shamt;
//                         break;
//                 }
//                 break;
//             }
//         }
//     }
// };

#endif // EXECUTOR_H

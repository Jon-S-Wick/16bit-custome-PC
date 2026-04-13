#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "control.h"
#include "instruction.h"
#include "x16.h"
#include "trap.h"
#include "decode.h"


// Update condition code based on result
void update_cond(x16_t* machine, reg_t reg) {
    uint16_t result = x16_reg(machine, reg);
    if (result == 0) {
        x16_set(machine, R_COND, FL_ZRO);
    } else if (is_negative(result)) {
        x16_set(machine, R_COND, FL_NEG);
    } else {
        x16_set(machine, R_COND, FL_POS);
    }
}



// Execute a single instruction in the given X16 machine. Update
// memory and registers as required. PC is advanced as appropriate.
// Return 0 on success, or -1 if an error or HALT is encountered.
int execute_instruction(x16_t* machine) {
    // Fetch the instruction and advance the program counter
    uint16_t pc = x16_pc(machine);
    uint16_t instruction = x16_memread(machine, pc);
    x16_set(machine, R_PC, pc + 1);

    if (LOG) {
        fprintf(LOGFP, "0x%x: %s\n", pc, decode(instruction));
    }

    // Variables we might need in various instructions
    reg_t dst, src1, src2, base;
    uint16_t result, indirect, offset, imm, cond, jsrflag, op1, op2;

    // Decode the instruction
    uint16_t opcode = getopcode(instruction);
    switch (opcode) {
        case OP_ADD:
            dst = getbits(instruction, 9, 3);
            src1 = getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);

            if (getbit(instruction, 5) == 0){
                src2 = getbits(instruction, 0, 3);
                op2 = x16_reg(machine, src2);

            } else {
                imm = getbits(instruction, 0, 5);
                op2 = sign_extend(imm, 5);
            }
            result = op1 + op2;
            // printf("result %d op1 %d op2 %d\n",result, op1, op2);
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;

        case OP_AND:
            dst = getbits(instruction, 9, 3);
            src1 = getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);

            if (getbit(instruction, 5) == 0){
                src2 = getbits(instruction, 0, 3);
                op2 = x16_reg(machine, src2);

            } else {
                imm = getbits(instruction, 0, 5);
                op2 = sign_extend(imm, 5);
            }
            result = op1 & op2;
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;

        case OP_NOT:
            dst = getbits(instruction, 9, 3);
            src1 = getbits(instruction, 6, 3);
            result = ~x16_reg(machine, src1);
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;


        case OP_BR:
            bool n = getbit(instruction, 11);
            bool z = getbit(instruction, 10);
            bool p = getbit(instruction, 9);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            cond = x16_reg(machine, R_COND);
            if (((n == z) && (z == p))
                || ((n && (cond == FL_NEG))
                || (z && (cond == FL_ZRO))
                || (p && (cond == FL_POS)))){
                x16_set(machine, R_PC, x16_reg(machine, R_PC) + offset);
            }
            break;

        case OP_JMP:
            base = getbits(instruction, 6, 3);
            result = x16_reg(machine, base);

            x16_set(machine, R_PC, result);

            break;

        case OP_JSR:
            bool jsr = getbit(instruction, 11);
            pc = x16_reg(machine, R_PC);
            x16_set(machine, R_R7, pc);
            if (jsr){
                offset = sign_extend(getbits(instruction, 0, 11), 11);
                x16_set(machine, R_PC, offset + pc);
            } else {
                base =  x16_reg(machine, getbits(instruction, 6, 3));
                x16_set(machine, R_PC, base);
            }
            break;

        case OP_LD:
            pc = x16_reg(machine, R_PC);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            result = x16_memread(machine, pc + offset);
            dst = getbits(instruction, 9, 3);
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;

        case OP_LDI:
            pc = x16_reg(machine, R_PC);
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            result = x16_memread(machine, pc + offset);
            result = x16_memread(machine, result);
            dst = getbits(instruction, 9, 3);
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;

        case OP_LDR:
            pc = x16_reg(machine, R_PC);
            offset = sign_extend(getbits(instruction, 0, 5), 5);
            src1 = getbits(instruction, 6, 3);
            base = x16_reg(machine, src1);
            result = x16_memread(machine, base + offset);
            dst = getbits(instruction, 9, 3);
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;

        case OP_LEA:
            pc = x16_reg(machine, R_PC);
            offset = sign_extend(getbits(instruction, 0, 8), 8);
            dst = getbits(instruction, 9, 3);
            result = pc + offset;
            x16_set(machine, dst, result);
            x16_setcc(machine, result);
            break;

        case OP_ST:
            pc = x16_reg(machine, R_PC);
            src1 = getbits(instruction, 9, 3);
            result = x16_reg(machine, src1);
            offset = sign_extend(getbits(instruction, 0, 8), 8);
            x16_memwrite(machine, pc+offset, result);
            break;

        case OP_STI:
            pc = x16_reg(machine, R_PC);
            src1 = getbits(instruction, 9, 3);
            result = x16_reg(machine, src1);
            offset = sign_extend(getbits(instruction, 0, 8), 8);
            base = x16_memread(machine, pc+offset);
            x16_memwrite(machine, base, result);
            break;

        case OP_STR:
            src1 = getbits(instruction, 9, 3);
            base =  x16_reg(machine, getbits(instruction, 6, 3));
            offset = sign_extend(getbits(instruction, 0, 6), 6);
            result = x16_reg(machine, src1);
            x16_memwrite(machine, base + offset, result);
            break;

        case OP_TRAP:
            // Execute the trap -- do not rewrite
            return trap(machine, instruction);

        case OP_RES:
        case OP_RTI:
        default:
            // Bad codes, never used
            abort();
    }

    return 0;
}

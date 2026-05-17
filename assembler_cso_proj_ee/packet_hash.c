#define _CRT_SECURE_NO_WARNINGS // Disables scanf/printf warnings in VS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Opcode table based on your SIMP processor spec
const char* OPCODE_NAMES[] = {
    "add", "sub", "mul", "mac", "and", "or", "xor", "sll", "sra", "srl",
    "beq", "bne", "blt", "bgt", "ble", "bge", "jal", "lw", "sw", "reti",
    "in", "out", "halt"
};

typedef struct {
    uint8_t opcode;
    uint8_t rd;
    uint8_t rs;
    uint8_t rt;
    uint16_t imm1; // 12-bit immediate from Row 1
    uint32_t imm2; // 32-bit immediate from Row 2 (optional)
} Instruction;

// --- Function to pack values INTO a 32-bit instruction (Assembler Logic) ---
uint32_t pack_instruction(Instruction instr) {
    uint32_t packet = 0;
    packet |= ((uint32_t)(instr.opcode & 0xFF) << 24);
    packet |= ((uint32_t)(instr.rd & 0x0F) << 20);
    packet |= ((uint32_t)(instr.rs & 0x0F) << 16);
    packet |= ((uint32_t)(instr.rt & 0x0F) << 12);
    packet |= (instr.imm1 & 0x0FFF);
    return packet;
}

// --- Function to parse bits FROM a 32-bit row (Disassembler/Simulator Logic) ---
void parse_instruction(uint32_t row1, int row2_exists, uint32_t row2) {
    Instruction instr;

    // Extracting fields using masks and shifts
    instr.opcode = (row1 >> 24) & 0xFF;
    instr.rd = (row1 >> 20) & 0x0F;
    instr.rs = (row1 >> 16) & 0x0F;
    instr.rt = (row1 >> 12) & 0x0F;
    instr.imm1 = (row1 & 0x0FFF);

    printf("--- Decoded Instruction ---\n");
    if (instr.opcode < 23) {
        printf("Operation: %s (Opcode %d)\n", OPCODE_NAMES[instr.opcode], instr.opcode);
    }
    else {
        printf("Operation: UNKNOWN (%d)\n", instr.opcode);
    }

    printf("Registers: rd=$r%d, rs=$r%d, rt=$r%d\n", instr.rd, instr.rs, instr.rt);
    printf("Imm1 (12-bit): 0x%03X (%d)\n", instr.imm1, instr.imm1);

    if (row2_exists) {
        instr.imm2 = row2;
        printf("Imm2 (32-bit): 0x%08X\n", instr.imm2);
    }
    printf("---------------------------\n\n");
}

int main() {
    // Example 1: Parsing an existing 32-bit packet (hex 0x11234ABC)
    // Opcode 0x11 (17 = lw), Rd=2, Rs=3, Rt=4, Imm1=ABC
    uint32_t test_packet = 0x11234ABC;
    parse_instruction(test_packet, 0, 0);

    // Example 2: Packing an instruction manually
    Instruction myInstr;
    myInstr.opcode = 0;    // add
    myInstr.rd = 5;        // $r5
    myInstr.rs = 1;        // $r1
    myInstr.rt = 2;        // $r2
    myInstr.imm1 = 0x000;  // no immediate

    uint32_t packed = pack_instruction(myInstr);
    printf("Packed ADD $r5, $r1, $r2 into hex: 0x%08X\n", packed);

    return 0;
}
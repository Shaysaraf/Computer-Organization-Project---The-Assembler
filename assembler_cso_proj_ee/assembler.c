#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

// --- Constants based on Project Specs ---
#define MAX_MEM 4096
#define MAX_LABELS 1024
#define MAX_LINE 500

typedef struct {
    char name[50];
    int address;
} Label;

typedef struct {
    const char* name;
    int opcode;
} Opcode;

// --- Global Tables ---
Label label_table[MAX_LABELS];
int label_count = 0;
uint32_t memory[MAX_MEM] = { 0 }; // Initialized to zero as per specs
int max_pc_reached = 0;

Opcode OPCODE_TABLE[] = {
    {"add", 0}, {"sub", 1}, {"mul", 2}, {"mac", 3}, {"and", 4}, {"or", 5}, {"xor", 6},
    {"sll", 7}, {"sra", 8}, {"srl", 9}, {"beq", 10}, {"bne", 11}, {"blt", 12}, {"bgt", 13},
    {"ble", 14}, {"bge", 15}, {"jal", 16}, {"lw", 17}, {"sw", 18}, {"reti", 19}, {"in", 20},
    {"out", 21}, {"halt", 22}
};

// --- Helper: Clean and Tokenize ---
char* clean_token(char* token) {
    if (!token) return NULL;
    // Remove trailing commas or whitespace
    size_t len = strlen(token);
    while (len > 0 && (token[len - 1] == ',' || isspace(token[len - 1]))) {
        token[--len] = '\0';
    }
    // Remove leading whitespace
    while (*token && isspace(*token)) token++;
    return token;
}

// --- Helper: Parse Register (Resolves $sp, $ra, etc.) ---
int parse_reg(char* str) {
    str = clean_token(str);
    if (!str) return 0;
    if (_stricmp(str, "$zero") == 0) return 0;
    if (_stricmp(str, "$imm1") == 0) return 1;
    if (_stricmp(str, "$imm2") == 0) return 2;
    if (_stricmp(str, "$v0") == 0) return 3;
    if (_stricmp(str, "$a0") == 0) return 4;
    if (_stricmp(str, "$a1") == 0) return 5;
    if (_stricmp(str, "$a2") == 0) return 6;
    if (_stricmp(str, "$a3") == 0) return 7;
    if (_stricmp(str, "$t0") == 0) return 8;
    if (_stricmp(str, "$t1") == 0) return 9;
    if (_stricmp(str, "$t2") == 0) return 10;
    if (_stricmp(str, "$s0") == 0) return 11;
    if (_stricmp(str, "$s1") == 0) return 12;
    if (_stricmp(str, "$gp") == 0) return 13;
    if (_stricmp(str, "$sp") == 0) return 14;
    if (_stricmp(str, "$ra") == 0) return 15;
    // Handle numeric registers like $14 or 14
    while (*str && !isdigit(*str) && *str != '-') str++;
    return atoi(str);
}

// --- Helper: Resolve Immediates (Numbers or Labels) ---
int resolve_val(char* str) {
    str = clean_token(str);
    if (!str) return 0;
    // Check if it's a Label
    for (int i = 0; i < label_count; i++) {
        if (strcmp(str, label_table[i].name) == 0) return label_table[i].address;
    }
    // Handle Hex (0x) or Decimal
    return (int)strtol(str, NULL, 0);
}

// --- PASS 1: Calculate Addresses and Map Labels ---
void pass_one(const char* filename) {
    FILE* f = fopen(filename, "r");
    char line[MAX_LINE];
    int pc = 0;

    while (fgets(line, sizeof(line), f)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        // Check BEFORE strtok modifies line
        int uses_imm2 = (strstr(line, "$imm2") || strstr(line, "$IMM2")) ? 1 : 0;

        char* token = strtok(line, " \t\n,");
        if (!token) continue;

        // Check for Label definition (ends with :)
        if (token[strlen(token) - 1] == ':') {
            token[strlen(token) - 1] = '\0';
            strcpy(label_table[label_count].name, token);
            label_table[label_count].address = pc;
            label_count++;
            token = strtok(NULL, " \t\n,");
            if (!token) continue;
        }

        // Handle Directives
        if (token[0] == '.') continue;

        // Advance PC by 1 or 2
        pc += (uses_imm2 ? 2 : 1);
    }
    fclose(f);
}

// --- PASS 2: Assemble to Memory ---
void pass_two(const char* filename) {
    FILE* f = fopen(filename, "r");
    char line[MAX_LINE];
    int pc = 0;

    while (fgets(line, sizeof(line), f)) {
        char* comment = strchr(line, '#');
        if (comment) *comment = '\0';

        char* token = strtok(line, " \t\n,");
        if (!token) continue;

        // Skip label if present
        if (token[strlen(token) - 1] == ':') {
            token = strtok(NULL, " \t\n,");
            if (!token) continue;
        }

        // --- Handle Pseudo-Instructions ---
        if (strcmp(token, ".word") == 0) {
            int addr = resolve_val(strtok(NULL, " \t\n,"));
            uint32_t data = (uint32_t)resolve_val(strtok(NULL, " \t\n,"));
            memory[addr] = data;
            if (addr >= max_pc_reached) max_pc_reached = addr + 1;
            continue;
        }

        // --- Handle Instructions ---
        int opcode_val = -1;
        for (int i = 0; i < 23; i++) {
            if (_stricmp(token, OPCODE_TABLE[i].name) == 0) {
                opcode_val = OPCODE_TABLE[i].opcode;
                break;
            }
        }

        if (opcode_val != -1) {
            int rd = parse_reg(strtok(NULL, " \t\n,"));
            int rs = parse_reg(strtok(NULL, " \t\n,"));
            int rt = parse_reg(strtok(NULL, " \t\n,"));
            int imm1_val = resolve_val(strtok(NULL, " \t\n,"));
            int imm2_val = resolve_val(strtok(NULL, " \t\n,"));

            // Pack Row 1: [opcode 31:24] [rd 23:20] [rs 19:16] [rt 15:12] [imm12 11:0]
            uint32_t row1 = 0;
            row1 |= (opcode_val & 0xFF) << 24;
            row1 |= (rd & 0x0F) << 20;
            row1 |= (rs & 0x0F) << 16;
            row1 |= (rt & 0x0F) << 12;
            row1 |= (imm1_val & 0x0FFF);

            memory[pc++] = row1;

            // Check if rs, rt, or rd was $imm2 to trigger Second Row
            if (rd == 2 || rs == 2 || rt == 2) {
                memory[pc++] = (uint32_t)imm2_val;
            }
            if (pc > max_pc_reached) max_pc_reached = pc;
        }
    }
    fclose(f);
}

// --- Final Step: Write memin.txt ---
void write_memin(const char* filename) {
    FILE* f = fopen(filename, "w");
    // Write up to max_pc_reached or full memory per specs
    for (int i = 0; i < max_pc_reached; i++) {
        fprintf(f, "%08X\n", memory[i]);
    }
    fclose(f);
}

int main(int argc, char* argv[]) {
    // Command line arguments for test: asm.exe program.asm memin.txt
    if (argc < 3) {
        printf("Usage: asm.exe <input.asm> <output.txt>\n");
        return 1;
    }

    pass_one(argv[1]);
    pass_two(argv[1]);
    write_memin(argv[2]);

    printf("Assembly complete. Labels found: %d. Memory lines: %d\n", label_count, max_pc_reached);
    return 0;
}

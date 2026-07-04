#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MEM_SIZE 4096
#define NUM_REGS 16
#define NUM_HW_REGS 23
#define DISK_SECTORS 128
#define DISK_SECTOR_SIZE 128

// Architectural-Hardware State Variables
int32_t R[NUM_REGS];       
int32_t MEM[MEM_SIZE];     
int32_t PC = 0;             
int32_t HW_R[NUM_HW_REGS]; 
uint8_t monitor[256][256]; 

// Disk Storage Matrix (128 sectors * 128 words) 
int32_t DISK[DISK_SECTORS][DISK_SECTOR_SIZE];

uint64_t cycle_count = 0;
int in_interrupt_handler = 0; 

// Dynamic array to store asynchronous irq2 cycles inputs
uint64_t *irq2_cycles = NULL;
int irq2_count = 0;
int irq2_idx = 0;

int disk_timer = 0;
int debug_mode = 0;

// Global File Handles for Trace Streaming Logging
FILE *f_trace = NULL;
FILE *f_hwregtrace = NULL;
FILE *f_leds = NULL;
FILE *f_display7seg = NULL;

typedef struct {
    int opcode;
    int rd;
    int rs;
    int rt;
    int32_t imm12;
    int is_imm2;    
    int32_t imm2_val;
} Instruction;

// Helper to resolve and construct directory file path strings safely
void join_path(char *dest, const char *dir, const char *file) {
    int len = strlen(dir);
    if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\')) {
        sprintf(dest, "%s%s", dir, file);
    } else {
        sprintf(dest, "%s/%s", dir, file);
    }
}

// Maps Hardware IO indices directly to their textual literal names
const char* get_hw_name(int index) {
    switch(index) {
        case 0:  return "irq0enable";   case 1:  return "irq1enable";   case 2:  return "irq2enable";
        case 3:  return "irq0status";   case 4:  return "irq1status";   case 5:  return "irq2status";
        case 6:  return "irqhandler";   case 7:  return "irqreturn";    case 8:  return "clks";
        case 9:  return "leds";         case 10: return "display7seg";  case 11: return "timerenable";
        case 12: return "timercurrent"; case 13: return "timermax";     case 14: return "diskcmd";
        case 15: return "disksector";   case 16: return "diskbuffer";   case 17: return "diskstatus";
        case 20: return "monitoraddr";  case 21: return "monitordata";  case 22: return "monitorcmd";
        default: return "reserved";
    }
}

int32_t sign_extend_12(int32_t imm12) {
    if (imm12 & 0x800) {
        return imm12 | 0xFFFFF000;
    }
    return imm12 & 0x00000FFF;
}

void load_memory(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] Could not open memory file: %s\n", filename);
        exit(1);
    }
    char line[64];
    int addr = 0;
    while (fgets(line, sizeof(line), file) && addr < MEM_SIZE) {
        // FIX: Used strtoull to safely cast full 32-bit hex without overflow
        MEM[addr] = (int32_t)strtoull(line, NULL, 16);
        addr++;
    }
    fclose(file);
}

void load_disk(const char *filename) {
    memset(DISK, 0, sizeof(DISK));
    FILE *file = fopen(filename, "r");
    if (!file) return; 

    char line[64];
    int sector = 0, word_idx = 0;
    while (fgets(line, sizeof(line), file) && sector < DISK_SECTORS) {
        // FIX: Used strtoull to safely cast full 32-bit hex without overflow
        DISK[sector][word_idx] = (int32_t)strtoull(line, NULL, 16);
        word_idx++;
        if (word_idx >= DISK_SECTOR_SIZE) {
            word_idx = 0;
            sector++;
        }
    }
    fclose(file);
}

void load_irq2_inputs(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return;

    char line[64];
    while (fgets(line, sizeof(line), file)) {
        if (strlen(line) > 1 && line[0] != '\n') {
            irq2_count++;
        }
    }
    if (irq2_count > 0) {
        irq2_cycles = (uint64_t *)malloc(irq2_count * sizeof(uint64_t));
        fseek(file, 0, SEEK_SET);
        int i = 0;
        while (fgets(line, sizeof(line), file) && i < irq2_count) {
            irq2_cycles[i] = (uint64_t)strtoull(line, NULL, 10);
            i++;
        }
    }
    fclose(file);
}

// Unified tick function to call once per actual clock cycle
void tick_peripherals(uint64_t current_cycle) {
    HW_R[8] = (int32_t)(current_cycle & 0xFFFFFFFF); // Update clock hardware register

    // Handle Timer
    if (HW_R[11] == 1) { 
        if (HW_R[12] >= HW_R[13]) { 
            HW_R[3] = 1;  
            HW_R[12] = 0; 
        } else {
            HW_R[12]++;   
        }
    }

    // Handle Disk DMA
    if (HW_R[14] != 0 && HW_R[17] == 1) { 
        disk_timer++;
        if (disk_timer >= 1024) { 
            int sector = HW_R[15];
            int mem_buffer = HW_R[16];
            
            if (HW_R[14] == 1) { // Read
                for (int i = 0; i < DISK_SECTOR_SIZE; i++) {
                    if ((mem_buffer + i) < MEM_SIZE) MEM[mem_buffer + i] = DISK[sector][i];
                }
            } else if (HW_R[14] == 2) { // Write
                for (int i = 0; i < DISK_SECTOR_SIZE; i++) {
                    if ((mem_buffer + i) < MEM_SIZE) DISK[sector][i] = MEM[mem_buffer + i];
                }
            }
            
            HW_R[14] = 0; 
            HW_R[17] = 0; 
            HW_R[4] = 1;  
            disk_timer = 0;
        }
    }

    // Handle async irq2
    if (irq2_idx < irq2_count && current_cycle == irq2_cycles[irq2_idx]) {
        HW_R[5] = 1; 
        irq2_idx++;
    }
}

int main(int argc, char *argv[]) {
    char paths[14][1024];
    int folder_arg_idx = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (folder_arg_idx == -1) {
            folder_arg_idx = i;
        }
    }

    // Resolve unified calling strings paths
    if (argc >= 2 && folder_arg_idx != -1 && (argc == 2 || (argc == 3 && debug_mode))) {
        printf("Directory Target Routing: Mapping from internal files inside '%s'...\n", argv[folder_arg_idx]);
        join_path(paths[1],  argv[folder_arg_idx], "memin.txt");
        join_path(paths[2],  argv[folder_arg_idx], "diskin.txt");
        join_path(paths[3],  argv[folder_arg_idx], "irq2in.txt");
        join_path(paths[4],  argv[folder_arg_idx], "memout.txt");
        join_path(paths[5],  argv[folder_arg_idx], "regout.txt");
        join_path(paths[6],  argv[folder_arg_idx], "trace.txt");
        join_path(paths[7],  argv[folder_arg_idx], "hwregtrace.txt");
        join_path(paths[8],  argv[folder_arg_idx], "cycles.txt");
        join_path(paths[9],  argv[folder_arg_idx], "leds.txt");
        join_path(paths[10], argv[folder_arg_idx], "display7seg.txt");
        join_path(paths[11], argv[folder_arg_idx], "diskout.txt");
        join_path(paths[12], argv[folder_arg_idx], "monitor.txt");
        join_path(paths[13], argv[folder_arg_idx], "monitor.yuv");
    } else if (argc >= 14) {
        int idx = 1;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) continue;
            if (idx <= 13) strcpy(paths[idx++], argv[i]);
        }
    } else {
        printf("Usage Errors:\n  %s <folder_name> [-d]\n  %s <memin.txt> ... <monitor.yuv> [-d]\n", argv[0], argv[0]);
        return 1;
    }

    memset(R, 0, sizeof(R));
    memset(MEM, 0, sizeof(MEM));
    memset(HW_R, 0, sizeof(HW_R));
    memset(monitor, 0, sizeof(monitor));

    load_memory(paths[1]);        
    load_disk(paths[2]);
    load_irq2_inputs(paths[3]);    

    f_trace = fopen(paths[6], "w");
    f_hwregtrace = fopen(paths[7], "w");
    f_leds = fopen(paths[9], "w");
    f_display7seg = fopen(paths[10], "w");

    while (1) {
        // 1. Tick Peripherals for the first cycle of this instruction
        tick_peripherals(cycle_count);

        // 2. Service Interrupt Signals (Before Fetch)
        int irq = (HW_R[0] & HW_R[3]) | (HW_R[1] & HW_R[4]) | (HW_R[2] & HW_R[5]); 
        if (irq == 1 && !in_interrupt_handler) { 
            in_interrupt_handler = 1;
            HW_R[7] = PC;         
            PC = HW_R[6] & 0xFFF; 
        }

        // 3. Fetch Instruction
        int32_t current_pc = PC; 
        int32_t raw_word = MEM[current_pc];
        
        Instruction inst;
        inst.opcode = (raw_word >> 24) & 0xFF;
        inst.rd     = (raw_word >> 20) & 0x0F;
        inst.rs     = (raw_word >> 16) & 0x0F;
        inst.rt     = (raw_word >> 12) & 0x0F;
        inst.imm12  = raw_word & 0x0FFF;
        inst.is_imm2 = (inst.rd == 2 || inst.rs == 2 || inst.rt == 2); 
        
        int32_t next_pc = current_pc + 1; 
        
        if (inst.is_imm2) {
            inst.imm2_val = MEM[next_pc]; 
            next_pc++; 
        }

        // Stream Trace logs accurately to output file
        if (f_trace) {
            int32_t r1_log = sign_extend_12(inst.imm12);
            int32_t r2_log = inst.is_imm2 ? inst.imm2_val : 0;
            int cycle_for_trace = inst.is_imm2 ? cycle_count + 1 : cycle_count;
            
            fprintf(f_trace, "%08X %03X %08X 00000000 %08X %08X ", 
                    cycle_for_trace, current_pc, raw_word, r1_log, r2_log);
            for (int i = 3; i < NUM_REGS; i++) {
                fprintf(f_trace, "%08X%c", R[i], i == 15 ? '\n' : ' ');
            }
        }

        if (inst.opcode == 22) { // halt 
            cycle_count++; // Halt takes 1 cycle
            break;
        }

        // 4. Tick secondary cycle for 2-word instructions
        if (inst.is_imm2) {
            cycle_count++;
            tick_peripherals(cycle_count);
        }

        // 5. Execute 
        int32_t val_rd = R[inst.rd];
        int32_t val_rs = R[inst.rs];
        int32_t val_rt = R[inst.rt];

        if (inst.rd == 1) val_rd = sign_extend_12(inst.imm12); 
        if (inst.rs == 1) val_rs = sign_extend_12(inst.imm12); 
        if (inst.rt == 1) val_rt = sign_extend_12(inst.imm12); 
        
        if (inst.rd == 2) val_rd = inst.imm2_val; 
        if (inst.rs == 2) val_rs = inst.imm2_val; 
        if (inst.rt == 2) val_rt = inst.imm2_val; 

        int pc_branch_taken = 0;
        int32_t branch_target = 0;

        switch (inst.opcode) {
            case 0: R[inst.rd] = val_rs + val_rt; break;  
            case 1: R[inst.rd] = val_rs - val_rt; break;  
            case 2: R[inst.rd] = val_rs * val_rt; break;  
            case 3: R[inst.rd] = (val_rs * val_rt) + val_rd; break; // FIX: using val_rd
            case 4: R[inst.rd] = val_rs & val_rt; break;  
            case 5: R[inst.rd] = val_rs | val_rt; break;  
            case 6: R[inst.rd] = val_rs ^ val_rt; break;  
            case 7: R[inst.rd] = val_rs << val_rt; break;  
            case 8: R[inst.rd] = val_rs >> val_rt; break;  
            case 9: R[inst.rd] = (int32_t)((uint32_t)val_rs >> val_rt); break;  
            
            case 10: if (val_rs == val_rt) { branch_target = val_rd & 0xFFF; pc_branch_taken = 1; } break; 
            case 11: if (val_rs != val_rt) { branch_target = val_rd & 0xFFF; pc_branch_taken = 1; } break; 
            case 12: if (val_rs <  val_rt) { branch_target = val_rd & 0xFFF; pc_branch_taken = 1; } break; 
            case 13: if (val_rs >  val_rt) { branch_target = val_rd & 0xFFF; pc_branch_taken = 1; } break; 
            case 14: if (val_rs <= val_rt) { branch_target = val_rd & 0xFFF; pc_branch_taken = 1; } break; 
            case 15: if (val_rs >= val_rt) { branch_target = val_rd & 0xFFF; pc_branch_taken = 1; } break; 
            
            case 16: // jal 
                R[inst.rd] = next_pc; 
                branch_target = val_rs & 0xFFF; 
                pc_branch_taken = 1; 
                break;
                
            case 17: // lw 
                if ((val_rs + val_rt) >= 0 && (val_rs + val_rt) < MEM_SIZE) {
                    R[inst.rd] = MEM[val_rs + val_rt];
                }
                break;
                
            case 18: // sw 
                if ((val_rs + val_rt) >= 0 && (val_rs + val_rt) < MEM_SIZE) {
                    MEM[val_rs + val_rt] = val_rd; // FIX: using val_rd
                }
                break;
                
            case 19: // reti 
                branch_target = HW_R[7] & 0xFFF; 
                in_interrupt_handler = 0; 
                pc_branch_taken = 1;
                break;

            case 20: // in 
                {
                    int idx = val_rs + val_rt;
                    if (idx >= 0 && idx < NUM_HW_REGS) {
                        R[inst.rd] = (idx == 22) ? 0 : HW_R[idx]; 
                        if (f_hwregtrace) fprintf(f_hwregtrace, "%08llX READ %s %08X\n", cycle_count, get_hw_name(idx), R[inst.rd]); 
                    }
                }
                break;

            case 21: // out 
                {
                    int idx = val_rs + val_rt;
                    if (idx >= 0 && idx < NUM_HW_REGS) {
                        int32_t old_val = HW_R[idx];
                        HW_R[idx] = val_rd; // FIX: using val_rd
                        if (f_hwregtrace) fprintf(f_hwregtrace, "%08llX WRITE %s %08X\n", cycle_count, get_hw_name(idx), HW_R[idx]); 
                        
                        if (idx == 9 && HW_R[9] != old_val && f_leds) { 
                            fprintf(f_leds, "%08llX %08X\n", cycle_count, HW_R[9]);
                        }
                        if (idx == 10 && HW_R[10] != old_val && f_display7seg) { 
                            fprintf(f_display7seg, "%08llX %08X\n", cycle_count, HW_R[10]);
                        }
                        if (idx == 14 && HW_R[14] != 0) {
                            HW_R[17] = 1; 
                            disk_timer = 0;
                        }
                        if (idx == 22 && HW_R[22] == 1) { 
                            int m_addr = HW_R[20];
                            int m_data = HW_R[21];
                            int y = m_addr / 256;
                            int x = m_addr % 256;
                            if (x >= 0 && x < 256 && y >= 0 && y < 256) {
                                monitor[y][x] = (uint8_t)m_data;
                            }
                            HW_R[22] = 0; 
                        }
                    }
                }
                break;
        }

        if (pc_branch_taken) {
            PC = branch_target;
        } else {
            PC = next_pc;
        }

        R[0] = 0; R[1] = 0; R[2] = 0; // Constants protected 
        
        // 6. Conclude Instruction Frame
        cycle_count++;
    }

    // Write final operational images out 
    FILE *f_memout = fopen(paths[4], "w");
    for (int i = 0; i < MEM_SIZE; i++) fprintf(f_memout, "%08X\n", MEM[i]);
    fclose(f_memout);

    FILE *f_regout = fopen(paths[5], "w");
    for (int i = 3; i < NUM_REGS; i++) fprintf(f_regout, "%08X\n", R[i]);
    fclose(f_regout);

    FILE *f_cycles = fopen(paths[8], "w");
    fprintf(f_cycles, "%08llX\n", cycle_count);
    fclose(f_cycles);

    FILE *f_diskout = fopen(paths[11], "w");
    for (int s = 0; s < DISK_SECTORS; s++) {
        for (int w = 0; w < DISK_SECTOR_SIZE; w++) {
            fprintf(f_diskout, "%08X\n", DISK[s][w]);
        }
    }
    fclose(f_diskout);

    FILE *f_monitor = fopen(paths[12], "w");
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            fprintf(f_monitor, "%02X\n", monitor[y][x]);
        }
    }
    fclose(f_monitor);

    FILE *f_yuv = fopen(paths[13], "wb");
    if (f_yuv) {
        for (int y = 0; y < 256; y++) {
            fwrite(monitor[y], sizeof(uint8_t), 256, f_yuv);
        }
        fclose(f_yuv);
    }

    if (f_trace) fclose(f_trace);
    if (f_hwregtrace) fclose(f_hwregtrace);
    if (f_leds) fclose(f_leds);
    if (f_display7seg) fclose(f_display7seg);
    if (irq2_cycles) free(irq2_cycles);

    printf("Simulation completed successfully. All outputs generated.\n");
    return 0;
}
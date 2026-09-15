//  Humboldt-Universitaet zu Berlin
//  Institut fuer Informatik
//  Digitale Systeme SoSe 2026
//  C-Programmierprojekt: RV32IMC-Emulator
//
//  Jasper Liang (627991)
//
//  main.c
//  Erstellt von Jasper Liang am 20.08.2026


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>


enum opcode_decode {R = 0x33, I = 0x13, S = 0x23, L = 0x03, B = 0x63, JALR = 0x67, JAL = 0x6F, AUIPC = 0x17, LUI = 0x37};

/* Speicherabgebildete Ausgabe (Memory Mapped IO):
 * Ein per SB auf diese Adresse geschriebenes Byte wird als Zeichen auf der
 * Standardausgabe ausgegeben, statt in den Datenspeicher zu wandern. */
#define MMIO_OUT_ADDR 0x5000u

typedef struct {
    size_t data_mem_size_;
    uint32_t regfile_[32];
    uint32_t pc_;
    uint8_t* instr_mem_;
    uint8_t* data_mem_;
} CPU;

void CPU_open_instruction_mem(CPU* cpu, const char* filename);
void CPU_load_data_mem(CPU* cpu, const char* filename);

CPU* CPU_init(const char* path_to_inst_mem, const char* path_to_data_mem) {
    CPU* cpu = (CPU*) malloc(sizeof(CPU));
    cpu->data_mem_size_ = 0x400000;
    cpu->pc_ = 0x0;
    CPU_open_instruction_mem(cpu, path_to_inst_mem);
    CPU_load_data_mem(cpu, path_to_data_mem);
    return cpu;
}

void CPU_open_instruction_mem(CPU* cpu, const char* filename) {
    uint32_t  instr_mem_size;
    FILE* input_file = fopen(filename, "r");
    if (!input_file) {
            printf("no input\n");
            exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (stat(filename, &sb) == -1) {
            printf("error stat\n");
            perror("stat");
            exit(EXIT_FAILURE);
    }
    printf("size of instruction memory: %ld Byte\n\n",(long)sb.st_size);
    instr_mem_size =  sb.st_size;
    cpu->instr_mem_ = malloc(instr_mem_size);
    fread(cpu->instr_mem_, sb.st_size, 1, input_file);
    fclose(input_file);
    return;
}

void CPU_load_data_mem(CPU* cpu, const char* filename) {
    FILE* input_file = fopen(filename, "r");
    if (!input_file) {
            printf("no input\n");
            exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (stat(filename, &sb) == -1) {
            printf("error stat\n");
            perror("stat");
            exit(EXIT_FAILURE);
    }
    printf("read data for data memory: %ld Byte\n\n",(long)sb.st_size);

    cpu->data_mem_ = malloc(cpu->data_mem_size_);
    fread(cpu->data_mem_, sb.st_size, 1, input_file);
    fclose(input_file);
    return;
}


/**
 * Speichert ein Byte (Befehl SB) -- VORGEGEBEN, bitte nicht veraendern.
 *
 * Liegt die Zieladresse auf der speicherabgebildeten Ausgabeadresse
 * MMIO_OUT_ADDR (0x5000), wird das Byte als Zeichen auf der Standardausgabe
 * ausgegeben (Zeichenausgabe / Memory Mapped IO). Andernfalls wird es ganz
 * normal in den Datenspeicher geschrieben.
 *
 * Rufen Sie diese Funktion bei der Implementierung des Befehls SB auf:
 *     CPU_store_byte(cpu, cpu->regfile_[rs1] + imm, (uint8_t)cpu->regfile_[rs2]);
 * Saemtliche Textausgabe der Testprogramme laeuft ueber diese Ausgabe.
 */
void CPU_store_byte(CPU* cpu, uint32_t addr, uint8_t value) {
    if (addr == MMIO_OUT_ADDR) {
        putchar((int)value);
    } else {
        cpu->data_mem_[addr] = value;
    }
}

// Expandiert einen 16-Bit-Compressed-Befehl in sein 32-Bit-Aequivalent. Gibt 0 zurueck, falls der Befehl ungueltig ist.
uint32_t expand_compressed(uint16_t c) {
    uint8_t op = c & 0x3;             // op[1:0] -> Quadrant
    uint8_t funct3 = (c >> 13) & 0x7; // funct3 ueber alle Quadranten
    uint8_t rd, rs1, rs2; 
    uint32_t uimm, inst;
    int32_t imm;
    uint32_t shamt = (((c >> 12) & 0x1) << 5) | ((c >> 2) & 0x1Fu);
    
    switch (op) {
        case 0x0: // Quadrant 0
            switch (funct3) {
                case 0x0: // C.ADDI4SPN -> addi rd', x2, nzuimm
                    // TODO: Immediate gemaess CIW-Format extrahieren
                    uimm = ((c >> 7) & 0x30)
                         | ((c >> 1) & 0x3C0)
                         | ((c >> 4) & 0x4)
                         | ((c >> 2) & 0x8);
                    
                    // TODO: rd' = ((c >> 2) & 0x7) + 8;
                    rd = ((c >> 2) & 0x7) + 8;
                    
                    // TODO: 32-Bit ADDI-Befehl zusammensetzen
                    inst = (uimm << 20)
                         | (0x2u << 15) // stack point x2 (rs1 = 2)
                         | (0x0u << 12)
                         | ((uint32_t)rd << 7)
                         | 0x13u;
                    
                    return inst;
                    
                case 0x2: // C.LW
                    rd = ((c >> 2) & 0x7) + 8;  
                    rs1 = ((c >> 7) & 0x7) + 8; 
                    
                    uimm = ((c >> 7) & 0x38)   
                         | ((c >> 4) & 0x04)   
                         | ((c << 1) & 0x40);  
                         
                    inst = (uimm << 20)           
                         | ((uint32_t)rs1 << 15)  
                         | (0x2u << 12)           
                         | ((uint32_t)rd << 7)    
                         | 0x03u;
                    
                    return inst;
                    
                case 0x6: // C.SW
                    rs2 = ((c >> 2) & 0x7) + 8;
                    rs1 = ((c >> 7) & 0x7) + 8;
                    
                    uimm = ((c >> 7) & 0x38)   
                         | ((c >> 4) & 0x04)   
                         | ((c << 1) & 0x40);  
                         
                    inst = ((uimm & 0xFE0) << 20)
                         | ((uint32_t)rs2 << 20)
                         | ((uint32_t)rs1 << 15)
                         | (0x2u << 12)
                         | ((uimm & 0x01F) << 7)
                         | 0x23u;
                         
                    return inst;
            }
            break;
            
        case 0x1: // Quadrant 1
            // TODO: C.NOP, C.ADDI, C.JAL, C.LI, C.ADDI16SP, C.LUI,
            //       C.SRLI, C.SRAI, C.ANDI, C.SUB, C.XOR, C.OR,
            //       C.AND, C.J, C.BEQZ, C.BNEZ
            switch (funct3) {
                case 0x0: // C.ADDI / C.NOP
                    // Zielregister rd extrahieren (CI-Format, Bits 11:7)
                    rd = (c >> 7) & 0x1F;
                    
                    // Immediate extrahieren
                    imm = ((c >> 7) & 0x20) | ((c >> 2) & 0x1F);
                    
                    // Vorzeichenerweiterung von 6 auf 32 Bit
                    if (imm & 0x20) imm |= 0xFFFFFFC0;
                    
                    // 32-Bit ADDI-Befehl zusammensetzen (rs1 = rd)
                    inst = ((uint32_t)(imm & 0xFFF) << 20)
                         | ((uint32_t)rd << 15)   // rs1 = rd
                         | (0x0u << 12)
                         | ((uint32_t)rd << 7)
                         | 0x13u;
                         
                    return inst;
                    
                case 0x1: // C.JAL
                    imm = (((c >> 12) & 0x1) << 11) | (((c >> 11) & 0x1) << 4) | (((c >> 9) & 0x3) << 8) | (((c >> 8) & 0x1) << 10) | (((c >> 7) & 0x1) << 6) | (((c >> 6) & 0x1) << 7) | (((c >> 3) & 0x7) << 1) | (((c >> 2) & 0x1) << 5);
                    
                    if (imm & 0x800) imm |= 0xFFFFF000;
                    
                    // J-Format: imm[20|10:1|11|19:12] | rd | opcode
                    inst = (((imm >> 20) & 0x1) << 31)    // imm[20]
                         | (((imm >> 1) & 0x3FF) << 21)   // imm[10:1]
                         | (((imm >> 11) & 0x1) << 20)    // imm[11]
                         | (((imm >> 12) & 0xFF) << 12)   // imm[19:12]
                         | (0x1u << 7)                    // rd = x1 (JAL)
                         | 0x6Fu;                         // opcode = 1101111
                    
                    return inst;
                    
                case 0x2: // C.LI
                    rd = (c >> 7) & 0x1F;
                    
                    if (rd == 0) return 0; // Ungueltige Instruktion
                    
                    imm = ((c >> 7) & 0x20) | ((c >> 2) & 0x1F);
                    
                    if (imm & 0x20) imm |= 0xFFFFFFC0; // Vorzeichenerweiterung von 6 auf 32 Bit
                    
                    inst = ((uint32_t)(imm & 0xFFF) << 20)
                         | (0x0u << 15)           // rs1 = 0 (Register x0)
                         | (0x0u << 12)           // funct3 = 000 (ADDI)
                         | ((uint32_t)rd << 7)    // Zielregister rd
                         | 0x13u;                 // opcode = 0010011 (OP-IMM)
                    
                    return inst;
                    
                case 0x3: // C.ADDI16SP / C.LUI
                    // Zielregister extrahieren (Bits 11:7, keine komprimierten Register)
                    rd = (c >> 7) & 0x1F;

                    if (rd == 2) { // C.ADDI16SP -> addi x2, x2, nzimm
                        imm = (((c >> 12) & 0x1) << 9)
                            | (((c >> 6) & 0x1) << 4)
                            | (((c >> 5) & 0x1) << 6)
                            | (((c >> 3) & 0x3) << 7)
                            | (((c >> 2) & 0x1) << 5);
                        
                        if (imm & 0x200) imm |= 0xFFFFFC00; // Vorzeichenerweiterung (10-Bit auf 32-Bit)
                        
                        // 32-Bit ADDI-Befehl zusammensetzen (rs1 = x2, rd = x2)
                        inst = (((uint32_t)imm & 0xFFF) << 20)
                             | (0x2u << 15)           // rs1 = x2 (Stack Pointer)
                             | (0x0u << 12)           // funct3 = 000 (ADDI)
                             | (0x2u << 7)            // rd = x2 (Stack Pointer)
                             | 0x13u;                 // opcode = 0010011 (0x13)
                        
                        return inst;
                             
                    } else if (rd != 0) { // C.LUI -> lui rd, nzimm
                        imm = (((c >> 12) & 0x1) << 17) | (((c >> 2) & 0x1F) << 12);
                        
                        // Vorzeichenerweiterung (18-Bit auf 32-Bit)
                        if (imm & 0x20000) imm |= 0xFFFC0000;
                        
                        // 32-Bit LUI-Befehl zusammensetzen (U-Format)
                        inst = (uint32_t)imm          // imm[31:12]
                             | ((uint32_t)rd << 7)    // rd
                             | 0x37u;                 // opcode = 0110111 (0x37)
                        
                        return inst;
                        
                    } else {
                        return 0;
                    }
                    
                case 0x4: // C.SRLI, C.SRAI, C.ANDI, C.SUB, C.XOR, C.OR, C.AND
                    rd = ((c >> 7) & 0x7) + 8;
                    uint8_t op1 = (c >> 10) & 0x3;
                    uint8_t op2 = (c >> 5) & 0x3;
                    
                    switch (op1) {
                        case 0x0: // C.SRLI
                            inst = (0x00u << 25)
                                 | ((uint32_t)shamt << 20)
                                 | ((uint32_t)rd << 15)
                                 | (0x5u << 12)
                                 | ((uint32_t)rd << 7)
                                 | 0x13u;
                            
                            return inst;
                            
                        case 0x1: // C.SRAI
                            inst = (0x20u << 25)
                                 | ((uint32_t)shamt << 20)
                                 | ((uint32_t)rd << 15)
                                 | (0x5u << 12)
                                 | ((uint32_t)rd << 7)
                                 | 0x13u;
                            
                            return inst;
                            
                        case 0x2: // C.ANDI
                            imm = shamt;
                            
                            if (imm & 0x20) imm |= 0xFFFFFFC0; // Vorzeichenerweiterung von 6 auf 32 Bit
                            
                            inst = (((uint32_t)imm & 0xFFF) << 20)
                                 | ((uint32_t)rd << 15)
                                 | (0x7u << 12)
                                 | ((uint32_t)rd << 7)
                                 | 0x13u;
                            
                            return inst;
                            
                        case 0x3: // C.SUB, C.XOR, C.OR, C.AND
                            rs2 = ((c >> 2) & 0x7) + 8;
                            
                            switch (op2) {
                                case 0x0: // C.SUB
                                    inst = (0x20u << 25)
                                         | ((uint32_t)rs2 << 20)
                                         | ((uint32_t)rd << 15)
                                         | (0x0u << 12)
                                         | ((uint32_t)rd << 7)
                                         | 0x33u;
                                    
                                    return inst;
                                    
                                case 0x1: // C.XOR
                                    inst = (0x00u << 25)
                                         | ((uint32_t)rs2 << 20)
                                         | ((uint32_t)rd << 15)
                                         | (0x4u << 12)
                                         | ((uint32_t)rd << 7)
                                         | 0x33u;
                                    
                                    return inst;
                                    
                                case 0x2: // C.OR
                                    inst = (0x00u << 25)
                                         | ((uint32_t)rs2 << 20)
                                         | ((uint32_t)rd << 15)
                                         | (0x6u << 12)
                                         | ((uint32_t)rd << 7)
                                         | 0x33u;
                                    
                                    return inst;
                                    
                                case 0x3: // C.AND
                                    inst = (0x00u << 25)
                                         | ((uint32_t)rs2 << 20)
                                         | ((uint32_t)rd << 15)
                                         | (0x7u << 12)
                                         | ((uint32_t)rd << 7)
                                         | 0x33u;
                                    
                                    return inst;
                            }
                            
                            break;
                    }
                    
                    break;
                    
                case 0x5: // C.J
                    // 1. Immediate extrahieren (identisch mit C.JAL)
                    imm = (((c >> 12) & 0x1) << 11) | (((c >> 11) & 0x1) << 4) | (((c >> 9) & 0x3) << 8) | (((c >> 8) & 0x1) << 10) | (((c >> 7) & 0x1) << 6) | (((c >> 6) & 0x1) << 7) | (((c >> 3) & 0x7) << 1) | (((c >> 2) & 0x1) << 5);
                    
                    // 2. Vorzeichenerweiterung (Sign Extension) auf 32-Bit
                    if (imm & 0x800) imm |= 0xFFFFF000;
                    
                    // 3. 32-Bit JAL-Befehl zusammensetzen (rd = x0 fuer C.J)
                    inst = (((imm >> 20) & 0x1) << 31)    // imm[20]
                         | (((imm >> 1) & 0x3FF) << 21)   // imm[10:1]
                         | (((imm >> 11) & 0x1) << 20)    // imm[11]
                         | (((imm >> 12) & 0xFF) << 12)   // imm[19:12]
                         | (0x0u << 7)                    // rd = x0
                         | 0x6Fu;                         // opcode = 1101111
                    
                    return inst;
                    
                case 0x6: // C.BEQZ
                    rs1 = ((c >> 7) & 0x7) + 8;
                    
                    imm = (((c >> 12) & 0x1) << 8) | (((c >> 10) & 0x3) << 3) | (((c >> 5) & 0x3) << 6) | (((c >> 3) & 0x3) << 1) | (((c >> 2) & 0x1) << 5);
                    
                    if (imm & 0x100) imm |= 0xFFFFFE00; // Vorzeichenerweiterung von 9 auf 32 Bit
                    
                    inst = (((imm >> 12) & 0x1) << 31)
                         | (((imm >> 5) & 0x3F) << 25)
                         | (((imm >> 1) & 0xF) << 8)
                         | (((imm >> 11) & 0x1) << 7)
                         | (0x0u << 20)
                         | ((uint32_t)rs1 << 15)
                         | (0x0u << 12)
                         | 0x63u;
                    
                    return inst;
                    
                case 0x7: // C.BNEZ
                    rs1 = ((c >> 7) & 0x7) + 8;
                    
                    imm = (((c >> 12) & 0x1) << 8) | (((c >> 10) & 0x3) << 3) | (((c >> 5) & 0x3) << 6) | (((c >> 3) & 0x3) << 1) | (((c >> 2) & 0x1) << 5);
                    
                    if (imm & 0x100) imm |= 0xFFFFFE00; // Vorzeichenerweiterung von 9 auf 32 Bit
                    
                    inst = (((imm >> 12) & 0x1) << 31)
                         | (((imm >> 5) & 0x3F) << 25)
                         | (((imm >> 1) & 0xF) << 8)
                         | (((imm >> 11) & 0x1) << 7)
                         | (0x0u << 20)
                         | ((uint32_t)rs1 << 15)
                         | (0x1u << 12)
                         | 0x63u;
                    
                    return inst;
            }
            break;
            
        case 0x2: // Quadrant 2
            // TODO: C.SLLI, C.LWSP, C.JR, C.MV, C.EBREAK, C.JALR,
            //       C.ADD, C.SWSP
            rd = ((c >> 7) & 0x1F);
            
            switch (funct3) {
                case 0x0: // C.SLLI
                    if (rd == 0) return 0; // Ungueltige Instruktion
                    
                    inst = (0x00u << 25)
                         | ((uint32_t)shamt << 20)
                         | ((uint32_t)rd << 15)
                         | (0x1u << 12)
                         | ((uint32_t)rd << 7)
                         | 0x13u;
                    
                    return inst;
                    
                case 0x2: // C.LWSP
                    if (rd == 0) return 0; // Ungueltige Instruktion
                    
                    uimm = (((c >> 12) & 0x1) << 5)
                         | (((c >> 4) & 0x7) << 2)
                         | (((c >> 2) & 0x3) << 6);
                    
                    inst = (uimm << 20)
                         | (0x2u << 15)  // rs1 = x2 (Stack Pointer)
                         | (0x2u << 12)
                         | ((uint32_t)rd << 7)
                         | 0x03u;
                    
                    return inst;
                    
                case 0x4: // C.JR, C.MV, C.EBREAK, C.JALR, C.ADD
                    rs2 = (c >> 2) & 0x1F;
                    uint8_t op_12 = (c >> 12) & 0xF;
                    
                    switch (op_12) {
                        case 0x8: // Bit 12 == 0
                            if (rd == 0) return 0; // Ungueltige Instruktion
                            
                            if (rs2 == 0x0u) { // C.JR
                                rs1 = rd;
                                
                                inst = (0x0u << 20)
                                     | ((uint32_t)rs1 << 15)
                                     | (0x0u << 12)
                                     | (0x0u << 7)
                                     | 0x67u;
                                
                                return inst;
                            } else { // C.MV
                                inst = (0x0u << 25)
                                     | ((uint32_t)rs2 << 20)
                                     | (0x0u << 15)
                                     | (0x0u << 12)
                                     | ((uint32_t)rd << 7)
                                     | 0x33u;
                                
                                return inst;
                            }
                            break;
                            
                        case 0x9: // Bit 12 == 1
                            if (rd == 0x0u) { // C.EBREAK
                                if (rs2 != 0x0u) return 0;
                                
                                inst = 0x00000013u; // NOP (addi x0, x0, 0)
                                
                                return inst;
                            } else if (rs2 == 0x0u) { // C.JALR
                                rs1 = rd;
                                
                                inst = (0x0u << 20)
                                     | ((uint32_t)rs1 << 15)
                                     | (0x0u << 12)
                                     | (0x1u << 7)
                                     | 0x67u;
                                
                                return inst;
                            } else { // C.ADD
                                inst = (0x0u << 25)
                                     | ((uint32_t)rs2 << 20)
                                     | ((uint32_t)rd << 15)
                                     | (0x0u << 12)
                                     | ((uint32_t)rd << 7)
                                     | 0x33u;
                                
                                return inst;
                            }
                            
                            break;
                    }
                    
                    break;
                    
                case 0x6: // C.SWSP
                    uimm = (((c >> 9) & 0xF) << 2)
                         | (((c >> 7) & 0x3) << 6);
                    
                    rs2 = (c >> 2) & 0x1F;
                    
                    inst = (((uimm >> 5) & 0x7F) << 25)
                         | ((uint32_t)rs2 << 20)
                         | (0x2u << 15)  // rs1 = x2 (Stack Pointer)
                         | (0x2u << 12)
                         | ((uimm & 0x1F) << 7)
                         | 0x23u;
                    
                    return inst;
            }
            
            break;
    }
    
    return 0; // Ungueltige Instruktion
}

/**
 * Instruction fetch Instruction decode, Execute, Memory access, Write back
 */
void CPU_execute(CPU* cpu) {
    
    //uint32_t instruction = *(uint32_t*)(cpu->instr_mem_ + (cpu->pc_  & 0xFFFFF));

    // TODO
    //
    // Hinweis zur Zeichenausgabe: Verwenden Sie fuer den Befehl SB die bereits
    // vorgegebene Funktion CPU_store_byte(). So erscheinen Schreibzugriffe auf
    // die Adresse 0x5000 als Zeichenausgabe auf dem Terminal.
    
    
    // Zunaechst die unteren 16 Bits einlesen, um das Befehlsformat zu ueberpruefen
    uint16_t lower = *(uint16_t *)(cpu->instr_mem_ + (cpu->pc_ & 0xFFFFF));
    uint32_t instruction;
    int length;
    
    if ((lower & 0x3) == 0x3) {
        // 32-Bit-Befehl: Die kompletten 32 Bits einlesen
        instruction = *(uint32_t *)(cpu->instr_mem_ + (cpu->pc_ & 0xFFFFF));
        length = 4;
    } else {
        // 16-Bit-Befehl: Komprimierten Befehl expandieren
        instruction = expand_compressed(lower);
        length = 2;
    }
    
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    
    switch (opcode) {
        case 0x37: { // LUI (U-Format)
            uint32_t imm = instruction & 0xFFFFF000;
            
            if (rd != 0) cpu->regfile_[rd] = imm;
            cpu->pc_ += length;
            break;
        }
            
        case 0x17: { // AUIPC (U-Format)
            uint32_t imm = instruction & 0xFFFFF000;
            
            if (rd != 0) cpu->regfile_[rd] = cpu->pc_ + imm;
            cpu->pc_ += length;
            break;
        }
            
        case 0x6F: { // JAL (J-Format)
            uint32_t imm_20    = ((instruction >> 31) & 0x1)   << 20;
            uint32_t imm_10_1  = ((instruction >> 21) & 0x3FF) << 1;
            uint32_t imm_11    = ((instruction >> 20) & 0x1)   << 11;
            uint32_t imm_19_12 = ((instruction >> 12) & 0xFF)  << 12;
            
            uint32_t imm_u = imm_20 | imm_10_1 | imm_11 | imm_19_12;
            
            // Vorzeichenerweiterung
            int32_t imm = (imm_u & 0x100000) ? (int32_t)(imm_u | 0xFFE00000) : (int32_t)imm_u;
            
            if (rd != 0) {
                cpu->regfile_[rd] = cpu->pc_ + length;
            }
            
            cpu->pc_ = cpu->pc_ + (int32_t)imm;
            
            break;
        }
            
        case 0x67: { // JALR (I-Format)
            //uint32_t funct3 = (instruction >> 12) & 0x7;
            uint32_t imm_u = (instruction >> 20) & 0xFFF;
            int32_t imm = (imm_u & 0x800) ? (int32_t)(imm_u | 0xFFFFF000)
                                          : (int32_t)imm_u;
            
            uint32_t target_pc = cpu->regfile_[rs1] + (int32_t)imm;
            
            // Ruecksprungadresse speichern (falls rd != x0)
            if (rd != 0) {
                cpu->regfile_[rd] = cpu->pc_ + length;
            }
            
            // Gemaess RISC-V-Spezifikation muss das niedrigste Bit (LSB) bei JALR auf 0 gesetzt werden
            cpu->pc_ = target_pc & ~1;
            
            break;
        }
            
        case 0x63: { // B-Format
            uint32_t funct3 = (instruction >> 12) & 0x7;
            uint32_t rs2 = (instruction >> 20) & 0x1F;
            
            uint32_t imm_12 = ((instruction >> 31) & 0x1) << 12;
            uint32_t imm_10_5 = ((instruction >> 25) & 0x3F) << 5;
            uint32_t imm_4_1 = ((instruction >> 8) & 0xF) << 1;
            uint32_t imm_11 = ((instruction >> 7) & 0x1) << 11;
            
            uint32_t imm_u = imm_12 | imm_10_5 | imm_4_1 | imm_11;
            int32_t imm = (imm_u & 0x1000) ? (int32_t)(imm_u | 0xFFFFE000) : (int32_t)imm_u;
            
            switch (funct3) {
                case 0x0: // BEQ
                    if (cpu->regfile_[rs1] == cpu->regfile_[rs2]) {
                        cpu->pc_ += (int32_t)imm;
                    } else {
                        cpu->pc_ += length;
                    }
                    break;
                    
                case 0x1: // BNE
                    if (cpu->regfile_[rs1] != cpu->regfile_[rs2]) {
                        cpu->pc_ += (int32_t)imm;
                    } else {
                        cpu->pc_ += length;
                    }
                    break;
                    
                case 0x4: // BLT
                    if ((int32_t)cpu->regfile_[rs1] < (int32_t)cpu->regfile_[rs2]) {
                        cpu->pc_ += (int32_t)imm;
                    } else {
                        cpu->pc_ += length;
                    }
                    break;
                    
                case 0x5: // BGE
                    if ((int32_t)cpu->regfile_[rs1] >= (int32_t)cpu->regfile_[rs2]) {
                        cpu->pc_ += (int32_t)imm;
                    } else {
                        cpu->pc_ += length;
                    }
                    break;
                    
                case 0x6: // BLTU
                    if (cpu->regfile_[rs1] < cpu->regfile_[rs2]) {
                        cpu->pc_ += (int32_t)imm;
                    } else {
                        cpu->pc_ += length;
                    }
                    break;
                    
                case 0x7: // BGEU
                    if (cpu->regfile_[rs1] >= cpu->regfile_[rs2]) {
                        cpu->pc_ += (int32_t)imm;
                    } else {
                        cpu->pc_ += length;
                    }
                    break;
            }
            
            break;
        }
            
        case 0x03: { // I-Format
            uint32_t funct3 = (instruction >> 12) & 0x7;
            uint32_t imm_u = (instruction >> 20) & 0xFFF;
            int32_t imm = (imm_u & 0x800) ? (int32_t)(imm_u | 0xFFFFF000)
            : (int32_t)imm_u;
            
            if (rd != 0) {
                switch (funct3) {
                    case 0x0: // LB
                        cpu->regfile_[rd] = (int32_t)(int8_t) cpu->data_mem_[cpu->regfile_[rs1] + imm];
                        break;
                        
                    case 0x1: // LH
                        cpu->regfile_[rd] = (int32_t)(int16_t)*(uint16_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + imm);
                        break;
                        
                    case 0x2: // LW
                        cpu->regfile_[rd] = *(uint32_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + imm);
                        break;
                        
                    case 0x4: // LBU
                        cpu->regfile_[rd] = cpu->data_mem_[cpu->regfile_[rs1] + imm];
                        break;
                        
                    case 0x5: // LHU
                        cpu->regfile_[rd] = *(uint16_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + imm);
                        break;
                }
            }
            
            cpu->pc_ += length;
            break;
        }
            
        case 0x23: { // S-Format
            uint32_t rs2 = (instruction >> 20) & 0x1F;
            uint32_t imm_lo = (instruction >> 7) & 0x1F; // imm[4:0]
            uint32_t imm_hi = (instruction >> 25) & 0x7F; // imm[11:5]
            uint32_t imm_u = (imm_hi << 5) | imm_lo;
            int32_t imm = (imm_u & 0x800) ? (int32_t)(imm_u | 0xFFFFF000)
            : (int32_t)imm_u;
            uint32_t funct3 = (instruction >> 12) & 0x7;
            
            switch (funct3) {
                case 0x0: // SB
                    CPU_store_byte(cpu, cpu->regfile_[rs1] + imm,
                    (uint8_t)cpu->regfile_[rs2]);
                    break;
                    
                case 0x1: // SH
                    *(uint16_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + imm) = (uint16_t)cpu->regfile_[rs2];
                    break;
                    
                case 0x2: // SW
                    *(uint32_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + imm) = (uint32_t)cpu->regfile_[rs2];
                    break;
            }
            
            cpu->pc_ += length;
            break;
        }
            
        case 0x13: { // I-Format / R-Format
            uint32_t funct3 = (instruction >> 12) & 0x7;
            uint32_t funct7 = (instruction >> 25) & 0x7F;
            uint32_t shamt = (instruction >> 20) & 0x1F;
            // 12-Bit-Immediate aus den Bits 31:20 herausschneiden
            uint32_t imm_u = (instruction >> 20) & 0xFFF;
            // Vorzeichenerweiterung von 12 auf 32 Bit
            int32_t imm = (imm_u & 0x800) ? (int32_t)(imm_u | 0xFFFFF000)
                                          : (int32_t)imm_u;
            
            if (rd != 0) {
                switch (funct3) {
                    case 0x0: // ADDI (I-Format)
                        cpu->regfile_[rd] = cpu->regfile_[rs1] + (int32_t)imm;
                        break;
                        
                    case 0x2: // SLTI (I-Format)
                        cpu->regfile_[rd] = (int32_t)cpu->regfile_[rs1] < (int32_t)imm;
                        break;
                        
                    case 0x3: // SLTIU (I-Format)
                        cpu->regfile_[rd] = cpu->regfile_[rs1] < (uint32_t)imm;
                        break;
                        
                    case 0x4: // XORI (I-Format)
                        cpu->regfile_[rd] = cpu->regfile_[rs1] ^ imm;
                        break;
                        
                    case 0x6: // ORI (I-Format)
                        cpu->regfile_[rd] = cpu->regfile_[rs1] | imm;
                        break;
                        
                    case 0x7: // ANDI (I-Format)
                        cpu->regfile_[rd] = cpu->regfile_[rs1] & imm;
                        break;
                        
                    case 0x1: // SLLI (R-Format)
                        cpu->regfile_[rd] = cpu->regfile_[rs1] << shamt;
                        break;
                        
                    case 0x5: // SRLI / SRAI (R-Format)
                        switch (funct7) {
                            case 0x00: // SRLI
                                cpu->regfile_[rd] = cpu->regfile_[rs1] >> shamt;
                                break;
                                
                            case 0x20: // SRAI
                                cpu->regfile_[rd] = (int32_t)cpu->regfile_[rs1] >> shamt;
                                break;
                        }
                        
                        break;
                }
            }
            
            cpu->pc_ += length;
            break;
        }
            
        case 0x33: { // R-Format
            uint32_t rs2 = (instruction >> 20) & 0x1F;
            uint32_t funct3 = (instruction >> 12) & 0x7;
            uint32_t funct7 = (instruction >> 25) & 0x7F;
            
            int32_t val1 = (int32_t)cpu->regfile_[rs1];
            int32_t val2 = (int32_t)cpu->regfile_[rs2];
            uint32_t uval1 = (uint32_t)cpu->regfile_[rs1];
            uint32_t uval2 = (uint32_t)cpu->regfile_[rs2];
            
            if (rd != 0) {
                if (funct7 == 0x01) {
                    switch (funct3) {
                        case 0x0: // MUL
                            cpu->regfile_[rd] = (uint32_t)((int64_t)(int32_t)cpu->regfile_[rs1] * (int64_t)(int32_t)cpu->regfile_[rs2]);
                            break;
                            
                        case 0x1: // MULH
                            cpu->regfile_[rd] = ((int64_t)(int32_t)cpu->regfile_[rs1] * (int64_t)(int32_t)cpu->regfile_[rs2]) >> 32;
                            break;
                            
                        case 0x2: // MULHSU
                            cpu->regfile_[rd] = ((int64_t)(int32_t)cpu->regfile_[rs1] * (uint64_t)cpu->regfile_[rs2]) >> 32;
                            break;
                            
                        case 0x3: // MULHU
                            cpu->regfile_[rd] = ((uint64_t)cpu->regfile_[rs1] * (uint64_t)cpu->regfile_[rs2]) >> 32;
                            break;
                            
                        case 0x4: { // DIV
                            if (val2 == 0) { // Division durch 0
                                cpu->regfile_[rd] = 0xFFFFFFFF;
                            } else if (val1 == 0x80000000 && val2 == -1) { // Überlauf (INT_MIN / -1)
                                cpu->regfile_[rd] = 0x80000000;
                            } else { // vorzeichenbehaftete Division
                                cpu->regfile_[rd] = val1 / val2;
                            }
                            break;
                        }
                            
                        case 0x5: { // DIVU
                            if (uval2 == 0) { // Division durch 0
                                cpu->regfile_[rd] = 0xFFFFFFFF;
                            } else { // vorzeichenlose Division
                                cpu->regfile_[rd] = uval1 / uval2;
                            }
                            break;
                        }
                            
                        case 0x6: // REM
                            if (val2 == 0) { // Division durch 0
                                cpu->regfile_[rd] = val1;
                            } else if (val1 == 0x80000000 && val2 == -1) { // Überlauf (INT_MIN / -1)
                                cpu->regfile_[rd] = 0;
                            } else { // vorzeichenbehafteter Rest
                                cpu->regfile_[rd] = val1 % val2;
                            }
                            break;
                            
                        case 0x7: // REMU
                            if (uval2 == 0) { // Division durch 0
                                cpu->regfile_[rd] = uval1;
                            } else { // vorzeichenloser Rest
                                cpu->regfile_[rd] = uval1 % uval2;
                            }
                            break;
                    }
                } else {
                    switch (funct3) {
                        case 0x0: // ADD / SUB
                            if (funct7 == 0x00) { // ADD
                                cpu->regfile_[rd] = cpu->regfile_[rs1] + cpu->regfile_[rs2];
                            } else if (funct7 == 0x20) { // SUB
                                cpu->regfile_[rd] = cpu->regfile_[rs1] - cpu->regfile_[rs2];
                            }
                            break;
                            
                        case 0x1: // SLL
                            cpu->regfile_[rd] = cpu->regfile_[rs1] << (cpu->regfile_[rs2] & 0x1F);
                            break;
                            
                        case 0x2: // SLT (Set Less Than)
                            if ((int32_t)cpu->regfile_[rs1] < (int32_t)cpu->regfile_[rs2]) {
                                cpu->regfile_[rd] = 1;
                            } else {
                                cpu->regfile_[rd] = 0;
                            }
                            break;
                            
                        case 0x3: // SLTU
                            cpu->regfile_[rd] = cpu->regfile_[rs1] < cpu->regfile_[rs2];
                            break;
                            
                        case 0x4: // XOR
                            cpu->regfile_[rd] = cpu->regfile_[rs1] ^ cpu->regfile_[rs2];
                            break;
                            
                        case 0x5: // SRL/SRA
                            if (funct7 == 0x00) { // SRL
                                cpu->regfile_[rd] = cpu->regfile_[rs1] >> (cpu->regfile_[rs2] & 0x1F);
                            } else if (funct7 == 0x20) { // SRA
                                cpu->regfile_[rd] = (int32_t)cpu->regfile_[rs1] >> (cpu->regfile_[rs2] & 0x1F);
                            }
                            break;
                            
                        case 0x6: // OR
                            cpu->regfile_[rd] = cpu->regfile_[rs1] | cpu->regfile_[rs2];
                            break;
                            
                        case 0x7: // AND
                            cpu->regfile_[rd] = cpu->regfile_[rs1] & cpu->regfile_[rs2];
                            break;
                    }
                }
            }
            
            cpu->pc_ += length;
            break;
        }
            
            // Unbekannter Befehl: pc bleibt stehen -> Programm haelt an
    }

}

int main(int argc, char* argv[]) {
    printf("C Praktikum\nHU Risc-V  Emulator 2026\n");

    CPU* cpu_inst;

    cpu_inst = CPU_init(argv[1], argv[2]);
    for(uint32_t i = 0; i <1000000; i++) { // Hauptschleife: fuehrt Befehle aus, bis die Obergrenze erreicht ist
        CPU_execute(cpu_inst);
    }

    printf("\n-----------------------RISC-V program terminate------------------------\nRegfile values:\n");

    //output Regfile
    for(uint32_t i = 0; i <= 31; i++) {
        printf("%d: %X\n",i,cpu_inst->regfile_[i]);
    }
    fflush(stdout);

    return 0;
}



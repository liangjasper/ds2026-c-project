# RV32IMC RISC-V CPU Emulator

![C Standard](https://img.shields.io/badge/C-C11-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-RISC--V%2032--bit-orange.svg)
![Status](https://img.shields.io/badge/Status-All%20Tests%20Passed-brightgreen.svg)

## 📌 Overview

A cycle-accurate instruction set emulator for the RISC-V 32-bit architecture, featuring full support for the **RV32I** Base Integer Instruction Set, **RV32M** Standard Extension for Integer Multiplication and Division, and **RV32C** Standard Extension for Compressed Instructions.

This emulator was developed as the final project for the "Digitale Systeme" (Digital Systems) C-Praktikum at Humboldt-Universität zu Berlin. It is capable of executing raw compiled RISC-V binaries, mapping memory, and outputting results via Memory-Mapped I/O (MMIO).

## 🚀 Technical Highlights

This emulator was built with a strong focus on clean architecture, low-level memory management, and strict standard compliance:

* **Strict C11 Compliance**: Developed and tested against strict GCC `-std=c11` compiler flags with zero warnings.
* **Intelligent RV32C Decoding (DRY Principle)**: Instead of maintaining separate execution pipelines for 16-bit and 32-bit instructions, compressed instructions (Quadrants 0-2) are dynamically intercepted at the fetch stage and expanded into their equivalent 32-bit base instructions. This guarantees high maintainability and avoids logic duplication.
* **Precise Sign Extension & Bitwise Operations**: Implements robust bitwise masking and sign-extension logic to handle immediate values (e.g., converting 6-bit or 12-bit signed immediates to 32-bit two's complement integers) securely across all instruction formats (I, S, B, U, J).
* **Hardware-Accurate Memory Alignment**: Enforces architectural hardware constraints, such as clearing the least significant bit (LSB) during `JALR` instructions (`target_pc & ~1`) to prevent instruction address misaligned exceptions.
* **Memory-Mapped I/O (MMIO)**: Supports basic MMIO character output by intercepting `SB` (Store Byte) instructions directed at a specific memory address (`0x5000`).

## ⚙️ Supported Instruction Set

* **RV32I (Base Integer)**: R-Type, I-Type, S-Type, B-Type, U-Type, J-Type (e.g., `ADD`, `ADDI`, `BEQ`, `JAL`, `JALR`, `LUI`, `AUIPC`, `LW`, `SW`)
* **RV32M (Multiplication/Division)**: `MUL`, `MULH`, `DIV`, `DIVU`, `REM`, `REMU`
* **RV32C (Compressed)**: `C.ADDI4SPN`, `C.LW`, `C.SW`, `C.ADDI`, `C.JAL`, `C.LI`, `C.ADDI16SP`, `C.LUI`, `C.SRLI`, `C.SRAI`, `C.ANDI`, `C.SUB`, `C.XOR`, `C.OR`, `C.AND`, `C.J`, `C.BEQZ`, `C.BNEZ`, `C.SLLI`, `C.LWSP`, `C.JR`, `C.MV`, `C.EBREAK`, `C.JALR`, `C.ADD`, `C.SWSP`

## 🛠️ Build & Run

### Compilation
The project consists of a single `main.c` file for maximum portability. Compile it using GCC with the C11 standard:

```bash
gcc main.c -o hu-risc-v_emu -std=c11
```

### Execution
The emulator requires two binary files to run: the instruction memory and the data memory. 

```bash
./hu-risc-v_emu <path_to_instruction_memory.bin> <path_to_data_memory.bin>
```

### Output Example
Upon execution, the emulator will output standard memory allocation info, execute the instructions, print any MMIO output (e.g., `Hallo`), and finally dump the contents of all 32 registers.

```text
C Praktikum
HU Risc-V  Emulator 2026
size of instruction memory: 56 Byte

read data for data memory: 16 Byte

Hallo
Hallo

-----------------------RISC-V program terminate------------------------
Regfile values:
0: 0
1: 0
2: 0
// ... (Registers 3 to 31)
```

## 👨‍💻 Author

**Jasper Liang**  
B.Sc. Computer Science (Informatik)  
Humboldt-Universität zu Berlin

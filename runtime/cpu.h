#ifndef CPU_H_
#define CPU_H_

#include "commons.h"

static inline void push(Byte b) {
	ram[0x100 + SP] = b;
	SP--;
}

static inline Byte pull() {
	SP++;
	return ram[0x100 + SP];
}

static inline Byte flags() {
	return (((C != 0) << 0) | ((Z != 0) << 1) | ((I != 0) << 2) |
			((D != 0) << 3) | ((B != 0) << 4) | ((1 != 0) << 5) |
			((V != 0) << 6) | ((N != 0) << 7)) &
		0xFF;
}

// #define DEBUG

#ifdef DEBUG
#	define DEBUG_PRINT() \
	Byte ins = cpu_read(PC); \
	printf("%04X  %02X        %-30s A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%zu\n", \
			PC,  \
			ins,  \
			OPS[ins].name, \
			A, X, Y, flags(), SP, total_cpu_cycles)
#else
#	define DEBUG_PRINT() ((void) 0)
#endif // DEBUG

#define TICK(ins_size, cycles) do { \
	DEBUG_PRINT();                  \
	if (cycle_budget < 0)           \
		return;                     \
	PC += ins_size;                 \
	cycle_budget -= cycles;         \
	total_cpu_cycles += cycles;     \
} while (0);

// --- Addressing modes ---

#define CHECK_PAGE_CROSS(base, effective) do {         \
	if (((base) & 0xFF00) != ((effective) & 0xFF00)) { \
		cycle_budget -= 1;                             \
		total_cpu_cycles += 1;                         \
	}                                                  \
} while (0)

#define ADDR_MODE_NONE_GET(to, operand, check_crossing)
#define ADDR_MODE_ACC_GET(to, operand, check_crossing) to = A
#define ADDR_MODE_IMM_GET(to, operand, check_crossing) to = operand
#define ADDR_MODE_IMM2_GET(to, operand, check_crossing) to = operand
#define ADDR_MODE_ABS_GET(to, operand, check_crossing) to = cpu_read(operand)
#define ADDR_MODE_ABS_X_GET(to, operand, check_crossing) do { \
	to = cpu_read(operand + X);                               \
	if (check_crossing) {                                     \
		CHECK_PAGE_CROSS(operand, operand + X);               \
	}                                                         \
} while (0)
#define ADDR_MODE_ABS_Y_GET(to, operand, check_crossing) do { \
	to = cpu_read(operand + Y);                               \
	if (check_crossing) {                                     \
		CHECK_PAGE_CROSS(operand, operand + Y);               \
	}                                                         \
} while (0)
#define ADDR_MODE_ZP_GET(to, operand, check_crossing) to = cpu_read(operand)
#define ADDR_MODE_ZP_X_GET(to, operand, check_crossing) \
	to = cpu_read((operand + X) & 0xFF)
#define ADDR_MODE_ZP_Y_GET(to, operand, check_crossing) \
	to = cpu_read((operand + Y) & 0xFF)
#define ADDR_MODE_IND_GET(to, operand, check_crossing) do { \
	Byte lo = cpu_read(operand);                            \
	Byte hi;                                                \
	if ((operand & 0x00FF) == 0x00FF) { /* page wrap bug */ \
		hi = cpu_read(operand & 0xFF00);                    \
	} else {                                                \
		hi = cpu_read(operand + 1);                         \
	}                                                       \
	to = lo + (hi << 8);                                    \
} while (0)
#define ADDR_MODE_IND_X_GET(to, operand, check_crossing) do { \
	Byte lo = cpu_read((operand + X) & 0xFF);                 \
	Byte hi = cpu_read((operand + X + 1) & 0xFF);             \
	to = cpu_read((lo + hi * 256) & 0xFFFF);                  \
} while (0)
#define ADDR_MODE_IND_Y_GET(to, operand, check_crossing)  do { \
	Byte lo = cpu_read((operand) & 0xFF);                      \
	Byte hi = cpu_read((operand + 1) & 0xFF);                  \
	to = cpu_read((lo + hi * 256 + Y) & 0xFFFF);               \
	if (check_crossing) {                                      \
		CHECK_PAGE_CROSS(lo + hi * 256, lo + hi * 256 + Y);    \
	}                                                          \
} while (0)

#define ADDR_MODE_ACC_SET(operand, value) A = value
#define ADDR_MODE_ABS_SET(operand, value) cpu_write(operand, value)
#define ADDR_MODE_ABS_X_SET(operand, value) cpu_write(operand + X, value)
#define ADDR_MODE_ABS_Y_SET(operand, value) cpu_write(operand + Y, value)
#define ADDR_MODE_ZP_SET(operand, value) cpu_write(operand, value)
#define ADDR_MODE_ZP_X_SET(operand, value) \
	cpu_write((operand + X) & 0xFF, value)
#define ADDR_MODE_ZP_Y_SET(operand, value) \
	cpu_write((operand + Y) & 0xFF, value)
#define ADDR_MODE_IND_X_SET(operand, value) do {  \
	Byte lo = cpu_read((operand + X) & 0xFF);     \
	Byte hi = cpu_read((operand + X + 1) & 0xFF); \
	cpu_write((lo + hi * 256) & 0xFFFF, value);   \
} while (0)
#define ADDR_MODE_IND_Y_SET(operand, value) do {    \
	Byte lo = cpu_read((operand) & 0xFF);           \
	Byte hi = cpu_read((operand + 1) & 0xFF);       \
	cpu_write((lo + hi * 256 + Y) & 0xFFFF, value); \
} while (0)

// --- Arithmetic ---

#define ADC(mode, operand) do {                      \
	Byte mem;                                        \
	ADDR_##mode##_GET(mem, operand, true);           \
	uint16_t result = (uint16_t)A + mem + C;         \
	C = result > 0xFF;                               \
	V = ((result ^ A) & (result ^ mem) & 0x80) != 0; \
	A = result & 0xFF;                               \
	Z = A == 0;                                      \
	N = (A & 0x80) != 0;                             \
} while (0)

#define SBC(mode, operand) do {                      \
	Byte mem;                                        \
	ADDR_##mode##_GET(mem, operand, true);           \
	mem = ~mem;                                      \
	uint16_t result = (uint16_t)A + mem + C;         \
	C = result > 0xFF;                               \
	V = ((result ^ A) & (result ^ mem) & 0x80) != 0; \
	A = result & 0xFF;                               \
	Z = A == 0;                                      \
	N = (A & 0x80) != 0;                             \
} while (0)

#define DEC(mode, operand) do {             \
	Byte mem;                               \
	ADDR_##mode##_GET(mem, operand, false); \
	Byte result = mem - 1;                  \
	Z = result == 0;                        \
	N = (result & 0x80) != 0;               \
	ADDR_##mode##_SET(operand, result);     \
} while (0)

#define DEX(mode, o) do {        \
	Byte result = X - 1;      \
	X = result;               \
	Z = result == 0;          \
	N = (result & 0x80) != 0; \
} while (0)

#define DEY(mode, o) do {        \
	Byte result = Y - 1;      \
	Y = result;               \
	Z = result == 0;          \
	N = (result & 0x80) != 0; \
} while (0)

#define INC(mode, operand) do {             \
	Byte mem;                               \
	ADDR_##mode##_GET(mem, operand, false); \
	Byte result = mem + 1;                  \
	Z = result == 0;                        \
	N = (result & 0x80) != 0;               \
	ADDR_##mode##_SET(operand, result);     \
} while (0)

#define INX(mode, o) do {        \
	Byte result = X + 1;      \
	X = result;               \
	Z = result == 0;          \
	N = (result & 0x80) != 0; \
} while (0)

#define INY(mode, o) do {        \
	Byte result = Y + 1;      \
	Y = result;               \
	Z = result == 0;          \
	N = (result & 0x80) != 0; \
} while (0)

// --- Register stuff ---

#define LD(reg, mode, operand) do {        \
	Byte mem;                              \
	ADDR_##mode##_GET(mem, operand, true); \
	reg = mem;                             \
	Z = reg == 0;                          \
	N = (reg & 0x80) != 0;                 \
} while (0)

#define LDA(mode, operand) LD(A, mode, operand)

#define LDX(mode, operand) LD(X, mode, operand)

#define LDY(mode, operand) LD(Y, mode, operand)

#define ST(reg, mode, operand) do {  \
	ADDR_##mode##_SET(operand, reg); \
} while (0)

#define STA(mode, operand) ST(A, mode, operand)

#define STX(mode, operand) ST(X, mode, operand)

#define STY(mode, operand) ST(Y, mode, operand)

#define T__(reg1, reg2) do { \
	reg2 = reg1;             \
	Z = reg2 == 0;           \
	N = (reg2 & 0x80) != 0;  \
} while (0)

#define TXS(mode, o) do { \
    SP = X;            \
} while (0)

#define TSX(mode, o) T__(SP, X)

#define TYA(mode, o) T__(Y, A)

#define TXA(mode, o) T__(X, A)

#define TAY(mode, o) T__(A, Y)

#define TAX(mode, o) T__(A, X)

#define PHP(mode, o) push(flags() | 0x10)

#define PLP(mode, o) do {       \
	uint8_t f = pull();      \
	C = ((f >> 0) & 1) != 0; \
	Z = ((f >> 1) & 1) != 0; \
	I = ((f >> 2) & 1) != 0; \
	D = ((f >> 3) & 1) != 0; \
	V = ((f >> 6) & 1) != 0; \
	N = ((f >> 7) & 1) != 0; \
} while (0)

#define PHA(mode, o) push(A)

#define PLA(mode, o) do {   \
    A = pull();          \
    Z = A == 0;          \
    N = (A & 0x80) != 0; \
} while (0)

// --- Bitwise ---

#define AND(mode, operand) do {            \
	Byte mem;                              \
	ADDR_##mode##_GET(mem, operand, true); \
	A = A & mem;                           \
	Z = A == 0;                            \
	N = (A & 0x80) != 0;                   \
} while (0)

#define EOR(mode, operand) do {            \
	Byte mem;                              \
	ADDR_##mode##_GET(mem, operand, true); \
	A = A ^ mem;                           \
	Z = A == 0;                            \
	N = (A & 0x80) != 0;                   \
} while (0)

#define ORA(mode, operand) do {            \
	Byte mem;                              \
	ADDR_##mode##_GET(mem, operand, true); \
	A = A | mem;                           \
	Z = A == 0;                            \
	N = (A & 0x80) != 0;                   \
} while (0)

#define ASL(mode, operand) do {                \
	Byte mem;                                  \
	ADDR_##mode##_GET(mem, operand, false);    \
	uint16_t result = (uint16_t)mem << 1;      \
	C = result > 0xFF;                         \
	Z = (result & 0xFF) == 0;                  \
	N = (result & 0x80) != 0;                  \
	ADDR_##mode##_SET(operand, result & 0xFF); \
} while (0)

#define LSR(mode, operand) do {                \
	Byte mem;                                  \
	ADDR_##mode##_GET(mem, operand, false);    \
	uint16_t result = (uint16_t)mem >> 1;      \
	C = mem & 1;                               \
	Z = (result & 0xFF) == 0;                  \
	N = (result & 0x80) != 0;                  \
	ADDR_##mode##_SET(operand, result & 0xFF); \
} while (0)

#define ROL(mode, operand) do {                      \
	Byte mem;                                        \
	ADDR_##mode##_GET(mem, operand, false);          \
	uint16_t result = (uint16_t)mem << 1 | (C != 0); \
	C = mem >> 7;                                    \
	Z = (result & 0xFF) == 0;                        \
	N = (result & 0x80) != 0;                        \
	ADDR_##mode##_SET(operand, result & 0xFF);       \
} while (0)

#define ROR(mode, operand) do {                      \
	Byte mem;                                        \
	ADDR_##mode##_GET(mem, operand, false);          \
	uint16_t result = (uint16_t)mem >> 1 | (C << 7); \
	C = mem & 1;                                     \
	Z = (result & 0xFF) == 0;                        \
	N = (result & 0x80) != 0;                        \
	ADDR_##mode##_SET(operand, result & 0xFF);       \
} while (0)

// --- Flag Setting ---

#define BIT(mode, operand) do {             \
	Byte mem;                               \
	ADDR_##mode##_GET(mem, operand, false); \
	Byte result = A & mem;                  \
	Z = result == 0;                        \
	N = (mem & 0x80) != 0;                  \
	V = (mem & 0x40) != 0;                  \
} while (0)

#define CLC(mode, o) C = 0

#define CLD(mode, o) D = 0

#define CLI(mode, o) I = 0

#define CLV(mode, o) V = 0

#define SEC(mode, o) C = 1

#define SED(mode, o) D = 1

#define SEI(mode, o) I = 1

#define COMPARE(reg, mode, operand) do {   \
	Byte mem;                              \
	ADDR_##mode##_GET(mem, operand, true); \
	uint16_t result = (uint16_t)reg - mem; \
	C = reg >= mem;                        \
	Z = result == 0;                       \
	N = (result & 0x80) != 0;              \
} while (0)

#define CMP(mode, operand) COMPARE(A, mode, operand)

#define CPX(mode, operand) COMPARE(X, mode, operand)

#define CPY(mode, operand) COMPARE(Y, mode, operand)

// --- Control flow ---

#define BRANCH(cond, offs) do {            \
	if (cond) {                            \
		int16_t rel_offs = (int8_t)offs;   \
		CHECK_PAGE_CROSS(PC, PC+rel_offs); \
		PC += rel_offs;                    \
		total_cpu_cycles++;                \
		cycle_budget -= 1;                 \
		return;                            \
	}                                      \
} while (0)

#define BCC(mode, operand) BRANCH(!C, operand)

#define BCS(mode, operand) BRANCH(C, operand)

#define BNE(mode, operand) BRANCH(!Z, operand)

#define BEQ(mode, operand) BRANCH(Z, operand)

#define BPL(mode, operand) BRANCH(!N, operand)

#define BMI(mode, operand) BRANCH(N, operand)

#define BVC(mode, operand) BRANCH(!V, operand)

#define BVS(mode, operand) BRANCH(V, operand)

#define BRK(mode, operand) do {                      \
	push((Byte)((PC) >> 8));                         \
	push((Byte)((PC) & 0xFF));                       \
	push(flags() | 0x30);                            \
	I = true;                                        \
	PC = cpu_read(0xFFFE) | (cpu_read(0xFFFF) << 8); \
	return;                                          \
} while (0)

#define JMP(mode, operand) do {             \
	uint16_t mem;                           \
	ADDR_##mode##_GET(mem, operand, false); \
	PC = mem;                               \
	return;                                 \
} while (0)

#define JSR(mode, operand) do {    \
	push((Byte)((PC - 1) >> 8));   \
	push((Byte)((PC - 1) & 0xFF)); \
	JMP(mode, operand);            \
} while (0)

#define RTI(mode, o) do {   \
	PLP(mode, o);           \
	PC = pull();         \
	PC |= (pull() << 8); \
	return;              \
} while (0)

#define RTS(mode, o) do {   \
	PC = pull();         \
	PC |= (pull() << 8); \
	PC++;                \
	return;              \
} while (0)

// --- Other ---

#define NOP(mode, operand) do {            \
	Byte mem;                              \
	ADDR_##mode##_GET(mem, operand, true); \
	(void) mem;\
} while(0);


#endif // CPU_H_

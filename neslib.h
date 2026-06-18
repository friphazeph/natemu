#ifndef NESLIB_H
#define NESLIB_H

#include <stdio.h>
#include <stdlib.h>
#include "nes.h"

#define TODO(message) do { fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); abort(); } while(0)

// REGISTERS
Byte A;
Byte X;
Byte Y;
Byte SP;

// CPU flags
bool C;
bool Z;
bool V;
bool N;
bool I;
bool B;
bool D;

Addr PC;

Byte ram[0x800];

Byte *cpu_read(Addr addr) {
	if (addr < 0x2000) {
		return &ram[addr & 0x07FF];
	} else {
		TODO("cpu_read");
	}
}

static inline void push(Byte b) {
	ram[0x100+SP] = b;
	SP--;
}

static inline Byte pull() {
	SP++;
	return ram[0x100+SP];
}

static inline Byte flags() {
	return ( ((C != 0) << 0)
	       | ((Z != 0) << 1)
	       | ((I != 0) << 2)
	       | ((D != 0) << 3)
	       | ((B != 0) << 4)
	       | ((1 != 0) << 5)
	       | ((V != 0) << 6)
	       | ((N != 0) << 7)
           ) & 0xFF;
}

// --- Addressing modes ---

#define mem_mode_acc() &A

static inline Byte *mem_mode_zp(Byte arg) {
	return &ram[arg];
}

static inline Byte *mem_mode_zp_x(Byte arg) {
	return &ram[(arg+X)%256];
}

static inline Byte *mem_mode_zp_y(Byte arg) {
	return &ram[(arg+Y)%256];
}

static inline Byte *mem_mode_abs(Addr arg) {
	return cpu_read(arg);
}

static inline Byte *mem_mode_abs_x(Addr arg) {
	return cpu_read(arg+X);
}

static inline Byte *mem_mode_abs_y(Addr arg) {
	return cpu_read(arg+Y);
}

static inline Byte *mem_mode_ind(Addr arg) {
	Byte lo = *cpu_read(arg);
	Byte hi;
	if ((arg & 0x00FF) == 0x00FF) {
		// page wrap bug
		hi = *cpu_read(arg & 0xFF00); 
	} else {
		hi = *cpu_read(arg + 1);
	}
	return cpu_read(lo + hi * 256);
}

static inline Byte *mem_mode_ind_x(Byte arg) {
	Byte lo = ram[(arg+X) % 256];
	Byte hi = ram[(arg+X+1) % 256];
	return &ram[lo + hi*256];
}

static inline Byte *mem_mode_ind_y(Byte arg) {
	Byte lo = ram[(arg) % 256];
	Byte hi = ram[(arg+1) % 256];
	return &ram[lo + hi*256 + Y];
}

#define OP_DECL static inline void

#define ADDR_MODE_IMM(operand) operand

// --- Arithmetic ---

OP_DECL adc(Byte mem) {
	uint16_t result = (uint16_t) A + mem + C;
	C = result > 0xFF;
	V = (result ^ A) & (result ^ mem) & 0x80; 
	A = result & 0xFF;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

#define ADC(mode, operand) do {               \
	Byte mem = ADDR_##mode##(operand);        \
	uint16_t result = (uint16_t) A + mem + C; \
	C = result > 0xFF;                        \
	V = (result ^ A) & (result ^ mem) & 0x80; \
	A = result & 0xFF;                        \
	Z = A == 0;                               \
	N = (A & 0x80) != 0;                      \
} while(0)

OP_DECL sbc(Byte mem) {
	adc(~mem);
}

OP_DECL dec(Byte *mem) {
	Byte result = *mem - 1;
	*mem = result;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL dex() {
	Byte result = X - 1;
	X = result;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL dey() {
	Byte result = Y - 1;
	Y = result;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL inc(Byte *mem) {
	Byte result = *mem + 1;
	*mem = result;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL inx() {
	Byte result = X + 1;
	X = result;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL iny() {
	Byte result = Y + 1;
	Y = result;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

// --- Register stuff ---

OP_DECL lda(Byte mem) {
	A = mem;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL ldx(Byte mem) {
	X = mem;
	Z = X == 0;
	N = (X & 0x80) != 0;
}

OP_DECL ldy(Byte mem) {
	Y = mem;
	Z = Y == 0;
	N = (Y & 0x80) != 0;
}

OP_DECL sta(Byte *mem) {
	*mem = A;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL stx(Byte *mem) {
	*mem = X;
	Z = X == 0;
	N = (X & 0x80) != 0;
}

OP_DECL sty(Byte *mem) {
	*mem = Y;
	Z = Y == 0;
	N = (Y & 0x80) != 0;
}

OP_DECL tax() {
	X = A;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL tay() {
	Y = A;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL txa() {
	A = X;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL tya() {
	A = Y;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL tsx() {
	X = SP;
	Z = SP == 0;
	N = (SP & 0x80) != 0;
}

OP_DECL txs() {
	SP = X;
	Z = SP == 0;
	N = (SP & 0x80) != 0;
}

OP_DECL php() {
	push(flags());
}

OP_DECL plp() {
	uint8_t f = pull();
	C = ((f >> 0) & 1) != 0;
	Z = ((f >> 1) & 1) != 0;
	I = ((f >> 2) & 1) != 0;
	D = ((f >> 3) & 1) != 0;
	V = ((f >> 6) & 1) != 0;
	N = ((f >> 7) & 1) != 0;
}

OP_DECL pha() {
	push(A);
}

OP_DECL pla() {
	A = pull();
}

// --- Bitwise ---

OP_DECL and(Byte mem) {
	A = A & mem;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL eor(Byte mem) {
	A = A ^ mem;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL ora(Byte mem) {
	A = A | mem;
	Z = A == 0;
	N = (A & 0x80) != 0;
}

OP_DECL asl(Byte *mem) {
	uint16_t result = (uint16_t) *mem << 1;
	C = result > 0xFF;
	*mem = result & 0xFF;
	Z = *mem == 0;
	N = (*mem & 0x80) != 0;
}

OP_DECL lsr(Byte *mem) {
	uint16_t result = (uint16_t) *mem >> 1;
	C = *mem & 1;
	*mem = result & 0xFF;
	Z = *mem == 0;
	N = (*mem & 0x80) != 0;
}

OP_DECL rol(Byte *mem) {
	uint16_t result = (uint16_t) *mem << 1 | (C != 0);
	C = *mem >> 7;
	*mem = result & 0xFF;
	Z = *mem == 0;
	N = (*mem & 0x80) != 0;
}

OP_DECL ror(Byte *mem) {
	uint16_t result = (uint16_t) *mem >> 1 | (C << 7);
	C = *mem & 1;
	*mem = result & 0xFF;
	Z = *mem == 0;
	N = (*mem & 0x80) != 0;
}

// --- Flag Setting ---

OP_DECL bit(Byte mem) {
	Byte result = A & mem;
	Z = result == 0;
	N = (result & 0x80) != 0;
	V = (result & 0x40) != 0;
}

OP_DECL clc() {
	C = 0;
}

OP_DECL cld() {
	D = 0;
}

OP_DECL cli() {
	I = 0;
}

OP_DECL clv() {
	V = 0;
}

OP_DECL sec() {
	C = 1;
}

OP_DECL sed() {
	D = 1;
}

OP_DECL sei() {
	I = 1;
}

OP_DECL cmp(Byte mem) {
	uint16_t result = (uint16_t) A - mem;
	C = A >= mem;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL cpx(Byte mem) {
	uint16_t result = (uint16_t) X - mem;
	C = X >= mem;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

OP_DECL cpy(Byte mem) {
	uint16_t result = (uint16_t) Y - mem;
	C = Y >= mem;
	Z = result == 0;
	N = (result & 0x80) != 0;
}

// --- Control flow ---
// TODO: For all branches and jumps, handle the goto correctly
OP_DECL bcc(Byte mem) {
	PC = PC + !C * (int8_t) mem;
}

OP_DECL bcs(Byte mem) {
	PC = PC + (C != 0) * (int8_t) mem;
}

OP_DECL bne(Byte mem) {
	PC = PC + !Z * (int8_t) mem;
}

OP_DECL beq(Byte mem) {
	PC = PC + (Z != 0) * (int8_t) mem;
}

OP_DECL bpl(Byte mem) {
	PC = PC + !N * (int8_t) mem;
}

OP_DECL bmi(Byte mem) {
	PC = PC + (N != 0) * (int8_t) mem;
}

OP_DECL bvc(Byte mem) {
	PC = PC + !V * (int8_t) mem;
}

OP_DECL bvs(Byte mem) {
	PC = PC + (V != 0) * (int8_t) mem;
}

OP_DECL brk() {
	push((Byte) ((PC+2) & 0xFF));
	push((Byte) ((PC+2) >> 8));
	push(flags());
	I = true;
	PC = ram[0xFFFE];
}

OP_DECL jmp(Addr mem) {
	PC = mem;
}

OP_DECL jsr(Addr mem) {
	push((Byte) ((PC+2) & 0xFF));
	push((Byte) ((PC+2) >> 8));
	PC = mem;
}

OP_DECL rti() {
	plp();
	PC = pull() << 8;
	PC |= pull();
}

OP_DECL rts() {
	PC = pull() << 8;
	PC |= pull();
	PC++;
}

// --- Other ---

OP_DECL nop() {}

#endif // NESLIB_H

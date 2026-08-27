#ifndef COMMONS_H_
#define COMMONS_H_
#ifdef COMMONS_IMPLEMENTATION
#	define COMDEF
#else
#	define COMDEF extern
#endif // COMMONS_IMPLEMENTATION

#include <stddef.h>
#include <stdint.h>

typedef uint16_t Addr;
typedef uint8_t  Byte;

COMDEF unsigned int current_line;
COMDEF uint64_t total_cpu_cycles;
COMDEF uint64_t cpu_cycles_line_start;
COMDEF int64_t cycle_budget;

// REGISTERS
COMDEF Byte A;
COMDEF Byte X;
COMDEF Byte Y;
COMDEF Byte SP;

COMDEF Addr PC;

// CPU flags
COMDEF bool C;
COMDEF bool Z;
COMDEF bool V;
COMDEF bool N;
COMDEF bool I;
COMDEF bool B;
COMDEF bool D;

extern const Byte prg_rom[];
extern const size_t prg_rom_len;
extern const Byte chr_rom[];
extern const size_t chr_rom_len;
extern uint8_t mapper;

COMDEF Byte controller_buffer;

COMDEF Byte ram[0x800];
COMDEF Byte prg_ram[0x2000];
COMDEF Byte APU_IO_reg[0x18];

Byte cpu_read(Addr addr);
void cpu_write(Addr addr, Byte val);

#endif // COMMONS_H_

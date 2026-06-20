#ifndef NESLIB_H
#define NESLIB_H

#include "nes.h"
#include <stdio.h>
#include <stdlib.h>

extern int64_t cycle_budget;
extern Addr PC;
size_t addr_to_prg_rom(Addr addr);
void nes_init(void);
void ppu_catch_up(void);
void trigger_nmi(void);
extern void (*const global_dispatch[])(size_t offs);

#define PIXELS_W 256
#define PIXELS_H 240
extern uint32_t screen_buffer[PIXELS_H * PIXELS_W];

#ifdef NESLIB_IMPLEMENTATION

#define TODO(message) do {                                             \
	fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); \
	abort();                                                           \
} while (0)

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
Byte PPU_reg[0x8];
Byte APU_IO_reg[0x18];
uint8_t mapper;
const Byte prg_rom[];
size_t prg_rom_len;
const Byte chr_rom[];
size_t chr_rom_len;

uint32_t screen_buffer[PIXELS_H * PIXELS_W];

uint64_t total_cpu_cycles;
int64_t cycle_budget;

Byte vram[0x4000];
Byte palette_ram[0x20];
Byte oam[0x100];

uint16_t scanline = 0;   // 0 to 261
uint16_t cycle = 0;      // 0 to 340
uint64_t ppu_cycles = 0; // Total absolute PPU clock ticks

uint16_t vram_addr = 0;   // Internal VRAM pointer
Byte vram_latch = 0;      // Tracks whether a write is the 1st or 2nd byte
Byte ppu_data_buffer = 0; // Internal read buffer delay

Byte scroll_x = 0;
Byte scroll_y = 0;

static inline uint16_t mirror_nametable(uint16_t addr) {
	addr =
		(addr - 0x2000) & 0x0FFF; // Fold everything down to $0000 - $0FFF range

	// Vertical Mirroring (Hardcoded for Mapper 0 for now)
	return addr & 0x07FF;
}

static inline Byte ppu_read(Addr addr) {
	addr &= 0x3FFF;

	if (addr < 0x2000) {
		return chr_rom[addr];
	} else if (addr >= 0x3F00) {
		uint16_t palette_addr = addr & 0x001F;
		if ((palette_addr & 0x03) == 0)
			palette_addr &= 0x0F;
		return palette_ram[palette_addr];
	} else {
		return vram[mirror_nametable(addr)];
	}
}

static inline void ppu_write(Addr addr, Byte val) {
	addr &= 0x3FFF;
	if (addr < 0x2000) {
		fprintf(stderr, "Tried to write to CHR ROM ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr >= 0x3F00) {
		uint16_t palette_addr = addr & 0x001F;
		if ((palette_addr & 0x03) == 0)
			palette_addr &= 0x0F;
		palette_ram[palette_addr] = val;
	} else {
		vram[mirror_nametable(addr)] = val;
	}
}

// A quick hardcoded system palette (NES Classic colors)
// Index 0 = Black, 1 = White, 2 = Gray, 3 = Red (Just for debugging!)
uint32_t debug_palette[4] = {0xFF000000, 0xFFFFFFFF, 0xFF888888, 0xFFFF0000};

static inline uint32_t render_pixel(int x, int y) {
	// 1. Calculate global wrapped coordinates
	// NES treats the 4 logical nametables as a 512x480 pixel space.
	int bg_x = (x + scroll_x + ((PPU_reg[0] & 0x01) ? 256 : 0)) % 512;
	int bg_y = (y + scroll_y + ((PPU_reg[0] & 0x02) ? 240 : 0)) % 480;
	// 2. Determine which logical nametable we hit
	int nt_col = bg_x / 256; // 0 or 1
	int nt_row = bg_y / 240; // 0 or 1
	uint16_t nt_base = 0x2000 + (nt_col * 0x0400) + (nt_row * 0x0800);
	// 3. Local tile coordinates inside that nametable
	int local_x = bg_x % 256;
	int local_y = bg_y % 240;
	int tile_x = local_x >> 3;
	int tile_y = local_y >> 3;

	int fine_x = local_x & 7;
	int fine_y = local_y & 7;
	// 4. Look up Tile ID
	uint16_t nt_addr = nt_base + (tile_y * 32) + tile_x;
	uint8_t tile_id = ppu_read(nt_addr);
	// 5. Check Pattern Table
	uint16_t pattern_table_base = (PPU_reg[0] & 0x10) ? 0x1000 : 0x0000;
	uint16_t tile_row_addr = pattern_table_base + (tile_id * 16) + fine_y;
	uint8_t plane1 = ppu_read(tile_row_addr);
	uint8_t plane2 = ppu_read(tile_row_addr + 8);

	uint8_t bit_shift = 7 - fine_x;
	uint8_t pixel_color_idx =
		((plane1 >> bit_shift) & 0x01) | (((plane2 >> bit_shift) & 0x01) << 1);

	for (int i = 63; i >= 0; i--) {
		int sprite_y = oam[i * 4 + 0];
		int sprite_x = oam[i * 4 + 3];

		// Is the current screen x,y inside this 8x8 sprite bounding box?
		if (x >= sprite_x && x < sprite_x + 8 && y >= sprite_y &&
				y < sprite_y + 8) {
			uint8_t tile_id = oam[i * 4 + 1];
			uint8_t attr = oam[i * 4 + 2];

			int fine_x = x - sprite_x;
			int fine_y = y - sprite_y;

			// Handle Sprite Flipping flags
			if (attr & 0x40)
				fine_x = 7 - fine_x; // Flip Horizontal
			if (attr & 0x80)
				fine_y = 7 - fine_y; // Flip Vertical

			// PPUCTRL Bit 3 determines Sprite pattern table address
			uint16_t sprite_base = (PPU_reg[0] & 0x08) ? 0x1000 : 0x0000;
			uint16_t row_addr = sprite_base + (tile_id * 16) + fine_y;

			uint8_t plane1 = ppu_read(row_addr);
			uint8_t plane2 = ppu_read(row_addr + 8);

			uint8_t shift = 7 - fine_x;
			uint8_t pixel = ((plane1 >> shift) & 1) | (((plane2 >> shift) & 1) << 1);

			if (pixel != 0) {
				pixel_color_idx =
					pixel; // You can worry about BG Priority (attr & 0x20) later
			}
		}
	}

	// 7. Return the color from our temporary debug array
	return debug_palette[pixel_color_idx];
}

void ppu_catch_up(void) {
	// TODO("ppu_catch_up");
	uint64_t target_ppu_cycles = total_cpu_cycles * 3;

	// Instead of a separate function call, run a tight inline loop
	while (ppu_cycles < target_ppu_cycles) {

		// 1. Visible rendering zone
		if (scanline < 240 && cycle >= 1 && cycle <= 256) {
			int x = cycle - 1;
			int y = scanline;

			// Inline your pixel rendering logic right here
			// or ensure 'render_pixel' is marked inline!
			screen_buffer[y * 256 + x] = render_pixel(x, y);
		}

		// 2. Sprite 0 Hit Fast Check
		if (scanline == 30 && cycle == 120 && (PPU_reg[1] & 0x18)) {
			PPU_reg[2] |= 0x40;
		}

		// 3. VBlank Timing
		if (scanline == 241 && cycle == 1) {
			PPU_reg[2] |= 0x80;
		}

		if (scanline == 261 && cycle == 1) {
			PPU_reg[2] &= 0x3F;
		}

		// 4. Clock grid progression
		cycle++;
		if (cycle >= 341) {
			cycle = 0;
			scanline++;
			if (scanline >= 262)
				scanline = 0;
		}

		ppu_cycles++;
	}
}

size_t addr_to_prg_rom(Addr addr) {
	switch (mapper) {
		case 0:
			if (addr < 0x8000) {
				fprintf(stderr, "Tried to access prg_rom outside of range ! (0x%04X)\n",
						addr);
				exit(1);
			}
			return (addr - 0x8000) % prg_rom_len;
		default:
			TODO("Only mapper 0 is implemented");
	}
}

Byte ppu_read_reg(Addr addr) {
	ppu_catch_up();
	uint16_t reg_index = (addr - 0x2000) & 0x7;

	switch (reg_index) {
		case 2: { // $2002 - PPUSTATUS
					Byte status = PPU_reg[2];
					vram_latch = 0;
					PPU_reg[2] &= 0x7F;
					return status;
				}
		case 4: // $2004 - OAMDATA
				return oam[PPU_reg[3]];
		case 7: { // $2007 - PPUDATA (Buffered read quirk)
					Byte data = ppu_data_buffer;
					ppu_data_buffer = ppu_read(vram_addr);

					if (vram_addr >= 0x3F00) {
						data = ppu_data_buffer;
					}

					vram_addr += (PPU_reg[0] & 0x04) ? 32 : 1;
					vram_addr &= 0x3FFF;
					return data;
				}
		default:
				return PPU_reg[reg_index];
	}
}

Byte cpu_read(Addr addr) {
	if (addr < 0x2000) {
		return ram[addr & 0x07FF];
	} else if (addr < 0x4000) {
		return ppu_read_reg(addr);
	} else if (addr < 0x4018) {
		return APU_IO_reg[addr - 0x4000];
	} else if (addr < 0x401F) {
		fprintf(stderr, "Tried to access test-mode memory ! (0x%04X)\n", addr);
		exit(1);
	} else {
		// handle mappers after
		size_t i = addr_to_prg_rom(addr);
		return prg_rom[i];
	}
}

void ppu_write_reg(Addr addr, Byte value) {
	uint16_t reg_index = (addr - 0x2000) & 0x7;
	PPU_reg[reg_index] = value;

	switch (reg_index) {
		case 0: // $2000 - PPUCTRL
			{
				// bool old_nmi_enable = (PPU_reg[0] & 0x80) != 0;
				// bool new_nmi_enable = (value & 0x80) != 0;
				// bool vblank_active = (PPU_reg[2] & 0x80) != 0;
				//
				PPU_reg[0] = value;
				//
				// if (!old_nmi_enable && new_nmi_enable && vblank_active) {
				// 	nmi = true;
				// }
			} break;
		case 1: // $2001 - PPUMASK
			break;
		case 3: // $2003 - OAMADDR
			break;
		case 4: // $2004 - OAMDATA
			oam[PPU_reg[3]] = value;
			PPU_reg[3]++;
			break;
		case 5: // $2005 - PPUSCROLL
			if (vram_latch == 0) {
				scroll_x = value;
				vram_latch = 1;
			} else {
				scroll_y = value;
				vram_latch = 0;
			}
			break;
		case 6: // $2006 - PPUADDR
			if (vram_latch == 0) {
				vram_addr = (vram_addr & 0x00FF) | ((uint16_t)value << 8);
				vram_latch = 1;
			} else {
				vram_addr = (vram_addr & 0xFF00) | value;
				vram_latch = 0;
			}
			break;
		case 7: // $2007 - PPUDATA
				// printf("CPU wrote data 0x%02X to PPU VRAM 0x%04X\n", value, vram_addr);
			ppu_write(vram_addr, value);
			vram_addr += (PPU_reg[0] & 0x04) ? 32 : 1;
			vram_addr &= 0x3FFF;
			break;
	}
}

void cpu_write(Addr addr, Byte value) {
	if (addr < 0x2000) {
		ram[addr & 0x07FF] = value;
	} else if (addr < 0x4000) {
		ppu_write_reg(addr, value);
	} else if (addr < 0x4018) {
		if (addr == 0x4014) {
			uint16_t page = value << 8;
			for (int i = 0; i < 256; i++) {
				oam[i] = cpu_read(page + i);
			}

			cycle_budget -= 513;
			total_cpu_cycles += 513;
		}

		APU_IO_reg[addr - 0x4000] = value;
	} else if (addr < 0x401F) {
		fprintf(stderr, "Tried to write to test-mode memory ! (0x%04X)\n", addr);
		exit(1);
	} else {
		fprintf(stderr, "Tried to write to ROM ! (0x%04X)\n", addr);
		exit(1);
	}
	return;
}

void nes_init(void) {
	PC = cpu_read(0xFFFC) & 0xFF;
	PC |= cpu_read(0xFFFD) << 8;
	total_cpu_cycles = 0;
}

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

void trigger_nmi(void) {
	if (!(PPU_reg[0] & 0x80)) return;
	push((PC >> 8) & 0xFF);
	push(PC & 0xFF);
	push((flags() & 0xEF) | 0x20);

	uint8_t low = cpu_read(0xFFFA);
	uint8_t high = cpu_read(0xFFFB);

	PC = (high << 8) | low;
	total_cpu_cycles += 7;
}

// printf("PC: %04X | A: %02X X: %02X Y: %02X SP: %02X\n", PC, A, X, Y, SP);

#define TICK(ins_size, cycles) do { \
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

#define DEX(mode) do {        \
	Byte result = X - 1;      \
	X = result;               \
	Z = result == 0;          \
	N = (result & 0x80) != 0; \
} while (0)

#define DEY(mode) do {        \
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

#define INX(mode) do {        \
	Byte result = X + 1;      \
	X = result;               \
	Z = result == 0;          \
	N = (result & 0x80) != 0; \
} while (0)

#define INY(mode) do {        \
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

#define TXS(mode) T__(X, SP)

#define TSX(mode) T__(SP, X)

#define TYA(mode) T__(Y, A)

#define TXA(mode) T__(X, A)

#define TAY(mode) T__(A, Y)

#define TAX(mode) T__(A, X)

#define PHP(mode) push(flags() | 0x10)

#define PLP(mode) do {       \
	uint8_t f = pull();      \
	C = ((f >> 0) & 1) != 0; \
	Z = ((f >> 1) & 1) != 0; \
	I = ((f >> 2) & 1) != 0; \
	D = ((f >> 3) & 1) != 0; \
	V = ((f >> 6) & 1) != 0; \
	N = ((f >> 7) & 1) != 0; \
} while (0)

#define PHA(mode) push(A)

#define PLA(mode) A = pull()

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

#define CLC(mode) C = 0

#define CLD(mode) D = 0

#define CLI(mode) I = 0

#define CLV(mode) V = 0

#define SEC(mode) C = 1

#define SED(mode) D = 1

#define SEI(mode) I = 1

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

#define BRANCH(cond, offs) do {          \
	if (cond) {                          \
		int16_t rel_offs = (int8_t)offs; \
		PC += rel_offs;                  \
		return;                          \
	}                                    \
} while (0)

#define BCC(mode, operand) BRANCH(!C, operand)

#define BCS(mode, operand) BRANCH(C, operand)

#define BNE(mode, operand) BRANCH(!Z, operand)

#define BEQ(mode, operand) BRANCH(Z, operand)

#define BPL(mode, operand) BRANCH(!N, operand)

#define BMI(mode, operand) BRANCH(N, operand)

#define BVC(mode, operand) BRANCH(!V, operand)

#define BVS(mode, operand) BRANCH(V, operand)

#define BRK(mode, operand) do { \
	push((Byte)((PC) >> 8));    \
	push((Byte)((PC) & 0xFF));  \
	push(flags() | 0x10);       \
	I = true;                   \
	PC = cpu_read(0xFFFE);      \
	return;                     \
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

#define RTI(mode) do {   \
	PLP(mode);           \
	PC = pull();         \
	PC |= (pull() << 8); \
	return;              \
} while (0)

#define RTS(mode) do {   \
	PC = pull();         \
	PC |= (pull() << 8); \
	PC++;                \
	return;              \
} while (0)

// --- Other ---

#define NOP(mode)

#endif // NESLIB_IMPLEMENTATION

#endif // NESLIB_H

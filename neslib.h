#ifndef NESLIB_H
#define NESLIB_H

#include "nes.h"
#include <stdio.h>
#include "raylib.h"
#include <stdlib.h>

extern int64_t cycle_budget;
extern Addr PC;
extern void (*const global_dispatch[])(size_t offs);
extern Byte APU_IO_reg[0x18];
extern Byte controller_buffer;
size_t addr_to_prg_rom(Addr addr);
void nes_init(void);
void ppu_catch_up(void);
void trigger_nmi(void);
void interpret_pc(void);

#define PIXELS_W 256
#define PIXELS_H 240
extern Color screen_buffer[PIXELS_H * PIXELS_W];

#ifdef NESLIB_IMPLEMENTATION

#define CUT_IMPLEMENTATION
#include "cut.h"

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
Byte prg_ram[0x2000];
Byte APU_IO_reg[0x18];
uint8_t mapper;
const Byte prg_rom[];
const size_t prg_rom_len;
const Byte chr_rom[];
const size_t chr_rom_len;

Byte controller_buffer;
Color screen_buffer[PIXELS_H * PIXELS_W];
static const Color palette[64] = {
    { 0x7C, 0x7C, 0x7C, 0xFF }, { 0x00, 0x00, 0xFC, 0xFF }, { 0x00, 0x00, 0xBC, 0xFF }, { 0x44, 0x28, 0xBC, 0xFF },
    { 0x94, 0x00, 0x84, 0xFF }, { 0xA8, 0x00, 0x20, 0xFF }, { 0xA8, 0x10, 0x00, 0xFF }, { 0x88, 0x14, 0x00, 0xFF },
    { 0x50, 0x30, 0x00, 0xFF }, { 0x00, 0x78, 0x00, 0xFF }, { 0x00, 0x68, 0x00, 0xFF }, { 0x00, 0x58, 0x00, 0xFF },
    { 0x00, 0x40, 0x58, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
    { 0xBC, 0xBC, 0xBC, 0xFF }, { 0x00, 0x78, 0xF8, 0xFF }, { 0x00, 0x58, 0xF8, 0xFF }, { 0x68, 0x44, 0xFC, 0xFF },
    { 0xD8, 0x00, 0xCC, 0xFF }, { 0xE4, 0x00, 0x58, 0xFF }, { 0xF8, 0x38, 0x00, 0xFF }, { 0xE4, 0x5C, 0x10, 0xFF },
    { 0xAC, 0x7C, 0x00, 0xFF }, { 0x00, 0xB8, 0x00, 0xFF }, { 0x00, 0xA8, 0x00, 0xFF }, { 0x00, 0xA8, 0x44, 0xFF },
    { 0x00, 0x88, 0x88, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
    { 0xF8, 0xF8, 0xF8, 0xFF }, { 0x3C, 0xBC, 0xFC, 0xFF }, { 0x68, 0x88, 0xFC, 0xFF }, { 0x98, 0x78, 0xF8, 0xFF },
    { 0xF8, 0x78, 0xF8, 0xFF }, { 0xF8, 0x58, 0x98, 0xFF }, { 0xF8, 0x78, 0x58, 0xFF }, { 0xFC, 0xA0, 0x44, 0xFF },
    { 0xF8, 0xB8, 0x00, 0xFF }, { 0xB8, 0xF8, 0x18, 0xFF }, { 0x58, 0xD8, 0x54, 0xFF }, { 0x58, 0xF8, 0x98, 0xFF },
    { 0x00, 0xE8, 0xD8, 0xFF }, { 0x78, 0x78, 0x78, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },
    { 0xFC, 0xFC, 0xFC, 0xFF }, { 0xA4, 0xE4, 0xFC, 0xFF }, { 0xB8, 0xB8, 0xF8, 0xFF }, { 0xD8, 0xB8, 0xF8, 0xFF },
    { 0xF8, 0xB8, 0xF8, 0xFF }, { 0xF8, 0xA4, 0xC0, 0xFF }, { 0xF0, 0xD0, 0xB0, 0xFF }, { 0xFC, 0xE0, 0xA8, 0xFF },
    { 0xF8, 0xD8, 0x78, 0xFF }, { 0xD8, 0xF8, 0x78, 0xFF }, { 0xB8, 0xF8, 0xB8, 0xFF }, { 0xB8, 0xF8, 0xD8, 0xFF },
    { 0x00, 0xFC, 0xFC, 0xFF }, { 0xF8, 0xD8, 0xF8, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }
};

uint64_t total_cpu_cycles;
int64_t cycle_budget;

typedef struct {
	uint64_t at;
	Addr addr;
	Byte value;
} PPU_cmd;

struct {
	PPU_cmd *queue;
	Byte palette_ram[0x20];
	Byte vram[0x800];
	Byte oam[0x100];
	Byte oam_addr;
	bool even;
	uint64_t last_update;
	uint16_t line; // 0-261
	uint16_t dot;  // 0-340
	uint16_t base_nt;
	uint16_t x_scroll;
	uint16_t y_scroll;
	uint16_t v;
	uint16_t t;
	uint16_t bg_table_addr; // 0 or 0x1000
	uint16_t sprite_table_addr; // 0 or 0x1000
	uint8_t sprite_height; // 8 or 16
	uint8_t increment; // 1 or 32
	uint8_t buffer;
	bool nmi_enable;
	bool sprite_enable;
	bool bg_enable;
	bool vblank;
	bool sprite0hit;
	bool sprite_overflow;
	bool w;
} PPU;

static inline void ppu_write(Addr addr, Byte value) {
	// Only mapper 0 atm
	if (addr < 0x2000) {
		// just don't write to rom
		return;
	} else if (addr < 0x3F00) {
		PPU.vram[(addr - 0x2000) & 0x7FF] = value;
	} else if (addr < 0x4000) {
		Byte palette_addr = (addr - 0x3F00) & 0x1F;
        if ((palette_addr & 0x13) == 0x10) palette_addr &= 0x0F;
        PPU.palette_ram[palette_addr] = value;
	} else {
		fprintf(stderr, "Tried to write outside of PPU ram !\n");
		exit(1);
	}
}

static inline Byte ppu_read(Addr addr) {
	// Only mapper 0 atm
	if (addr < 0x2000) {
		return chr_rom[addr];
	} else if (addr < 0x3F00) {
		return PPU.vram[(addr - 0x2000) & 0x7FF];
	} else if (addr < 0x4000) {
		Byte palette_addr = (addr - 0x3F00) & 0x1F;
        if ((palette_addr & 0x13) == 0x10) palette_addr &= 0x0F;
        return PPU.palette_ram[palette_addr];
	} else {
		fprintf(stderr, "Tried to read outside of PPU ram !\n");
		exit(1);
	}
}

Byte ppu_read_reg(Addr addr) {
	ppu_catch_up();
	uint8_t ret;
	switch (addr & 0x7) {
		case 2: // PPU STATUS
			ret = (PPU.vblank << 7) 
				| (PPU.sprite0hit << 6) 
				| (PPU.sprite_overflow << 5);
			PPU.vblank = false;
			PPU.w = false;
			// if (PPU.vblank)
			// 	printf("vblank !\n");
			return ret;
			break;
		case 4: // OAM DATA
			return PPU.oam[PPU.oam_addr];
		case 7: // PPU DATA
			ret = PPU.buffer;
			PPU.buffer = ppu_read(PPU.v & 0x7FFF);
			PPU.v += PPU.increment;
			return ret;
	}
	return 0;
}

static inline void ppu_write_reg(Addr addr, Byte value) {
	switch (addr & 0x7) {
		case 0: // PPU CTRL
			bool nmi_old = PPU.nmi_enable;
			PPU.base_nt = value & 0x03;
			PPU.increment = (value & 0x04) ? 32 : 1;
			PPU.sprite_table_addr = (value & 0x08) ? 0x1000 : 0x0;
			PPU.bg_table_addr = (value & 0x10) ? 0x1000 : 0x0;
			PPU.sprite_height = (value & 0x20) ? 16 : 8;
			PPU.nmi_enable = value >> 7;

			if (!nmi_old && PPU.nmi_enable) {
				if (PPU.vblank)
					trigger_nmi();
			}
			break;
		case 1: // PPU MASK
			PPU.bg_enable = (value & 0x10) != 0;
			PPU.sprite_enable = (value & 0x08) != 0;
			break;
		case 3: // PPU OAM ADDR
			PPU.oam_addr = value;
			break;
		case 4:
			PPU.oam[PPU.oam_addr++] = value;
			break;
		case 5: // PPU SCROLL
			if (PPU.w) {
				// printf("Scroll Y = 0x%02X\n", value);
				PPU.y_scroll = value;
				PPU.w = false;
			} else {
				// printf("Scroll X = 0x%02X\n", value);
				PPU.x_scroll = value;
				PPU.w = true;
			}
			break;
		case 6: // TODO: transfer to PPU.t before on low byte write or something
			if (!PPU.w) {
				// First write: high byte
				PPU.t = (PPU.t & 0x00FF) | ((value & 0x3F) << 8);
				PPU.w = true;
			} else {
				// Second write: low byte
				PPU.t = (PPU.t & 0xFF00) | value;
				PPU.v = PPU.t; // Current VRAM pointer updates to match temporary pointer
				PPU.w = false;

				PPU.base_nt = (PPU.v >> 10) & 3; 
			}
			break;
		case 7: // PPU DATA
			ppu_write(PPU.v & 0x7FFF, value);
			PPU.v += PPU.increment;
			break;
	}
}

static inline void ppu_exec_cmd(PPU_cmd *c) {
	uint64_t up_to = c ? c->at * 3 : total_cpu_cycles * 3;
	while (PPU.last_update < up_to) {
		if (PPU.dot == 0 && PPU.line == 0) {
			PPU.sprite0hit = false;
			PPU.vblank = false;
		}
		if (PPU.dot == 1 && PPU.line == 241) {
			PPU.vblank = true;
		}
		if (PPU.vblank) goto incr;

		if (1 <= PPU.dot && PPU.dot <= 256) {
			if (PPU.line < 240) {

				uint8_t bg_color_index = 0;
				Color bg_pixel = palette[ppu_read(0x3F00) & 0x3F]; // universal BG

				if (PPU.bg_enable) {
					int wx = (PPU.dot - 1) + PPU.x_scroll;
					int wy = PPU.line + PPU.y_scroll;

					// 1. Calculate global tile positions factoring in the starting nametable
					int total_tile_x = (wx >> 3) + ((PPU.base_nt & 1) * 32);
					int total_tile_y = (wy >> 3) + ((PPU.base_nt & 2) ? 30 : 0);

					// 2. Wrap coordinates into local individual nametable indices
					int tile_x = total_tile_x % 32;
					int tile_y = total_tile_y % 30;

					// 3. Resolve the actual nametable base address
					uint16_t nt_base = 0x2000;
					if (total_tile_x & 32)       nt_base |= 0x400; // Toggle right nametable
					if ((total_tile_y / 30) & 1) nt_base |= 0x800; // Toggle bottom nametable

					// 4. Fetch tile index from calculated VRAM position
					uint16_t tile_addr = nt_base + (tile_y * 32) + tile_x;
					uint8_t  tile      = ppu_read(tile_addr);

					// 5. Fetch attribute byte and calculate correct palette quadrant
					uint16_t attr_addr = (nt_base | 0x03C0) + ((tile_y >> 2) * 8) + (tile_x >> 2);
					uint8_t  attr      = ppu_read(attr_addr);

					int pal_shift     = ((tile_y & 2) << 1) | (tile_x & 2);
					int pal_offset    = (attr >> pal_shift) & 0x03;

					// 6. Fine pixel offsets within the chosen tile
					int fine_x = wx & 7;
					int fine_y = wy & 7;

					uint16_t plane0_addr = PPU.bg_table_addr + (tile * 16) + fine_y;
					uint8_t  low         = ppu_read(plane0_addr);
					uint8_t  high        = ppu_read(plane0_addr + 8);

					int bit = 7 - fine_x;
					bg_color_index = ((high >> bit) & 1) << 1 | ((low >> bit) & 1);

					if (bg_color_index) {
						uint16_t pal_addr = 0x3F00 + (pal_offset * 4) + bg_color_index;
						bg_pixel = palette[ppu_read(pal_addr) & 0x3F];
					}
					// int wx = (PPU.dot - 1) + PPU.x_scroll;
					// int wy = PPU.line + PPU.y_scroll;
					// int raw_tile_x = (wx >> 3);
					// int raw_tile_y = (wy >> 3);
					//
					// // Tile coordinates inside the chosen nametable
					// int tile_x = raw_tile_x & 31;
					// int tile_y = raw_tile_y % 30;
					//
					// // Fine offsets within the tile
					// int fine_x = wx & 7;
					// int fine_y = wy & 7;
					//
					// uint16_t nt_base = 0x2000 | (PPU.v & 0x0C00);
					// if (raw_tile_x & 0x20)
					// 	nt_base ^= 0x400;
					//
					// // Tile address in VRAM
					// uint16_t tile_addr = nt_base + (tile_y * 32) + tile_x;
					// uint8_t  tile      = ppu_read(tile_addr);
					//
					// // Attribute table address
					// uint16_t attr_addr = (nt_base | 0x03C0)
					// 	+ ((tile_y >> 2) * 8)
					// 	+ (tile_x >> 2);
					// uint8_t attr      = ppu_read(attr_addr);
					//
					// int pal_shift     = ((tile_y & 2) << 1) | (tile_x & 2);
					// int pal_offset    = (attr >> pal_shift) & 0x03;
					//
					// uint16_t plane0_addr = PPU.bg_table_addr + (tile * 16) + fine_y;
					// uint8_t  low         = ppu_read(plane0_addr);
					// uint8_t  high        = ppu_read(plane0_addr + 8);
					//
					// int bit = 7 - fine_x;
					// bg_color_index = ((high >> bit) & 1) << 1 | ((low >> bit) & 1);
					//
					// if (bg_color_index) {
					// 	uint16_t pal_addr = 0x3F00 + (pal_offset * 4) + bg_color_index;
					// 	bg_pixel = palette[ppu_read(pal_addr) & 0x3F];
					// }
				}

				// Sprites
				Color sprite_pixel    = {0};     // transparent
				bool  sprite_priority = false;   // false = in front of BG
				bool  sprite0_this_px = false;

				if (PPU.sprite_enable) {
					int px = PPU.dot - 1;           // screen X (0-255)

					// Walk all 64 sprites; render the FIRST (lowest OAM index)
					// non-transparent one that covers this pixel.
					for (int s = 0; s < 64; s++) {
						uint8_t sy    = PPU.oam[s * 4 + 0]; // Y position (top-1)
						uint8_t stile = PPU.oam[s * 4 + 1]; // tile index
						uint8_t sattr = PPU.oam[s * 4 + 2]; // attributes
						uint8_t sx    = PPU.oam[s * 4 + 3]; // X position

						// Check X overlap
						if (px < sx || px >= sx + 8) continue;

						// Check Y overlap
						int row = PPU.line - (int)sy - 1; // sy is stored as (top - 1)
						if (row < 0 || row >= PPU.sprite_height) continue;

						bool flip_h = (sattr & 0x40) != 0;
						bool flip_v = (sattr & 0x80) != 0;
						int  s_pal  = sattr & 0x03;

						if (flip_v) row = PPU.sprite_height - 1 - row;

						uint16_t s_table;
						uint8_t  s_tile_idx;

						if (PPU.sprite_height == 16) {
							// 8×16: tile bit 0 selects pattern table; top/bottom half
							s_table    = (stile & 0x01) ? 0x1000 : 0x0000;
							s_tile_idx = stile & 0xFE;
							if (row >= 8) { s_tile_idx++; row -= 8; }
						} else {
							s_table    = PPU.sprite_table_addr;
							s_tile_idx = stile;
						}

						uint16_t s_plane0 = s_table + (s_tile_idx * 16) + row;
						uint8_t  s_low    = ppu_read(s_plane0);
						uint8_t  s_high   = ppu_read(s_plane0 + 8);

						int s_bit         = flip_h ? (px - sx) : (7 - (px - sx));
						int s_color_index = (((s_high >> s_bit) & 1) << 1)
							|  ((s_low  >> s_bit) & 1);

						if (s_color_index == 0) continue; // transparent

						uint16_t s_pal_addr = 0x3F10 + (s_pal * 4) + s_color_index;
						sprite_pixel    = palette[ppu_read(s_pal_addr) & 0x3F];
						sprite_priority = (sattr & 0x20) != 0; // 1 = behind BG
						sprite0_this_px = (s == 0);
						break; // first non-transparent sprite wins
					}
				}

				if (sprite0_this_px && bg_color_index && PPU.dot != 256) {
					PPU.sprite0hit = true;
				}

				Color final_pixel = bg_pixel;  // start with BG (or universal BG)
				if (*(uint32_t *) &sprite_pixel && (!sprite_priority || !bg_color_index)) {
					final_pixel = sprite_pixel;
				}

				screen_buffer[256 * PPU.line + PPU.dot - 1] = final_pixel;
			}

		} else if (PPU.dot <= 320) {
			PPU.oam_addr = 0;
		}

incr:
		PPU.last_update++;
		PPU.dot++;
		if (PPU.dot > 340) {
			PPU.dot = 0;
			PPU.line++;
			if (PPU.line > 261)
				PPU.line = 0;
		}
	}
	if (!c) return;
	// fprintf(stderr, "0x%04X, 0x%02X\n", c->addr, c->value);
	ppu_write_reg(c->addr, c->value);
}

void ppu_catch_up(void) {
	arr_foreach(c, PPU.queue) {
		ppu_exec_cmd(c);
	}
	if (PPU.queue) arr_clear(PPU.queue);
	ppu_exec_cmd(NULL);
}

size_t addr_to_prg_rom(Addr addr) {
	switch (mapper) {
		case 0:
			if (addr < 0x8000) {
				// fprintf(stderr, "Tried to access prg_rom outside of range ! (0x%04X)\n",
				// 		addr);
				// exit(1);
				return -1;
			}
			return (addr - 0x8000) % prg_rom_len;
		default:
			TODO("Only mapper 0 is implemented");
	}
}

Byte cpu_read(Addr addr) {
	if (addr < 0x2000) {
		return ram[addr & 0x07FF];
	} else if (addr < 0x4000) {
		return ppu_read_reg(addr);
	} else if (addr < 0x4018) {
		if (addr == 0x4016) {
			Byte ret = controller_buffer & 1;
			controller_buffer >>= 1;
			return ret;
		}
		return APU_IO_reg[addr - 0x4000];
	} else if (addr < 0x401F) {
		fprintf(stderr, "Tried to access test-mode memory ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr < 0x6000) {
		size_t i = addr_to_prg_rom(addr);
		return prg_rom[i];
	} else if (addr < 0x8000) {
		return prg_ram[addr-0x6000];
	} else {
		// handle mappers after
		size_t i = addr_to_prg_rom(addr);
		return prg_rom[i];
	}
}

void cpu_write(Addr addr, Byte value) {
	if (addr < 0x2000) {
		ram[addr & 0x07FF] = value;
	} else if (addr < 0x4000) {
		PPU_cmd c = (PPU_cmd) {.at = total_cpu_cycles, .addr = addr, .value = value};
		arr_append(PPU.queue, c);
	} else if (addr < 0x4018) {
		if (addr == 0x4014) {
			ppu_catch_up();
			uint16_t page = value << 8;
			for (int i = 0; i < 256; i++) {
				PPU.oam[i] = cpu_read(page + i);
			}

			cycle_budget -= 513;
			total_cpu_cycles += 513;
		}

		APU_IO_reg[addr - 0x4000] = value;
	} else if (addr < 0x401F) {
		fprintf(stderr, "Tried to write to test-mode memory ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr < 0x6000) {
		fprintf(stderr, "Tried to write to ROM ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr < 0x8000) {
		if (addr == 0x6000) {
			// 0x80 means "running/resetting", so we only care if it's 
			// a final result (0x00 for pass, 0x01+ for fail)
			if (value != 0x80) {
				printf("\n--- BLARGG TEST FINISHED ---\n");
				if (value == 0x00) {
					printf("Result: PASSED!\n");
				} else {
					printf("Result: FAILED (Error Code: 0x%02X)\n", value);
				}

				// Print the text buffer from 0x6004
				printf("Console Output:\n%s\n", (char*)&prg_ram[0x4]);
			}
		}
		prg_ram[addr-0x6000] = value;
	} else {
		fprintf(stderr, "Tried to write to ROM ! (0x%04X)\n", addr);
		exit(1);
	}
	return;
}

void nes_init(void) {
	PC = cpu_read(0xFFFC) & 0xFF;
	PC |= cpu_read(0xFFFD) << 8;
	total_cpu_cycles = 7;
	SP = 0xFD;
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
	if (!PPU.nmi_enable) return;

	push((PC >> 8) & 0xFF);
	push(PC & 0xFF);
	push((flags() & 0xEF) | 0x20);

	uint8_t low = cpu_read(0xFFFA);
	uint8_t high = cpu_read(0xFFFB);

	PC = (high << 8) | low;
	total_cpu_cycles += 7;
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


// -- Interpreter --

void interpret_pc(void) {
	OpKind ins = cpu_read(PC);
	Op op = OPS[ins];
	uint16_t operands = 0;
	switch (op.size) {
		case 0:
			fprintf(stderr, "Tried to execute non-instruction (0x%02X) in ram !\n", ins);
			exit(1);
			return;
		case 1:
			break;
		case 2:
			operands = cpu_read(PC+1);
			break;
		case 3:
			operands = cpu_read(PC+1);
			operands |= cpu_read(PC+2) << 8;
			break;
	}
	TICK(op.size, base_cycles(op));
	switch ((Byte) ins) {
		case ADC_IMM:
			ADC(MODE_IMM, operands);
			break;
		case ADC_ZP:
			ADC(MODE_ZP, operands);
			break;
		case ADC_ZP_X:
			ADC(MODE_ZP_X, operands);
			break;
		case ADC_ABS:
			ADC(MODE_ABS, operands);
			break;
		case ADC_ABS_X:
			ADC(MODE_ABS_X, operands);
			break;
		case ADC_ABS_Y:
			ADC(MODE_ABS_Y, operands);
			break;
		case ADC_IND_X:
			ADC(MODE_IND_X, operands);
			break;
		case ADC_IND_Y:
			ADC(MODE_IND_Y, operands);
			break;
		case AND_IMM:
			AND(MODE_IMM, operands);
			break;
		case AND_ZP:
			AND(MODE_ZP, operands);
			break;
		case AND_ZP_X:
			AND(MODE_ZP_X, operands);
			break;
		case AND_ABS:
			AND(MODE_ABS, operands);
			break;
		case AND_ABS_X:
			AND(MODE_ABS_X, operands);
			break;
		case AND_ABS_Y:
			AND(MODE_ABS_Y, operands);
			break;
		case AND_IND_X:
			AND(MODE_IND_X, operands);
			break;
		case AND_IND_Y:
			AND(MODE_IND_Y, operands);
			break;
		case ASL_ACC:
			ASL(MODE_ACC, operands);
			break;
		case ASL_ZP:
			ASL(MODE_ZP, operands);
			break;
		case ASL_ZP_X:
			ASL(MODE_ZP_X, operands);
			break;
		case ASL_ABS:
			ASL(MODE_ABS, operands);
			break;
		case ASL_ABS_X:
			ASL(MODE_ABS_X, operands);
			break;
		case BCC:
            BCC(MODE_IMM, operands);
            break;
		case BCS:
            BCS(MODE_IMM, operands);
            break;
		case BEQ:
            BEQ(MODE_IMM, operands);
            break;
		case BIT_ZP:
			BIT(MODE_ZP, operands);
			break;
		case BIT_ABS:
			BIT(MODE_ABS, operands);
			break;
		case BMI:
            BMI(MODE_IMM, operands);
            break;
		case BNE:
            BNE(MODE_IMM, operands);
            break;
		case BPL:
            BPL(MODE_IMM, operands);
            break;
		case BVC:
            BVC(MODE_IMM, operands);
            break;
		case BVS:
            BVS(MODE_IMM, operands);
            break;
		case BRK:
            BRK(MODE_NONE, operands);
            break;
		case CLC:
            CLC(MODE_NONE, operands);
            break;
		case CLD:
            CLD(MODE_NONE, operands);
            break;
		case CLI:
            CLI(MODE_NONE, operands);
            break;
		case CLV:
            CLV(MODE_NONE, operands);
            break;
		case CMP_IMM:
			CMP(MODE_IMM, operands);
			break;
		case CMP_ZP:
			CMP(MODE_ZP, operands);
			break;
		case CMP_ZP_X:
			CMP(MODE_ZP_X, operands);
			break;
		case CMP_ABS:
			CMP(MODE_ABS, operands);
			break;
		case CMP_ABS_X:
			CMP(MODE_ABS_X, operands);
			break;
		case CMP_ABS_Y:
			CMP(MODE_ABS_Y, operands);
			break;
		case CMP_IND_X:
			CMP(MODE_IND_X, operands);
			break;
		case CMP_IND_Y:
			CMP(MODE_IND_Y, operands);
			break;
		case CPX_IMM:
			CPX(MODE_IMM, operands);
			break;
		case CPX_ZP:
			CPX(MODE_ZP, operands);
			break;
		case CPX_ABS:
			CPX(MODE_ABS, operands);
			break;
		case CPY_IMM:
			CPY(MODE_IMM, operands);
			break;
		case CPY_ZP:
			CPY(MODE_ZP, operands);
			break;
		case CPY_ABS:
			CPY(MODE_ABS, operands);
			break;
		case DEC_ZP:
			DEC(MODE_ZP, operands);
			break;
		case DEC_ZP_X:
			DEC(MODE_ZP_X, operands);
			break;
		case DEC_ABS:
			DEC(MODE_ABS, operands);
			break;
		case DEC_ABS_X:
			DEC(MODE_ABS_X, operands);
			break;
		case DEX:
            DEX(MODE_NONE, operands);
            break;
		case DEY:
            DEY(MODE_NONE, operands);
            break;
		case EOR_IMM:
			EOR(MODE_IMM, operands);
			break;
		case EOR_ZP:
			EOR(MODE_ZP, operands);
			break;
		case EOR_ZP_X:
			EOR(MODE_ZP_X, operands);
			break;
		case EOR_ABS:
			EOR(MODE_ABS, operands);
			break;
		case EOR_ABS_X:
			EOR(MODE_ABS_X, operands);
			break;
		case EOR_ABS_Y:
			EOR(MODE_ABS_Y, operands);
			break;
		case EOR_IND_X:
			EOR(MODE_IND_X, operands);
			break;
		case EOR_IND_Y:
			EOR(MODE_IND_Y, operands);
			break;
		case INC_ZP:
			INC(MODE_ZP, operands);
			break;
		case INC_ZP_X:
			INC(MODE_ZP_X, operands);
			break;
		case INC_ABS:
			INC(MODE_ABS, operands);
			break;
		case INC_ABS_X:
			INC(MODE_ABS_X, operands);
			break;
		case INX:
            INX(MODE_NONE, operands);
            break;
		case INY:
            INY(MODE_NONE, operands);
            break;
		case JMP_ABS:
			JMP(MODE_IMM2, operands);
			break;
		case JMP_IND:
			JMP(MODE_IND, operands);
			break;
		case JSR:
            JSR(MODE_IMM2, operands);
            break;
		case LDA_IMM:
			LDA(MODE_IMM, operands);
			break;
		case LDA_ZP:
			LDA(MODE_ZP, operands);
			break;
		case LDA_ZP_X:
			LDA(MODE_ZP_X, operands);
			break;
		case LDA_ABS:
			LDA(MODE_ABS, operands);
			break;
		case LDA_ABS_X:
			LDA(MODE_ABS_X, operands);
			break;
		case LDA_ABS_Y:
			LDA(MODE_ABS_Y, operands);
			break;
		case LDA_IND_X:
			LDA(MODE_IND_X, operands);
			break;
		case LDA_IND_Y:
			LDA(MODE_IND_Y, operands);
			break;
		case LDX_IMM:
			LDX(MODE_IMM, operands);
			break;
		case LDX_ZP:
			LDX(MODE_ZP, operands);
			break;
		case LDX_ZP_Y:
			LDX(MODE_ZP_Y, operands);
			break;
		case LDX_ABS:
			LDX(MODE_ABS, operands);
			break;
		case LDX_ABS_Y:
			LDX(MODE_ABS_Y, operands);
			break;
		case LDY_IMM:
			LDY(MODE_IMM, operands);
			break;
		case LDY_ZP:
			LDY(MODE_ZP, operands);
			break;
		case LDY_ZP_X:
			LDY(MODE_ZP_X, operands);
			break;
		case LDY_ABS:
			LDY(MODE_ABS, operands);
			break;
		case LDY_ABS_X:
			LDY(MODE_ABS_X, operands);
			break;
		case LSR_ACC:
			LSR(MODE_ACC, operands);
			break;
		case LSR_ZP:
			LSR(MODE_ZP, operands);
			break;
		case LSR_ZP_X:
			LSR(MODE_ZP_X, operands);
			break;
		case LSR_ABS:
			LSR(MODE_ABS, operands);
			break;
		case LSR_ABS_X:
			LSR(MODE_ABS_X, operands);
			break;
		case 0x1A: case 0x3A: case 0x5A:
		case 0x7A: case 0xDA: case 0xFA:
		case NOP:
            NOP(MODE_NONE, operands);
            break;
		case 0x04: case 0x44: case 0x64:
            NOP(MODE_ZP, operands);
            break;
		case 0x14: case 0x34: case 0x54:
		case 0x74: case 0xD4: case 0xF4:
            NOP(MODE_ZP_X, operands);
            break;
		case 0x0C:
            NOP(MODE_ABS, operands);
            break;
		case 0x1C: case 0x3C: case 0x5C:
		case 0x7C: case 0xDC: case 0xFC:
            NOP(MODE_ABS_X, operands);
            break;
		case 0x80:
            NOP(MODE_IMM, operands);
            break;
		case ORA_IMM:
			ORA(MODE_IMM, operands);
			break;
		case ORA_ZP:
			ORA(MODE_ZP, operands);
			break;
		case ORA_ZP_X:
			ORA(MODE_ZP_X, operands);
			break;
		case ORA_ABS:
			ORA(MODE_ABS, operands);
			break;
		case ORA_ABS_X:
			ORA(MODE_ABS_X, operands);
			break;
		case ORA_ABS_Y:
			ORA(MODE_ABS_Y, operands);
			break;
		case ORA_IND_X:
			ORA(MODE_IND_X, operands);
			break;
		case ORA_IND_Y:
			ORA(MODE_IND_Y, operands);
			break;
		case PHA:
            PHA(MODE_NONE, operands);
            break;
		case PHP:
            PHP(MODE_NONE, operands);
            break;
		case PLA:
            PLA(MODE_NONE, operands);
            break;
		case PLP:
            PLP(MODE_NONE, operands);
            break;
		case ROL_ACC:
			ROL(MODE_ACC, operands);
			break;
		case ROL_ZP:
			ROL(MODE_ZP, operands);
			break;
		case ROL_ZP_X:
			ROL(MODE_ZP_X, operands);
			break;
		case ROL_ABS:
			ROL(MODE_ABS, operands);
			break;
		case ROL_ABS_X:
			ROL(MODE_ABS_X, operands);
			break;
		case ROR_ACC:
			ROR(MODE_ACC, operands);
			break;
		case ROR_ZP:
			ROR(MODE_ZP, operands);
			break;
		case ROR_ZP_X:
			ROR(MODE_ZP_X, operands);
			break;
		case ROR_ABS:
			ROR(MODE_ABS, operands);
			break;
		case ROR_ABS_X:
			ROR(MODE_ABS_X, operands);
			break;
		case RTI:
            RTI(MODE_NONE, operands);
            break;
		case RTS:
            RTS(MODE_NONE, operands);
            break;
		case SBC_IMM:
			SBC(MODE_IMM, operands);
			break;
		case SBC_ZP:
			SBC(MODE_ZP, operands);
			break;
		case SBC_ZP_X:
			SBC(MODE_ZP_X, operands);
			break;
		case SBC_ABS:
			SBC(MODE_ABS, operands);
			break;
		case SBC_ABS_X:
			SBC(MODE_ABS_X, operands);
			break;
		case SBC_ABS_Y:
			SBC(MODE_ABS_Y, operands);
			break;
		case SBC_IND_X:
			SBC(MODE_IND_X, operands);
			break;
		case SBC_IND_Y:
			SBC(MODE_IND_Y, operands);
			break;
		case SEC:
            SEC(MODE_NONE, operands);
            break;
		case SED:
            SED(MODE_NONE, operands);
            break;
		case SEI:
            SEI(MODE_NONE, operands);
            break;
		case STA_ZP:
			STA(MODE_ZP, operands);
			break;
		case STA_ZP_X:
			STA(MODE_ZP_X, operands);
			break;
		case STA_ABS:
			STA(MODE_ABS, operands);
			break;
		case STA_ABS_X:
			STA(MODE_ABS_X, operands);
			break;
		case STA_ABS_Y:
			STA(MODE_ABS_Y, operands);
			break;
		case STA_IND_X:
			STA(MODE_IND_X, operands);
			break;
		case STA_IND_Y:
			STA(MODE_IND_Y, operands);
			break;
		case STX_ZP:
			STX(MODE_ZP, operands);
			break;
		case STX_ZP_Y:
			STX(MODE_ZP_Y, operands);
			break;
		case STX_ABS:
			STX(MODE_ABS, operands);
			break;
		case STY_ZP:
			STY(MODE_ZP, operands);
			break;
		case STY_ZP_X:
			STY(MODE_ZP_X, operands);
			break;
		case STY_ABS:
			STY(MODE_ABS, operands);
			break;
		case TAX:
			TAX(MODE_NONE, operands);
			break;
		case TAY:
			TAY(MODE_NONE, operands);
			break;
		case TSX:
			TSX(MODE_NONE, operands);
			break;
		case TXA:
			TXA(MODE_NONE, operands);
			break;
		case TXS:
			TXS(MODE_NONE, operands);
			break;
		case TYA:
			TYA(MODE_NONE, operands);
			break;
	}
}

#endif // NESLIB_IMPLEMENTATION

#endif // NESLIB_H

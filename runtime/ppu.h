#ifndef PPU_H_
#define PPU_H_

#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "commons.h"

#define PIXELS_W 256
#define PIXELS_H 240

extern Color screen_buffer[PIXELS_H * PIXELS_W];

typedef struct {
	uint64_t at;
	Addr addr;
	Byte value;
} PPU_cmd;

typedef struct {
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
} PPU_state;

extern PPU_state PPU;

void ppu_init(void);
void ppu_catch_up(void);
void trigger_nmi(void);
void ppu_exec_cmd(PPU_cmd *c);

Texture2D ppu_get_texture(void);

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

#endif // PPU_H_

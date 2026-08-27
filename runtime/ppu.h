#ifndef PPU_H_
#define PPU_H_

#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "commons.h"

#define SCANLINES_PER_FRAME    262
#define DOTS_PER_SCANLINE      341

#define VISIBLE_LINE_MAX       239
#define PRERENDER_LINE         261
#define POSTRENDER_LINE        240
#define VBLANK_START_LINE      241

#define FETCH_WINDOW_A_START   1
#define FETCH_WINDOW_A_END     256
#define FETCH_WINDOW_B_START   321
#define FETCH_WINDOW_B_END     336
#define TILE_FETCH_PERIOD      8

extern Color screen_buffer[PIXELS_H * PIXELS_W];

typedef struct {
	uint16_t value;
	bool changed;
} VRecord;

typedef struct {
	Byte nametables[0x800];
	Byte pal_ram[0x20];
	Byte oam[4*64];

	Byte oam_addr;

	uint16_t v;   // current VRAM address (15 bits used)
	uint16_t t;   // temporary VRAM address / scroll latch
	uint8_t  x;   // fine X scroll (3 bits used)
	bool     w;   // write toggle
	
	VRecord v_record[32];
	unsigned int last_v_record_dot;
	unsigned int sprite0hit_dot;
	bool sprite0hit_this_line;
	bool oam_changed;
	
	uint16_t bg_table_addr;     // 0 or 0x1000
	uint16_t sprite_table_addr; // 0 or 0x1000
	uint8_t  sprite_height;     // 8 or 16
	uint8_t  increment;         // 1 or 32
	uint8_t  buffer;            // PPUDATA read buffer

	bool nmi_enable;
	bool sprite_enable;
	bool bg_enable;
	bool bg_left_enable;
	bool sprite_left_enable;
	bool vblank;
	bool sprite0hit;
	bool sprite_overflow;
	bool odd_frame;
} PPU_state;

extern PPU_state PPU;

void ppu_init(void);
void ppu_catch_up(void);
void trigger_nmi(void);
void populate_shader_textures(unsigned int n);

Texture2D ppu_get_texture(void);

static inline void ppu_write(Addr addr, Byte value) {
	// Only mapper 0 atm
	if (addr < 0x2000) {
		// just don't write to rom
		return;
	} else if (addr < 0x3F00) {
		PPU.nametables[(addr - 0x2000) & 0x7FF] = value;
	} else if (addr < 0x4000) {
		Byte palette_addr = (addr - 0x3F00) & 0x1F;
        if ((palette_addr & 0x13) == 0x10) palette_addr &= 0x0F;
        PPU.pal_ram[palette_addr] = value;
	} else {
		fprintf(stderr, "Tried to write outside of PPU ram (0x%04X) !\n", addr);
		exit(1);
	}
}

static inline Byte ppu_read(Addr addr) {
	// Only mapper 0 atm
	if (addr < 0x2000) {
		return chr_rom[addr];
	} else if (addr < 0x3F00) {
		return PPU.nametables[(addr - 0x2000) & 0x7FF];
	} else if (addr < 0x4000) {
		Byte palette_addr = (addr - 0x3F00) & 0x1F;
        if ((palette_addr & 0x13) == 0x10) palette_addr &= 0x0F;
        return PPU.pal_ram[palette_addr];
	} else {
		fprintf(stderr, "Tried to read outside of PPU ram (0x%04X) !\n", addr);
		exit(1);
	}
}

#endif // PPU_H_

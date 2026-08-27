// ppu.c: natemu PPU emulation - rendering, registers, shader upload
//
// Copyright (C) 2026 Maxime Delhaye
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "ppu.h"
#include "cpu.h"
#include "bus.h"
#include "../cut.h"

static const float PALETTE[64 * 3] = {
    0x7C/255.0f, 0x7C/255.0f, 0x7C/255.0f,  0x00/255.0f, 0x00/255.0f, 0xFC/255.0f,  0x00/255.0f, 0x00/255.0f, 0xBC/255.0f,  0x44/255.0f, 0x28/255.0f, 0xBC/255.0f,
    0x94/255.0f, 0x00/255.0f, 0x84/255.0f,  0xA8/255.0f, 0x00/255.0f, 0x20/255.0f,  0xA8/255.0f, 0x10/255.0f, 0x00/255.0f,  0x88/255.0f, 0x14/255.0f, 0x00/255.0f,
    0x50/255.0f, 0x30/255.0f, 0x00/255.0f,  0x00/255.0f, 0x78/255.0f, 0x00/255.0f,  0x00/255.0f, 0x68/255.0f, 0x00/255.0f,  0x00/255.0f, 0x58/255.0f, 0x00/255.0f,
    0x00/255.0f, 0x40/255.0f, 0x58/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,

    0xBC/255.0f, 0xBC/255.0f, 0xBC/255.0f,  0x00/255.0f, 0x78/255.0f, 0xF8/255.0f,  0x00/255.0f, 0x58/255.0f, 0xF8/255.0f,  0x68/255.0f, 0x44/255.0f, 0xFC/255.0f,
    0xD8/255.0f, 0x00/255.0f, 0xCC/255.0f,  0xE4/255.0f, 0x00/255.0f, 0x58/255.0f,  0xF8/255.0f, 0x38/255.0f, 0x00/255.0f,  0xE4/255.0f, 0x5C/255.0f, 0x10/255.0f,
    0xAC/255.0f, 0x7C/255.0f, 0x00/255.0f,  0x00/255.0f, 0xB8/255.0f, 0x00/255.0f,  0x00/255.0f, 0xA8/255.0f, 0x00/255.0f,  0x00/255.0f, 0xA8/255.0f, 0x44/255.0f,
    0x00/255.0f, 0x88/255.0f, 0x88/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,

    0xF8/255.0f, 0xF8/255.0f, 0xF8/255.0f,  0x3C/255.0f, 0xBC/255.0f, 0xFC/255.0f,  0x68/255.0f, 0x88/255.0f, 0xFC/255.0f,  0x98/255.0f, 0x78/255.0f, 0xF8/255.0f,
    0xF8/255.0f, 0x78/255.0f, 0xF8/255.0f,  0xF8/255.0f, 0x58/255.0f, 0x98/255.0f,  0xF8/255.0f, 0x78/255.0f, 0x58/255.0f,  0xFC/255.0f, 0xA0/255.0f, 0x44/255.0f,
    0xF8/255.0f, 0xB8/255.0f, 0x00/255.0f,  0xB8/255.0f, 0xF8/255.0f, 0x18/255.0f,  0x58/255.0f, 0xD8/255.0f, 0x54/255.0f,  0x58/255.0f, 0xF8/255.0f, 0x98/255.0f,
    0x00/255.0f, 0xE8/255.0f, 0xD8/255.0f,  0x78/255.0f, 0x78/255.0f, 0x78/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,

    0xFC/255.0f, 0xFC/255.0f, 0xFC/255.0f,  0xA4/255.0f, 0xE4/255.0f, 0xFC/255.0f,  0xB8/255.0f, 0xB8/255.0f, 0xF8/255.0f,  0xD8/255.0f, 0xB8/255.0f, 0xF8/255.0f,
    0xF8/255.0f, 0xB8/255.0f, 0xF8/255.0f,  0xF8/255.0f, 0xA4/255.0f, 0xC0/255.0f,  0xF0/255.0f, 0xD0/255.0f, 0xB0/255.0f,  0xFC/255.0f, 0xE0/255.0f, 0xA8/255.0f,
    0xF8/255.0f, 0xD8/255.0f, 0x78/255.0f,  0xD8/255.0f, 0xF8/255.0f, 0x78/255.0f,  0xB8/255.0f, 0xF8/255.0f, 0xB8/255.0f,  0xB8/255.0f, 0xF8/255.0f, 0xD8/255.0f,
    0x00/255.0f, 0xFC/255.0f, 0xFC/255.0f,  0xF8/255.0f, 0xD8/255.0f, 0xF8/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f,  0x00/255.0f, 0x00/255.0f, 0x00/255.0f
};

PPU_state PPU;

// ---- Loopy register (v/t) bit layout ----
#define LOOPY_COARSE_X_MASK   0x001F
#define LOOPY_COARSE_Y_MASK   0x03E0
#define LOOPY_NT_MASK         0x0C00
#define LOOPY_NT_X_BIT        0x0400
#define LOOPY_NT_Y_BIT        0x0800
#define LOOPY_FINE_Y_MASK     0x7000

#define LOOPY_COARSE_X_SPAN   32       // coarse X wraps mod 32
#define LOOPY_COARSE_Y_MAX    31
#define LOOPY_COARSE_Y_WRAP   29

#define LOOPY_HORI_RELOAD_MASK  (LOOPY_COARSE_X_MASK | LOOPY_NT_X_BIT)
#define LOOPY_VERT_RELOAD_MASK  (LOOPY_FINE_Y_MASK | LOOPY_COARSE_Y_MASK | LOOPY_NT_Y_BIT)

#define LOOPY_FINE_Y_STEP     0x1000

#define Y_INCREMENT_DOT       256
#define HORI_RELOAD_DOT       257
#define VERT_RELOAD_START_DOT 280
#define VERT_RELOAD_END_DOT   304

#define OAM_ATTR_FLIP_H_BIT      0x40
#define OAM_ATTR_FLIP_V_BIT      0x80
#define OAM_TILE16_BANK_BIT       0x01              // 0: $0000, 1: $1000
#define OAM_TILE16_INDEX_MASK     0xFE               // top 7 bits select the tile pair
#define OAM_TILE16_GET_BANK(t)    (((t) & OAM_TILE16_BANK_BIT) ? 0x1000 : 0x0000)
#define OAM_TILE16_GET_INDEX(t)   ((t) & OAM_TILE16_INDEX_MASK)

// ---- Access macros ----
#define LOOPY_GET_COARSE_X(r) ((r) & LOOPY_COARSE_X_MASK)
#define LOOPY_GET_COARSE_Y(r) (((r) & LOOPY_COARSE_Y_MASK) >> 5)
#define LOOPY_GET_FINE_Y(r)   (((r) & LOOPY_FINE_Y_MASK) >> 12)
#define LOOPY_GET_NT(r)       (((r) & LOOPY_NT_MASK) >> 10)
#define NT_ADDR_FROM_V(v) (0x2000 | ((v) & 0x0FFF))
#define ATTR_ADDR_FROM_V(v) \
    (0x23C0 | ((v) & LOOPY_NT_MASK) | (((v) >> 4) & 0x38) | (((v) >> 2) & 0x07))
#define ATTR_QUADRANT_FROM_COARSE(coarse_x, coarse_y) \
    ((((coarse_y) & 0x02) << 1) | ((coarse_x) & 0x02))
#define ATTR_GET_PALETTE(byte, quadrant_shift) (((byte) >> (quadrant_shift)) & 0x03)

static inline bool is_rendering_line(unsigned int line) {
    return line <= VISIBLE_LINE_MAX || line == PRERENDER_LINE;
}

static inline void advance_coarse_x(uint16_t *v, unsigned int count) {
    unsigned int total = LOOPY_GET_COARSE_X(*v) + count;
    unsigned int new_coarse_x = total % LOOPY_COARSE_X_SPAN;
    unsigned int wraps = total / LOOPY_COARSE_X_SPAN;

    *v = (*v & ~LOOPY_COARSE_X_MASK) | new_coarse_x;
    if (wraps & 1) {
        *v ^= LOOPY_NT_X_BIT;
    }
}

static inline void increment_y(uint16_t *v) {
    if ((*v & LOOPY_FINE_Y_MASK) != LOOPY_FINE_Y_MASK) {
        *v += LOOPY_FINE_Y_STEP;
		return;
    }         
	*v &= ~LOOPY_FINE_Y_MASK;
	uint16_t coarse_y = LOOPY_GET_COARSE_Y(*v);

	switch (coarse_y) {
		case LOOPY_COARSE_Y_WRAP:
			coarse_y = 0;
			*v ^= LOOPY_NT_Y_BIT;
			break;
		case LOOPY_COARSE_Y_MAX:
			coarse_y = 0;
			break;
		default:
			coarse_y += 1;
			break;
	}
	*v = (*v & ~LOOPY_COARSE_Y_MASK) | (coarse_y << 5);
}

void catch_up_v(unsigned int current_dot, unsigned int line) {
    if (!(PPU.bg_enable || PPU.sprite_enable)) return;
    if (!(line <= VISIBLE_LINE_MAX || line == PRERENDER_LINE)) return;

    unsigned int lo = PPU.last_v_record_dot + 1;
	unsigned int hi = current_dot;
    if (lo > hi) return;

    // Coarse X: count multiples of 8 in (last_dot, current_dot], clipped
    // to the two fetch windows.
    unsigned int steps = 0;
    unsigned int a_hi = hi < FETCH_WINDOW_A_END ? hi : FETCH_WINDOW_A_END;
    if (lo <= a_hi) steps += a_hi / 8 - (lo - 1) / 8;

    if (hi >= FETCH_WINDOW_B_START) {
        unsigned int b_lo = lo > FETCH_WINDOW_B_START ? lo : FETCH_WINDOW_B_START;
        unsigned int b_hi = hi < FETCH_WINDOW_B_END ? hi : FETCH_WINDOW_B_END;
        if (b_lo <= b_hi) steps += b_hi / 8 - (b_lo - 1) / 8;
    }

    if (steps) advance_coarse_x(&PPU.v, steps);

    if (line != PRERENDER_LINE &&
        lo <= Y_INCREMENT_DOT && Y_INCREMENT_DOT <= hi)
        increment_y(&PPU.v);

    if (lo <= HORI_RELOAD_DOT && HORI_RELOAD_DOT <= hi)
        PPU.v = (PPU.v & ~LOOPY_HORI_RELOAD_MASK) | (PPU.t & LOOPY_HORI_RELOAD_MASK);

    if (line == PRERENDER_LINE && lo <= VERT_RELOAD_END_DOT && hi >= VERT_RELOAD_START_DOT)
        PPU.v = (PPU.v & ~LOOPY_VERT_RELOAD_MASK) | (PPU.t & LOOPY_VERT_RELOAD_MASK);
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

Byte ppu_read_reg(Addr addr) {
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
			unsigned int dot = (total_cpu_cycles - cpu_cycles_line_start) * 3;
			ret = PPU.buffer;
			catch_up_v(dot, current_line);
			PPU.buffer = ppu_read(PPU.v & 0x7FFF);

			PPU.v += PPU.increment;

			PPU.last_v_record_dot = dot;
			size_t index = (dot) / 8;
			if (index < 32)
				PPU.v_record[index] = (VRecord) {.changed = true, .value = PPU.v};
			return ret;
	}
	return 0;
}

inline void ppu_write_reg(Addr addr, Byte value) {
	switch (addr & 0x7) {
		case 0: // PPU CTRL
			{
				bool nmi_old = PPU.nmi_enable;
				PPU.t = (PPU.t & ~LOOPY_NT_MASK) | ((value & 0x03) << 10);
				PPU.increment = (value & 0x04) ? 32 : 1;
				PPU.sprite_table_addr = (value & 0x08) ? 0x1000 : 0x0;
				PPU.bg_table_addr = (value & 0x10) ? 0x1000 : 0x0;
				PPU.sprite_height = (value & 0x20) ? 16 : 8;
				PPU.nmi_enable = value >> 7;

				if (!nmi_old && PPU.nmi_enable) {
					if (PPU.vblank) {
						trigger_nmi();
					}
				}
			}
			break;
		case 1: // PPU MASK
			PPU.bg_enable = (value & 0x10) != 0;
			PPU.sprite_enable = (value & 0x08) != 0;
			PPU.bg_left_enable = (value & 0x02) != 0;
			PPU.sprite_left_enable = (value & 0x04) != 0;
			break;
		case 3: // PPU OAM ADDR
			PPU.oam_changed = true;
			PPU.oam_addr = value;
			break;
		case 4:
			PPU.oam_changed = true;
			PPU.oam[PPU.oam_addr++] = value;
			break;
		case 5: // PPU SCROLL
			catch_up_v((total_cpu_cycles - cpu_cycles_line_start) * 3, current_line);
			if (PPU.w) {
				PPU.t = (PPU.t & 0x0C1F) | (((uint16_t) value & 0x07) << 12) | (((uint16_t) value & 0xF8) << 2);
				PPU.w = false;
			} else {
				PPU.t = (PPU.t & ~0b11111) | value >> 3;
				PPU.x = value & 0b111;
				PPU.w = true;
			}
			break;
		case 6: 
			if (!PPU.w) {
				// First write: high byte
				PPU.t = (PPU.t & 0x00FF) | ((value & 0x3F) << 8);
				PPU.w = true;
			} else {
				// Second write: low byte
				PPU.t = (PPU.t & 0xFF00) | value;
				PPU.v = PPU.t; // Current VRAM pointer updates to match temporary pointer
				PPU.last_v_record_dot = (total_cpu_cycles - cpu_cycles_line_start) * 3;
				size_t index = (PPU.last_v_record_dot) / 8;
				if (index < 32)
					PPU.v_record[index] = (VRecord) {.changed = true, .value = PPU.v};
				PPU.w = false;
			}
			break;
		case 7: // PPU DATA
			unsigned int dot = (total_cpu_cycles - cpu_cycles_line_start) * 3;
			if (dot > 340) dot = 340;
			catch_up_v(dot, current_line);
			ppu_write(PPU.v & 0x7FFF, value);
			PPU.v += PPU.increment;
			PPU.last_v_record_dot = dot;
			size_t index = (dot) / 8;
			if (index < 32)
				PPU.v_record[index] = (VRecord) {.changed = true, .value = PPU.v};
			break;
	}
}

// ===== Shader textures (packed RGBA8) =====
static Byte bg_tex  [32 * 240 * 4];  // R=lo  G=hi  B=attr
static Byte spr_tex [ 8 * 240 * 4];  // R=y   G=x   B=attr  A=tile
static Byte misc_tex[32 * 240 * 4];  // R=pal G=fine_x B=spr_count
// CPU-side only, packed into misc_tex at upload time:
static Byte spr_count[240];
static Byte fine_x[240];
static Byte pal_ram[32 * 240];

void populate_bg_textures(unsigned int n) {
	uint16_t running_v = 0; // except for the very first line of the first frame, this will be overwritten
	if (n < 240) {
		for (size_t i = 0; i < 32; i++) {
			VRecord r = PPU.v_record[i];
			if (r.changed)
				running_v = r.value;

			uint16_t nt_addr = NT_ADDR_FROM_V(running_v);
			int tile_index = ppu_read(nt_addr);
			size_t o = (32*n + i) * 4;
			{ // attr
				uint16_t attr_addr = ATTR_ADDR_FROM_V(running_v);
				Byte attr_byte = ppu_read(attr_addr);
				uint8_t coarse_x = LOOPY_GET_COARSE_X(running_v);
				uint8_t coarse_y = LOOPY_GET_COARSE_Y(running_v);
				uint8_t quadrant = ATTR_QUADRANT_FROM_COARSE(coarse_x, coarse_y);
				bg_tex[o + 2] = ATTR_GET_PALETTE(attr_byte, quadrant);
			}
			{ // pattern
				Byte fine_y = LOOPY_GET_FINE_Y(running_v);
				uint16_t pattern_addr = PPU.bg_table_addr | (tile_index << 4) | fine_y;
				bg_tex[o + 0] = ppu_read(pattern_addr);
				bg_tex[o + 1] = ppu_read(pattern_addr | 0x8);
			}

			advance_coarse_x(&running_v, 1);
		}
	}

	catch_up_v(339, n);
	memset(PPU.v_record, 0, sizeof(VRecord) * 32);
	PPU.v_record[0] = (VRecord) { .changed = true, .value = PPU.v };
}

void check_sprite0_hit_speculative(unsigned int n) {
    if (PPU.sprite0hit) return;
    if (!(PPU.bg_enable && PPU.sprite_enable)) return;

	unsigned int next_line = (n == PRERENDER_LINE) ? 0 : n + 1;
	if (next_line >= 240) return; 

    Byte y = PPU.oam[0], tile = PPU.oam[1], attr = PPU.oam[2], sx = PPU.oam[3];
    if (y >= 240) return; // off-screen sentinel

    int top = y + 1;
    int bottom = top + PPU.sprite_height - 1;
    if ((int)next_line < top || (int)next_line > bottom) return;

    int row = next_line - top;
    if (attr & OAM_ATTR_FLIP_V_BIT) row = (PPU.sprite_height - 1) - row;

    uint16_t addr;
    if (PPU.sprite_height == 8) {
        addr = PPU.sprite_table_addr | (tile << 4) | row;
    } else {
        uint16_t bank = OAM_TILE16_GET_BANK(tile);
        uint16_t base_index = tile & OAM_TILE16_INDEX_MASK;
        uint16_t sub_tile = (row >= 8) ? 1 : 0;
        addr = bank | ((base_index + sub_tile) << 4) | (row & 0x7);
    }

    Byte sp_lo = ppu_read(addr);
	Byte sp_hi = ppu_read(addr | 8);
    uint16_t v = PPU.v; // speculative baseline: v as of start of this line

    for (int dx = 0; dx < 8; dx++) {
        int px = sx + dx;
        if (px >= 255) break;
        if (px < 8 && (!PPU.bg_left_enable || !PPU.sprite_left_enable))
            continue;

        int sbit = (attr & OAM_ATTR_FLIP_H_BIT) ? dx : 7 - dx;
        int sp_pixel = ((sp_hi >> sbit) & 1) << 1 | ((sp_lo >> sbit) & 1);
        if (sp_pixel == 0) continue;

		uint16_t tile_v = v;
		int physical_px = px + PPU.x;
		advance_coarse_x(&tile_v, physical_px / 8);
		int bit_in_tile = 7 - (physical_px % 8);

        int tile_index = ppu_read(NT_ADDR_FROM_V(tile_v));
        Byte fine_y = LOOPY_GET_FINE_Y(tile_v);
        uint16_t paddr = PPU.bg_table_addr | (tile_index << 4) | fine_y;
        Byte lo = ppu_read(paddr), hi = ppu_read(paddr | 8);
        int bg_pixel = ((hi >> bit_in_tile) & 1) << 1 | ((lo >> bit_in_tile) & 1);
        if (bg_pixel == 0) continue;

        PPU.sprite0hit_this_line = true;
        PPU.sprite0hit_dot = px + 2;
        break;
    }
}

void populate_spr_textures(unsigned int n) {
    if (!(n == PRERENDER_LINE || PPU.oam_changed)) return;
    if (!(n == PRERENDER_LINE || (n <= 238))) return;

    int first_line_to_clear = n == PRERENDER_LINE ? 0 : n + 1;
    for (int L = first_line_to_clear; L <= 239; L++) {
        spr_count[L] = 0;
    }

    for (int s = 0; s < 64; s++) {
        Byte y    = PPU.oam[4*s + 0];
        Byte tile = PPU.oam[4*s + 1];
        Byte attr = PPU.oam[4*s + 2];
        Byte x    = PPU.oam[4*s + 3];

        if (y >= 240) continue;

        int top = y + 1; // sprite's first visible scanline
        int bottom = top + PPU.sprite_height - 1;

        int lo = top > first_line_to_clear ? top : first_line_to_clear;
        int hi = bottom < 239 ? bottom : 239;

        for (int L = lo; L <= hi; L++) {
			if (spr_count[L] < 8) {
                uint8_t slot = spr_count[L]++;
                size_t o = (8*L + slot) * 4;
                spr_tex[o + 0] = y;
                spr_tex[o + 1] = x;
                spr_tex[o + 2] = attr;
                spr_tex[o + 3] = tile;
            } else {
                PPU.sprite_overflow = true;
            }
        }
    }

    PPU.oam_changed = false;
}

void populate_scroll_textures(unsigned int n) {
	if (!(n == PRERENDER_LINE || n < 239)) return;
	fine_x[(n+1) % 262] = PPU.x;
}
void populate_pal_ram_texture(unsigned int n) {
	if (!(n == PRERENDER_LINE || n < 239)) return;
	size_t target_i = ((n+1) % 262) * 32;
	memcpy(&pal_ram[target_i], PPU.pal_ram, 32);
}

static void pack_misc_tex(void) {
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 32; x++) {
            size_t o = (y * 32 + x) * 4;
            misc_tex[o + 0] = pal_ram[y * 32 + x];
            misc_tex[o + 1] = fine_x[y];
            misc_tex[o + 2] = spr_count[y];
            misc_tex[o + 3] = 0;
        }
    }
}

void populate_shader_textures(unsigned int n) {
	populate_bg_textures(n);
	check_sprite0_hit_speculative(n);
	populate_spr_textures(n);
	populate_scroll_textures(n);
	populate_pal_ram_texture(n);
}

typedef struct {
	Texture2D dummy_tex;

    Shader shader;
    RenderTexture2D framebuffer; // 256x240 output target
	
	Texture2D bg_tex;
    Texture2D spr_tex;
    Texture2D misc_tex;
    Texture2D chr_rom;

    int loc_bg_tex;
    int loc_spr_tex;
    int loc_misc_tex;
    int loc_chr_rom;
	int loc_sprite_table_addr;
	int loc_sprite_height;
    int loc_nes_palette;
} GPU_state;

static GPU_state gpu;

static Texture2D make_rgba_texture(int w, int h) {
    Image img = {
        .data = calloc((size_t)w * h * 4, 1),
        .width = w,
        .height = h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
    UnloadImage(img);
    return tex;
}

static Texture2D make_byte_texture(int w, int h) {
    Image img = {
        .data = calloc((size_t)w * h, 1),
        .width = w,
        .height = h,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT); // these are indices/data, never interpolate
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);
    UnloadImage(img);
    return tex;
}

void ppu_init(void) {
    gpu.shader = LoadShader(0, "runtime/ppu.fs");

    gpu.framebuffer = LoadRenderTexture(256, 240);
    SetTextureFilter(gpu.framebuffer.texture, TEXTURE_FILTER_POINT);

	gpu.bg_tex   = make_rgba_texture(32, 240);
    gpu.spr_tex  = make_rgba_texture(8, 240);
    gpu.misc_tex = make_rgba_texture(32, 240);
    gpu.chr_rom  = make_byte_texture(128, 64);

    gpu.loc_bg_tex   = GetShaderLocation(gpu.shader, "u_bg_tex");
    gpu.loc_spr_tex  = GetShaderLocation(gpu.shader, "u_spr_tex");
    gpu.loc_misc_tex = GetShaderLocation(gpu.shader, "u_misc_tex");
    gpu.loc_chr_rom  = GetShaderLocation(gpu.shader, "u_chr_rom_tex");

	gpu.loc_nes_palette       = GetShaderLocation(gpu.shader, "u_nes_palette");
	gpu.loc_sprite_height     = GetShaderLocation(gpu.shader, "u_sprite_height");
	gpu.loc_sprite_table_addr = GetShaderLocation(gpu.shader, "u_sprite_table_addr");

	SetShaderValueV(gpu.shader, gpu.loc_nes_palette, PALETTE, SHADER_UNIFORM_VEC3, 64);

	Image white_img = GenImageColor(1, 1, WHITE);
    gpu.dummy_tex = LoadTextureFromImage(white_img);
    SetTextureFilter(gpu.dummy_tex, TEXTURE_FILTER_POINT);
    UnloadImage(white_img);

}

Texture2D ppu_get_texture(void) {
    pack_misc_tex();
    UpdateTexture(gpu.bg_tex,   bg_tex);
    UpdateTexture(gpu.spr_tex,  spr_tex);
    UpdateTexture(gpu.misc_tex, misc_tex);
    UpdateTexture(gpu.chr_rom,  chr_rom);

    BeginTextureMode(gpu.framebuffer);
        ClearBackground(BLACK);
        BeginShaderMode(gpu.shader);
            SetShaderValueTexture(gpu.shader, gpu.loc_bg_tex,   gpu.bg_tex);
            SetShaderValueTexture(gpu.shader, gpu.loc_spr_tex,  gpu.spr_tex);
            SetShaderValueTexture(gpu.shader, gpu.loc_misc_tex, gpu.misc_tex);
            SetShaderValueTexture(gpu.shader, gpu.loc_chr_rom,  gpu.chr_rom);

            int sprite_height_val = PPU.sprite_height;
            int sprite_table_addr_val = PPU.sprite_table_addr;
            SetShaderValue(gpu.shader, gpu.loc_sprite_height, &sprite_height_val, SHADER_UNIFORM_INT);
            SetShaderValue(gpu.shader, gpu.loc_sprite_table_addr, &sprite_table_addr_val, SHADER_UNIFORM_INT);

            DrawTexturePro(gpu.dummy_tex,
               (Rectangle){ 0, 0, 1, 1 },
               (Rectangle){ 0, 0, 256, 240 },
               (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndShaderMode();
    EndTextureMode();

    return gpu.framebuffer.texture;
}

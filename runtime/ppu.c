#include "ppu.h"
#include "cpu.h"
#include "bus.h"
#include "../cut.h"

inline void ppu_exec_cmd(PPU_cmd *c) {
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

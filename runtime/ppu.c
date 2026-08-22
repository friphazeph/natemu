#include "ppu.h"
#include "cpu.h"
#include "bus.h"
#include "../cut.h"

#define DOTS_PER_LINE  341
#define LINES          262
#define FRAME_DOTS     (LINES*DOTS_PER_LINE)

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

typedef struct {
    Shader shader;
    RenderTexture2D framebuffer;
    
    // Texture memory buffers
    Texture2D chr_tex;
    Texture2D vram_tex;
    Texture2D oam_tex;

    // Uniform locations
    int loc_start_dot;
    int loc_vram_tex, loc_oam_tex, loc_chr_tex;
    int loc_palette_ram, loc_nes_palette;
    
    // PPUState struct field locations
    int loc_x_scroll, loc_y_scroll, loc_base_nt;
    int loc_bg_table_addr, loc_sprite_table_addr;
    int loc_sprite_height, loc_bg_enable, loc_sprite_enable;

    uint32_t last_gpu_dot;
} PPU_GPU;

static PPU_GPU gpu;

void ppu_init(void) {
	gpu.shader = LoadShader(NULL, "./runtime/ppu.fs");
    gpu.framebuffer = LoadRenderTexture(256, 240);

	int chr_height = (chr_rom_len > 0) ? (int)(chr_rom_len / 256) : 32;
    Image chr_img = {
        .data = (void*)chr_rom,
        .width = 256,
        .height = chr_height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };
    gpu.chr_tex = LoadTextureFromImage(chr_img);

	Image vram_img = {
		.data = PPU.vram,
		.width = 256,
		.height = 8,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
	};
	gpu.vram_tex = LoadTextureFromImage(vram_img);

	Image oam_img = {
		.data = PPU.oam,
		.width = 256,
		.height = 1,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
	};
	gpu.oam_tex = LoadTextureFromImage(oam_img);

    gpu.loc_start_dot          = GetShaderLocation(gpu.shader, "u_start_dot");
    gpu.loc_vram_tex           = GetShaderLocation(gpu.shader, "u_vram_tex");
    gpu.loc_oam_tex            = GetShaderLocation(gpu.shader, "u_oam_tex");
    gpu.loc_chr_tex            = GetShaderLocation(gpu.shader, "u_chr_tex");
    gpu.loc_palette_ram        = GetShaderLocation(gpu.shader, "u_palette_ram");
    gpu.loc_nes_palette        = GetShaderLocation(gpu.shader, "u_nes_palette");
    
    gpu.loc_x_scroll           = GetShaderLocation(gpu.shader, "u_ppu.x_scroll");
    gpu.loc_y_scroll           = GetShaderLocation(gpu.shader, "u_ppu.y_scroll");
    gpu.loc_base_nt            = GetShaderLocation(gpu.shader, "u_ppu.base_nt");
    gpu.loc_bg_table_addr      = GetShaderLocation(gpu.shader, "u_ppu.bg_table_addr");
    gpu.loc_sprite_table_addr  = GetShaderLocation(gpu.shader, "u_ppu.sprite_table_addr");
    gpu.loc_sprite_height      = GetShaderLocation(gpu.shader, "u_ppu.sprite_height");
    gpu.loc_bg_enable          = GetShaderLocation(gpu.shader, "u_ppu.bg_enable");
    gpu.loc_sprite_enable      = GetShaderLocation(gpu.shader, "u_ppu.sprite_enable");

    SetShaderValueV(gpu.shader, gpu.loc_nes_palette, PALETTE, SHADER_UNIFORM_VEC3, 64);

    int tex_unit_chr = 0, tex_unit_vram = 1, tex_unit_oam = 2;
    SetShaderValue(gpu.shader, gpu.loc_chr_tex,  &tex_unit_chr,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_vram_tex, &tex_unit_vram, SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_oam_tex,  &tex_unit_oam,  SHADER_UNIFORM_INT);

    gpu.last_gpu_dot = 0;
}

#define VBLANK_SET_DOT   82182  // Line 241, Dot 1
#define VBLANK_CLEAR_DOT 89002  // Line 261, Dot 1

static void update_ppu_events(uint64_t prev_dot, uint64_t current_dot) {
    for (uint64_t dot = prev_dot; dot < current_dot; dot++) {
        uint32_t frame_dot = dot % FRAME_DOTS;

        if (frame_dot == VBLANK_SET_DOT) {
            PPU.vblank = true;
            // if (PPU.nmi_enable) {
            //     trigger_nmi();
            // }
        } else if (frame_dot == VBLANK_CLEAR_DOT) {
            PPU.vblank = false;
            PPU.sprite0hit = false;
            PPU.sprite_overflow = false;
        }
    }
}

void check_sprite0_hit(uint64_t start_dot, uint64_t end_dot) {
    if (PPU.sprite0hit || !PPU.bg_enable || !PPU.sprite_enable) return;

	int sy       = (int)PPU.oam[0] + 1;
	uint8_t tile = PPU.oam[1];
	uint8_t attr = PPU.oam[2];
	uint8_t sx   = PPU.oam[3];

	for (uint64_t d = start_dot; d < end_dot; d++) {
		uint32_t frame_dot = d % FRAME_DOTS;
		int line  = frame_dot / DOTS_PER_LINE;
		int dot_x = frame_dot % DOTS_PER_LINE;

		if (dot_x < 1 || dot_x >= 256 || line >= 240) continue;
		int px = dot_x - 1;
		if (px < sx || px >= sx + 8) continue;
		if (line < sy || line >= sy + PPU.sprite_height) continue;

		int py  = line - sy;
		int row = (attr & 0x80) ? (PPU.sprite_height - 1 - py) : py; // Flip V

		uint16_t table = (PPU.sprite_height == 16) ? ((tile & 1) ? 0x1000 : 0x00) : PPU.sprite_table_addr;
		uint8_t  idx   = (PPU.sprite_height == 16) ? ((tile & 0xFE) + (row >= 8 ? 1 : 0)) : tile;

		uint8_t low  = ppu_read(table + (idx * 16) + (row % 8));
		uint8_t high = ppu_read(table + (idx * 16) + (row % 8) + 8);

		int s_bit   = (attr & 0x40) ? (px - sx) : (7 - (px - sx)); // Flip H, relative to sprite
		int s_color = (((high >> s_bit) & 1) << 1) | ((low >> s_bit) & 1);
		if (s_color == 0) continue;

        // Fetch background color index at dot_x, PPU.line
        int wx = px + PPU.x_scroll;
        int wy = line + PPU.y_scroll;

        int total_tx = (wx >> 3) + ((PPU.base_nt & 1) * 32);
        int total_ty = (wy >> 3) + ((PPU.base_nt & 2) ? 30 : 0);

        uint16_t nt = 0x2000 | ((total_tx & 32) ? 0x400 : 0) | (((total_ty / 30) & 1) ? 0x800 : 0);
        uint8_t  bg_tile = ppu_read(nt + ((total_ty % 30) * 32) + (total_tx % 32));

        uint8_t bg_low  = ppu_read(PPU.bg_table_addr + (bg_tile * 16) + (wy & 7));
        uint8_t bg_high = ppu_read(PPU.bg_table_addr + (bg_tile * 16) + (wy & 7) + 8);

        int bg_bit   = 7 - (wx & 7);
        int bg_color = (((bg_high >> bg_bit) & 1) << 1) | ((bg_low >> bg_bit) & 1);

        if (bg_color != 0) {
            PPU.sprite0hit = true;
            return;
        }
    }
}

void ppu_render_pass(int start_dot) {
	uint32_t frame_dot = (uint32_t)start_dot % FRAME_DOTS;
	if (frame_dot > 81754) return; // last active pixel: line 239, dot 255

	UpdateTexture(gpu.vram_tex, PPU.vram);
	UpdateTexture(gpu.oam_tex,  PPU.oam);

    int palette_buf[32];
    for (int i = 0; i < 32; i++) {
        palette_buf[i] = PPU.palette_ram[i] & 0x3F;
    }
    SetShaderValueV(gpu.shader, gpu.loc_palette_ram, palette_buf, SHADER_UNIFORM_INT, 32);

    int xs = PPU.x_scroll,          ys = PPU.y_scroll;
    int nt = PPU.base_nt,           bg = PPU.bg_table_addr;
    int sp = PPU.sprite_table_addr, sh = PPU.sprite_height;
    int bge = PPU.bg_enable ? 1 : 0, spe = PPU.sprite_enable ? 1 : 0;

    SetShaderValue(gpu.shader, gpu.loc_x_scroll,          &xs,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_y_scroll,          &ys,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_base_nt,           &nt,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_bg_table_addr,     &bg,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_sprite_table_addr, &sp,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_sprite_height,     &sh,  SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_bg_enable,         &bge, SHADER_UNIFORM_INT);
    SetShaderValue(gpu.shader, gpu.loc_sprite_enable,     &spe, SHADER_UNIFORM_INT);

    int s_dot = (int)frame_dot;
    SetShaderValue(gpu.shader, gpu.loc_start_dot, &s_dot, SHADER_UNIFORM_INT);

    BeginTextureMode(gpu.framebuffer);
        BeginShaderMode(gpu.shader);
            SetShaderValueTexture(gpu.shader, gpu.loc_chr_tex,  gpu.chr_tex);
            SetShaderValueTexture(gpu.shader, gpu.loc_vram_tex, gpu.vram_tex);
            SetShaderValueTexture(gpu.shader, gpu.loc_oam_tex,  gpu.oam_tex);
            DrawRectangle(0, 0, 256, 240, WHITE);
        EndShaderMode();
    EndTextureMode();
}

void ppu_exec_cmd(PPU_cmd *c) {
    uint64_t up_to = c ? c->at * 3 : total_cpu_cycles * 3;
    int start_dot  = (int)(gpu.last_gpu_dot + up_to - PPU.last_update);
    
    check_sprite0_hit(gpu.last_gpu_dot, start_dot);
    update_ppu_events(gpu.last_gpu_dot, start_dot);

    PPU.last_update = up_to;

    if (c) {
        ppu_write_reg(c->addr, c->value);
    }

	ppu_render_pass(start_dot);
    gpu.last_gpu_dot = start_dot;

// 	while (PPU.last_update < up_to) {
// 		if (PPU.dot == 0 && PPU.line == 0) {
// 			PPU.sprite0hit = false;
// 			PPU.vblank = false;
// 		}
// 		if (PPU.dot == 1 && PPU.line == 241) {
// 			PPU.vblank = true;
// 		}
// 		if (PPU.vblank) goto incr;
//
// 		if (1 <= PPU.dot && PPU.dot <= 256) {
// 			if (PPU.line < 240) {
//
// 				uint8_t bg_color_index = 0;
// 				Color bg_pixel = palette[ppu_read(0x3F00) & 0x3F]; // universal BG
//
// 				if (PPU.bg_enable) {
// 					int wx = (PPU.dot - 1) + PPU.x_scroll;
// 					int wy = PPU.line + PPU.y_scroll;
//
// 					// 1. Calculate global tile positions factoring in the starting nametable
// 					int total_tile_x = (wx >> 3) + ((PPU.base_nt & 1) * 32);
// 					int total_tile_y = (wy >> 3) + ((PPU.base_nt & 2) ? 30 : 0);
//
// 					// 2. Wrap coordinates into local individual nametable indices
// 					int tile_x = total_tile_x % 32;
// 					int tile_y = total_tile_y % 30;
//
// 					// 3. Resolve the actual nametable base address
// 					uint16_t nt_base = 0x2000;
// 					if (total_tile_x & 32)       nt_base |= 0x400; // Toggle right nametable
// 					if ((total_tile_y / 30) & 1) nt_base |= 0x800; // Toggle bottom nametable
//
// 					// 4. Fetch tile index from calculated VRAM position
// 					uint16_t tile_addr = nt_base + (tile_y * 32) + tile_x;
// 					uint8_t  tile      = ppu_read(tile_addr);
//
// 					// 5. Fetch attribute byte and calculate correct palette quadrant
// 					uint16_t attr_addr = (nt_base | 0x03C0) + ((tile_y >> 2) * 8) + (tile_x >> 2);
// 					uint8_t  attr      = ppu_read(attr_addr);
//
// 					int pal_shift     = ((tile_y & 2) << 1) | (tile_x & 2);
// 					int pal_offset    = (attr >> pal_shift) & 0x03;
//
// 					// 6. Fine pixel offsets within the chosen tile
// 					int fine_x = wx & 7;
// 					int fine_y = wy & 7;
//
// 					uint16_t plane0_addr = PPU.bg_table_addr + (tile * 16) + fine_y;
// 					uint8_t  low         = ppu_read(plane0_addr);
// 					uint8_t  high        = ppu_read(plane0_addr + 8);
//
// 					int bit = 7 - fine_x;
// 					bg_color_index = ((high >> bit) & 1) << 1 | ((low >> bit) & 1);
//
// 					if (bg_color_index) {
// 						uint16_t pal_addr = 0x3F00 + (pal_offset * 4) + bg_color_index;
// 						bg_pixel = palette[ppu_read(pal_addr) & 0x3F];
// 					}
// 				}
//
// 				// Sprites
// 				Color sprite_pixel    = {0};     // transparent
// 				bool  sprite_priority = false;   // false = in front of BG
// 				bool  sprite0_this_px = false;
//
// 				if (PPU.sprite_enable) {
// 					int px = PPU.dot - 1;           // screen X (0-255)
//
// 					// Walk all 64 sprites; render the FIRST (lowest OAM index)
// 					// non-transparent one that covers this pixel.
// 					for (int s = 0; s < 64; s++) {
// 						uint8_t sy    = PPU.oam[s * 4 + 0]; // Y position (top-1)
// 						uint8_t stile = PPU.oam[s * 4 + 1]; // tile index
// 						uint8_t sattr = PPU.oam[s * 4 + 2]; // attributes
// 						uint8_t sx    = PPU.oam[s * 4 + 3]; // X position
//
// 						// Check X overlap
// 						if (px < sx || px >= sx + 8) continue;
//
// 						// Check Y overlap
// 						int row = PPU.line - (int)sy - 1; // sy is stored as (top - 1)
// 						if (row < 0 || row >= PPU.sprite_height) continue;
//
// 						bool flip_h = (sattr & 0x40) != 0;
// 						bool flip_v = (sattr & 0x80) != 0;
// 						int  s_pal  = sattr & 0x03;
//
// 						if (flip_v) row = PPU.sprite_height - 1 - row;
//
// 						uint16_t s_table;
// 						uint8_t  s_tile_idx;
//
// 						if (PPU.sprite_height == 16) {
// 							// 8×16: tile bit 0 selects pattern table; top/bottom half
// 							s_table    = (stile & 0x01) ? 0x1000 : 0x0000;
// 							s_tile_idx = stile & 0xFE;
// 							if (row >= 8) { s_tile_idx++; row -= 8; }
// 						} else {
// 							s_table    = PPU.sprite_table_addr;
// 							s_tile_idx = stile;
// 						}
//
// 						uint16_t s_plane0 = s_table + (s_tile_idx * 16) + row;
// 						uint8_t  s_low    = ppu_read(s_plane0);
// 						uint8_t  s_high   = ppu_read(s_plane0 + 8);
//
// 						int s_bit         = flip_h ? (px - sx) : (7 - (px - sx));
// 						int s_color_index = (((s_high >> s_bit) & 1) << 1)
// 							|  ((s_low  >> s_bit) & 1);
//
// 						if (s_color_index == 0) continue; // transparent
//
// 						uint16_t s_pal_addr = 0x3F10 + (s_pal * 4) + s_color_index;
// 						sprite_pixel    = palette[ppu_read(s_pal_addr) & 0x3F];
// 						sprite_priority = (sattr & 0x20) != 0; // 1 = behind BG
// 						sprite0_this_px = (s == 0);
// 						break; // first non-transparent sprite wins
// 					}
// 				}
//
// 				if (sprite0_this_px && bg_color_index && PPU.dot != 256) {
// 					PPU.sprite0hit = true;
// 				}
//
// 				Color final_pixel = bg_pixel;  // start with BG (or universal BG)
// 				if (*(uint32_t *) &sprite_pixel && (!sprite_priority || !bg_color_index)) {
// 					final_pixel = sprite_pixel;
// 				}
//
// 				screen_buffer[256 * PPU.line + PPU.dot - 1] = final_pixel;
// 			}
//
// 		} else if (PPU.dot <= 320) {
// 			PPU.oam_addr = 0;
// 		}
//
// incr:
// 		PPU.last_update++;
// 		PPU.dot++;
// 		if (PPU.dot > 340) {
// 			PPU.dot = 0;
// 			PPU.line++;
// 			if (PPU.line > 261)
// 				PPU.line = 0;
// 		}
// 	}
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

Texture2D ppu_get_texture(void) {
	return gpu.framebuffer.texture;
}

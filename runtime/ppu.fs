#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform int u_start_dot;

uniform sampler2D u_chr_tex; // 8 kb 256*32 or 128*64
uniform sampler2D u_oam_tex; // 256 b 256*1
uniform sampler2D u_vram_tex; // 2 kb 256*8

uniform int u_palette_ram[32];
uniform vec3 u_nes_palette[64];

struct PPUState {
    int x_scroll;
    int y_scroll;
    int base_nt;
    int bg_table_addr;      // 0x0000 or 0x1000
    int sprite_table_addr;  // 0x0000 or 0x1000
    int sprite_height;      // 8 or 16
    bool bg_enable;
    bool sprite_enable;
};

uniform PPUState u_ppu;

int read_vram(int addr) {
    int offset = (addr - 0x2000) & 0x7FF;
    ivec2 coord = ivec2(offset % 256, offset / 256);
    return int(texelFetch(u_vram_tex, coord, 0).r * 255.0 + 0.5);
}

int read_oam(int offset) {
    return int(texelFetch(u_oam_tex, ivec2(offset, 0), 0).r * 255.0 + 0.5);
}

int read_chr(int addr) {
    ivec2 coord = ivec2(addr % 256, addr / 256);
    return int(texelFetch(u_chr_tex, coord, 0).r * 255.0 + 0.5);
}

void main() {
    int px = int(gl_FragCoord.x);
    int py = int(gl_FragCoord.y);

    int pixel_dot = (py * 341) + px;

    if (pixel_dot < u_start_dot) {
        discard;
    }

	// -------------------------------------------------------------------------
    // 1. Background Pixel Fetching
    // -------------------------------------------------------------------------
    int bg_color_index = 0;
    vec3 bg_pixel = u_nes_palette[u_palette_ram[0] & 0x3F]; // Universal BG ($3F00)

    if (u_ppu.bg_enable) {
        int wx = px + u_ppu.x_scroll;
        int wy = py + u_ppu.y_scroll;

        // Global tile coordinates factoring in starting nametable
        int total_tile_x = (wx >> 3) + ((u_ppu.base_nt & 1) * 32);
        int total_tile_y = (wy >> 3) + (((u_ppu.base_nt & 2) != 0) ? 30 : 0);

        int tile_x = total_tile_x % 32;
        int tile_y = total_tile_y % 30;

        // Resolve nametable base address with screen wrapping
        int nt_base = 0x2000;
        if ((total_tile_x & 32) != 0)       nt_base |= 0x400;
        if (((total_tile_y / 30) & 1) != 0) nt_base |= 0x800;

        // Fetch tile byte and attribute byte from VRAM
        int tile_addr = nt_base + (tile_y * 32) + tile_x;
        int tile      = read_vram(tile_addr);

        int attr_addr = (nt_base | 0x03C0) + ((tile_y >> 2) * 8) + (tile_x >> 2);
        int attr      = read_vram(attr_addr);

        int pal_shift  = ((tile_y & 2) << 1) | (tile_x & 2);
        int pal_offset = (attr >> pal_shift) & 0x03;

        // Fetch fine offsets & tile pattern bitplanes
        int fine_x = wx & 7;
        int fine_y = wy & 7;

        int plane0_addr = u_ppu.bg_table_addr + (tile * 16) + fine_y;
        int low         = read_chr(plane0_addr);
        int high        = read_chr(plane0_addr + 8);

        int bit = 7 - fine_x;
        bg_color_index = (((high >> bit) & 1) << 1) | ((low >> bit) & 1);

        if (bg_color_index != 0) {
            int pal_addr = 0x3F00 + (pal_offset * 4) + bg_color_index;
            bg_pixel = u_nes_palette[u_palette_ram[(pal_addr - 0x3F00) & 0x1F] & 0x3F];
        }
    }

    // -------------------------------------------------------------------------
    // 2. Sprite Pixel Fetching
    // -------------------------------------------------------------------------
    vec3 sprite_pixel    = vec3(0.0);
    bool sprite_drawn    = false;
    bool sprite_priority = false; // false = in front of BG

    if (u_ppu.sprite_enable) {
        for (int s = 0; s < 64; s++) {
            int sy    = read_oam(s * 4 + 0); // Y position (top - 1)
            int stile = read_oam(s * 4 + 1); // Tile index
            int sattr = read_oam(s * 4 + 2); // Attributes
            int sx    = read_oam(s * 4 + 3); // X position

            // X bounding check
            if (px < sx || px >= sx + 8) continue;

            // Y bounding check
            int row = py - sy - 1;
            if (row < 0 || row >= u_ppu.sprite_height) continue;

            bool flip_h = (sattr & 0x40) != 0;
            bool flip_v = (sattr & 0x80) != 0;
            int  s_pal  = sattr & 0x03;

            if (flip_v) row = u_ppu.sprite_height - 1 - row;

            int s_table;
            int s_tile_idx;

            if (u_ppu.sprite_height == 16) {
                // 8x16 Mode: Bit 0 selects pattern table
                s_table    = ((stile & 1) != 0) ? 0x1000 : 0x0000;
                s_tile_idx = stile & 0xFE;
                if (row >= 8) { s_tile_idx++; row -= 8; }
            } else {
                s_table    = u_ppu.sprite_table_addr;
                s_tile_idx = stile;
            }

            int s_plane0 = s_table + (s_tile_idx * 16) + row;
            int s_low    = read_chr(s_plane0);
            int s_high   = read_chr(s_plane0 + 8);

            int s_bit         = flip_h ? (px - sx) : (7 - (px - sx));
            int s_color_index = (((s_high >> s_bit) & 1) << 1) | ((s_low >> s_bit) & 1);

            if (s_color_index == 0) continue; // Transparent pixel

            int s_pal_addr  = 0x3F10 + (s_pal * 4) + s_color_index;
            sprite_pixel    = u_nes_palette[u_palette_ram[(s_pal_addr - 0x3F00) & 0x1F] & 0x3F];
            sprite_priority = (sattr & 0x20) != 0; // 1 = behind BG
            sprite_drawn    = true;
            break; // First non-transparent sprite in OAM wins
        }
    }

    // -------------------------------------------------------------------------
    // 3. Pixel Composition
    // -------------------------------------------------------------------------
    vec3 final_pixel = bg_pixel;
    if (sprite_drawn && (!sprite_priority || bg_color_index == 0)) {
        final_pixel = sprite_pixel;
    }

    finalColor = vec4(final_pixel, 1.0);
}

// ppu.fs: natemu fragment shader
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

#version 330
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D u_bg_tex;    // 32x240 RGBA: r=lo, g=hi, b=attr
uniform sampler2D u_spr_tex;   // 8x240  RGBA: r=y, g=x, b=attr, a=tile
uniform sampler2D u_misc_tex;  // 32x240 RGBA: r=pal, g=fine_x, b=spr_count
uniform sampler2D u_chr_rom_tex;

uniform int u_sprite_height;        // 8 or 16
uniform int u_sprite_table_addr;    // 0 or 0x1000, used for 8x8 sprites only

uniform vec3 u_nes_palette[64];

ivec4 readBytes(sampler2D tex, ivec2 coord) {
    return ivec4(texelFetch(tex, coord, 0) * 255.0 + 0.5);
}

int chrRead(int addr) {
    addr = addr & 0x1FFF;
    ivec2 coord = ivec2(addr & 127, addr >> 7);
    return int(texelFetch(u_chr_rom_tex, coord, 0).r * 255.0 + 0.5);
}

void main(void) {
    int px = int(floor(fragTexCoord.x * 256.0));
    int py = int(floor(fragTexCoord.y * 240.0));
    px = clamp(px, 0, 255);
    py = clamp(py, 0, 239);

    // We read x=0 to grab the fineX scroll and sprCount for the current scanline
    ivec4 line0 = readBytes(u_misc_tex, ivec2(0, py));
    int fineX    = line0.g;
    int sprCount = line0.b;

    // ---- Background sample ----
    int tileIndex = px / 8;
    int bitInTile = (px % 8) + fineX;
    if (bitInTile >= 8) {
        bitInTile -= 8;
        tileIndex += 1;
    }
    tileIndex = clamp(tileIndex, 0, 31);

    ivec4 bg = readBytes(u_bg_tex, ivec2(tileIndex, py));
    int lo = bg.r;
    int hi = bg.g;
    int bgPalette = bg.b;
    
    int bgBit = 7 - bitInTile;
    int bgPixel = (((hi >> bgBit) & 1) << 1) | ((lo >> bgBit) & 1);

    // ---- Sprite sample: first opaque match in OAM-index order wins ----
    bool spriteFound = false;
    int spPixel = 0;
    int sprPalette = 0;
    bool spritePriorityBehind = false;

    for (int slot = 0; slot < 8; slot++) {
        if (slot >= sprCount) break;

        ivec4 s = readBytes(u_spr_tex, ivec2(slot, py));
        int sy   = s.r;
        int sx   = s.g;
        int attr = s.b;
        int tile = s.a;

        if (px < sx || px >= sx + 8) continue;

        bool flipH = (attr & 0x40) != 0;
        bool flipV = (attr & 0x80) != 0;

        int spriteTop = sy + 1;
        if (py < spriteTop || py >= spriteTop + u_sprite_height) {
            continue; // Sprite is vertically off-screen for this pixel
        }

        int row = py - spriteTop;
        if (flipV) row = (u_sprite_height - 1) - row;

        int addr;
        if (u_sprite_height == 8) {
            addr = u_sprite_table_addr | (tile << 4) | row;
        } else {
            int bank = ((tile & 1) != 0) ? 0x1000 : 0x0000;
            int baseIndex = tile & 0xFE;
            int subTile = (row >= 8) ? 1 : 0;
            addr = bank | ((baseIndex + subTile) << 4) | (row & 7);
        }

        int spLo = chrRead(addr);
        int spHi = chrRead(addr | 8);

        int rawCol = px - sx;
        int bit = flipH ? rawCol : (7 - rawCol);
        int pixel = (((spHi >> bit) & 1) << 1) | ((spLo >> bit) & 1);
        if (pixel == 0) continue;

        spriteFound = true;
        spPixel = pixel;
        sprPalette = attr & 0x03;
        spritePriorityBehind = (attr & 0x20) != 0;
        break;
    }

    // ---- Compositing ----
    bool spriteWins = spriteFound && !(spritePriorityBehind && bgPixel != 0);

    int palIndex;
    if (spriteWins) {
        palIndex = 16 + sprPalette * 4 + spPixel;
    } else if (bgPixel != 0) {
        palIndex = bgPalette * 4 + bgPixel;
    } else {
        palIndex = 0; // universal backdrop
    }

    // We read the palette directly from u_misc_tex at x=palIndex
    int colorByte = readBytes(u_misc_tex, ivec2(palIndex, py)).r;
    finalColor = vec4(u_nes_palette[colorByte & 63], 1.0);
}

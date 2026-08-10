#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

// --- Données NES ---
uniform sampler2D chrRom;      // Texture contenant la CHR-ROM (ex: 256x256 pixels)
uniform sampler2D vram;        // Texture 64x60 représentant les 4 Nametables (2x2)
uniform sampler2D paletteRam;  // Texture 32x1 pour la Palette RAM (0x3F00)
uniform vec4 systemPalette[12]; // Les 64 couleurs NES définies dans ton code [13]

// --- Registres PPU ---
uniform int xScroll;
uniform int yScroll;
uniform int baseNt;            // 0, 1, 2, 3
uniform int bgTableAddr;       // 0 ou 4096
uniform int spriteTableAddr;
uniform int spriteHeight;      // 8 ou 16
uniform bool bgEnable;
uniform bool spriteEnable;

// --- Masque de Rattrapage ---
uniform int startDot;          // last_update
uniform int endDot;            // up_to (cycles * 3)

// --- OAM (Sprites) ---
struct Sprite {
    int y;
    int tile;
    int attr;
    int x;
};
uniform Sprite oam[12];

void main() {
    int dot = int(fragTexCoord.x * 256.0);
    int line = int(fragTexCoord.y * 240.0);
    int currentPixelIndex = line * 256 + dot;

    // --- APPLICATION DU MASQUE ---
    if (currentPixelIndex < startDot || currentPixelIndex >= endDot) {
        discard; 
    }

    // Couleur de fond universelle [3]
    vec4 bgPixel = systemPalette[int(texelFetch(paletteRam, isampler2D(0), 0).r * 255.0) & 0x3F];
    int bgColorIndex = 0;

    // --- RENDU BACKGROUND [3-6] ---
    if (bgEnable) {
        int wx = dot + xScroll;
        int wy = line + yScroll;

        int totalTileX = (wx / 8) + ((baseNt & 1) * 32);
        int totalTileY = (wy / 8) + ((baseNt & 2) != 0 ? 30 : 0);

        int tileX = totalTileX % 32;
        int tileY = totalTileY % 30;
        
        // Nametable fetch
        int ntX = (totalTileX / 32) % 2;
        int ntY = (totalTileY / 30) % 2;
        int tileIndex = int(texelFetch(vram, ivec2(tileX + ntX*32, tileY + ntY*30), 0).r * 255.0);

        // Attribute fetch [5]
        int attrX = tileX / 4;
        int attrY = tileY / 4;
        int attr = int(texelFetch(vram, ivec2(attrX + ntX*32, attrY + ntY*30 + 0x3C0/32), 0).r * 255.0);
        int palShift = ((tileY & 2) << 1) | (tileX & 2);
        int palOffset = (attr >> palShift) & 0x03;

        // Pixel fetch depuis CHR-ROM [6]
        int fineX = wx % 8;
        int fineY = wy % 8;
        // Simulé ici : lecture des deux plans de bits
        // En pratique, chrRom doit être mappée pour que texelFetch(tileIndex, fineX, fineY) fonctionne
        // bgColorIndex = ...
        
        if (bgColorIndex > 0) {
            int palAddr = (palOffset * 4) + bgColorIndex;
            bgPixel = systemPalette[int(texelFetch(paletteRam, isampler2D(palAddr), 0).r * 255.0) & 0x3F];
        }
    }

    // --- RENDU SPRITES [7-10] ---
    vec4 spritePixel = vec4(0.0);
    bool spritePriority = false;

    if (spriteEnable) {
        for (int s = 0; s < 64; s++) {
            int sy = oam[s].y;
            int sx = oam[s].x;
            if (dot < sx || dot >= sx + 8) continue;
            
            int row = line - sy - 1;
            if (row < 0 || row >= spriteHeight) continue;

            // Logique de flip et de table [8, 9]
            // int sColorIndex = fetch_from_chr(oam[s].tile, row, dot-sx, ...);
            
            // if (sColorIndex > 0) {
            //    spritePixel = ...
            //    spritePriority = (oam[s].attr & 0x20) != 0;
            //    break;
            // }
        }
    }

    // --- COMPOSITION FINALE [11] ---
    if (spritePixel.a > 0.0 && (!spritePriority || bgColorIndex == 0)) {
        finalColor = spritePixel;
    } else {
        finalColor = bgPixel;
    }
}

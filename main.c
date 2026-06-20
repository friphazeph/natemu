#include "neslib.h"
#include <raylib.h>

void run_frame(void) {
	cycle_budget = 29780;

	while(cycle_budget > 0) {
		size_t offs = addr_to_prg_rom(PC);
		global_dispatch[offs](offs);
	}
}

#define SCALE 3

int main(void) {
	InitWindow(PIXELS_W * SCALE, PIXELS_H * SCALE, "SMB recomp");
	SetTargetFPS(60);

	Image nes_frame_image = {
		.data = screen_buffer,
		.width = PIXELS_W,
		.height = PIXELS_H,
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
	};
	Texture2D nes_texture = LoadTextureFromImage(nes_frame_image);

	nes_init();

	while (!WindowShouldClose()) {
		trigger_nmi();
		run_frame();
		ppu_catch_up();

		UpdateTexture(nes_texture, screen_buffer);

		BeginDrawing();
			ClearBackground(BLACK);
			DrawTexturePro(
					nes_texture,
					(Rectangle) {0, 0, PIXELS_W, PIXELS_H},
					(Rectangle) {0, 0, PIXELS_W * SCALE, PIXELS_H * SCALE},
					(Vector2) {0, 0}, 0.0f, WHITE
					);
			DrawFPS(10,10);
		EndDrawing();
	}
}

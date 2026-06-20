#include "neslib.h"
#include "game.c"
#include <raylib.h>

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

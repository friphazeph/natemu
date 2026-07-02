#include "neslib.h"
#include <raylib.h>

void handle_input(void) {
    uint8_t pad_state = 0;

    // NES Button bit positions (Standard order: A, B, Select, Start, Up, Down, Left, Right)
    if (IsKeyDown(KEY_Z))           pad_state |= (1 << 0); // A
    if (IsKeyDown(KEY_X))           pad_state |= (1 << 1); // B
    if (IsKeyDown(KEY_SPACE))       pad_state |= (1 << 2); // Select
    if (IsKeyDown(KEY_ENTER))       pad_state |= (1 << 3); // Start
    if (IsKeyDown(KEY_UP))          pad_state |= (1 << 4); // Up
    if (IsKeyDown(KEY_DOWN))        pad_state |= (1 << 5); // Down
    if (IsKeyDown(KEY_LEFT))        pad_state |= (1 << 6); // Left
    if (IsKeyDown(KEY_RIGHT))       pad_state |= (1 << 7); // Right

	controller_buffer = pad_state;
	// printf("%08b\r", pad_state);
	// fflush(stdout);
}

void run_for(size_t budget) {
	cycle_budget += budget;

	while(cycle_budget > 0) {
		size_t offs = addr_to_prg_rom(PC);
		if (offs == (size_t) -1) {
			interpret_pc();
			continue;
		}
		// printf("0x%04X: 0x%04lX\n", PC, offs);
		void (*branch)(size_t) = global_dispatch[offs];
		if (!branch) {
			interpret_pc();
		} else {
			branch(offs);
		}
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

	cycle_budget = 0;
	nes_init();
	run_for(27393);

	while (!WindowShouldClose()) {
		handle_input();

		trigger_nmi();
		run_for(29780-7);
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

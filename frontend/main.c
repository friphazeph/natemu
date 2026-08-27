#include "runtime.h"
#include "ppu.h"
#include "cpu.h"
#include "input.h"
#include <raylib.h>

#define SCALE 6

int main(void) {
	InitWindow(PIXELS_W * SCALE, PIXELS_H * SCALE, "SMB recomp");
	SetTargetFPS(60);

	ppu_init();

	cycle_budget = 0;
	nes_init();

	while (!WindowShouldClose()) {
		handle_input();

		BeginDrawing();
			ClearBackground(BLACK);
			DrawTexturePro(
					run_frame(),
					(Rectangle) {0, 0, PIXELS_W, - PIXELS_H},
					(Rectangle) {0, 0, PIXELS_W * SCALE, PIXELS_H * SCALE},
					(Vector2) {0, 0}, 0.0f, WHITE
					);
			DrawFPS(10,10);
		EndDrawing();
	}
}

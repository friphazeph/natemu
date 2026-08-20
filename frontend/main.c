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
	run_for(27393);

	while (!WindowShouldClose()) {
		handle_input();

		trigger_nmi();
		run_for(29780-7);
		ppu_catch_up();

		BeginDrawing();
			ClearBackground(BLACK);
			DrawTexturePro(
					ppu_get_texture(),
					(Rectangle) {0, 0, PIXELS_W, PIXELS_H},
					(Rectangle) {0, 0, PIXELS_W * SCALE, PIXELS_H * SCALE},
					(Vector2) {0, 0}, 0.0f, WHITE
					);
			DrawFPS(10,10);
		EndDrawing();
	}
}

// main.c: natemu frontend main loop
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

#include "runtime.h"
#include "ppu.h"
#include "cpu.h"
#include "input.h"
#include <raylib.h>

#define SCALE 6

int main(void) {
	InitWindow(PIXELS_W * SCALE, PIXELS_H * SCALE, "natemu");
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

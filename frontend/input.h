#ifndef INPUT_H_
#define INPUT_H_

#include <stdint.h>
#include <raylib.h>

extern uint8_t controller_buffer;

static inline void handle_input(void) {
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

#endif // INPUT_H_

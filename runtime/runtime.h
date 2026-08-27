#ifndef RUNTIME_H_
#define RUNTIME_H_

#include <stddef.h>
#include <raylib.h>

extern void (*const global_dispatch[])(size_t offs);

Texture2D run_frame(void);
void nes_init(void);
void interpret_pc(void);

#endif // RUNTIME_H_

#ifndef RUNTIME_H_
#define RUNTIME_H_

#include <stddef.h>

extern void (*const global_dispatch[])(size_t offs);

void run_for(size_t budget);
void nes_init(void);
void interpret_pc(void);

#endif // RUNTIME_H_

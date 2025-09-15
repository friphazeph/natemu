#ifndef STATIR_H_
#define STATIR_H_

#include <stdint.h>
#include <stddef.h>

typedef enum {
	X86_64_OP,
	GRAPHICS_OP,
	AUDIO_OP,
} IROpKind;

typedef uint64_t IROperand;

typedef struct {
	IROperand *items;
	size_t count;
	size_t capacity;
} IROperands;

typedef struct {
	IROpKind kind;
	IROperands operands;
} IROp;

#endif // STATIR_H_

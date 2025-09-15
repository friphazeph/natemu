#ifndef STATIR_H_
#define STATIR_H_

#ifndef _SE_PLATFORM_OPS
	_Static_assert(0, "ERROR: You must define the platform-specific op kinds above statir.h");
#endif // _SE_PLATFORM_OPS

#include <stdint.h>
#include <stddef.h>

typedef enum {
	FASM_INVALID = 0,
	FASM_ADC,
} FasmOpKind;

typedef enum {
	FF_INVALID = 0,
	FF_CF,
	FF_ZF,
	FF_OF,
	FF_SF,
} FasmFlags;

typedef enum {
	FREG_INVALID = 0,
	FREG_AL,
	FREG_BL,
	FREG_CL,
	FREG_DL,
} FasmRegs;

typedef enum {
	F_IMM,
	F_REG,
	F_MEM,
	F_PLUS,
	F_MOD,
	F_MUL
} FasmOperandKind;

typedef enum {
	IR_OP_INVALID = 0,
	FASM_OP,
	GRAPHICS_OP,
	AUDIO_OP,
} IROpKind;

typedef union {
	FasmOpKind fasm;
	GraphicsOpKind graphics;
	AudioOpKind audio;
} SubKind;

typedef struct {
	FasmOperandKind kind;
	uint64_t value;
} IROperand;

#define IROP(_kind, _value) ((IROperand) {.kind=(_kind), .value=(_value)})
#define imm(_value) (IROP(F_IMM, _value))
#define reg(_value) (IROP(F_REG, _value))
#define mem(_value) (IROP(F_MEM, _value))
#define PLUS (IROP(F_PLUS, 0))
#define MOD (IROP(F_MOD, 0))
#define MUL (IROP(F_MUL, 0))

typedef struct {
	IROperand *items;
	size_t count;
	size_t capacity;
} IROperands;

typedef struct {
	IROpKind kind;
	SubKind subkind;
	IROperands operands;
} IROp;

typedef struct {
	IROp *items;
	size_t count;
	size_t capacity;
} IROps;

#endif // STATIR_H_

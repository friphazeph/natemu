#ifndef NES_H_
#define NES_H_

#include <stdint.h>
#include <stddef.h>

#ifndef _SE_PLATFORM_OPS
#define _SE_PLATFORM_OPS

typedef enum {
	GR_OP_INVALID = 0,
} GraphicsOpKind;

typedef enum {
	AUD_OP_INVALID = 0,
} AudioOpKind;

#endif // _SE_PLATFORM_OPS

// ===== INES PARSING ===== //

typedef uint8_t Byte;

typedef struct {
	Byte *items;
	size_t count;
	size_t capacity;
} Bytes;

typedef struct {
	Byte magic[4];
	Byte prg_len;
	Byte chr_len;
	Byte flags6;
	Byte flags7;
	Byte flags8;
	Byte flags9;
	Byte flags10;
	Byte _flags[5];
} NesHdr;

typedef struct {
	NesHdr hdr;
	Bytes prg;
} NesRom;

NesRom load_rom(const char *path);

// ===== NES/6502 OPS ===== //

typedef enum {
	// ADC
	ADC_IMM   = 0x69,
	ADC_ZP    = 0x65,
	ADC_ZP_X  = 0x75,
	ADC_ABS   = 0x6D,
	ADC_ABS_X = 0x7D,
	ADC_ABS_Y = 0x79,
	ADC_IND_X = 0x61,
	ADC_IND_Y = 0x71,

	// AND
	AND_IMM   = 0x29,
	AND_ZP    = 0x25,
	AND_ZP_X  = 0x35,
	AND_ABS   = 0x2D,
	AND_ABS_X = 0x3D,
	AND_ABS_Y = 0x39,
	AND_IND_X = 0x21,
	AND_IND_Y = 0x31,

	// ASL
	ASL_ACC   = 0x0A,
	ASL_ZP    = 0x06,
	ASL_ZP_X  = 0x16,
	ASL_ABS   = 0x0E,
	ASL_ABS_X = 0x1E,

	BCC       = 0x90,
	BCS       = 0xB0,
	BEQ       = 0xF0,

	// BIT
	BIT_ZP    = 0x24,
	BIT_ABS   = 0x2C,

	BMI       = 0x30,
	BNE       = 0xD0,
	BPL       = 0x10,
	BRK       = 0x00,
	BVC       = 0x50,
	BVS       = 0x70,
	CLC       = 0x18,
	CLD       = 0xD8,
	CLI       = 0x58,
	CLV       = 0xB8,

	// CMP
	CMP_IMM   = 0xC9,
	CMP_ZP    = 0xC5,
	CMP_ZP_X  = 0xD5,
	CMP_ABS   = 0xCD,
	CMP_ABS_X = 0xDD,
	CMP_ABS_Y = 0xD9,
	CMP_IND_X = 0xC1,
	CMP_IND_Y = 0xD1,

	// CPX
	CPX_IMM   = 0xE0,
	CPX_ZP    = 0xE4,
	CPX_ABS   = 0xEC,

	// CPY
	CPY_IMM   = 0xC0,
	CPY_ZP    = 0xC4,
	CPY_ABS   = 0xCC,

	// DEC
	DEC_ZP    = 0xC6,
	DEC_ZP_X  = 0xD6,
	DEC_ABS   = 0xCE,
	DEC_ABS_X = 0xDE,

	DEX       = 0xCA,
	DEY       = 0x88,

	// EOR
	EOR_IMM   = 0x49,
	EOR_ZP    = 0x45,
	EOR_ZP_X  = 0x55,
	EOR_ABS   = 0x4D,
	EOR_ABS_X = 0x5D,
	EOR_ABS_Y = 0x59,
	EOR_IND_X = 0x41,
	EOR_IND_Y = 0x51,

	// INC
	INC_ZP    = 0xE6,
	INC_ZP_X  = 0xF6,
	INC_ABS   = 0xEE,
	INC_ABS_X = 0xFE,

	INX       = 0xE8,
	INY       = 0xC8,

	// JMP
	JMP_ABS   = 0x4C,
	JMP_IND   = 0x6C,

	JSR       = 0x20,

	// LDA
	LDA_IMM   = 0xA9,
	LDA_ZP    = 0xA5,
	LDA_ZP_X  = 0xB5,
	LDA_ABS   = 0xAD,
	LDA_ABS_X = 0xBD,
	LDA_ABS_Y = 0xB9,
	LDA_IND_X = 0xA1,
	LDA_IND_Y = 0xB1,

	// LDX
	LDX_IMM   = 0xA2,
	LDX_ZP    = 0xA6,
	LDX_ZP_Y  = 0xB6,
	LDX_ABS   = 0xAE,
	LDX_ABS_Y = 0xBE,

	// LDY
	LDY_IMM   = 0xA0,
	LDY_ZP    = 0xA4,
	LDY_ZP_X  = 0xB4,
	LDY_ABS   = 0xAC,
	LDY_ABS_X = 0xBC,

	// LSR
	LSR_ACC   = 0x4A,
	LSR_ZP    = 0x46,
	LSR_ZP_X  = 0x56,
	LSR_ABS   = 0x4E,
	LSR_ABS_X = 0x5E,

	NOP       = 0xEA,

	// ORA
	ORA_IMM   = 0x09,
	ORA_ZP    = 0x05,
	ORA_ZP_X  = 0x15,
	ORA_ABS   = 0x0D,
	ORA_ABS_X = 0x1D,
	ORA_ABS_Y = 0x19,
	ORA_IND_X = 0x01,
	ORA_IND_Y = 0x11,

	PHA       = 0x48,
	PHP       = 0x08,
	PLA       = 0x68,
	PLP       = 0x28,

	// ROL
	ROL_ACC   = 0x2A,
	ROL_ZP    = 0x26,
	ROL_ZP_X  = 0x36,
	ROL_ABS   = 0x2E,
	ROL_ABS_X = 0x3E,

	// ROR
	ROR_ACC   = 0x6A,
	ROR_ZP    = 0x66,
	ROR_ZP_X  = 0x76,
	ROR_ABS   = 0x6E,
	ROR_ABS_X = 0x7E,

	RTI       = 0x40,
	RTS       = 0x60,

	// SBC
	SBC_IMM   = 0xE9,
	SBC_ZP    = 0xE5,
	SBC_ZP_X  = 0xF5,
	SBC_ABS   = 0xED,
	SBC_ABS_X = 0xFD,
	SBC_ABS_Y = 0xF9,
	SBC_IND_X = 0xE1,
	SBC_IND_Y = 0xF1,

	SEC       = 0x38,
	SED       = 0xF8,
	SEI       = 0x78,

	// STA
	STA_ZP    = 0x85,
	STA_ZP_X  = 0x95,
	STA_ABS   = 0x8D,
	STA_ABS_X = 0x9D,
	STA_ABS_Y = 0x99,
	STA_IND_X = 0x81,
	STA_IND_Y = 0x91,
	
	// STX
	STX_ZP    = 0x86,
	STX_ZP_Y  = 0x96,
	STX_ABS   = 0x8E,

	// STY
	STY_ZP    = 0x84,
	STY_ZP_Y  = 0x94,
	STY_ABS   = 0x8C,

	TAX       = 0xAA,
	TAY       = 0xA8,
	TSX       = 0xBA,
	TXA       = 0x8A,
	TXS       = 0x9A,
	TYA       = 0x98,
} OpKind;

typedef enum {
	MODE_NONE = 0,
	MODE_ACC,
	MODE_IMM,
	MODE_ZP,
	MODE_ZP_X,
	MODE_ZP_Y,
	MODE_ABS,
	MODE_ABS_X,
	MODE_ABS_Y,
	MODE_IND,
	MODE_IND_X,
	MODE_IND_Y,
} AddrMode;

typedef enum {
	META_ADC,
	META_AND,
	META_ASL,
	META_BIT,
	META_CMP,
	META_CPX,
	META_CPY,
	META_DEC,
	META_EOR,
	META_INC,
	META_JMP,
	META_LDA,
	META_LDX,
	META_LDY,
	META_LSR,
	META_ORA,
	META_ROL,
	META_ROR,
	META_SBC,
	META_STA,
	META_STX,
	META_STY,
	META_BCC,
	META_BCS,
	META_BEQ,
	META_BMI,
	META_BNE,
	META_BPL,
	META_BRK,
	META_BVC,
	META_BVS,
	META_CLC,
	META_CLD,
	META_CLI,
	META_CLV,
	META_DEX,
	META_DEY,
	META_INX,
	META_INY,
	META_JSR,
	META_NOP,
	META_PHA,
	META_PHP,
	META_PLA,
	META_PLP,
	META_RTI,
	META_RTS,
	META_SEC,
	META_SED,
	META_SEI,
	META_TAX,
	META_TAY,
	META_TSX,
	META_TXA,
	META_TXS,
	META_TYA,
} MetaOpKind;

typedef struct {
	const char *name;
	size_t size;
	AddrMode addr_mode;
	MetaOpKind meta_kind;
} Op;

static const Op OPS[0x100] = {
	// ADC
	[ADC_IMM]   = {"ADC_IMM",   2, MODE_IMM, META_ADC},
	[ADC_ZP]    = {"ADC_ZP",    2, MODE_ZP, META_ADC},
	[ADC_ZP_X]  = {"ADC_ZP_X",  2, MODE_ZP_X, META_ADC},
	[ADC_ABS]   = {"ADC_ABS",   3, MODE_ABS, META_ADC},
	[ADC_ABS_X] = {"ADC_ABS_X", 3, MODE_ABS_X, META_ADC},
	[ADC_ABS_Y] = {"ADC_ABS_Y", 3, MODE_ABS_Y, META_ADC},
	[ADC_IND_X] = {"ADC_IND_X", 2, MODE_IND_X, META_ADC},
	[ADC_IND_Y] = {"ADC_IND_Y", 2, MODE_IND_Y, META_ADC},

	// AND
	[AND_IMM]   = {"AND_IMM",   2, MODE_IMM, META_AND},
	[AND_ZP]    = {"AND_ZP",    2, MODE_ZP, META_AND},
	[AND_ZP_X]  = {"AND_ZP_X",  2, MODE_ZP_X, META_AND},
	[AND_ABS]   = {"AND_ABS",   3, MODE_ABS, META_AND},
	[AND_ABS_X] = {"AND_ABS_X", 3, MODE_ABS_X, META_AND},
	[AND_ABS_Y] = {"AND_ABS_Y", 3, MODE_ABS_Y, META_AND},
	[AND_IND_X] = {"AND_IND_X", 2, MODE_IND_X, META_AND},
	[AND_IND_Y] = {"AND_IND_Y", 2, MODE_IND_Y, META_AND},

	// ASL
	[ASL_ACC]   = {"ASL_ACC",   1, MODE_ACC, META_ASL},
	[ASL_ZP]    = {"ASL_ZP",    2, MODE_ZP, META_ASL},
	[ASL_ZP_X]  = {"ASL_ZP_X",  2, MODE_ZP_X, META_ASL},
	[ASL_ABS]   = {"ASL_ABS",   3, MODE_ABS, META_ASL},
	[ASL_ABS_X] = {"ASL_ABS_X", 3, MODE_ABS_X, META_ASL},

	[BCC]       = {"BCC",       2, MODE_NONE, META_BCC},
	[BCS]       = {"BCS",       2, MODE_NONE, META_BCS},
	[BEQ]       = {"BEQ",       2, MODE_NONE, META_BEQ},

	// BIT
	[BIT_ZP]    = {"BIT_ZP",    2, MODE_ZP, META_BIT},
	[BIT_ABS]   = {"BIT_ABS",   3, MODE_ABS, META_BIT},

	[BMI]       = {"BMI",       2, MODE_NONE, META_BMI},
	[BNE]       = {"BNE",       2, MODE_NONE, META_BNE},
	[BPL]       = {"BPL",       2, MODE_NONE, META_BPL},
	[BRK]       = {"BRK",       2, MODE_NONE, META_BRK},
	[BVC]       = {"BVC",       2, MODE_NONE, META_BVC},
	[BVS]       = {"BVS",       2, MODE_NONE, META_BVS},
	[CLC]       = {"CLC",       1, MODE_NONE, META_CLC},
	[CLD]       = {"CLD",       1, MODE_NONE, META_CLD},
	[CLI]       = {"CLI",       1, MODE_NONE, META_CLI},
	[CLV]       = {"CLV",       1, MODE_NONE, META_CLV},

	// CMP
	[CMP_IMM]   = {"CMP_IMM",   2, MODE_IMM, META_CMP},
	[CMP_ZP]    = {"CMP_ZP",    2, MODE_ZP, META_CMP},
	[CMP_ZP_X]  = {"CMP_ZP_X",  2, MODE_ZP_X, META_CMP},
	[CMP_ABS_X] = {"CMP_ABS_X", 3, MODE_ABS_X, META_CMP},
	[CMP_ABS]   = {"CMP_ABS",   3, MODE_ABS, META_CMP},
	[CMP_ABS_Y] = {"CMP_ABS_Y", 3, MODE_ABS_Y, META_CMP},
	[CMP_IND_X] = {"CMP_IND_X", 2, MODE_IND_X, META_CMP},
	[CMP_IND_Y] = {"CMP_IND_Y", 2, MODE_IND_Y, META_CMP},

	// CPX
	[CPX_IMM]   = {"CPX_IMM",   2, MODE_IMM, META_CPX},
	[CPX_ZP]    = {"CPX_ZP",    2, MODE_ZP, META_CPX},
	[CPX_ABS]   = {"CPX_ABS",   3, MODE_ABS, META_CPX},

	// CPY
	[CPY_IMM]   = {"CPY_IMM",   2, MODE_IMM, META_CPY},
	[CPY_ZP]    = {"CPY_ZP",    2, MODE_ZP, META_CPY},
	[CPY_ABS]   = {"CPY_ABS",   3, MODE_ABS, META_CPY},

	// DEC
	[DEC_ZP]    = {"DEC_ZP",    2, MODE_ZP, META_DEC},
	[DEC_ZP_X]  = {"DEC_ZP_X",  2, MODE_ZP_X, META_DEC},
	[DEC_ABS]   = {"DEC_ABS",   3, MODE_ABS, META_DEC},
	[DEC_ABS_X] = {"DEC_ABS_X", 3, MODE_ABS_X, META_DEC},

	[DEX]       = {"DEX",       1, MODE_NONE, META_DEX},
	[DEY]       = {"DEY",       1, MODE_NONE, META_DEY},

	// EOR
	[EOR_IMM]   = {"EOR_IMM",   2, MODE_IMM, META_EOR},
	[EOR_ZP]    = {"EOR_ZP",    2, MODE_ZP, META_EOR},
	[EOR_ZP_X]  = {"EOR_ZP_X",  2, MODE_ZP_X, META_EOR},
	[EOR_ABS]   = {"EOR_ABS",   3, MODE_ABS, META_EOR},
	[EOR_ABS_X] = {"EOR_ABS_X", 3, MODE_ABS_X, META_EOR},
	[EOR_ABS_Y] = {"EOR_ABS_Y", 3, MODE_ABS_Y, META_EOR},
	[EOR_IND_X] = {"EOR_IND_X", 2, MODE_IND_X, META_EOR},
	[EOR_IND_Y] = {"EOR_IND_Y", 2, MODE_IND_Y, META_EOR},

	// INC
	[INC_ZP]    = {"INC_ZP",    2, MODE_ZP, META_INC},
	[INC_ZP_X]  = {"INC_ZP_X",  2, MODE_ZP_X, META_INC},
	[INC_ABS]   = {"INC_ABS",   3, MODE_ABS, META_INC},
	[INC_ABS_X] = {"INC_ABS_X", 3, MODE_ABS_X, META_INC},

	[INX]       = {"INX",       1, MODE_NONE, META_INX},
	[INY]       = {"INY",       1, MODE_NONE, META_INY},

	// JMP
	[JMP_ABS]   = {"JMP_ABS",   3, MODE_ABS, META_JMP},
	[JMP_IND]   = {"JMP_IND",   3, MODE_IND, META_JMP},

	[JSR]       = {"JSR",       3, MODE_NONE, META_JSR},

	// LDA
	[LDA_IMM]   = {"LDA_IMM",   2, MODE_IMM, META_LDA},
	[LDA_ZP]    = {"LDA_ZP",    2, MODE_ZP, META_LDA},
	[LDA_ZP_X]  = {"LDA_ZP_X",  2, MODE_ZP_X, META_LDA},
	[LDA_ABS]   = {"LDA_ABS",   3, MODE_ABS, META_LDA},
	[LDA_ABS_X] = {"LDA_ABS_X", 3, MODE_ABS_X, META_LDA},
	[LDA_ABS_Y] = {"LDA_ABS_Y", 3, MODE_ABS_Y, META_LDA},
	[LDA_IND_X] = {"LDA_IND_X", 2, MODE_IND_X, META_LDA},
	[LDA_IND_Y] = {"LDA_IND_Y", 2, MODE_IND_Y, META_LDA},

	// LDX
	[LDX_IMM]   = {"LDX_IMM",   2, MODE_IMM, META_LDX},
	[LDX_ZP]    = {"LDX_ZP",    2, MODE_ZP, META_LDX},
	[LDX_ZP_Y]  = {"LDX_ZP_Y",  2, MODE_ZP_Y, META_LDX},
	[LDX_ABS]   = {"LDX_ABS",   3, MODE_ABS, META_LDX},
	[LDX_ABS_Y] = {"LDX_ABS_Y", 3, MODE_ABS_Y, META_LDX},

	// LDY
	[LDY_IMM]   = {"LDY_IMM",   2, MODE_IMM, META_LDY},
	[LDY_ZP]    = {"LDY_ZP",    2, MODE_ZP, META_LDY},
	[LDY_ZP_X]  = {"LDY_ZP_X",  2, MODE_ZP_X, META_LDY},
	[LDY_ABS]   = {"LDY_ABS",   3, MODE_ABS, META_LDY},
	[LDY_ABS_X] = {"LDY_ABS_X", 3, MODE_ABS_X, META_LDY},

	// LSR
	[LSR_ACC]   = {"LSR_ACC",   1, MODE_ACC, META_LSR},
	[LSR_ZP]    = {"LSR_ZP",    2, MODE_ZP, META_LSR},
	[LSR_ZP_X]  = {"LSR_ZP_X",  2, MODE_ZP_X, META_LSR},
	[LSR_ABS]   = {"LSR_ABS",   3, MODE_ABS, META_LSR},
	[LSR_ABS_X] = {"LSR_ABS_X", 3, MODE_ABS_X, META_LSR},

	[NOP]       = {"NOP",       1, MODE_NONE, META_NOP},

	// ORA
	[ORA_IMM]   = {"ORA_IMM",   2, MODE_IMM, META_ORA},
	[ORA_ZP]    = {"ORA_ZP",    2, MODE_ZP, META_ORA},
	[ORA_ZP_X]  = {"ORA_ZP_X",  2, MODE_ZP_X, META_ORA},
	[ORA_ABS]   = {"ORA_ABS",   3, MODE_ABS, META_ORA},
	[ORA_ABS_X] = {"ORA_ABS_X", 3, MODE_ABS_X, META_ORA},
	[ORA_ABS_Y] = {"ORA_ABS_Y", 3, MODE_ABS_Y, META_ORA},
	[ORA_IND_X] = {"ORA_IND_X", 2, MODE_IND_X, META_ORA},
	[ORA_IND_Y] = {"ORA_IND_Y", 2, MODE_IND_Y, META_ORA},

	[PHA]       = {"PHA",       1, MODE_NONE, META_PHA},
	[PHP]       = {"PHP",       1, MODE_NONE, META_PHP},
	[PLA]       = {"PLA",       1, MODE_NONE, META_PLA},
	[PLP]       = {"PLP",       1, MODE_NONE, META_PLP},

	// ROL
	[ROL_ACC]   = {"ROL_ACC",   1, MODE_ACC, META_ROL},
	[ROL_ZP]    = {"ROL_ZP",    2, MODE_ZP, META_ROL},
	[ROL_ZP_X]  = {"ROL_ZP_X",  2, MODE_ZP_X, META_ROL},
	[ROL_ABS]   = {"ROL_ABS",   3, MODE_ABS, META_ROL},
	[ROL_ABS_X] = {"ROL_ABS_X", 3, MODE_ABS_X, META_ROL},

	// ROR
	[ROR_ACC]   = {"ROR_ACC",   1, MODE_ACC, META_ROR},
	[ROR_ZP]    = {"ROR_ZP",    2, MODE_ZP, META_ROR},
	[ROR_ZP_X]  = {"ROR_ZP_X",  2, MODE_ZP_X, META_ROR},
	[ROR_ABS]   = {"ROR_ABS",   3, MODE_ABS, META_ROR},
	[ROR_ABS_X] = {"ROR_ABS_X", 3, MODE_ABS_X, META_ROR},

	[RTI]       = {"RTI",       1, MODE_NONE, META_RTI},
	[RTS]       = {"RTS",       1, MODE_NONE, META_RTS},

	// SBC
	[SBC_IMM]   = {"SBC_IMM",   2, MODE_IMM, META_SBC},
	[SBC_ZP]    = {"SBC_ZP",    2, MODE_ZP, META_SBC},
	[SBC_ZP_X]  = {"SBC_ZP_X",  2, MODE_ZP_X, META_SBC},
	[SBC_ABS]   = {"SBC_ABS",   3, MODE_ABS, META_SBC},
	[SBC_ABS_X] = {"SBC_ABS_X", 3, MODE_ABS_X, META_SBC},
	[SBC_ABS_Y] = {"SBC_ABS_Y", 3, MODE_ABS_Y, META_SBC},
	[SBC_IND_X] = {"SBC_IND_X", 2, MODE_IND_X, META_SBC},
	[SBC_IND_Y] = {"SBC_IND_Y", 2, MODE_IND_Y, META_SBC},

	[SEC]       = {"SEC",       1, MODE_NONE, META_SEC},
	[SED]       = {"SED",       1, MODE_NONE, META_SED},
	[SEI]       = {"SEI",       1, MODE_NONE, META_SEI},

	// STA
	[STA_ZP]    = {"STA_ZP",    2, MODE_ZP, META_STA},
	[STA_ZP_X]  = {"STA_ZP_X",  2, MODE_ZP_X, META_STA},
	[STA_ABS]   = {"STA_ABS",   3, MODE_ABS, META_STA},
	[STA_ABS_X] = {"STA_ABS_X", 3, MODE_ABS_X, META_STA},
	[STA_ABS_Y] = {"STA_ABS_Y", 3, MODE_ABS_Y, META_STA},
	[STA_IND_X] = {"STA_IND_X", 2, MODE_IND_X, META_STA},
	[STA_IND_Y] = {"STA_IND_Y", 2, MODE_IND_Y, META_STA},

	// STX
	[STX_ZP]    = {"STX_ZP",    2, MODE_ZP, META_STX},
	[STX_ZP_Y]  = {"STX_ZP_Y",  2, MODE_ZP_Y, META_STX},
	[STX_ABS]   = {"STX_ABS",   3, MODE_ABS, META_STX},

	// STY
	[STY_ZP]    = {"STY_ZP",    2, MODE_ZP, META_STY},
	[STY_ZP_Y]  = {"STY_ZP_Y",  2, MODE_ZP_Y, META_STY},
	[STY_ABS]   = {"STY_ABS",   3, MODE_ABS, META_STY},

	[TAX]       = {"TAX",       1, MODE_NONE, META_TAX},
	[TAY]       = {"TAY",       1, MODE_NONE, META_TAY},
	[TSX]       = {"TSX",       1, MODE_NONE, META_TSX},
	[TXA]       = {"TXA",       1, MODE_NONE, META_TXA},
	[TXS]       = {"TXS",       1, MODE_NONE, META_TXS},
	[TYA]       = {"TYA",       1, MODE_NONE, META_TYA},
};

#endif // NES_H_

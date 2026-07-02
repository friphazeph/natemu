#ifndef NES_H_
#define NES_H_

#include <stdint.h>
#include <stddef.h>

// ===== INES PARSING ===== //

typedef uint8_t Byte;

typedef struct {
	Byte magic[4];
	Byte prg_chunk_n;
	Byte chr_chunk_n;
	Byte flags6;
	Byte flags7;
	Byte flags8;
	Byte flags9;
	Byte flags10;
	Byte flags11;
	Byte flags12;
	Byte flags13;
	Byte flags14;
	Byte flags15;
} NesHdr;

#define trainer(h) ((h)->flags6 >> 2 & 1)

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
	STY_ZP_X  = 0x94,
	STY_ABS   = 0x8C,

	TAX       = 0xAA,
	TAY       = 0xA8,
	TSX       = 0xBA,
	TXA       = 0x8A,
	TXS       = 0x9A,
	TYA       = 0x98,
} OpKind;

typedef enum {
	MEM_KIND_NONE = 0,
	MEM_KIND_R,
	MEM_KIND_W,
	MEM_KIND_RMW,
	MEM_KIND_COUNT,
} MemKind;

static const size_t MEM_KIND_CYCLES[MEM_KIND_COUNT] = {
	[MEM_KIND_NONE] = 0,
	[MEM_KIND_R]    = 0,
	[MEM_KIND_W]    = 0,
	[MEM_KIND_RMW]  = 2
};

typedef enum {
	MODE_NONE = 0,
	MODE_ACC,
	MODE_IMM,
	MODE_IMM2,
	MODE_ZP,
	MODE_ZP_X,
	MODE_ZP_Y,
	MODE_ABS,
	MODE_ABS_X,
	MODE_ABS_Y,
	MODE_IND,
	MODE_IND_X,
	MODE_IND_Y,
	MODE_COUNT
} AddrMode;

static const char *MODE_STR[MODE_COUNT] = {
	[MODE_NONE]  = "MODE_NONE",
	[MODE_ACC]   = "MODE_ACC",
	[MODE_IMM]   = "MODE_IMM",
	[MODE_IMM2]  = "MODE_IMM2",
	[MODE_ZP]    = "MODE_ZP",
	[MODE_ZP_X]  = "MODE_ZP_X",
	[MODE_ZP_Y]  = "MODE_ZP_Y",
	[MODE_ABS]   = "MODE_ABS",
	[MODE_ABS_X] = "MODE_ABS_X",
	[MODE_ABS_Y] = "MODE_ABS_Y",
	[MODE_IND]   = "MODE_IND",
	[MODE_IND_X] = "MODE_IND_X",
	[MODE_IND_Y] = "MODE_IND_Y",
}; 

// Cycles tax (base, +1 on certain crossings with MEM_R if oops) for R, W, RMW
static const size_t MODE_CYCLES[MODE_COUNT] = {
	[MODE_NONE]  = 0,
	[MODE_ACC]   = 0,
	[MODE_IMM]   = 0,
	[MODE_IMM2]  = 0,
	[MODE_ZP]    = 1,
	[MODE_ZP_X]  = 2,
	[MODE_ZP_Y]  = 2,
	[MODE_ABS]   = 2,
	[MODE_ABS_X] = 2,
	[MODE_ABS_Y] = 2,
	[MODE_IND]   = 2,
	[MODE_IND_X] = 4,
	[MODE_IND_Y] = 3,
}; 

static const uint8_t OOPS[MODE_COUNT] = {
	[MODE_NONE]  = 0, 
	[MODE_ACC]   = 0, 
	[MODE_IMM]   = 0, 
	[MODE_IMM2]  = 0, 
	[MODE_ZP]    = 0, 
	[MODE_ZP_X]  = 0, 
	[MODE_ZP_Y]  = 0, 
	[MODE_ABS]   = 0, 
	[MODE_ABS_X] = 1,
	[MODE_ABS_Y] = 1, 
	[MODE_IND]   = 0, 
	[MODE_IND_X] = 0, 
	[MODE_IND_Y] = 1, 
}; 

typedef enum {
	META_ADC = 0,
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
	META_COUNT
} MetaOpKind;

static const char *META_STR[META_COUNT] = {
	[META_ADC] = "ADC",
	[META_AND] = "AND",
	[META_ASL] = "ASL",
	[META_BIT] = "BIT",
	[META_CMP] = "CMP",
	[META_CPX] = "CPX",
	[META_CPY] = "CPY",
	[META_DEC] = "DEC",
	[META_EOR] = "EOR",
	[META_INC] = "INC",
	[META_JMP] = "JMP",
	[META_LDA] = "LDA",
	[META_LDX] = "LDX",
	[META_LDY] = "LDY",
	[META_LSR] = "LSR",
	[META_ORA] = "ORA",
	[META_ROL] = "ROL",
	[META_ROR] = "ROR",
	[META_SBC] = "SBC",
	[META_STA] = "STA",
	[META_STX] = "STX",
	[META_STY] = "STY",
	[META_BCC] = "BCC",
	[META_BCS] = "BCS",
	[META_BEQ] = "BEQ",
	[META_BMI] = "BMI",
	[META_BNE] = "BNE",
	[META_BPL] = "BPL",
	[META_BRK] = "BRK",
	[META_BVC] = "BVC",
	[META_BVS] = "BVS",
	[META_CLC] = "CLC",
	[META_CLD] = "CLD",
	[META_CLI] = "CLI",
	[META_CLV] = "CLV",
	[META_DEX] = "DEX",
	[META_DEY] = "DEY",
	[META_INX] = "INX",
	[META_INY] = "INY",
	[META_JSR] = "JSR",
	[META_NOP] = "NOP",
	[META_PHA] = "PHA",
	[META_PHP] = "PHP",
	[META_PLA] = "PLA",
	[META_PLP] = "PLP",
	[META_RTI] = "RTI",
	[META_RTS] = "RTS",
	[META_SEC] = "SEC",
	[META_SED] = "SED",
	[META_SEI] = "SEI",
	[META_TAX] = "TAX",
	[META_TAY] = "TAY",
	[META_TSX] = "TSX",
	[META_TXA] = "TXA",
	[META_TXS] = "TXS",
	[META_TYA] = "TYA",
}; 

static const size_t META_CYCLES[META_COUNT] = {
	[META_ADC] = 2,
	[META_AND] = 2,
	[META_ASL] = 2,
	[META_BIT] = 2,
	[META_CMP] = 2,
	[META_CPX] = 2,
	[META_CPY] = 2,
	[META_DEC] = 2,
	[META_EOR] = 2,
	[META_INC] = 2,
	[META_JMP] = 3,
	[META_LDA] = 2,
	[META_LDX] = 2,
	[META_LDY] = 2,
	[META_LSR] = 2,
	[META_ORA] = 2,
	[META_ROL] = 2,
	[META_ROR] = 2,
	[META_SBC] = 2,
	[META_STA] = 2,
	[META_STX] = 2,
	[META_STY] = 2,
	[META_BCC] = 2,
	[META_BCS] = 2,
	[META_BEQ] = 2,
	[META_BMI] = 2,
	[META_BNE] = 2,
	[META_BPL] = 2,
	[META_BRK] = 7,
	[META_BVC] = 2,
	[META_BVS] = 2,
	[META_CLC] = 2,
	[META_CLD] = 2,
	[META_CLI] = 2,
	[META_CLV] = 2,
	[META_DEX] = 2,
	[META_DEY] = 2,
	[META_INX] = 2,
	[META_INY] = 2,
	[META_JSR] = 6,
	[META_NOP] = 2,
	[META_PHA] = 3,
	[META_PHP] = 3,
	[META_PLA] = 4,
	[META_PLP] = 4,
	[META_RTI] = 6,
	[META_RTS] = 6,
	[META_SEC] = 2,
	[META_SED] = 2,
	[META_SEI] = 2,
	[META_TAX] = 2,
	[META_TAY] = 2,
	[META_TSX] = 2,
	[META_TXA] = 2,
	[META_TXS] = 2,
	[META_TYA] = 2,
}; 

typedef struct {
	const char *name;
	size_t size;
	AddrMode addr_mode;
	MetaOpKind meta_kind;
	MemKind mem_kind;
} Op;

static const Op OPS[0x100] = {
	// ADC
	[ADC_IMM]   = {"ADC_IMM",   2, MODE_IMM,   META_ADC, MEM_KIND_R},
	[ADC_ZP]    = {"ADC_ZP",    2, MODE_ZP,    META_ADC, MEM_KIND_R},
	[ADC_ZP_X]  = {"ADC_ZP_X",  2, MODE_ZP_X,  META_ADC, MEM_KIND_R},
	[ADC_ABS]   = {"ADC_ABS",   3, MODE_ABS,   META_ADC, MEM_KIND_R},
	[ADC_ABS_X] = {"ADC_ABS_X", 3, MODE_ABS_X, META_ADC, MEM_KIND_R},
	[ADC_ABS_Y] = {"ADC_ABS_Y", 3, MODE_ABS_Y, META_ADC, MEM_KIND_R},
	[ADC_IND_X] = {"ADC_IND_X", 2, MODE_IND_X, META_ADC, MEM_KIND_R},
	[ADC_IND_Y] = {"ADC_IND_Y", 2, MODE_IND_Y, META_ADC, MEM_KIND_R},

	// AND
	[AND_IMM]   = {"AND_IMM",   2, MODE_IMM,   META_AND, MEM_KIND_R},
	[AND_ZP]    = {"AND_ZP",    2, MODE_ZP,    META_AND, MEM_KIND_R},
	[AND_ZP_X]  = {"AND_ZP_X",  2, MODE_ZP_X,  META_AND, MEM_KIND_R},
	[AND_ABS]   = {"AND_ABS",   3, MODE_ABS,   META_AND, MEM_KIND_R},
	[AND_ABS_X] = {"AND_ABS_X", 3, MODE_ABS_X, META_AND, MEM_KIND_R},
	[AND_ABS_Y] = {"AND_ABS_Y", 3, MODE_ABS_Y, META_AND, MEM_KIND_R},
	[AND_IND_X] = {"AND_IND_X", 2, MODE_IND_X, META_AND, MEM_KIND_R},
	[AND_IND_Y] = {"AND_IND_Y", 2, MODE_IND_Y, META_AND, MEM_KIND_R},

	// ASL
	[ASL_ACC]   = {"ASL_ACC",   1, MODE_ACC,   META_ASL, MEM_KIND_R},
	[ASL_ZP]    = {"ASL_ZP",    2, MODE_ZP,    META_ASL, MEM_KIND_RMW},
	[ASL_ZP_X]  = {"ASL_ZP_X",  2, MODE_ZP_X,  META_ASL, MEM_KIND_RMW},
	[ASL_ABS]   = {"ASL_ABS",   3, MODE_ABS,   META_ASL, MEM_KIND_RMW},
	[ASL_ABS_X] = {"ASL_ABS_X", 3, MODE_ABS_X, META_ASL, MEM_KIND_RMW},

	// BIT
	[BIT_ZP]    = {"BIT_ZP",    2, MODE_ZP,    META_BIT, MEM_KIND_R},
	[BIT_ABS]   = {"BIT_ABS",   3, MODE_ABS,   META_BIT, MEM_KIND_R},

	[BRK]       = {"BRK",       2, MODE_NONE,  META_BRK, MEM_KIND_NONE},

	[BCC]       = {"BCC",       2, MODE_IMM,   META_BCC, MEM_KIND_R},
	[BCS]       = {"BCS",       2, MODE_IMM,   META_BCS, MEM_KIND_R},
	[BEQ]       = {"BEQ",       2, MODE_IMM,   META_BEQ, MEM_KIND_R},
	[BMI]       = {"BMI",       2, MODE_IMM,   META_BMI, MEM_KIND_R},
	[BNE]       = {"BNE",       2, MODE_IMM,   META_BNE, MEM_KIND_R},
	[BPL]       = {"BPL",       2, MODE_IMM,   META_BPL, MEM_KIND_R},
	[BVC]       = {"BVC",       2, MODE_IMM,   META_BVC, MEM_KIND_R},
	[BVS]       = {"BVS",       2, MODE_IMM,   META_BVS, MEM_KIND_R},
	[CLC]       = {"CLC",       1, MODE_IMM,   META_CLC, MEM_KIND_R},
	[CLD]       = {"CLD",       1, MODE_IMM,   META_CLD, MEM_KIND_R},
	[CLI]       = {"CLI",       1, MODE_IMM,   META_CLI, MEM_KIND_R},
	[CLV]       = {"CLV",       1, MODE_IMM,   META_CLV, MEM_KIND_R},

	// CMP
	[CMP_IMM]   = {"CMP_IMM",   2, MODE_IMM,   META_CMP, MEM_KIND_R},
	[CMP_ZP]    = {"CMP_ZP",    2, MODE_ZP,    META_CMP, MEM_KIND_R},
	[CMP_ZP_X]  = {"CMP_ZP_X",  2, MODE_ZP_X,  META_CMP, MEM_KIND_R},
	[CMP_ABS]   = {"CMP_ABS",   3, MODE_ABS,   META_CMP, MEM_KIND_R},
	[CMP_ABS_X] = {"CMP_ABS_X", 3, MODE_ABS_X, META_CMP, MEM_KIND_R},
	[CMP_ABS_Y] = {"CMP_ABS_Y", 3, MODE_ABS_Y, META_CMP, MEM_KIND_R},
	[CMP_IND_X] = {"CMP_IND_X", 2, MODE_IND_X, META_CMP, MEM_KIND_R},
	[CMP_IND_Y] = {"CMP_IND_Y", 2, MODE_IND_Y, META_CMP, MEM_KIND_R},

	// CPX
	[CPX_IMM]   = {"CPX_IMM",   2, MODE_IMM,   META_CPX, MEM_KIND_R},
	[CPX_ZP]    = {"CPX_ZP",    2, MODE_ZP,    META_CPX, MEM_KIND_R},
	[CPX_ABS]   = {"CPX_ABS",   3, MODE_ABS,   META_CPX, MEM_KIND_R},

	// CPY
	[CPY_IMM]   = {"CPY_IMM",   2, MODE_IMM,   META_CPY, MEM_KIND_R},
	[CPY_ZP]    = {"CPY_ZP",    2, MODE_ZP,    META_CPY, MEM_KIND_R},
	[CPY_ABS]   = {"CPY_ABS",   3, MODE_ABS,   META_CPY, MEM_KIND_R},

	// DEC
	[DEC_ZP]    = {"DEC_ZP",    2, MODE_ZP,    META_DEC, MEM_KIND_RMW},
	[DEC_ZP_X]  = {"DEC_ZP_X",  2, MODE_ZP_X,  META_DEC, MEM_KIND_RMW},
	[DEC_ABS]   = {"DEC_ABS",   3, MODE_ABS,   META_DEC, MEM_KIND_RMW},
	[DEC_ABS_X] = {"DEC_ABS_X", 3, MODE_ABS_X, META_DEC, MEM_KIND_RMW},

	[DEX]       = {"DEX",       1, MODE_NONE,  META_DEX, MEM_KIND_NONE},
	[DEY]       = {"DEY",       1, MODE_NONE,  META_DEY, MEM_KIND_NONE},

	// EOR
	[EOR_IMM]   = {"EOR_IMM",   2, MODE_IMM,   META_EOR, MEM_KIND_R},
	[EOR_ZP]    = {"EOR_ZP",    2, MODE_ZP,    META_EOR, MEM_KIND_R},
	[EOR_ZP_X]  = {"EOR_ZP_X",  2, MODE_ZP_X,  META_EOR, MEM_KIND_R},
	[EOR_ABS]   = {"EOR_ABS",   3, MODE_ABS,   META_EOR, MEM_KIND_R},
	[EOR_ABS_X] = {"EOR_ABS_X", 3, MODE_ABS_X, META_EOR, MEM_KIND_R},
	[EOR_ABS_Y] = {"EOR_ABS_Y", 3, MODE_ABS_Y, META_EOR, MEM_KIND_R},
	[EOR_IND_X] = {"EOR_IND_X", 2, MODE_IND_X, META_EOR, MEM_KIND_R},
	[EOR_IND_Y] = {"EOR_IND_Y", 2, MODE_IND_Y, META_EOR, MEM_KIND_R},

	// INC
	[INC_ZP]    = {"INC_ZP",    2, MODE_ZP,    META_INC, MEM_KIND_RMW},
	[INC_ZP_X]  = {"INC_ZP_X",  2, MODE_ZP_X,  META_INC, MEM_KIND_RMW},
	[INC_ABS]   = {"INC_ABS",   3, MODE_ABS,   META_INC, MEM_KIND_RMW},
	[INC_ABS_X] = {"INC_ABS_X", 3, MODE_ABS_X, META_INC, MEM_KIND_RMW},

	[INX]       = {"INX",       1, MODE_NONE,  META_INX, MEM_KIND_NONE},
	[INY]       = {"INY",       1, MODE_NONE,  META_INY, MEM_KIND_NONE},

	// JMP
	[JMP_ABS]   = {"JMP_ABS",   3, MODE_IMM2,  META_JMP, MEM_KIND_R}, // Immediate (2 bytes) mode on purpose, though it doesn't match hardware
	[JMP_IND]   = {"JMP_IND",   3, MODE_IND,   META_JMP, MEM_KIND_R},
	[JSR]       = {"JSR",       3, MODE_IMM2,  META_JSR, MEM_KIND_R}, // Immediate (2 bytes) mode on purpose, though it doesn't match hardware

	// LDA
	[LDA_IMM]   = {"LDA_IMM",   2, MODE_IMM,   META_LDA, MEM_KIND_R},
	[LDA_ZP]    = {"LDA_ZP",    2, MODE_ZP,    META_LDA, MEM_KIND_R},
	[LDA_ZP_X]  = {"LDA_ZP_X",  2, MODE_ZP_X,  META_LDA, MEM_KIND_R},
	[LDA_ABS]   = {"LDA_ABS",   3, MODE_ABS,   META_LDA, MEM_KIND_R},
	[LDA_ABS_X] = {"LDA_ABS_X", 3, MODE_ABS_X, META_LDA, MEM_KIND_R},
	[LDA_ABS_Y] = {"LDA_ABS_Y", 3, MODE_ABS_Y, META_LDA, MEM_KIND_R},
	[LDA_IND_X] = {"LDA_IND_X", 2, MODE_IND_X, META_LDA, MEM_KIND_R},
	[LDA_IND_Y] = {"LDA_IND_Y", 2, MODE_IND_Y, META_LDA, MEM_KIND_R},

	// LDX
	[LDX_IMM]   = {"LDX_IMM",   2, MODE_IMM,   META_LDX, MEM_KIND_R},
	[LDX_ZP]    = {"LDX_ZP",    2, MODE_ZP,    META_LDX, MEM_KIND_R},
	[LDX_ZP_Y]  = {"LDX_ZP_Y",  2, MODE_ZP_Y,  META_LDX, MEM_KIND_R},
	[LDX_ABS]   = {"LDX_ABS",   3, MODE_ABS,   META_LDX, MEM_KIND_R},
	[LDX_ABS_Y] = {"LDX_ABS_Y", 3, MODE_ABS_Y, META_LDX, MEM_KIND_R},

	// LDY
	[LDY_IMM]   = {"LDY_IMM",   2, MODE_IMM,   META_LDY, MEM_KIND_R},
	[LDY_ZP]    = {"LDY_ZP",    2, MODE_ZP,    META_LDY, MEM_KIND_R},
	[LDY_ZP_X]  = {"LDY_ZP_X",  2, MODE_ZP_X,  META_LDY, MEM_KIND_R},
	[LDY_ABS]   = {"LDY_ABS",   3, MODE_ABS,   META_LDY, MEM_KIND_R},
	[LDY_ABS_X] = {"LDY_ABS_X", 3, MODE_ABS_X, META_LDY, MEM_KIND_R},

	// LSR
	[LSR_ACC]   = {"LSR_ACC",   1, MODE_ACC,   META_LSR, MEM_KIND_R},
	[LSR_ZP]    = {"LSR_ZP",    2, MODE_ZP,    META_LSR, MEM_KIND_RMW},
	[LSR_ZP_X]  = {"LSR_ZP_X",  2, MODE_ZP_X,  META_LSR, MEM_KIND_RMW},
	[LSR_ABS]   = {"LSR_ABS",   3, MODE_ABS,   META_LSR, MEM_KIND_RMW},
	[LSR_ABS_X] = {"LSR_ABS_X", 3, MODE_ABS_X, META_LSR, MEM_KIND_RMW},

	[NOP]       = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},

	// ORA
	[ORA_IMM]   = {"ORA_IMM",   2, MODE_IMM,   META_ORA, MEM_KIND_R},
	[ORA_ZP]    = {"ORA_ZP",    2, MODE_ZP,    META_ORA, MEM_KIND_R},
	[ORA_ZP_X]  = {"ORA_ZP_X",  2, MODE_ZP_X,  META_ORA, MEM_KIND_R},
	[ORA_ABS]   = {"ORA_ABS",   3, MODE_ABS,   META_ORA, MEM_KIND_R},
	[ORA_ABS_X] = {"ORA_ABS_X", 3, MODE_ABS_X, META_ORA, MEM_KIND_R},
	[ORA_ABS_Y] = {"ORA_ABS_Y", 3, MODE_ABS_Y, META_ORA, MEM_KIND_R},
	[ORA_IND_X] = {"ORA_IND_X", 2, MODE_IND_X, META_ORA, MEM_KIND_R},
	[ORA_IND_Y] = {"ORA_IND_Y", 2, MODE_IND_Y, META_ORA, MEM_KIND_R},

	[PHA]       = {"PHA",       1, MODE_NONE,  META_PHA, MEM_KIND_NONE},
	[PHP]       = {"PHP",       1, MODE_NONE,  META_PHP, MEM_KIND_NONE},
	[PLA]       = {"PLA",       1, MODE_NONE,  META_PLA, MEM_KIND_NONE},
	[PLP]       = {"PLP",       1, MODE_NONE,  META_PLP, MEM_KIND_NONE},

	// ROL
	[ROL_ACC]   = {"ROL_ACC",   1, MODE_ACC,   META_ROL, MEM_KIND_R},
	[ROL_ZP]    = {"ROL_ZP",    2, MODE_ZP,    META_ROL, MEM_KIND_RMW},
	[ROL_ZP_X]  = {"ROL_ZP_X",  2, MODE_ZP_X,  META_ROL, MEM_KIND_RMW},
	[ROL_ABS]   = {"ROL_ABS",   3, MODE_ABS,   META_ROL, MEM_KIND_RMW},
	[ROL_ABS_X] = {"ROL_ABS_X", 3, MODE_ABS_X, META_ROL, MEM_KIND_RMW},

	// ROR
	[ROR_ACC]   = {"ROR_ACC",   1, MODE_ACC,   META_ROR, MEM_KIND_R},
	[ROR_ZP]    = {"ROR_ZP",    2, MODE_ZP,    META_ROR, MEM_KIND_RMW},
	[ROR_ZP_X]  = {"ROR_ZP_X",  2, MODE_ZP_X,  META_ROR, MEM_KIND_RMW},
	[ROR_ABS]   = {"ROR_ABS",   3, MODE_ABS,   META_ROR, MEM_KIND_RMW},
	[ROR_ABS_X] = {"ROR_ABS_X", 3, MODE_ABS_X, META_ROR, MEM_KIND_RMW},

	[RTI]       = {"RTI",       1, MODE_NONE,  META_RTI, MEM_KIND_NONE},
	[RTS]       = {"RTS",       1, MODE_NONE,  META_RTS, MEM_KIND_NONE},

	// SBC
	[SBC_IMM]   = {"SBC_IMM",   2, MODE_IMM,   META_SBC, MEM_KIND_R},
	[SBC_ZP]    = {"SBC_ZP",    2, MODE_ZP,    META_SBC, MEM_KIND_R},
	[SBC_ZP_X]  = {"SBC_ZP_X",  2, MODE_ZP_X,  META_SBC, MEM_KIND_R},
	[SBC_ABS]   = {"SBC_ABS",   3, MODE_ABS,   META_SBC, MEM_KIND_R},
	[SBC_ABS_X] = {"SBC_ABS_X", 3, MODE_ABS_X, META_SBC, MEM_KIND_R},
	[SBC_ABS_Y] = {"SBC_ABS_Y", 3, MODE_ABS_Y, META_SBC, MEM_KIND_R},
	[SBC_IND_X] = {"SBC_IND_X", 2, MODE_IND_X, META_SBC, MEM_KIND_R},
	[SBC_IND_Y] = {"SBC_IND_Y", 2, MODE_IND_Y, META_SBC, MEM_KIND_R},

	[SEC]       = {"SEC",       1, MODE_NONE,  META_SEC, MEM_KIND_NONE},
	[SED]       = {"SED",       1, MODE_NONE,  META_SED, MEM_KIND_NONE},
	[SEI]       = {"SEI",       1, MODE_NONE,  META_SEI, MEM_KIND_NONE},

	// STA
	[STA_ZP]    = {"STA_ZP",    2, MODE_ZP,    META_STA, MEM_KIND_W},
	[STA_ZP_X]  = {"STA_ZP_X",  2, MODE_ZP_X,  META_STA, MEM_KIND_W},
	[STA_ABS]   = {"STA_ABS",   3, MODE_ABS,   META_STA, MEM_KIND_W},
	[STA_ABS_X] = {"STA_ABS_X", 3, MODE_ABS_X, META_STA, MEM_KIND_W},
	[STA_ABS_Y] = {"STA_ABS_Y", 3, MODE_ABS_Y, META_STA, MEM_KIND_W},
	[STA_IND_X] = {"STA_IND_X", 2, MODE_IND_X, META_STA, MEM_KIND_W},
	[STA_IND_Y] = {"STA_IND_Y", 2, MODE_IND_Y, META_STA, MEM_KIND_W},

	// STX
	[STX_ZP]    = {"STX_ZP",    2, MODE_ZP,    META_STX, MEM_KIND_W},
	[STX_ZP_Y]  = {"STX_ZP_Y",  2, MODE_ZP_Y,  META_STX, MEM_KIND_W},
	[STX_ABS]   = {"STX_ABS",   3, MODE_ABS,   META_STX, MEM_KIND_W},

	// STY
	[STY_ZP]    = {"STY_ZP",    2, MODE_ZP,    META_STY, MEM_KIND_W},
	[STY_ZP_X]  = {"STY_ZP_X",  2, MODE_ZP_X,  META_STY, MEM_KIND_W},
	[STY_ABS]   = {"STY_ABS",   3, MODE_ABS,   META_STY, MEM_KIND_W},

	[TAX]       = {"TAX",       1, MODE_NONE,  META_TAX, MEM_KIND_NONE},
	[TAY]       = {"TAY",       1, MODE_NONE,  META_TAY, MEM_KIND_NONE},
	[TSX]       = {"TSX",       1, MODE_NONE,  META_TSX, MEM_KIND_NONE},
	[TXA]       = {"TXA",       1, MODE_NONE,  META_TXA, MEM_KIND_NONE},
	[TXS]       = {"TXS",       1, MODE_NONE,  META_TXS, MEM_KIND_NONE},
	[TYA]       = {"TYA",       1, MODE_NONE,  META_TYA, MEM_KIND_NONE},

	// -- unofficials --
	[0x04]      = {"NOP",       2, MODE_ZP,    META_NOP, MEM_KIND_R},
	[0x44]      = {"NOP",       2, MODE_ZP,    META_NOP, MEM_KIND_R},
	[0x64]      = {"NOP",       2, MODE_ZP,    META_NOP, MEM_KIND_R},
	[0x14]      = {"NOP",       2, MODE_ZP_X,  META_NOP, MEM_KIND_R},
	[0x34]      = {"NOP",       2, MODE_ZP_X,  META_NOP, MEM_KIND_R},
	[0x54]      = {"NOP",       2, MODE_ZP_X,  META_NOP, MEM_KIND_R},
	[0x74]      = {"NOP",       2, MODE_ZP_X,  META_NOP, MEM_KIND_R},
	[0xD4]      = {"NOP",       2, MODE_ZP_X,  META_NOP, MEM_KIND_R},
	[0xF4]      = {"NOP",       2, MODE_ZP_X,  META_NOP, MEM_KIND_R},
	[0x0C]      = {"NOP",       3, MODE_ABS,   META_NOP, MEM_KIND_R},
	[0x1C]      = {"NOP",       3, MODE_ABS_X, META_NOP, MEM_KIND_R},
	[0x3C]      = {"NOP",       3, MODE_ABS_X, META_NOP, MEM_KIND_R},
	[0x5C]      = {"NOP",       3, MODE_ABS_X, META_NOP, MEM_KIND_R},
	[0x7C]      = {"NOP",       3, MODE_ABS_X, META_NOP, MEM_KIND_R},
	[0xDC]      = {"NOP",       3, MODE_ABS_X, META_NOP, MEM_KIND_R},
	[0xFC]      = {"NOP",       3, MODE_ABS_X, META_NOP, MEM_KIND_R},
	[0x80]      = {"NOP",       2, MODE_IMM,   META_NOP, MEM_KIND_R},
	[0x1A]      = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},
	[0x3A]      = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},
	[0x5A]      = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},
	[0x7A]      = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},
	[0xDA]      = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},
	[0xFA]      = {"NOP",       1, MODE_NONE,  META_NOP, MEM_KIND_R},
};

typedef uint16_t Addr;

// computes cycles, excluding runtime "oops"-cycles
static inline size_t base_cycles(Op op) {
    size_t cycles = META_CYCLES[op.meta_kind] + MODE_CYCLES[op.addr_mode] + MEM_KIND_CYCLES[op.mem_kind];
    
    if (op.mem_kind != MEM_KIND_R) {
        cycles += OOPS[op.addr_mode];
    }
    
    return cycles;
}

#endif // NES_H_

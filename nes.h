#ifndef NES_H_
#define NES_H_

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
} Op;

typedef struct {
	const char *name;
	size_t size;
} OpDesc;

static const OpDesc OP_DESCS[0x100] = {
	// ADC
	[ADC_IMM]   = {"ADC_IMM",   2},
	[ADC_ZP]    = {"ADC_ZP",    2},
	[ADC_ZP_X]  = {"ADC_ZP_X",  2},
	[ADC_ABS]   = {"ADC_ABS",   3},
	[ADC_ABS_X] = {"ADC_ABS_X", 3},
	[ADC_ABS_Y] = {"ADC_ABS_Y", 3},
	[ADC_IND_X] = {"ADC_IND_X", 2},
	[ADC_IND_Y] = {"ADC_IND_Y", 2},

	// AND
	[AND_IMM]   = {"AND_IMM",   2},
	[AND_ZP]    = {"AND_ZP",    2},
	[AND_ZP_X]  = {"AND_ZP_X",  2},
	[AND_ABS]   = {"AND_ABS",   3},
	[AND_ABS_X] = {"AND_ABS_X", 3},
	[AND_ABS_Y] = {"AND_ABS_Y", 3},
	[AND_IND_X] = {"AND_IND_X", 2},
	[AND_IND_Y] = {"AND_IND_Y", 2},

	// ASL
	[ASL_ACC]   = {"ASL_ACC",   1},
	[ASL_ZP]    = {"ASL_ZP",    2},
	[ASL_ZP_X]  = {"ASL_ZP_X",  2},
	[ASL_ABS]   = {"ASL_ABS",   3},
	[ASL_ABS_X] = {"ASL_ABS_X", 3},

	[BCC]       = {"BCC",       2},
	[BCS]       = {"BCS",       2},
	[BEQ]       = {"BEQ",       2},

	// BIT
	[BIT_ZP]    = {"BIT_ZP",    2},
	[BIT_ABS]   = {"BIT_ABS",   3},

	[BMI]       = {"BMI",       2},
	[BNE]       = {"BNE",       2},
	[BPL]       = {"BPL",       2},
	[BRK]       = {"BRK",       2},
	[BVC]       = {"BVC",       2},
	[BVS]       = {"BVS",       2},
	[CLC]       = {"CLC",       1},
	[CLD]       = {"CLD",       1},
	[CLI]       = {"CLI",       1},
	[CLV]       = {"CLV",       1},

	// CMP
	[CMP_IMM]   = {"CMP_IMM",   2},
	[CMP_ZP]    = {"CMP_ZP",    2},
	[CMP_ZP_X]  = {"CMP_ZP_X",  2},
	[CMP_ABS]   = {"CMP_ABS",   3},
	[CMP_ABS_X] = {"CMP_ABS_X", 3},
	[CMP_ABS_Y] = {"CMP_ABS_Y", 3},
	[CMP_IND_X] = {"CMP_IND_X", 2},
	[CMP_IND_Y] = {"CMP_IND_Y", 2},

	// CPX
	[CPX_IMM]   = {"CPX_IMM",   2},
	[CPX_ZP]    = {"CPX_ZP",    2},
	[CPX_ABS]   = {"CPX_ABS",   3},

	// CPY
	[CPY_IMM]   = {"CPY_IMM",   2},
	[CPY_ZP]    = {"CPY_ZP",    2},
	[CPY_ABS]   = {"CPY_ABS",   3},

	// DEC
	[DEC_ZP]    = {"DEC_ZP",    2},
	[DEC_ZP_X]  = {"DEC_ZP_X",  2},
	[DEC_ABS]   = {"DEC_ABS",   3},
	[DEC_ABS_X] = {"DEC_ABS_X", 3},

	[DEX]       = {"DEX",       1},
	[DEY]       = {"DEY",       1},

	// EOR
	[EOR_IMM]   = {"EOR_IMM",   2},
	[EOR_ZP]    = {"EOR_ZP",    2},
	[EOR_ZP_X]  = {"EOR_ZP_X",  2},
	[EOR_ABS]   = {"EOR_ABS",   3},
	[EOR_ABS_X] = {"EOR_ABS_X", 3},
	[EOR_ABS_Y] = {"EOR_ABS_Y", 3},
	[EOR_IND_X] = {"EOR_IND_X", 2},
	[EOR_IND_Y] = {"EOR_IND_Y", 2},

	// INC
	[INC_ZP]    = {"INC_ZP",    2},
	[INC_ZP_X]  = {"INC_ZP_X",  2},
	[INC_ABS]   = {"INC_ABS",   3},
	[INC_ABS_X] = {"INC_ABS_X", 3},

	[INX]       = {"INX",       1},
	[INY]       = {"INY",       1},

	// JMP
	[JMP_ABS]   = {"JMP_ABS",   3},
	[JMP_IND]   = {"JMP_IND",   3},

	[JSR]       = {"JSR",       3},

	// LDA
	[LDA_IMM]   = {"LDA_IMM",   2},
	[LDA_ZP]    = {"LDA_ZP",    2},
	[LDA_ZP_X]  = {"LDA_ZP_X",  2},
	[LDA_ABS]   = {"LDA_ABS",   3},
	[LDA_ABS_X] = {"LDA_ABS_X", 3},
	[LDA_ABS_Y] = {"LDA_ABS_Y", 3},
	[LDA_IND_X] = {"LDA_IND_X", 2},
	[LDA_IND_Y] = {"LDA_IND_Y", 2},

	// LDX
	[LDX_IMM]   = {"LDX_IMM",   2},
	[LDX_ZP]    = {"LDX_ZP",    2},
	[LDX_ZP_Y]  = {"LDX_ZP_Y",  2},
	[LDX_ABS]   = {"LDX_ABS",   3},
	[LDX_ABS_Y] = {"LDX_ABS_Y", 3},

	// LDY
	[LDY_IMM]   = {"LDY_IMM",   2},
	[LDY_ZP]    = {"LDY_ZP",    2},
	[LDY_ZP_X]  = {"LDY_ZP_X",  2},
	[LDY_ABS]   = {"LDY_ABS",   3},
	[LDY_ABS_X] = {"LDY_ABS_X", 3},

	// LSR
	[LSR_ACC]   = {"LSR_ACC",   1},
	[LSR_ZP]    = {"LSR_ZP",    2},
	[LSR_ZP_X]  = {"LSR_ZP_X",  2},
	[LSR_ABS]   = {"LSR_ABS",   3},
	[LSR_ABS_X] = {"LSR_ABS_X", 3},

	[NOP]       = {"NOP",       1},

	// ORA
	[ORA_IMM]   = {"ORA_IMM",   2},
	[ORA_ZP]    = {"ORA_ZP",    2},
	[ORA_ZP_X]  = {"ORA_ZP_X",  2},
	[ORA_ABS]   = {"ORA_ABS",   3},
	[ORA_ABS_X] = {"ORA_ABS_X", 3},
	[ORA_ABS_Y] = {"ORA_ABS_Y", 3},
	[ORA_IND_X] = {"ORA_IND_X", 2},
	[ORA_IND_Y] = {"ORA_IND_Y", 2},

	[PHA]       = {"PHA",       1},
	[PHP]       = {"PHP",       1},
	[PLA]       = {"PLA",       1},
	[PLP]       = {"PLP",       1},

	// ROL
	[ROL_ACC]   = {"ROL_ACC",   1},
	[ROL_ZP]    = {"ROL_ZP",    2},
	[ROL_ZP_X]  = {"ROL_ZP_X",  2},
	[ROL_ABS]   = {"ROL_ABS",   3},
	[ROL_ABS_X] = {"ROL_ABS_X", 3},

	// ROR
	[ROR_ACC]   = {"ROR_ACC",   1},
	[ROR_ZP]    = {"ROR_ZP",    2},
	[ROR_ZP_X]  = {"ROR_ZP_X",  2},
	[ROR_ABS]   = {"ROR_ABS",   3},
	[ROR_ABS_X] = {"ROR_ABS_X", 3},

	[RTI]       = {"RTI",       1},
	[RTS]       = {"RTS",       1},

	// SBC
	[SBC_IMM]   = {"SBC_IMM",   2},
	[SBC_ZP]    = {"SBC_ZP",    2},
	[SBC_ZP_X]  = {"SBC_ZP_X",  2},
	[SBC_ABS]   = {"SBC_ABS",   3},
	[SBC_ABS_X] = {"SBC_ABS_X", 3},
	[SBC_ABS_Y] = {"SBC_ABS_Y", 3},
	[SBC_IND_X] = {"SBC_IND_X", 2},
	[SBC_IND_Y] = {"SBC_IND_Y", 2},

	[SEC]       = {"SEC",       1},
	[SED]       = {"SED",       1},
	[SEI]       = {"SEI",       1},

	// STA
	[STA_ZP]    = {"STA_ZP",    2},
	[STA_ZP_X]  = {"STA_ZP_X",  2},
	[STA_ABS]   = {"STA_ABS",   3},
	[STA_ABS_X] = {"STA_ABS_X", 3},
	[STA_ABS_Y] = {"STA_ABS_Y", 3},
	[STA_IND_X] = {"STA_IND_X", 2},
	[STA_IND_Y] = {"STA_IND_Y", 2},

	// STX
	[STX_ZP]    = {"STX_ZP",    2},
	[STX_ZP_Y]  = {"STX_ZP_Y",  2},
	[STX_ABS]   = {"STX_ABS",   3},

	// STY
	[STY_ZP]    = {"STY_ZP",    2},
	[STY_ZP_Y]  = {"STY_ZP_Y",  2},
	[STY_ABS]   = {"STY_ABS",   3},

	[TAX]       = {"TAX",       1},
	[TAY]       = {"TAY",       1},
	[TSX]       = {"TSX",       1},
	[TXA]       = {"TXA",       1},
	[TXS]       = {"TXS",       1},
	[TYA]       = {"TYA",       1},
};

#endif // NES_H_

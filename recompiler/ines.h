#ifndef INES_H_
#define INES_H_

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

#define NES_PRG_BASE      0x8000
#define NES_PRG_END       0xFFFF
#define INES_TRAINER_SZ   512
#define INES_PRG_CHUNK_SZ (1 << 14)
#define INES_CHR_CHUNK_SZ (1 << 13)

#endif // INES_H_

#ifndef NESROM_H_
#define NESROM_H_

#include "../cut.h"
#include "ines.h"

typedef struct {
	File f;
	NesHdr *hdr;
	Byte (*trainer)[512];
	size_t prg_len;
	Byte *prg;
	size_t chr_len;
	Byte *chr;
    Byte (*INST_ROM)[8192]; // |- PlayChoice, usused atm
    Byte (*PROM)[32];       // |
} NesRom;

NesRom load_rom(const char *path);
void print_hdr(NesHdr h);
void dump_prg(const NesRom *rom, const char *path);
void dump_chr(const NesRom *rom, const char *path);

#endif // NESROM_H_

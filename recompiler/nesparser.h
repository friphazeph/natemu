#ifndef NESPARSER_H_
#define NESPARSER_H_

#include "nesrom.h"
#include "../nes.h"

typedef struct {
	Byte *bytes;
	OpKind op;
	Addr addr;
	size_t offs;
} Instr;

void print_instr(Instr in);

typedef struct {
	Instr *instrs;       // |
	Addr **parents;      // |- Parallel arrays
	bool *is_valid;      // |
	size_t *child_count; // |
	size_t len;
	uint8_t mapper;
} NesParser;

NesParser parse_file(const char *path);
void print_branches(NesParser *p);
void print_instrs(NesParser *p);

#endif // NESPARSER_H_

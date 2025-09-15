#include <stdio.h>
#include <stdbool.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include "nes.h"

#define ROMS_DIR "./roms/"

NesHdr chop_hdr(FILE *fd) {
	NesHdr hdr;
	fread(&hdr, 1, sizeof(NesHdr), fd);
	if (memcmp(hdr.magic, "NES\x1A", 4) != 0) {
		fprintf(stderr, "Invalid NES file!\n");
		exit(1);
	}
	return hdr;
}

void print_hdr(NesHdr h) {
	printf("Magic: ");
	for (size_t i = 0; i < 4; ++i) {
		putchar(h.magic[i]);
	}
	putchar('\n');
	printf("PRG len: %u\n", h.prg_len);
	printf("CHR len: %u\n", h.chr_len);
	printf("flags 6: %u\n", h.flags6);
	printf("flags 7: %u\n", h.flags7);
	printf("flags 8: %u\n", h.flags8);
	printf("flags 9: %u\n", h.flags9);
	printf("flags 10: %u\n", h.flags10);
	printf("flags 11-15: ");
	for (size_t i = 0; i < 5; ++i) {
		putchar(h._flags[i]);
	}
	putchar('\n');
}

void skip_trainer(NesHdr h, FILE *fd) {
	if (h.flags6 >> 2 & 1) { // trainer bit
		fseek(fd, 512, SEEK_CUR);
	}
}

NesRom load_rom(const char *path) {
	NesRom r = {0};
	FILE *fd = fopen(path, "rb");
	r.hdr = chop_hdr(fd);
	// print_hdr(r.hdr);
	skip_trainer(r.hdr, fd);
	size_t prg_len = r.hdr.prg_len * (1 << 14);
	r.prg.items = malloc(prg_len);
	if (!r.prg.items) {
		fprintf(stderr, "Memory allocation failed!\n");
		exit(1);
	}
	r.prg.count = fread(r.prg.items, 1, prg_len, fd);
	r.prg.capacity = r.prg.count;
	// printf("%zu\n", r.prg.count);
	fclose(fd);
	return r;
}

typedef uint16_t Addr;

uint64_t rom_read_at(const NesRom *rom, Addr cpu_addr, size_t num_bytes) {
    if (cpu_addr < 0x8000) {
        fprintf(stderr, "Error: Read out of bounds (offset 0x%X < 0x8000)\n",
                cpu_addr);
        exit(1);
    }

	cpu_addr -= 0x8000;

    if (cpu_addr + num_bytes > rom->prg.count) {
        fprintf(stderr, "Error: Read out of bounds (cpu_addr 0x%X + 0x%lX > 0x%lX)\n",
                cpu_addr, num_bytes, rom->prg.count);
        exit(1);
    }

    uint64_t value = 0;
    for (size_t i = 0; i < num_bytes; i++) {
        value |= ((uint64_t)rom->prg.items[cpu_addr + i]) << (8*i);
		// printf("%X\n", rom->prg.items[cpu_addr + i]);
	}
    return value;
}

typedef struct {
	Addr *items;
	size_t count;
	size_t capacity;
} Addrs;

// bit 0 (0b000001): done
// bit 1 (0b000010): code
// bit 2 (0b000100): branch
// bit 3 (0b001000): branch_done
// bit 4 (0b010000): jsr
// bit 5 (0b100000): jsr_dst
typedef uint8_t AddrStatus;

typedef struct {
	AddrStatus *items;
	size_t count;
	size_t capacity;
} AddrStatuses;


typedef struct {
	Bytes bytes;
	Op op;
	Addr loc;
} Instr;

void print_instr(Instr in) {
	printf("0x%04X: %10s: ", in.loc, OP_DESCS[in.op].name);
	for (size_t i = 0; i < in.bytes.count; i++) {
		printf("0x%02X ", in.bytes.items[i]);
	}
	putchar('\n');
}

typedef struct {
	Instr *items;
	size_t count;
	size_t capacity;
} Instrs;

typedef struct {
	Addrs *items;
	size_t count;
	size_t capacity;
} AddrArrays;

typedef struct {
	NesRom rom;
	Instrs instrs;         // Parallel arrays
	AddrArrays parents;    // Parallel arrays
	AddrStatuses statuses; // Parallel arrays
} NesParser;

NesParser create_parser(const char *path) {
	NesParser p = {0};
	p.rom = load_rom(path);
	size_t prg_len = p.rom.prg.count;
	p.statuses.items = calloc(prg_len, sizeof(AddrStatus));
	p.statuses.count = prg_len;
	p.statuses.capacity = prg_len;
	p.instrs.items = calloc(prg_len, sizeof(Instr));
	p.instrs.count = prg_len;
	p.instrs.capacity = prg_len;
	p.parents.items = calloc(prg_len, sizeof(Addrs));
	p.parents.count = prg_len;
	p.parents.capacity = prg_len;

	return p;
}

static inline bool is_done(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	return p->statuses.items[addr-0x8000] & 0b1;
}

static inline void set_done(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	p->statuses.items[addr-0x8000] |= 0b1;
	return;
}

static inline bool in_prg_range(NesParser *p, int32_t addr) {
    return addr >= 0x8000 && addr < (int32_t)(0x8000 + (int32_t)p->parents.count);
}

static inline void set_code(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
    p->statuses.items[addr-0x8000] |= 0b10;
}
static inline void clear_code(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
    p->statuses.items[addr-0x8000] &= ~0b10;
}
static inline void set_branch(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
    p->statuses.items[addr-0x8000] |= 0b100;
}
static inline void clear_branch(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
    p->statuses.items[addr-0x8000] &= ~0b100;
}
static inline bool is_code(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	return p->statuses.items[addr-0x8000] & 0b10;
}

static inline bool is_branch(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	return p->statuses.items[addr-0x8000] & 0b100;
}

static inline bool is_branch_done(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	return p->statuses.items[addr-0x8000] & 0b1000;
}

static inline void set_branch_done(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	p->statuses.items[addr-0x8000] |= 0b1000;
	return;
}

static inline bool is_jsr(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	return p->statuses.items[addr-0x8000] & 0b10000;
}

static inline void set_jsr(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	p->statuses.items[addr-0x8000] |= 0b10000;
	return;
}

static inline bool is_jsr_dst(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	return p->statuses.items[addr-0x8000] & 0b100000;
}

static inline void set_jsr_dst(NesParser *p, Addr addr) {
    assert(in_prg_range(p, addr));
	p->statuses.items[addr-0x8000] |= 0b100000;
	return;
}

void illegal_branch(NesParser *p, Addr a) {
	Addrs *parents = &p->parents.items[a - 0x8000];
	for (size_t i = 0; i < parents->count; i++) {
		Addr parent = parents->items[i];
		if (a == 0xFFFD) {
			printf("0x%04X\n", parent);
		}
		if (!is_code(p, parent)) continue;
		if (is_branch(p, parent)) {
			if (is_jsr(p, parent) && is_jsr_dst(p, a)) {
				clear_code(p, parent);
				illegal_branch(p, parent);
				continue;
			}
			if (is_branch_done(p, parent)) {
				clear_code(p, parent);
				illegal_branch(p, parent);
			} else {
				set_branch_done(p, parent);
			}
			continue;
		}
		clear_code(p, parent);
		illegal_branch(p, parent);
	}
	return;	
}

void set_parents(NesParser *p, Instr ins) {
	switch (ins.op) {
		case JMP_ABS: {
			Addr jump_addr = ins.bytes.items[1] | ins.bytes.items[2] << 8;
			if (in_prg_range(p, jump_addr)) {
				Addrs *parents = &p->parents.items[jump_addr-0x8000];
				da_append(parents, ins.loc);
			}
			return;
		} break;
		case JMP_IND: { // We don't know ehre it jumps, so no explicit child
			return;
		} break;
		case JSR: {
			Addr jump_addr = ins.bytes.items[1] | ins.bytes.items[2] << 8;
			if (in_prg_range(p, jump_addr)) {
				set_jsr(p, ins.loc);
				set_jsr_dst(p, jump_addr);
				Addrs *parents = &p->parents.items[jump_addr-0x8000];
				da_append(parents, ins.loc);
			} else {
				clear_code(p, ins.loc);
			}
		} break;
		case BCC: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BCS: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BEQ: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BMI: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BNE: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BPL: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BVC: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BVS: {
			int8_t offs = ins.bytes.items[1];
			if (in_prg_range(p, ins.loc + 2 + offs)) {
				set_branch(p, ins.loc);
				Addrs *parents = &p->parents.items[ins.loc + 2 + offs-0x8000];
				da_append(parents, ins.loc);
			} else {
				set_branch_done(p, ins.loc);
			}
		} break;
		case BRK: {
			return;
		} break;
		case RTS: {
			return;
		} break;
		case RTI: {
			return;
		} break;
		default: break;
	}
	OpDesc op_desc = OP_DESCS[ins.op];
	if (in_prg_range(p, ins.loc + op_desc.size)) {
		Addrs *parents = &p->parents.items[ins.loc + op_desc.size - 0x8000];
		da_append(parents, ins.loc);
	} else {
		if (is_branch(p, ins.loc)) {
			if (is_branch_done(p, ins.loc)) {
				clear_code(p, ins.loc);
			}
		} else {
			clear_code(p, ins.loc);
		}
	}
	return;
}

bool is_parent(NesParser *p, Instr in, Instr par) {
	if (!in_prg_range(p, in.loc)) return false;
	Addrs parents = p->parents.items[in.loc-0x8000];
	for (size_t i = 0; i < parents.count; i++) {
		Addr parent = parents.items[i];
		if (parent == par.loc) return true;
	}
	return false;
}

void print_branches(NesParser *p) {
	size_t prg_len = p->rom.prg.count;
	for (size_t i = 0; i < prg_len; i++) {
		Addr a = i + 0x8000;
		if (!is_code(p, a)) continue;
		if (is_done(p, a)) continue;
		set_done(p, a);

		Instr ins = p->instrs.items[i];
		printf("\n===== Instruction set at 0x%04X =====\n\n", ins.loc);
		print_instr(ins);
		size_t offs = i+OP_DESCS[ins.op].size;
		Instr next = p->instrs.items[offs];
		while (is_parent(p, next, ins)) {
			if (is_done(p, next.loc)) {
				printf("0x%04X: ... (attaches to already printed valid branch)\n", next.loc);
				break;
			}
			set_done(p, next.loc);
			print_instr(next);
			ins = next;
			offs += OP_DESCS[ins.op].size;
			next = p->instrs.items[offs];
		}
	}
}

void print_instrs(NesParser *p) {
	size_t prg_len = p->rom.prg.count;
	for (size_t i = 0; i < prg_len; i++) {
		Addr a = i + 0x8000;
		if (!is_code(p, a)) continue;

		Instr ins = p->instrs.items[i];
		print_instr(ins);
	}
}

NesParser parse_file(const char *path) {
	NesParser p = create_parser(path);
	size_t prg_len = p.rom.prg.count;

	// First pass
	for (size_t i = 0; i < prg_len; i++) {
		Addr a = i + 0x8000;
		Op op = (Op) rom_read_at(&p.rom, a, sizeof(Byte));
		OpDesc op_desc = OP_DESCS[op];
		if (op_desc.size == 0) { // Illegal instruction
			// printf("Unkown instruction: 0x%X at 0x%X\n", op, a);
			continue;
		}
		set_code(&p, a);
		Instr ins = {0};
		for (size_t i = 0; i < op_desc.size; i++) {
			da_append(&ins.bytes, rom_read_at(&p.rom, a+i, sizeof(Byte)));
		}
		ins.op = op;
		ins.loc = a;
		p.instrs.items[i] = ins;
	}

	// Second pass
	for (size_t i = 0; i < prg_len; i++) {
		Addr a = i + 0x8000;
		if (!is_code(&p, a)) continue;

		Instr ins = p.instrs.items[i];
		set_parents(&p, ins);
	}

	// Third pass
	for (size_t i = 0; i < prg_len; i++) {
		Addr a = i + 0x8000;
		if (is_code(&p, a)) continue;
		illegal_branch(&p, a);
	}

	print_branches(&p);
	// print_instrs(&p);
	
	return p;
}

int main(void) {
	// NesParser p = create_parser(ROMS_DIR"SMB.nes");
	// while(parser_next_instr(&p)) {}
	parse_file(ROMS_DIR"SMB.nes");
	return 0;
} 

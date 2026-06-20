#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#define CUT_IMPLEMENTATION
#include "cut.h"
#include "nes.h"

#define ROMS_DIR "./roms/"

#define NES_PRG_BASE      0x8000
#define NES_PRG_END       0xFFFF
#define INES_TRAINER_SZ   512
#define INES_PRG_CHUNK_SZ (1 << 14)
#define INES_CHR_CHUNK_SZ (1 << 13)

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

typedef struct {
	Byte *bytes;
	OpKind op;
	Addr addr;
	size_t offs;
} Instr;

typedef struct {
	Instr *instrs;       // |
	Addr **parents;      // |- Parallel arrays
	bool *is_valid;      // |
	size_t *child_count; // |
	size_t len;
	uint8_t mapper;
} NesParser;

static inline Byte rom_peek8(const NesRom rom, size_t offset) {
    if (offset >= rom.prg_len) return 0;
    return rom.prg[offset];
}

/* Eventually, cpu reads will have to return a dynamic array
 * of all possible physical addresses' values, but mapper 0 only has
 * one possible physical address per cpu read location */
static inline Byte cpu_read8(const NesRom rom, Addr cpu_addr) {
    if (cpu_addr < NES_PRG_BASE) {
        return 0; 
    }
    // Mapper 0: NROM-128 (16KB) mirrors itself at $C000-$FFFF
	//           NROM-256 doesn't mirror PRG-ROM
    size_t offset = ((size_t) cpu_addr - NES_PRG_BASE) % rom.prg_len;
    return rom_peek8(rom, offset);
}

static inline Addr cpu_read16(const NesRom rom, Addr cpu_addr) {
    uint8_t low  = cpu_read8(rom, cpu_addr);
    uint8_t high = cpu_read8(rom, cpu_addr + 1);
    return (Addr)(low | (high << 8));
}

void print_hdr(NesHdr h) {
	printf("Magic: ");
	for (size_t i = 0; i < 4; ++i) {
		putchar(h.magic[i]);
	}
	putchar('\n');
	printf("PRG len: %u\n", h.prg_chunk_n);
	printf("CHR len: %u\n", h.chr_chunk_n);
	printf("flags 6: %u\n", h.flags6);
	printf("flags 7: %u\n", h.flags7);
	printf("flags 8: %u\n", h.flags8);
	printf("flags 9: %u\n", h.flags9);
	printf("flags 10: %u\n", h.flags10);
	putchar('\n');
}

NesRom load_rom(const char *path) {
	NesRom r = {0};

	r.f = mmap_file(path);
	r.hdr = file_advance(&r.f, sizeof(NesHdr));
	if (!r.hdr || memcmp(&r.hdr->magic, "NES\x1A", 4) != 0) {
		fprintf(stderr, "Invalid NES file!\n");
		exit(1);
	}
	if (trainer(r.hdr))
		r.trainer = file_advance(&r.f, INES_TRAINER_SZ);

	r.prg_len = r.hdr->prg_chunk_n * INES_PRG_CHUNK_SZ;
	r.prg = file_byte_arr(&r.f, r.prg_len);

	r.chr_len = r.hdr->chr_chunk_n * INES_CHR_CHUNK_SZ;
	r.chr = file_byte_arr(&r.f, r.chr_len);
	if (r.chr_len == 0) r.chr = NULL;

	return r;
}

void dump_prg(const NesRom *rom, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen");
        return;
    }
    fwrite(rom->prg, 1, rom->prg_len, f);
    fclose(f);
}

void dump_chr(const NesRom *rom, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen");
        return;
    }
    fwrite(rom->chr, 1, rom->chr_len, f);
    fclose(f);
}

static inline size_t addr_to_index(Addr rom_addr) {
	return rom_addr - NES_PRG_BASE;
}

void print_instr(Instr in) {
	printf("0x%04X: %10s: ", in.addr, OPS[in.op].name);
	for (size_t i = 0; i < arr_len(in.bytes); i++) {
		printf("0x%02X ", in.bytes[i]);
	}
	putchar('\n');
}

NesParser create_parser(NesRom r) {
	NesParser p = {0};
	p.is_valid    = calloc(r.prg_len, sizeof(bool));
	p.child_count = calloc(r.prg_len, sizeof(size_t));
	p.parents     = calloc(r.prg_len, sizeof(Addr *));
	p.instrs      = calloc(r.prg_len, sizeof(Instr));
	p.len = r.prg_len;
	p.mapper = (r.hdr->flags6 >> 4) | (r.hdr->flags7 & 0xF0);
	dump_prg(&r, "prg_rom_embed.bin");
	dump_chr(&r, "chr_rom_embed.bin");
	return p;
}

#define in_prg_range(p, a) \
	(((size_t) a >= NES_PRG_BASE) && ((size_t) a < NES_PRG_BASE + (p)->len))

/* 
 * Pass in a known illegal address, propagate back
 * "illegalness" to parents. If it was the only remaining path
 * for the parent, then it is also illegal. Otherwise,
 * mark it as having only a single child
 */
void illegal_branch(NesParser *p, Addr init_a) {
	Addr *stack = arr_new(Addr);
	arr_append(stack, init_a);
	Addr *a;
	while ((a = arr_pop(stack))) {
		Addr *parents = p->parents[addr_to_index(*a)];
		arr_foreach(parent, parents) {
			size_t parent_i = addr_to_index(*parent);
			if (!p->is_valid[parent_i]) continue;

			p->child_count[parent_i]--;
			if (p->child_count[parent_i]) continue;

			p->is_valid[parent_i] = false;
			arr_append(stack, *parent);
		}
	}
	arr_free(stack);
	return;	
}

void set_parents(NesParser *p, Instr ins) {
	switch (ins.op) {
		case JSR: case JMP_ABS: {
			Addr jump_addr = ins.bytes[1] | ins.bytes[2] << 8;
			if (in_prg_range(p, jump_addr)) {
				arr_append(p->parents[addr_to_index(jump_addr)], ins.addr);
				p->child_count[ins.offs] = 1;
			} else {
				p->is_valid[ins.offs] = false;
			}
		} return;
		case BCC: case BCS: case BEQ: 
		case BMI: case BNE: case BPL: 
		case BVC: case BVS: {
			int8_t offs = ins.bytes[1];
			if (in_prg_range(p, ins.addr + 2 + offs)) {
				arr_append(p->parents[ins.offs + 2 + offs], ins.addr);
				p->child_count[ins.offs] = 1;
			} 
		} break;
		// We don't know where it jumps, so no explicit child, can never be killed
		// because it can always potentially jump somewhere valid at runtime
		case JMP_IND: case BRK: case RTS: case RTI: return;
		default: break;
	}

	Op op_desc = OPS[ins.op];
	if (in_prg_range(p, ins.addr + op_desc.size)) {
		arr_append(p->parents[ins.offs + op_desc.size], ins.addr);
		p->child_count[ins.offs] += 1;
	} else if (p->child_count[ins.offs] == 0) {
		p->is_valid[ins.offs] = false;
	}
	return;
}

void print_branches(NesParser *p) {
	bool *done = calloc(p->len, sizeof(bool));
	for (size_t i = 0; i < p->len; i++) {
		if (!p->is_valid[i]) continue;
		if (done[i]) continue;
		done[i] = true;

		Instr ins = p->instrs[i];
		printf("\n===== Instruction set at 0x%04X =====\n\n", ins.addr);
		print_instr(ins);

		size_t offs = i+OPS[ins.op].size;
		Instr next = p->instrs[offs];
		while (p->is_valid[next.offs]) {
			if (done[next.offs]) {
				printf("0x%04X: ... (attaches to already printed valid branch)\n", next.addr);
				break;
			}
			done[next.offs] = true;
			print_instr(next);
			ins = next;
			offs += OPS[ins.op].size;
			next = p->instrs[offs];
		}
	}
	free(done);
}

void print_instrs(NesParser *p) {
	for (size_t i = 0; i < p->len; i++) {
		if (!p->is_valid[i]) continue;

		Instr ins = p->instrs[i];
		print_instr(ins);
	}
}

NesParser parse_file(const char *path) {
	NesRom r = load_rom(path);
	NesParser p = create_parser(r);

	// First pass - Find all instructions indescriminately
	for (size_t i = 0; i < p.len; i++) {
		OpKind op = (OpKind) rom_peek8(r, i);
		Op op_desc = OPS[op];
		if (op_desc.size == 0) { // Illegal instruction
			// printf("Unkown instruction: 0x%X at 0x%X\n", op, a);
			continue;
		}

		p.is_valid[i] = true;
		Instr ins = {0};
		for (size_t j = 0; j < op_desc.size; j++) {
			arr_append(ins.bytes, rom_peek8(r, i+j));
		}
		ins.op = op;
		ins.offs = i;
		ins.addr = i + NES_PRG_BASE;
		p.instrs[i] = ins;
	}

	// Second pass - Build branches
	for (size_t i = 0; i < p.len; i++) {
		if (!p.is_valid[i]) continue;

		Instr ins = p.instrs[i];
		set_parents(&p, ins);
	}

	// Third pass - Trim illegal branches from valid code
	for (size_t i = 0; i < p.len; i++) {
		Addr a = i + NES_PRG_BASE;
		if (p.is_valid[i]) continue;
		illegal_branch(&p, a);
	}

	// print_branches(&p);
	// print_instrs(&p);
	
	return p;
}

void print_instr_c(Instr ins) {
	Op op = OPS[ins.op];
	printf("\tcase 0x%04lX:\n", ins.offs);
	// puts("\t\tprintf(\"PC: %04X | A: %02X X: %02X Y: %02X SP: %02X\\n\", PC, A, X, Y, SP);");
	const char *kind = META_STR[op.meta_kind];
	const char *mode = MODE_STR[op.addr_mode];
	printf("\t\tTICK(%zu, %zu);\n", arr_len(ins.bytes), op.cycles);
	if (op.addr_mode == MODE_ACC) { // because accumulator mode appears in meta_ops that accept operands
		printf("\t\t%s(%s, 0x00);\n", kind, mode);
		return;
	}
	switch (arr_len(ins.bytes)) {
		case 1:
			printf("\t\t%s(%s);\n", kind, mode);
			break;
		case 2:
			printf("\t\t%s(%s, 0x%02X);\n", kind, mode, ins.bytes[1]);
			break;
		case 3:
			printf("\t\t%s(%s, 0x%04X);\n", kind, mode, ins.bytes[1] | ins.bytes[2] << 8);
			break;
	}
}

void parsed_to_c(NesParser *p) {
	bool *done = calloc(p->len, sizeof(bool));
	size_t *starts = calloc(p->len, sizeof(size_t));
	printf(
		"#define NESLIB_IMPLEMENTATION\n"
		"#include \"neslib.h\"\n"
		"uint8_t mapper = 0x%X;\n"
		"size_t prg_rom_len = %zu;\n"
		"const Byte prg_rom[] = {\n"
		"\t#embed \"prg_rom_embed.bin\"\n"
		"};\n"
		"const Byte chr_rom[] = {\n"
		"\t#embed \"chr_rom_embed.bin\"\n"
		"};\n"
		"\n",
		p->mapper,
		p->len
	);
	for (size_t i = 0; i < p->len; i++) {
		if (!p->is_valid[i]) continue;
		if (done[i]) continue;
		done[i] = true;

		Instr ins = p->instrs[i];
		printf("\n// ===== Instruction set at 0x%04X =====\n\n", ins.addr);
		printf("void f_0x%04lX(size_t offs) {\n", ins.offs);
		printf("\tswitch (offs) {\n");
		print_instr_c(ins);
		starts[i] = i;
		size_t offs = i+OPS[ins.op].size;
		Instr next = p->instrs[offs];
		while (p->is_valid[next.offs]) {
			if (done[next.offs]) {
				printf("\t\treturn;\n");
				printf("// 0x%04X: ... (attaches to already printed valid branch)\n", next.addr);
				break;
			}
			done[next.offs] = true;
			print_instr_c(next);
			// printf("\t\tPC = 0x%04X;\n", next.addr);
			starts[next.offs] = i;
			ins = next;
			offs += OPS[ins.op].size;
			next = p->instrs[offs];
		}
		puts("\t}\n}");
	}
	printf(
		"\n\nvoid (*const global_dispatch[%zu])(size_t) = {\n\t",
		p->len
	);
	for (size_t i = 0; i < p->len; i++) {
		if (!p->is_valid[i]) continue;
		printf("[0x%04lX] = f_0x%04lX,\n\t", i, starts[i]);
	}
	puts("\n};");

	free(done);
	free(starts);
}

int main(void) {
	// NesParser p = parse_file(ROMS_DIR"nestest.nes");
	NesParser p = parse_file(ROMS_DIR"SMB.nes");
	// NesParser p = parse_file(ROMS_DIR"nes-test-roms/instr_test-v5/rom_singles/01-basics.nes");
	parsed_to_c(&p);
	return 0;
} 

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#define CUT_IMPLEMENTATION
#include "cut.h"
#include "nes.h"

#define ROMS_DIR "./roms/"

#define NES_PRG_BASE      0x8000
#define NES_PRG_END       0xFFFF
#define VEC_RESET_LOC     0xFFFC
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
	Addr reset;
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
	p.reset = cpu_read16(r, VEC_RESET_LOC);
	dump_prg(&r, "test.rom");
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
			// if (ins.addr == 0x8067)  {
			// 	fprintf(stderr, "[0] 0x%02X\n", ins.bytes[0]);
			// 	fprintf(stderr, "[1] 0x%02X\n", ins.bytes[1]);
			// 	fprintf(stderr, "[2] 0x%02X\n", ins.bytes[2]);
			// 	fprintf(stderr, "JMP 0x%04X\n", jump_addr);
			// }
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
	printf("case 0x%04X:\n", ins.addr); // goto label
	switch (op.addr_mode) {
		case MODE_NONE:  break;
		case MODE_ACC:
			printf("    mem = mem_mode_acc();\n");
			break;
		case MODE_IMM:   
			printf("    *mem = 0x%02X;\n", ins.bytes[1]);
			break;
		case MODE_ZP:
			printf("    mem = mem_mode_zp(0x%02X);\n", ins.bytes[1]);
			break;
		case MODE_ZP_X:
			printf("    mem = mem_mode_zp_x(0x%02X);\n", ins.bytes[1]);
			break;
		case MODE_ZP_Y:
			printf("    mem = mem_mode_zp_y(0x%02X);\n", ins.bytes[1]);
			break;
		case MODE_ABS:
			printf("    mem = mem_mode_abs(0x%04X);\n", ins.bytes[1] | ins.bytes[2] << 8);
			break;
		case MODE_ABS_X:
			printf("    mem = mem_mode_abs_x(0x%04X);\n", ins.bytes[1] | ins.bytes[2] << 8);
			break;
		case MODE_ABS_Y:
			printf("    mem = mem_mode_abs_y(0x%04X);\n", ins.bytes[1] | ins.bytes[2] << 8);
			break;
		case MODE_IND:
			printf("    mem = mem_mode_ind(0x%04X);\n", ins.bytes[1] | ins.bytes[2] << 8);
			break;
		case MODE_IND_X:
			printf("    mem = mem_mode_ind_x(0x%02X);\n", ins.bytes[1]);
			break;
		case MODE_IND_Y:
			printf("    mem = mem_mode_ind_y(0x%02X);\n", ins.bytes[1]);
			break;
		case MODE_COUNT:
			fprintf(stderr, "Unreachable MODE_COUNT");
			exit(1);
			break;
	}
	printf("    ");
	switch (op.meta_kind) {
		// --- Arithmetic ---
		case META_ADC: printf("adc(*mem);\n"); break;
		case META_SBC: printf("sbc(*mem);\n"); break;
		case META_DEC: printf("dec(mem);\n"); break;
		case META_DEX: printf("dex();\n"); break;
		case META_DEY: printf("dey();\n"); break;
		case META_INC: printf("inc(mem);\n"); break;
		case META_INX: printf("inx();\n"); break;
		case META_INY: printf("iny();\n"); break;
		// --- Register stuff ---
		case META_LDA: printf("lda(*mem);\n"); break;
		case META_LDX: printf("ldx(*mem);\n"); break;
		case META_LDY: printf("ldy(*mem);\n"); break;
		case META_STA: printf("sta(mem);\n"); break;
		case META_STX: printf("stx(mem);\n"); break;
		case META_STY: printf("sty(mem);\n"); break;
		case META_TAX: printf("tax();\n"); break;
		case META_TAY: printf("tay();\n"); break;
		case META_TXA: printf("txa();\n"); break;
		case META_TYA: printf("tya();\n"); break;
		case META_TSX: printf("tsx();\n"); break;
		case META_TXS: printf("txs();\n"); break;
		case META_PHP: printf("php();\n"); break;
		case META_PLP: printf("plp();\n"); break;
		case META_PHA: printf("pha();\n"); break;
		case META_PLA: printf("pla();\n"); break;
		// --- Bitwise ---
		case META_AND: printf("and(*mem);\n"); break;
		case META_EOR: printf("eor(*mem);\n"); break;
		case META_ORA: printf("ora(*mem);\n"); break;
		case META_ASL: printf("asl(mem);\n"); break;
		case META_LSR: printf("lsr(mem);\n"); break;
		case META_ROL: printf("rol(mem);\n"); break;
		case META_ROR: printf("ror(mem);\n"); break;
		// --- Flag Setting ---
		case META_BIT: printf("bit(*mem);\n"); break;
		case META_CLC: printf("clc();\n"); break;
		case META_CLD: printf("cld();\n"); break;
		case META_CLI: printf("cli();\n"); break;
		case META_CLV: printf("clv();\n"); break;
		case META_SEC: printf("sec();\n"); break;
		case META_SED: printf("sed();\n"); break;
		case META_SEI: printf("sei();\n"); break;
		case META_CMP: printf("cmp(*mem);\n"); break;
		case META_CPX: printf("cpx(*mem);\n"); break;
		case META_CPY: printf("cpy(*mem);\n"); break;
		// --- Control flow ---
		case META_BCC: printf("bcc(*mem);\n"); goto jump_to_PC;
		case META_BCS: printf("bcs(*mem);\n"); goto jump_to_PC;
		case META_BNE: printf("bne(*mem);\n"); goto jump_to_PC;
		case META_BEQ: printf("beq(*mem);\n"); goto jump_to_PC;
		case META_BPL: printf("bpl(*mem);\n"); goto jump_to_PC;
		case META_BMI: printf("bmi(*mem);\n"); goto jump_to_PC;
		case META_BVC: printf("bvc(*mem);\n"); goto jump_to_PC;
		case META_BVS: printf("bvs(*mem);\n"); goto jump_to_PC;
		case META_BRK: printf("brk();\n");     goto jump_to_PC;
		case META_JMP: printf("jmp(*mem);\n"); goto jump_to_PC;
		case META_JSR: printf("jsr(*mem);\n"); goto jump_to_PC;
		case META_RTI: printf("rti();\n");     goto jump_to_PC;
		case META_RTS: printf("rts();\n");     goto jump_to_PC;
		jump_to_PC:
			printf("    goto jump_to_PC;\n");
			break;
		// --- Other ---
		case META_NOP: printf("nop();\n"); break;
	}
}

void parsed_to_c(NesParser *p) {
	bool *done = calloc(p->len, sizeof(bool));
	printf(
		"#include \"neslib.h\"\n"
		"\n"
		"int main(void) {\n"
		"    PC = 0x%04X;\n"
		"jump_to_PC:\n"
		"    switch(PC) {\n",
		p->reset
	);
	for (size_t i = 0; i < p->len; i++) {
		if (!p->is_valid[i]) continue;
		if (done[i]) continue;
		done[i] = true;

		Instr ins = p->instrs[i];
		printf("\n// ===== Instruction set at 0x%04X =====\n\n", ins.addr);
		print_instr_c(ins);
		size_t offs = i+OPS[ins.op].size;
		Instr next = p->instrs[offs];
		while (p->is_valid[next.offs]) {
			if (done[next.offs]) {
				printf("// 0x%04X: ... (attaches to already printed valid branch)\n", next.addr);
				break;
			}
			done[next.offs] = true;
			print_instr_c(next);
			ins = next;
			offs += OPS[ins.op].size;
			next = p->instrs[offs];
		}
	}

	printf(
		"default:\n"
		"    fprintf(stderr, \"FATAL: Tried to jump to a statically invalid address!\\n\");\n"
		"    }\n"
		"}\n"
		);
	free(done);
}

int main(void) {
	// NesParser p = parse_file(ROMS_DIR"nestest.nes");
	NesParser p = parse_file(ROMS_DIR"SMB.nes");
	// NesParser p = parse_file(ROMS_DIR"nes-test-roms/instr_test-v5/rom_singles/01-basics.nes");
	parsed_to_c(&p);
	return 0;
} 

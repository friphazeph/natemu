#include <stdio.h>
#include <stdbool.h>
#include "../cut.h"
#include "nesparser.h"

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

static inline size_t addr_to_index(Addr rom_addr) {
	return rom_addr - NES_PRG_BASE;
}

NesParser create_parser(NesRom r) {
	NesParser p = {0};
	p.is_valid    = calloc(r.prg_len, sizeof(bool));
	p.child_count = calloc(r.prg_len, sizeof(size_t));
	p.parents     = calloc(r.prg_len, sizeof(Addr *));
	p.instrs      = calloc(r.prg_len, sizeof(Instr));
	p.len = r.prg_len;
	p.mapper = (r.hdr->flags6 >> 4) | (r.hdr->flags7 & 0xF0);
	dump_prg(&r, "./build/prg_rom_embed.bin");
	dump_chr(&r, "./build/chr_rom_embed.bin");
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

void print_instr(Instr in) {
	printf("0x%04X: %10s: ", in.addr, OPS[in.op].name);
	for (size_t i = 0; i < arr_len(in.bytes); i++) {
		printf("0x%02X ", in.bytes[i]);
	}
	putchar('\n');
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

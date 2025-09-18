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
	OpKind op;
	Addr loc;
} Instr;

void print_instr(Instr in) {
	printf("0x%04X: %10s: ", in.loc, OPS[in.op].name);
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
	Addr entry_point;
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
	p.entry_point = rom_read_at(&p.rom, 0xFFFC, sizeof(Addr));
	fprintf(stderr, "Entry: 0x%X\n", p.entry_point);

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
	Op op_desc = OPS[ins.op];
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
		size_t offs = i+OPS[ins.op].size;
		Instr next = p->instrs.items[offs];
		while (is_parent(p, next, ins)) {
			if (is_done(p, next.loc)) {
				printf("0x%04X: ... (attaches to already printed valid branch)\n", next.loc);
				break;
			}
			set_done(p, next.loc);
			print_instr(next);
			ins = next;
			offs += OPS[ins.op].size;
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
		OpKind op = (OpKind) rom_read_at(&p.rom, a, sizeof(Byte));
		Op op_desc = OPS[op];
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

	// print_branches(&p);
	// print_instrs(&p);
	
	return p;
}

void instr_to_fasm(NesParser *p, Instr ins, Addr a) {
	static const char *A = "al";
	static const char *A16 = "ax";
	static const char *A32 = "eax";
	UNUSED(A32);
	static const char *X = "bl";
	static const char *X32 = "ebx";
	static const char *X64 = "rbx";
	static const char *Y = "dl";
	static const char *Y32 = "edx";

	Byte *ins_bytes = ins.bytes.items;
	printf("lbl_0x%X: ;; %s\n", a, OPS[ins.op].name);

	Op op = OPS[ins.op];
	char *memory = NULL;

	switch (op.addr_mode) {
		case MODE_NONE: {
			asprintf(&memory, " ");
		} break;
		case MODE_ACC: {
			asprintf(&memory, "%s", A);
		} break;
		case MODE_IMM: {
			asprintf(&memory, "0x%02X", ins_bytes[1]);
		} break;
		case MODE_ZP: {
			printf("    xor esi, esi\n");
			printf("    add esi, 0x%X\n", ins_bytes[1]);
			printf("    and esi, 0xFF\n");
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
		case MODE_ZP_X: {
			printf("    movzx esi, %s\n", X);
			printf("    add esi, 0x%X\n", ins_bytes[1]);
			printf("    and esi, 0xFF\n");
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
		case MODE_ZP_Y: {
			printf("    movzx esi, %s\n", Y);
			printf("    add esi, 0x%X\n", ins_bytes[1]);
			printf("    and esi, 0xFF\n");
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
		case MODE_ABS: {
			Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
			asprintf(&memory, "byte [ram_start+0x%X]", addr);
		} break;
		case MODE_ABS_X: {
			Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
			printf("    mov esi, 0x%X\n", addr);
			printf("    movzx %s, %s\n", X32, X);
			printf("    add esi, %s\n", X32);
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
		case MODE_ABS_Y: {
			Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
			printf("    mov esi, 0x%X\n", addr);
			printf("    movzx %s, %s\n", Y32, Y);
			printf("    add esi, %s\n", Y32);
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
		case MODE_IND: {
			Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
			asprintf(&memory, "byte [ram_start+0x%X]", addr);
		} break;
		case MODE_IND_X: {
			printf("    push %s\n", A16);
			// 1. Calculate the zero-page address with the X register.
			// The result is an 8-bit address that wraps around.
			printf("    movzx esi, %s\n", X);
			printf("    add esi, 0x%02X\n", ins_bytes[1]);
			printf("    and esi, 0xFF\n");

			// 2. Load the low byte of the final address from the calculated zero-page address.
			printf("    movzx esi, byte [ram_start + esi]\n");

			// 3. Load the high byte from the next zero-page address.
			printf("    movzx edi, %s\n", X);
			printf("    add edi, 0x%02X\n", ins_bytes[1]);
			printf("    inc edi\n");
			printf("    and edi, 0xFF\n");
			printf("    movzx eax, byte [ram_start + edi]\n");
			printf("    shl eax, 8\n");

			// 4. Combine the low and high bytes.
			printf("    or esi, eax\n");
			printf("    pop %s\n", A16);
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
		case MODE_IND_Y: {
			// 1. Load the 16-bit base address from the zero page.
			// The low byte is at address 'a', the high byte is at 'a+1'.
			printf("    movzx esi, byte [ram_start+0x%02X]\n", ins_bytes[1]);
			printf("    movzx edi, byte [ram_start+0x%02X + 1]\n", ins_bytes[1]);
			printf("    shl edi, 8\n");
			printf("    or esi, edi\n");

			// 2. Add the Y register's value to the base address.
			printf("    add esi, %s\n", Y32);
			asprintf(&memory, "byte [ram_start+esi]");
		} break;
	}

	// A -> al
	// X -> bl
	// Y -> dl
	// C -> CF
	// Z -> ZF
	// V -> OF
	// N -> SF
	// D -> ??
	// I -> ??

	switch (op.meta_kind) {
		case META_ADC: {
			printf("    adc %s, %s\n", A, memory);
		} break;
		case META_AND: {
			printf("    and %s, %s\n", A, memory);
		} break;
		case META_ASL: {
			printf("    shl %s, 1\n", memory);
		} break;
		case META_BIT: {
			printf("    push %s\n", A16);
			printf("    and %s, %s\n", A, memory);
			printf("    pop %s\n", A16);
		} break;
		case META_CMP: {
			printf("    cmp %s, %s\n", A, memory);
		} break;
		case META_CPX: {
			printf("    cmp %s, %s\n", X, memory);
		} break;
		case META_CPY: {
			printf("    cmp %s, %s\n", Y, memory);
		} break;
		case META_DEC: {
			printf("    dec %s\n", memory);
		} break;
		case META_EOR: {
			printf("    xor %s, %s\n", A, memory);
		} break;
		case META_INC: {
			printf("    inc %s\n", memory);
		} break;
		case META_JMP: {
			if (op.addr_mode == MODE_ABS) {
				Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
				if (in_prg_range(p, addr) && is_code(p, addr)) {
					printf("    jmp lbl_0x%X\n", addr);
				}
			} else if (op.addr_mode == MODE_IND) {
				Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
				if (in_prg_range(p, addr) && is_code(p, addr)) {
					printf("    jmp [ram_start+0x%X]\n", addr);
				}
			}
		} break;
		case META_LDA: {
			printf("    mov %s, %s\n", A, memory);
		} break;
		case META_LDX: {
			printf("    mov %s, %s\n", X, memory);
		} break;
		case META_LDY: {
			printf("    mov %s, %s\n", Y, memory);
		} break;
		case META_LSR: {
			printf("    shr %s, 1\n", memory);
		} break;
		case META_ORA: {
			printf("    or %s, %s\n", X, memory);
		} break;
		case META_ROL: {
			printf("    rcl %s, 1\n", memory);
		} break;
		case META_ROR: {
			printf("    rcr %s, 1\n", memory);
		} break;
		case META_SBC: {
			printf("    sbb %s, %s\n", A, memory);
		} break;
		case META_STA: {
			printf("    mov %s, %s\n", memory, A);
		} break;
		case META_STX: {
			printf("    mov %s, %s\n", memory, X);
		} break;
		case META_STY: {
			printf("    mov %s, %s\n", memory, Y);
		} break;
		case META_BCC: {
			int8_t offset = (int8_t) ins_bytes[1];
			Addr addr = a + 2 + offset;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    jnc lbl_0x%X\n", addr);
			}
		} break;
		case META_BCS: {
			int8_t offset = (int8_t) ins_bytes[1];
			Addr addr = a + 2 + offset;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    jc lbl_0x%X\n", addr);
			}
		} break;
		case META_BEQ: {
			int8_t offset = (int8_t) ins_bytes[1];
			Addr addr = a + 2 + offset;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    je lbl_0x%X\n", addr);
			}
		} break;
		case META_BMI: {
			int8_t offset = (int8_t) ins_bytes[1];
			Addr addr = a + 2 + offset;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    js lbl_0x%X\n", addr);
			}
		} break;
		case META_BNE: {
			int8_t offset = (int8_t) ins_bytes[1];
			Addr addr = a + 2 + offset;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    jnz lbl_0x%X\n", addr);
			}
		} break;
		case META_BPL: {
			int8_t offset = (int8_t) ins_bytes[1];
			Addr addr = a + 2 + offset;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    jns lbl_0x%X\n", addr);
			}
		} break;
		case META_BRK: {
			printf("    mov edi, 0\n");
			printf("    mov eax, 60\n"); // exit call
			printf("    syscall\n");
		} break;
		case META_BVC: {
			printf(";; TODO: define overflow flag\n");
		} break;
		case META_BVS: {
			TODO("BVS");
		} break;
		case META_CLC: {
			printf("    clc\n");
		} break;
		case META_CLD: {
			printf(";; TODO\n");
		} break;
		case META_CLI: {
			printf(";; TODO\n");
		} break;
		case META_CLV: {
			printf(";; TODO\n");
		} break;
		case META_DEX: {
			printf("    dec %s\n", X);
		} break;
		case META_DEY: {
			printf("    dec %s\n", Y);
		} break;
		case META_INX: {
			printf("    inc %s\n", X);
		} break;
		case META_INY: {
			printf("    inc %s\n", Y);
		} break;
		case META_JSR: {
			Addr addr = ins_bytes[1] | ins_bytes[2] << 8;
			if (in_prg_range(p, addr) && is_code(p, addr)) {
				printf("    call lbl_0x%X\n", addr);
			}
		} break;
		case META_NOP: {
		} break;
		case META_PHA: {
			printf("    push %s\n", A16);
		} break;
		case META_PHP: {
			printf("    pushf\n");
		} break;
		case META_PLA: {
			printf("    pop %s\n", A16);
		} break;
		case META_PLP: {
			printf("    popf\n");
		} break;
		case META_RTI: {
			printf("    iret\n");
		} break;
		case META_RTS: {
			printf("    ret\n");
		} break;
		case META_SEC: {
			printf("    stc\n");
		} break;
		case META_SED: {
			printf(";; TODO\n");
		} break;
		case META_SEI: {
			printf(";; TODO\n");
		} break;
		case META_TAX: {
			printf("    mov %s, %s\n", X, A);
		} break;
		case META_TAY: {
			printf("    mov %s, %s\n", Y, A);
		} break;
		case META_TSX: {
			printf("    mov %s, rsp\n", X64);
			printf("    sub %s, stack_bottom\n", X64);
			printf("    and %s, 0xFF\n", X64);
		} break;
		case META_TXA: {
			printf("    mov %s, %s\n", A, X);
		} break;
		case META_TXS: {
			printf("    movzx rsp, %s\n", X);
			printf("    add rsp, stack_bottom\n");
		} break;
		case META_TYA: {
			printf("    mov %s, %s\n", A, Y);
		} break;
	}
}

void parsed_to_fasm(NesParser *p) {
	size_t prg_len = p->rom.prg.count;

	printf("format ELF64 executable\n");
	printf("entry _setup\n\n");
	printf(
		"segment readable writeable\n"
		"ram_start:\n"
		"    rb 0x100\n"
		"stack_bottom:\n"
		"    rb 0x100\n"
		"stack_top:\n"
		"    rb 0x10000-0x200\n\n"
		"segment readable executable\n\n"
	);
	printf(
		"_setup:\n"
		"    mov rsp, stack_top\n"
	    "    jmp lbl_0x%X\n\n", p->entry_point
	);
	for (size_t i = 0; i < prg_len; i++) {
		Addr a = i + 0x8000;
		if (!is_code(p, a)) continue;
		if (is_done(p, a)) continue;
		set_done(p, a);

		Instr ins = p->instrs.items[i];
		printf("\n;; ===== Instruction set at 0x%04X =====\n\n", ins.loc);
		instr_to_fasm(p, ins, a);
		size_t offs = i+OPS[ins.op].size;
		Instr next = p->instrs.items[offs];
		while (is_parent(p, next, ins)) {
			if (is_done(p, next.loc)) {
				printf("    jmp lbl_0x%X\n", next.loc);
				printf("    ;; 0x%04X: ... (attaches to already printed valid branch)\n", next.loc);
				break;
			}
			set_done(p, next.loc);
			instr_to_fasm(p, next, next.loc);
			ins = next;
			offs += OPS[ins.op].size;
			next = p->instrs.items[offs];
		}
	}
	return;
}

int main(void) {
	// NesParser p = create_parser(ROMS_DIR"SMB.nes");
	// while(parser_next_instr(&p)) {}
	// NesParser p = parse_file(ROMS_DIR"SMB.nes");
	NesParser p = parse_file(ROMS_DIR"nes-test-roms/instr_test-v5/rom_singles/01-basics.nes");
	parsed_to_fasm(&p);
	return 0;
} 

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

// bit 0 (0b0001): done
// bit 1 (0b0010): code
// bit 2 (0b0100): branch
// bit 3 (0b1000): branch_done
typedef uint8_t AddrStatus;

typedef struct {
	AddrStatus *items;
	size_t count;
	size_t capacity;
} AddrStatuses;


typedef struct {
	Bytes bytes;
	Op op;
	Addrs parents;
} Instr;

typedef struct {
	Instr *items;
	size_t count;
	size_t capacity;
} Instrs;

typedef struct {
	NesRom rom;
	Addrs jumps;
	Addr cur;
	Instrs instrs;
	AddrStatuses statuses;
} NesParser;

NesParser create_parser(const char *path) {
	NesParser p = {0};
	p.rom = load_rom(path);
	size_t prg_len = p.rom.prg.count;
	p.statuses.items = calloc(prg_len, sizeof(AddrStatus));
	p.statuses.count = prg_len;
	p.instrs.items = calloc(prg_len, sizeof(Instr));
	p.instrs.count = prg_len;

	Addr start = (Addr) rom_read_at(&p.rom, 0xFFFC, sizeof(Addr));
	p.cur = start;

	Addr nmi = (Addr) rom_read_at(&p.rom, 0xFFFA, sizeof(Addr));
	da_append(&p.jumps, nmi);

	// Addr interrupt = (Addr) rom_read_at(&p.rom, 0xFFFE, sizeof(Addr));
	// da_append(&p.jumps, interrupt);
	return p;
}

static inline bool is_done(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	return p->statuses.items[addr-0x8000] & 0b1;
}

static inline void mark_done(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	p->statuses.items[addr-0x8000] |= 0b1;
	return;
}

static inline bool is_code(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	return p->statuses.items[addr-0x8000] & 0b10;
}

static inline void toggle_code(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	p->statuses.items[addr-0x8000] ^= 0b10;
	return;
}

static inline bool is_branch(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	return p->statuses.items[addr-0x8000] & 0b100;
}

static inline void toggle_branch(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	p->statuses.items[addr-0x8000] ^= 0b100;
	return;
}

static inline bool is_branch_done(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	return p->statuses.items[addr-0x8000] & 0b1000;
}

static inline void mark_branch_done(NesParser *p, Addr addr) {
	assert(addr >= 0x8000);
	assert(addr <  0x8000 + p->statuses.count);
	p->statuses.items[addr-0x8000] |= 0b1000;
	return;
}

void add_jump(NesParser *p, Addr jump) {
	if (jump < 0x8000 ||
		jump >=  0x8000 + p->statuses.count){
		return;
	}
	if (!is_done(p, jump)) {
		da_append(&p->jumps, jump);
	}
}

// Calling this with p, p->cur_jump will automatically find the next to-do jump
bool jump_to(NesParser *p, Addr jump) {
	while (is_done(p, jump)) {
		if (p->jumps.count <= 0) {
			return false;
		}
		jump = da_last(&p->jumps);
		p->jumps.count--;
	}
	// printf("0x%X done, jumping to 0x%X\n", p->cur_jump, jump);
	p->cur = jump;
	return true;
}

void illegal_branch(NesParser *p, Addr a) {
	Addrs parents = p->instrs.items[a - 0x8000].parents;
	for (size_t i = 0; i < parents.count; i++) {
		Addr parent = parents.items[i];
		if (!is_code(p, parent)) continue;
		if (is_branch(p, parent)) {
			if (is_branch_done(p, parent)) {
				toggle_code(p, parent);
				illegal_branch(p, parent);
			} else {
				mark_branch_done(p, parent);
			}
			continue;
		}
		toggle_code(p, parent);
		illegal_branch(p, parent);
	}
	return;	
}

void to_child(NesParser *p, Addr a) {
	da_append(&p->instrs.items[p->cur - 0x8000].parents, a);
	p->cur = a;
}

bool parse_op(NesParser *p, Op op) {
	Instr ins = {0};
	OpDesc op_desc = OP_DESCS[op];
	if (op_desc.size == 0) { // Illegal instruction
		printf("Unkown instruction: 0x%X at 0x%X\n", op, p->cur);
		illegal_branch(p, p->cur);
		return true;
	}
	toggle_code(p, p->cur);
	printf("%X: %s\n    ", p->cur, op_desc.name);
	for (size_t i = 0; i < op_desc.size; i++) {
		da_append(&ins.bytes, rom_read_at(&p->rom, p->cur+i, sizeof(Byte)));
		printf("%X ", ins.bytes.items[i]);
	}
	putchar('\n');

	switch (op) {
		case JMP_ABS: {
			Addr jump_addr = ins.bytes.items[1] | ins.bytes.items[2] << 8;
			// if (!jump_to(p, jump_addr)) {
			// 	return false;
			// }
			to_child(p, jump_addr);
			p->instrs.items[p->cur-0x8000] = ins;
			return true;
		} break;
		case JMP_IND: {
			if (!jump_to(p, p->cur)) { // this will find the next to-do jump
				return false;
			}
			p->instrs.items[p->cur-0x8000] = ins;
			return true;
		} break;
		case JSR: {
			toggle_branch(p, p->cur);
			Addr jump_addr = ins.bytes.items[1] | ins.bytes.items[2] << 8;
			add_jump(p, jump_addr);
		} break;
		case BCC: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BCS: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BEQ: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BMI: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BNE: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BPL: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BVC: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BVS: {
			toggle_branch(p, p->cur);
			int8_t offs = ins.bytes.items[1];
			add_jump(p, p->cur + 2 + offs);
		} break;
		case BRK: {
			if (!jump_to(p, p->cur)) { // this will find the next to-do jump
				return false;
			}
			p->instrs.items[p->cur-0x8000] = ins;
			return true;
		} break;
		case RTS: {
			if (!jump_to(p, p->cur)) { // this will find the next to-do jump
				return false;
			}
			p->instrs.items[p->cur-0x8000] = ins;
			return true;
		} break;
		case RTI: {
			if (!jump_to(p, p->cur)) { // this will find the next to-do jump
				return false;
			}
			p->instrs.items[p->cur-0x8000] = ins;
			return true;
		} break;
        case ADC_IMM   : break;
        case ADC_ZP    : break;
        case ADC_ZP_X  : break;
        case ADC_ABS   : break;
        case ADC_ABS_X : break;
        case ADC_ABS_Y : break;
        case ADC_IND_X : break;
        case ADC_IND_Y : break;

        // AND
        case AND_IMM   : break;
        case AND_ZP    : break;
        case AND_ZP_X  : break;
        case AND_ABS   : break;
        case AND_ABS_X : break;
        case AND_ABS_Y : break;
        case AND_IND_X : break;
        case AND_IND_Y : break;

        // ASL
        case ASL_ACC   : break;
        case ASL_ZP    : break;
        case ASL_ZP_X  : break;
        case ASL_ABS   : break;
        case ASL_ABS_X : break;

        // BIT
        case BIT_ZP    : break;
        case BIT_ABS   : break;

        case CLC       : break;
        case CLD       : break;
        case CLI       : break;
        case CLV       : break;

        // CMP
        case CMP_IMM   : break;
        case CMP_ZP    : break;
        case CMP_ZP_X  : break;
        case CMP_ABS   : break;
        case CMP_ABS_X : break;
        case CMP_ABS_Y : break;
        case CMP_IND_X : break;
        case CMP_IND_Y : break;

        // CPX
        case CPX_IMM   : break;
        case CPX_ZP    : break;
        case CPX_ABS   : break;

        // CPY
        case CPY_IMM   : break;
        case CPY_ZP    : break;
        case CPY_ABS   : break;

        // DEC
        case DEC_ZP    : break;
        case DEC_ZP_X  : break;
        case DEC_ABS   : break;
        case DEC_ABS_X : break;

        case DEX       : break;
        case DEY       : break;

        // EOR
        case EOR_IMM   : break;
        case EOR_ZP    : break;
        case EOR_ZP_X  : break;
        case EOR_ABS   : break;
        case EOR_ABS_X : break;
        case EOR_ABS_Y : break;
        case EOR_IND_X : break;
        case EOR_IND_Y : break;

        // INC
        case INC_ZP    : break;
        case INC_ZP_X  : break;
        case INC_ABS   : break;
        case INC_ABS_X : break;

        case INX       : break;
        case INY       : break;

        // LDA
        case LDA_IMM   : break;
        case LDA_ZP    : break;
        case LDA_ZP_X  : break;
        case LDA_ABS   : break;
        case LDA_ABS_X : break;
        case LDA_ABS_Y : break;
        case LDA_IND_X : break;
        case LDA_IND_Y : break;

        // LDX
        case LDX_IMM   : break;
        case LDX_ZP    : break;
        case LDX_ZP_Y  : break;
        case LDX_ABS   : break;
        case LDX_ABS_Y : break;

        // LDY
        case LDY_IMM   : break;
        case LDY_ZP    : break;
        case LDY_ZP_X  : break;
        case LDY_ABS   : break;
        case LDY_ABS_X : break;

        // LSR
        case LSR_ACC   : break;
        case LSR_ZP    : break;
        case LSR_ZP_X  : break;
        case LSR_ABS   : break;
        case LSR_ABS_X : break;

        case NOP       : break;

        // ORA
        case ORA_IMM   : break;
        case ORA_ZP    : break;
        case ORA_ZP_X  : break;
        case ORA_ABS   : break;
        case ORA_ABS_X : break;
        case ORA_ABS_Y : break;
        case ORA_IND_X : break;
        case ORA_IND_Y : break;

        case PHA       : break;
        case PHP       : break;
        case PLA       : break;
        case PLP       : break;

        // ROL
        case ROL_ACC   : break;
        case ROL_ZP    : break;
        case ROL_ZP_X  : break;
        case ROL_ABS   : break;
        case ROL_ABS_X : break;

        // ROR
        case ROR_ACC   : break;
        case ROR_ZP    : break;
        case ROR_ZP_X  : break;
        case ROR_ABS   : break;
        case ROR_ABS_X : break;

        // SBC
        case SBC_IMM   : break;
        case SBC_ZP    : break;
        case SBC_ZP_X  : break;
        case SBC_ABS   : break;
        case SBC_ABS_X : break;
        case SBC_ABS_Y : break;
        case SBC_IND_X : break;
        case SBC_IND_Y : break;

        case SEC       : break;
        case SED       : break;
        case SEI       : break;

        // STA
        case STA_ZP    : break;
        case STA_ZP_X  : break;
        case STA_ABS   : break;
        case STA_ABS_X : break;
        case STA_ABS_Y : break;
        case STA_IND_X : break;
        case STA_IND_Y : break;

        // STX
        case STX_ZP    : break;
        case STX_ZP_Y  : break;
        case STX_ABS   : break;

        // STY
        case STY_ZP    : break;
        case STY_ZP_Y  : break;
        case STY_ABS   : break;

        case TAX       : break;
        case TAY       : break;
        case TSX       : break;
        case TXA       : break;
        case TXS       : break;
        case TYA       : break;
	}
	to_child(p, p->cur + op_desc.size);
	p->instrs.items[p->cur-0x8000] = ins;
	return true;
}

bool jump_next_not_done(NesParser *p) {
	for (size_t i = 0; i < p->rom.prg.count; i++) {
		Addr a = i + 0x8000;
		if (!is_done(p, a)) {
			p->cur = a;
			return true;
		}
	}
	return false;
}

bool parser_next_instr(NesParser *p) {
	if (is_done(p, p->cur)) {
		if (!jump_to(p, p->cur)) {
			if (!jump_next_not_done(p)) {
				return false;
			}
		}
	}
	mark_done(p, p->cur);
	Op op = (Op) rom_read_at(&p->rom, p->cur, sizeof(Byte));
	return parse_op(p, op);
}

int main(void) {
	NesParser p = create_parser(ROMS_DIR"SMB.nes");
	while(parser_next_instr(&p)) {}
	return 0;
} 

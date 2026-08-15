#define CUT_IMPLEMENTATION
#include "../cut.h"
#include "nesparser.h"

#define ROMS_DIR "./roms/"

void print_instr_c(Instr ins) {
	Op op = OPS[ins.op];
	printf("\tcase 0x%04lX:\n", ins.offs);
	const char *kind = META_STR[op.meta_kind];
	const char *mode = MODE_STR[op.addr_mode];
	printf("\t\tTICK(%zu, %zu);\n", op.size, base_cycles(op));
	switch (op.size) {
		case 1:
			printf("\t\t%s(%s, 0x00);\n", kind, mode);
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
		"#include \"cpu.h\"\n"
		"#include \"commons.h\"\n"
		"uint8_t mapper = 0x%X;\n"
		"const size_t prg_rom_len = %zu;\n"
		"const Byte prg_rom[] = {\n"
		"\t#embed \"prg_rom_embed.bin\"\n"
		"};\n"
		"const Byte chr_rom[] = {\n"
		"\t#embed \"chr_rom_embed.bin\"\n"
		"};\n"
		"const size_t chr_rom_len = sizeof(chr_rom);\n"
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
	// NesParser p = parse_file(ROMS_DIR"nes-test-roms/instr_test-v5/rom_singles/16-special.nes");
	parsed_to_c(&p);
	return 0;
} 

#include <stdio.h>
#include "../cut.h"
#include "nesrom.h"

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

void dump_prg(const NesRom *rom, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
		fprintf(stderr, "%s:", path);
        perror("fopen");
        return;
    }
    fwrite(rom->prg, 1, rom->prg_len, f);
    fclose(f);
}

void dump_chr(const NesRom *rom, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
		fprintf(stderr, "%s:", path);
        perror("fopen");
        return;
    }
    fwrite(rom->chr, 1, rom->chr_len, f);
    fclose(f);
}

NesRom load_rom(const char *path) {
	NesRom r = {0};

	r.f = mmap_file(path);
	if (!r.f.base) {
		fprintf(stderr, "Could not open file '%s'.\n", path);
		exit(1);
	}
	r.hdr = file_advance(&r.f, sizeof(NesHdr));
	if (!r.hdr || memcmp(&r.hdr->magic, "NES\x1A", 4) != 0) {
		fprintf(stderr, "Invalid NES file (magic: %s)!\n", (char *) &r.hdr->magic);
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

#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "../cut.h"

size_t addr_to_prg_rom(Addr addr) {
	switch (mapper) {
		case 0:
			if (addr < 0x8000) {
				// fprintf(stderr, "Tried to access prg_rom outside of range ! (0x%04X)\n",
				// 		addr);
				// exit(1);
				return -1;
			}
			return (addr - 0x8000) % prg_rom_len;
		default:
			TODO("Only mapper 0 is implemented");
	}
}

void cpu_write(Addr addr, Byte value) {
	if (addr < 0x2000) {
		ram[addr & 0x07FF] = value;
	} else if (addr < 0x4000) {
		ppu_write_reg(addr, value);
	} else if (addr < 0x4018) {
		if (addr == 0x4014) {
			uint16_t page = value << 8;
			for (int i = 0; i < 256; i++) {
				PPU.oam[i] = cpu_read(page + i);
			}
			PPU.oam_changed = true;

			cycle_budget -= 513;
			total_cpu_cycles += 513;
			cpu_cycles_line_start += 513;
		}

		APU_IO_reg[addr - 0x4000] = value;
	} else if (addr < 0x401F) {
		fprintf(stderr, "Tried to write to test-mode memory ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr < 0x6000) {
		fprintf(stderr, "Tried to write to ROM ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr < 0x8000) {
		if (addr == 0x6000) {
			// 0x80 means "running/resetting", so we only care if it's 
			// a final result (0x00 for pass, 0x01+ for fail)
			if (value != 0x80) {
				printf("\n--- BLARGG TEST FINISHED ---\n");
				if (value == 0x00) {
					printf("Result: PASSED!\n");
				} else {
					printf("Result: FAILED (Error Code: 0x%02X)\n", value);
				}

				// Print the text buffer from 0x6004
				printf("Console Output:\n%s\n", (char*)&prg_ram[0x4]);
			}
		}
		prg_ram[addr-0x6000] = value;
	} else {
		fprintf(stderr, "Tried to write to ROM ! (0x%04X)\n", addr);
		exit(1);
	}
	return;
}

Byte cpu_read(Addr addr) {
	if (addr < 0x2000) {
		return ram[addr & 0x07FF];
	} else if (addr < 0x4000) {
		return ppu_read_reg(addr);
	} else if (addr < 0x4018) {
		if (addr == 0x4016) {
			Byte ret = controller_buffer & 1;
			controller_buffer >>= 1;
			return ret;
		}
		return APU_IO_reg[addr - 0x4000];
	} else if (addr < 0x401F) {
		fprintf(stderr, "Tried to access test-mode memory ! (0x%04X)\n", addr);
		exit(1);
	} else if (addr < 0x6000) {
		size_t i = addr_to_prg_rom(addr);
		return prg_rom[i];
	} else if (addr < 0x8000) {
		return prg_ram[addr-0x6000];
	} else {
		// handle mappers after
		size_t i = addr_to_prg_rom(addr);
		return prg_rom[i];
	}
}

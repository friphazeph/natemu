#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "../cut.h"

Byte ppu_read_reg(Addr addr) {
	ppu_catch_up();
	uint8_t ret;
	switch (addr & 0x7) {
		case 2: // PPU STATUS
			ret = (PPU.vblank << 7) 
				| (PPU.sprite0hit << 6) 
				| (PPU.sprite_overflow << 5);
			PPU.vblank = false;
			PPU.w = false;
			// if (PPU.vblank)
			// 	printf("vblank !\n");
			return ret;
			break;
		case 4: // OAM DATA
			return PPU.oam[PPU.oam_addr];
		case 7: // PPU DATA
			ret = PPU.buffer;
			PPU.buffer = ppu_read(PPU.v & 0x7FFF);
			PPU.v += PPU.increment;
			return ret;
	}
	return 0;
}

inline void ppu_write_reg(Addr addr, Byte value) {
	switch (addr & 0x7) {
		case 0: // PPU CTRL
			bool nmi_old = PPU.nmi_enable;
			PPU.base_nt = value & 0x03;
			PPU.increment = (value & 0x04) ? 32 : 1;
			PPU.sprite_table_addr = (value & 0x08) ? 0x1000 : 0x0;
			PPU.bg_table_addr = (value & 0x10) ? 0x1000 : 0x0;
			PPU.sprite_height = (value & 0x20) ? 16 : 8;
			PPU.nmi_enable = value >> 7;

			if (!nmi_old && PPU.nmi_enable) {
				if (PPU.vblank && !PPU.replaying) {
					trigger_nmi();
				}
			}
			break;
		case 1: // PPU MASK
			PPU.bg_enable = (value & 0x10) != 0;
			PPU.sprite_enable = (value & 0x08) != 0;
			break;
		case 3: // PPU OAM ADDR
			PPU.oam_addr = value;
			break;
		case 4:
			PPU.oam[PPU.oam_addr++] = value;
			break;
		case 5: // PPU SCROLL
			if (PPU.w) {
				// printf("Scroll Y = 0x%02X\n", value);
				PPU.y_scroll = value;
				PPU.w = false;
			} else {
				// printf("Scroll X = 0x%02X\n", value);
				PPU.x_scroll = value;
				PPU.w = true;
			}
			break;
		case 6: // TODO: transfer to PPU.t before on low byte write or something
			if (!PPU.w) {
				// First write: high byte
				PPU.t = (PPU.t & 0x00FF) | ((value & 0x3F) << 8);
				PPU.w = true;
			} else {
				// Second write: low byte
				PPU.t = (PPU.t & 0xFF00) | value;
				PPU.v = PPU.t; // Current VRAM pointer updates to match temporary pointer
				PPU.w = false;

				PPU.base_nt = (PPU.v >> 10) & 3; 
			}
			break;
		case 7: // PPU DATA
			ppu_write(PPU.v & 0x7FFF, value);
			PPU.v += PPU.increment;
			break;
	}
}

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
		PPU_cmd c = (PPU_cmd) {.at = total_cpu_cycles, .addr = addr, .value = value};
		arr_append(PPU.queue, c);
	} else if (addr < 0x4018) {
		if (addr == 0x4014) {
			ppu_catch_up();
			uint16_t page = value << 8;
			for (int i = 0; i < 256; i++) {
				PPU.oam[i] = cpu_read(page + i);
			}

			cycle_budget -= 513;
			total_cpu_cycles += 513;
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

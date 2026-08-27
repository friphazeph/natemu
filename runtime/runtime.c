#include <stdio.h>
#include <stdlib.h>
#include "../nes.h"
#define CUT_IMPLEMENTATION
#include "../cut.h"
#define COMMONS_IMPLEMENTATION
#include "commons.h"
#include "runtime.h"
#include "cpu.h"
#include "ppu.h"
#include "bus.h"

Color screen_buffer[PIXELS_H * PIXELS_W];

void run_for(size_t budget) {
	cycle_budget += budget;

	while(cycle_budget > 0) {
		size_t offs = addr_to_prg_rom(PC);
		if (offs == (size_t) -1) {
			interpret_pc();
			continue;
		}

		void (*branch)(size_t) = global_dispatch[offs];
		if (!branch) {
			interpret_pc();
		} else {
			branch(offs);
		}
	}
}

static inline void run_line(unsigned int line_n) {
	current_line = line_n;
    cpu_cycles_line_start = total_cpu_cycles;
    PPU.last_v_record_dot = 0;
    int nth = line_n % 3;
    int cycles = DOTS_PER_SCANLINE/3 + (nth != 0);

	if (PPU.odd_frame) cycles--;
	
	if (PPU.sprite0hit_this_line) {
		PPU.sprite0hit_this_line = false;
		run_for(PPU.sprite0hit_dot / 3);
		PPU.sprite0hit = true;
		run_for(cycles - PPU.sprite0hit_dot / 3);
	} else {
		run_for(cycles);
	}
	
	populate_shader_textures(line_n);
}

Texture2D run_frame(void) {
	PPU.vblank = false;
	PPU.sprite0hit = false;
	PPU.sprite_overflow = false;
	PPU.last_v_record_dot = 0;
	PPU.odd_frame = !PPU.odd_frame;
	populate_shader_textures(PRERENDER_LINE); 
	for (size_t i = 0; i < SCANLINES_PER_FRAME; i++) {
		if (i == 241) {
			PPU.vblank = true;
			if (PPU.nmi_enable) {
				trigger_nmi();
			}
		}
		run_line(i);
	}

	return ppu_get_texture();
}

void nes_init(void) {
	PC = cpu_read(0xFFFC) & 0xFF;
	PC |= cpu_read(0xFFFD) << 8;
	total_cpu_cycles = 7;
	SP = 0xFD;
}

void interpret_pc(void) {
	OpKind ins = cpu_read(PC);
	Op op = OPS[ins];
	uint16_t operands = 0;
	switch (op.size) {
		case 0:
			fprintf(stderr, "Tried to execute non-instruction (0x%02X) in ram !\n", ins);
			exit(1);
			return;
		case 1:
			break;
		case 2:
			operands = cpu_read(PC+1);
			break;
		case 3:
			operands = cpu_read(PC+1);
			operands |= cpu_read(PC+2) << 8;
			break;
	}
	TICK(op.size, base_cycles(op));
	switch ((Byte) ins) {
		case ADC_IMM:
			ADC(MODE_IMM, operands);
			break;
		case ADC_ZP:
			ADC(MODE_ZP, operands);
			break;
		case ADC_ZP_X:
			ADC(MODE_ZP_X, operands);
			break;
		case ADC_ABS:
			ADC(MODE_ABS, operands);
			break;
		case ADC_ABS_X:
			ADC(MODE_ABS_X, operands);
			break;
		case ADC_ABS_Y:
			ADC(MODE_ABS_Y, operands);
			break;
		case ADC_IND_X:
			ADC(MODE_IND_X, operands);
			break;
		case ADC_IND_Y:
			ADC(MODE_IND_Y, operands);
			break;
		case AND_IMM:
			AND(MODE_IMM, operands);
			break;
		case AND_ZP:
			AND(MODE_ZP, operands);
			break;
		case AND_ZP_X:
			AND(MODE_ZP_X, operands);
			break;
		case AND_ABS:
			AND(MODE_ABS, operands);
			break;
		case AND_ABS_X:
			AND(MODE_ABS_X, operands);
			break;
		case AND_ABS_Y:
			AND(MODE_ABS_Y, operands);
			break;
		case AND_IND_X:
			AND(MODE_IND_X, operands);
			break;
		case AND_IND_Y:
			AND(MODE_IND_Y, operands);
			break;
		case ASL_ACC:
			ASL(MODE_ACC, operands);
			break;
		case ASL_ZP:
			ASL(MODE_ZP, operands);
			break;
		case ASL_ZP_X:
			ASL(MODE_ZP_X, operands);
			break;
		case ASL_ABS:
			ASL(MODE_ABS, operands);
			break;
		case ASL_ABS_X:
			ASL(MODE_ABS_X, operands);
			break;
		case BCC:
            BCC(MODE_IMM, operands); break;
		case BCS:
            BCS(MODE_IMM, operands);
            break;
		case BEQ:
            BEQ(MODE_IMM, operands);
            break;
		case BIT_ZP:
			BIT(MODE_ZP, operands);
			break;
		case BIT_ABS:
			BIT(MODE_ABS, operands);
			break;
		case BMI:
            BMI(MODE_IMM, operands);
            break;
		case BNE:
            BNE(MODE_IMM, operands);
            break;
		case BPL:
            BPL(MODE_IMM, operands);
            break;
		case BVC:
            BVC(MODE_IMM, operands);
            break;
		case BVS:
            BVS(MODE_IMM, operands);
            break;
		case BRK:
            BRK(MODE_NONE, operands);
            break;
		case CLC:
            CLC(MODE_NONE, operands);
            break;
		case CLD:
            CLD(MODE_NONE, operands);
            break;
		case CLI:
            CLI(MODE_NONE, operands);
            break;
		case CLV:
            CLV(MODE_NONE, operands);
            break;
		case CMP_IMM:
			CMP(MODE_IMM, operands);
			break;
		case CMP_ZP:
			CMP(MODE_ZP, operands);
			break;
		case CMP_ZP_X:
			CMP(MODE_ZP_X, operands);
			break;
		case CMP_ABS:
			CMP(MODE_ABS, operands);
			break;
		case CMP_ABS_X:
			CMP(MODE_ABS_X, operands);
			break;
		case CMP_ABS_Y:
			CMP(MODE_ABS_Y, operands);
			break;
		case CMP_IND_X:
			CMP(MODE_IND_X, operands);
			break;
		case CMP_IND_Y:
			CMP(MODE_IND_Y, operands);
			break;
		case CPX_IMM:
			CPX(MODE_IMM, operands);
			break;
		case CPX_ZP:
			CPX(MODE_ZP, operands);
			break;
		case CPX_ABS:
			CPX(MODE_ABS, operands);
			break;
		case CPY_IMM:
			CPY(MODE_IMM, operands);
			break;
		case CPY_ZP:
			CPY(MODE_ZP, operands);
			break;
		case CPY_ABS:
			CPY(MODE_ABS, operands);
			break;
		case DEC_ZP:
			DEC(MODE_ZP, operands);
			break;
		case DEC_ZP_X:
			DEC(MODE_ZP_X, operands);
			break;
		case DEC_ABS:
			DEC(MODE_ABS, operands);
			break;
		case DEC_ABS_X:
			DEC(MODE_ABS_X, operands);
			break;
		case DEX:
            DEX(MODE_NONE, operands);
            break;
		case DEY:
            DEY(MODE_NONE, operands);
            break;
		case EOR_IMM:
			EOR(MODE_IMM, operands);
			break;
		case EOR_ZP:
			EOR(MODE_ZP, operands);
			break;
		case EOR_ZP_X:
			EOR(MODE_ZP_X, operands);
			break;
		case EOR_ABS:
			EOR(MODE_ABS, operands);
			break;
		case EOR_ABS_X:
			EOR(MODE_ABS_X, operands);
			break;
		case EOR_ABS_Y:
			EOR(MODE_ABS_Y, operands);
			break;
		case EOR_IND_X:
			EOR(MODE_IND_X, operands);
			break;
		case EOR_IND_Y:
			EOR(MODE_IND_Y, operands);
			break;
		case INC_ZP:
			INC(MODE_ZP, operands);
			break;
		case INC_ZP_X:
			INC(MODE_ZP_X, operands);
			break;
		case INC_ABS:
			INC(MODE_ABS, operands);
			break;
		case INC_ABS_X:
			INC(MODE_ABS_X, operands);
			break;
		case INX:
            INX(MODE_NONE, operands);
            break;
		case INY:
            INY(MODE_NONE, operands);
            break;
		case JMP_ABS:
			JMP(MODE_IMM2, operands);
			break;
		case JMP_IND:
			JMP(MODE_IND, operands);
			break;
		case JSR:
            JSR(MODE_IMM2, operands);
            break;
		case LDA_IMM:
			LDA(MODE_IMM, operands);
			break;
		case LDA_ZP:
			LDA(MODE_ZP, operands);
			break;
		case LDA_ZP_X:
			LDA(MODE_ZP_X, operands);
			break;
		case LDA_ABS:
			LDA(MODE_ABS, operands);
			break;
		case LDA_ABS_X:
			LDA(MODE_ABS_X, operands);
			break;
		case LDA_ABS_Y:
			LDA(MODE_ABS_Y, operands);
			break;
		case LDA_IND_X:
			LDA(MODE_IND_X, operands);
			break;
		case LDA_IND_Y:
			LDA(MODE_IND_Y, operands);
			break;
		case LDX_IMM:
			LDX(MODE_IMM, operands);
			break;
		case LDX_ZP:
			LDX(MODE_ZP, operands);
			break;
		case LDX_ZP_Y:
			LDX(MODE_ZP_Y, operands);
			break;
		case LDX_ABS:
			LDX(MODE_ABS, operands);
			break;
		case LDX_ABS_Y:
			LDX(MODE_ABS_Y, operands);
			break;
		case LDY_IMM:
			LDY(MODE_IMM, operands);
			break;
		case LDY_ZP:
			LDY(MODE_ZP, operands);
			break;
		case LDY_ZP_X:
			LDY(MODE_ZP_X, operands);
			break;
		case LDY_ABS:
			LDY(MODE_ABS, operands);
			break;
		case LDY_ABS_X:
			LDY(MODE_ABS_X, operands);
			break;
		case LSR_ACC:
			LSR(MODE_ACC, operands);
			break;
		case LSR_ZP:
			LSR(MODE_ZP, operands);
			break;
		case LSR_ZP_X:
			LSR(MODE_ZP_X, operands);
			break;
		case LSR_ABS:
			LSR(MODE_ABS, operands);
			break;
		case LSR_ABS_X:
			LSR(MODE_ABS_X, operands);
			break;
		case 0x1A: case 0x3A: case 0x5A:
		case 0x7A: case 0xDA: case 0xFA:
		case NOP:
            NOP(MODE_NONE, operands);
            break;
		case 0x04: case 0x44: case 0x64:
            NOP(MODE_ZP, operands);
            break;
		case 0x14: case 0x34: case 0x54:
		case 0x74: case 0xD4: case 0xF4:
            NOP(MODE_ZP_X, operands);
            break;
		case 0x0C:
            NOP(MODE_ABS, operands);
            break;
		case 0x1C: case 0x3C: case 0x5C:
		case 0x7C: case 0xDC: case 0xFC:
            NOP(MODE_ABS_X, operands);
            break;
		case 0x80:
            NOP(MODE_IMM, operands);
            break;
		case ORA_IMM:
			ORA(MODE_IMM, operands);
			break;
		case ORA_ZP:
			ORA(MODE_ZP, operands);
			break;
		case ORA_ZP_X:
			ORA(MODE_ZP_X, operands);
			break;
		case ORA_ABS:
			ORA(MODE_ABS, operands);
			break;
		case ORA_ABS_X:
			ORA(MODE_ABS_X, operands);
			break;
		case ORA_ABS_Y:
			ORA(MODE_ABS_Y, operands);
			break;
		case ORA_IND_X:
			ORA(MODE_IND_X, operands);
			break;
		case ORA_IND_Y:
			ORA(MODE_IND_Y, operands);
			break;
		case PHA:
            PHA(MODE_NONE, operands);
            break;
		case PHP:
            PHP(MODE_NONE, operands);
            break;
		case PLA:
            PLA(MODE_NONE, operands);
            break;
		case PLP:
            PLP(MODE_NONE, operands);
            break;
		case ROL_ACC:
			ROL(MODE_ACC, operands);
			break;
		case ROL_ZP:
			ROL(MODE_ZP, operands);
			break;
		case ROL_ZP_X:
			ROL(MODE_ZP_X, operands);
			break;
		case ROL_ABS:
			ROL(MODE_ABS, operands);
			break;
		case ROL_ABS_X:
			ROL(MODE_ABS_X, operands);
			break;
		case ROR_ACC:
			ROR(MODE_ACC, operands);
			break;
		case ROR_ZP:
			ROR(MODE_ZP, operands);
			break;
		case ROR_ZP_X:
			ROR(MODE_ZP_X, operands);
			break;
		case ROR_ABS:
			ROR(MODE_ABS, operands);
			break;
		case ROR_ABS_X:
			ROR(MODE_ABS_X, operands);
			break;
		case RTI:
            RTI(MODE_NONE, operands);
            break;
		case RTS:
            RTS(MODE_NONE, operands);
            break;
		case SBC_IMM:
			SBC(MODE_IMM, operands);
			break;
		case SBC_ZP:
			SBC(MODE_ZP, operands);
			break;
		case SBC_ZP_X:
			SBC(MODE_ZP_X, operands);
			break;
		case SBC_ABS:
			SBC(MODE_ABS, operands);
			break;
		case SBC_ABS_X:
			SBC(MODE_ABS_X, operands);
			break;
		case SBC_ABS_Y:
			SBC(MODE_ABS_Y, operands);
			break;
		case SBC_IND_X:
			SBC(MODE_IND_X, operands);
			break;
		case SBC_IND_Y:
			SBC(MODE_IND_Y, operands);
			break;
		case SEC:
            SEC(MODE_NONE, operands);
            break;
		case SED:
            SED(MODE_NONE, operands);
            break;
		case SEI:
            SEI(MODE_NONE, operands);
            break;
		case STA_ZP:
			STA(MODE_ZP, operands);
			break;
		case STA_ZP_X:
			STA(MODE_ZP_X, operands);
			break;
		case STA_ABS:
			STA(MODE_ABS, operands);
			break;
		case STA_ABS_X:
			STA(MODE_ABS_X, operands);
			break;
		case STA_ABS_Y:
			STA(MODE_ABS_Y, operands);
			break;
		case STA_IND_X:
			STA(MODE_IND_X, operands);
			break;
		case STA_IND_Y:
			STA(MODE_IND_Y, operands);
			break;
		case STX_ZP:
			STX(MODE_ZP, operands);
			break;
		case STX_ZP_Y:
			STX(MODE_ZP_Y, operands);
			break;
		case STX_ABS:
			STX(MODE_ABS, operands);
			break;
		case STY_ZP:
			STY(MODE_ZP, operands);
			break;
		case STY_ZP_X:
			STY(MODE_ZP_X, operands);
			break;
		case STY_ABS:
			STY(MODE_ABS, operands);
			break;
		case TAX:
			TAX(MODE_NONE, operands);
			break;
		case TAY:
			TAY(MODE_NONE, operands);
			break;
		case TSX:
			TSX(MODE_NONE, operands);
			break;
		case TXA:
			TXA(MODE_NONE, operands);
			break;
		case TXS:
			TXS(MODE_NONE, operands);
			break;
		case TYA:
			TYA(MODE_NONE, operands);
			break;
	}
}

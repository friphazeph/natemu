#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void log_state(uint16_t pc, uint8_t A, uint8_t X, uint8_t Y, uint8_t P, uint8_t SP) {
    printf("%04X    A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", pc, A, X, Y, P, SP);
}

void invalid_pc(uint16_t addr_from, uint16_t addr_to) {
	fprintf(stderr, "FATAL: Tried to jump to an address without static code in rts at 0x%X, to 0x%X\n",
		 addr_from, addr_to
	);
	exit(1);
}

void nes_main(void);

int main(void) {
	nes_main();
	return 0;
}

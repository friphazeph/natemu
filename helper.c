#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

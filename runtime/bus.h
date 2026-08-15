#ifndef BUS_H_
#define BUS_H_

#include "commons.h"

size_t addr_to_prg_rom(Addr addr);
void cpu_write(Addr addr, Byte value);
Byte cpu_read(Addr addr);
void ppu_write_reg(Addr addr, Byte value);
Byte ppu_read_reg(Addr addr);

#endif // BUS_H_

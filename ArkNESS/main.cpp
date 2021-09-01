// Project:     ArkNES
// File:        main.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description:	main entry

#include "nessys.h"

const char* cart_file_name = "C:\\Users\\Arkesh\\emu\\roms\\nes\\Super Mario Bros. 3 (USA).nes";

int main()
{
	// create an nes instance
	nessys_t nes_inst;
	int r;
	nessys_init(&nes_inst);
	if (nessys_load_cart_filename(&nes_inst, cart_file_name)) {
		printf("Cartridge load successfully: %s\n", cart_file_name);
		printf("Using mapper id: %d\n", nes_inst.mapper_id);
		printf("PRG-ROM size: 0x%x\n", nes_inst.prg_rom_size);
		printf("CHR-ROM size: 0x%x\n", nes_inst.ppu.chr_rom_size);
		printf("PRG_ROM base: 0x%x\n", (uint32_t) nes_inst.prg_rom_base);
		int b;
		for (b = 0; b < NESSYS_PRG_NUM_BANKS; b++) {
			printf("PRG_ROM bank[%d]: 0x%x\n", b, (uint32_t) nes_inst.prg_rom_bank[b]);
		}
		k2win::k2WindowLoop();
		for (r = 0; r < 8; r++) {
			printf("PPUREG[%d]=0x%x\n", r, nes_inst.ppu.reg[r]);
		}
	} else {
		printf("Could not load cart file\n");
	}
	nessys_unload_cart(&nes_inst);
	nessys_cleanup(&nes_inst);

	return 0;
}

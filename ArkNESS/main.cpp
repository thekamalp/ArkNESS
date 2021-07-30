// Project:     ArkNES
// File:        main.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description:	main entry

#include "nessys.h"

const char* cart_file_name = "C:\\Users\\Arkesh\\emu\\roms\\nes\\Donkey Kong (GC).nes";

int main()
{
	// create an nes instance
	nessys_t nes_inst;
	int r, c;
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
		for (r = 0; r < 30; r++) {
			for (c = 0; c < 32; c++) {
				printf("0x%02x ", *nessys_ppu_mem(&nes_inst, 0x2000 + 32 * r + c));
			}
			printf("\n");
		}
		printf("pattern 0x49\n");
		for (r = 0; r < 16; r++) {
			printf("0x%02x\n", *nessys_ppu_mem(&nes_inst, 0x49 * 16 + r));
		}
		for (r = 0; r < 16; r++) {
			printf("pal[%d] = 0x%02x / 0x%02x = (%f, %f, %f)\n", r, *nessys_ppu_mem(&nes_inst, 0x3F00 + r), nes_inst.ppu.pal[r],
				NESSYS_PPU_PALETTE[3 * nes_inst.ppu.pal[r] + 0], NESSYS_PPU_PALETTE[3 * nes_inst.ppu.pal[r] + 1], NESSYS_PPU_PALETTE[3 * nes_inst.ppu.pal[r] + 2]);
		}
		for (r = 0; r < 3; r++) {
			printf("pal[15]= %f\n", NESSYS_PPU_PALETTE[3* nes_inst.ppu.pal[0] + r]);
		}
		for (r = 0; r < 8; r++) {
			printf("PPUREG[%d]=0x%x\n", r, nes_inst.ppu.reg[r]);
		}
	} else {
		printf("Could not load cart file\n");
	}
	nessys_unload_cart(&nes_inst);

	return 0;
}

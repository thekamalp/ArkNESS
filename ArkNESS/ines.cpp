// Project:     ArkNES
// File:        ines.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description: Load and parse ines and nes 2.0 rom files

#include "nessys.h"

bool ines_load_cart(nessys_t* nes, FILE* fh)
{
	long int cur_pos = ftell(fh);

	ines_header hdr = { 0 };
	fread(&hdr, sizeof(hdr), 1, fh);
	if (hdr.signature != INES_SIGNATURE) {
		// not an ines file
		fseek(fh, cur_pos, SEEK_SET);
		return false;
	}

	// continues parsing
	if (hdr.flags6 & INES_FLAGS6_TRAINER) {
		// skip over 512B trainer
		fseek(fh, 512, SEEK_CUR);
	}

	// get mirroring mode
	nes->ppu.name_tbl_vert_mirror = hdr.flags6 & INES_FLAGS6_MIRRORING;

	// get mapper id
	nes->mapper_id = ((hdr.flags6 & INES_FLAGS6_MAPPER) >> 4) | (hdr.flags7 & INES_FLAGS7_MAPPER);

	// allocate space for prg rom and chr rom
	if (hdr.prg_rom_size) {
		nes->prg_rom_size = 0x4000 * hdr.prg_rom_size;
		nes->prg_rom_base = (uint8_t*)malloc(nes->prg_rom_size);
		if (nes->prg_rom_base == NULL) return false;
		fread(nes->prg_rom_base, 0x4000, hdr.prg_rom_size, fh);
	}
	if (hdr.chr_rom_size) {
		nes->ppu.chr_rom_size = 0x2000 * hdr.chr_rom_size;
		nes->ppu.chr_rom_base = (uint8_t*)malloc(nes->ppu.chr_rom_size);
		if (nes->ppu.chr_rom_base == NULL) return false;
		fread(nes->ppu.chr_rom_base, 0x2000, hdr.chr_rom_size, fh);
	}
	return true;
}

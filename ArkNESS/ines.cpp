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

	bool nes2 = (hdr.flags7 & INES_FLAGS7_NES2) ? true : false;

	// get mirroring mode
	nes->ppu.name_tbl_vert_mirror = hdr.flags6 & INES_FLAGS6_MIRRORING;

	// get mapper id
	nes->mapper_id = ((hdr.flags6 & INES_FLAGS6_MAPPER) >> 4) | (hdr.flags7 & INES_FLAGS7_MAPPER);

	// allocate space for 4 screen vram
	if (hdr.flags6 & INES_FLAGS6_MIRROR_CTRL_DISABLE) {
		nes->ppu.mem_4screen = (uint8_t*)malloc(NESSYS_PPU_MEM_SIZE);
	}

	// allocate space for prg rom/ram and chr rom
	if (hdr.prg_rom_size) {
		nes->prg_rom_size = hdr.prg_rom_size;
		if (nes2) nes->prg_rom_size |= (hdr.flags9 & 0xf) << 8;
		nes->prg_rom_size *= 0x4000;
		nes->prg_rom_base = (uint8_t*)malloc(nes->prg_rom_size);
		if (nes->prg_rom_base == NULL) return false;
		fread(nes->prg_rom_base, 0x4000, hdr.prg_rom_size, fh);
	}
	uint32_t ram_size = (nes2) ? ((hdr.flags10 & 0xf) ? 64 << (hdr.flags10 & 0xf) : 0) :
		((hdr.prg_ram_size) ? hdr.prg_ram_size * 0x2000 : 0x2000);
	if (ram_size) {
		nes->prg_ram_size = ram_size;
		nes->prg_ram_base = (uint8_t*)malloc(ram_size);
	}
	if (hdr.chr_rom_size) {
		nes->ppu.chr_rom_size = hdr.chr_rom_size;
		if (nes2) nes->ppu.chr_rom_size |= (hdr.flags9 & 0xf0) << 4;
		nes->ppu.chr_rom_size *= 0x2000;
		nes->ppu.chr_rom_base = (uint8_t*)malloc(nes->ppu.chr_rom_size);
		if (nes->ppu.chr_rom_base == NULL) return false;
		fread(nes->ppu.chr_rom_base, 0x2000, hdr.chr_rom_size, fh);
	//} else if (hdr.flags11 & 0xf) {
	//	nes->ppu.chr_ram_size = 64 << (hdr.flags11 & 0xf);
	//	nes->ppu.chr_ram_base = (uint8_t*)malloc(nes->ppu.chr_ram_size);
	//	if (nes->ppu.chr_ram_base == NULL) return false;
	} else {
		nes->ppu.chr_ram_size = 0x2000;  // 8KB of ram
		nes->ppu.chr_ram_base = (uint8_t*)malloc(nes->ppu.chr_ram_size);
		if (nes->ppu.chr_ram_base == NULL) return false;
	}
	return true;
}

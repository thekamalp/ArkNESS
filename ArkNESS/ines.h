// Project:     ArkNES
// File:        ines.h
// Author:      Kamal Pillai
// Date:        7/12/2021
// Description:	Used to parse ines and nes 2.0 rom files

#pragma once

const uint32_t INES_SIGNATURE = 0x1A53454E;  // "NES<eof>"

const uint8_t INES_FLAGS6_MIRRORING           = 0x01;
const uint8_t INES_FLAGS6_PERS_PRG_RAM        = 0x02;
const uint8_t INES_FLAGS6_TRAINER             = 0x04;
const uint8_t INES_FLAGS6_MIRROR_CTRL_DISABLE = 0x08;
const uint8_t INES_FLAGS6_MAPPER              = 0xF0;

const uint8_t INES_FLAGS7_VS_UNISYS           = 0x01;
const uint8_t INES_FLAGS7_PLAYCHOICE10        = 0x02;
const uint8_t INES_FLAGS7_NES2                = 0x0C;
const uint8_t INES_FLAGS7_MAPPER              = 0xF0;

const uint8_t INES_FLAGS9_TV_SYS              = 0x01;

const uint8_t INES_FLAGS10_TV_SYS             = 0x03;
const uint8_t INES_FLAGS10_PRG_RAM            = 0x10;
const uint8_t INES_FLAGS10_BUS_CONFLICT       = 0x20;

#pragma pack(push, 1)
struct ines_header {
	uint32_t signature;    // must match INES_SIGNATURE
	uint8_t prg_rom_size;  // in 16KB units
	uint8_t chr_rom_size;  // in 8KB units
	uint8_t flags6;
	uint8_t flags7;
	uint8_t prg_ram_size;  // in 8KB units
	uint8_t flags9;
	uint8_t flags10;
	uint8_t flags11;
	uint8_t flags12;
	uint8_t flags13;
	uint8_t flags14;
	uint8_t flags15;
};
#pragma pack(pop)

bool ines_load_cart(nessys_t* nes, FILE* fh);

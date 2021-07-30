// Project:     ArkNES
// File:        nessys.h
// Author:      Kamal Pillai
// Date:        7/12/2021
// Description:	NES system structures

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <k2/k2win.hpp>

// memory map constnts
const uint32_t NESSYS_RAM_SIZE = 0x800;  // 2KB Ram
const uint32_t NESSYS_RAM_MASK = NESSYS_RAM_SIZE - 1;
const uint32_t NESSYS_RAM_WIN_MIN = 0x0;
const uint32_t NESSYS_RAM_WIN_SIZE = 0x2000; // 8KB ram window
const uint32_t NESSYS_RAM_WIN_MAX = NESSYS_RAM_WIN_MIN + NESSYS_RAM_WIN_SIZE - 1;

const uint32_t NESSYS_PPU_REG_SIZE = 0x8;
const uint32_t NESSYS_PPU_REG_MASK = NESSYS_PPU_REG_SIZE - 1;
const uint32_t NESSYS_PPU_WIN_MIN = 0x2000;
const uint32_t NESSYS_PPU_WIN_SIZE = 0x2000;
const uint32_t NESSYS_PPU_WIN_MAX = NESSYS_PPU_WIN_MIN + NESSYS_PPU_WIN_SIZE - 1;

const uint32_t NESSYS_APU_SIZE = 0x18;
const uint32_t NESSYS_APU_WIN_MIN = 0x4000;
const uint32_t NESSYS_APU_WIN_MAX = NESSYS_APU_WIN_MIN + NESSYS_APU_SIZE - 1;
const uint32_t NESSYS_APU_MASK = 0x1F;

const uint32_t NESSYS_PRG_WIN_MIN = 0x4000;
const uint32_t NESSYS_PRG_WIN_SIZE = 0xC000;
const uint32_t NESSYS_PRG_WIN_MAX = NESSYS_PRG_WIN_MIN + NESSYS_PRG_WIN_SIZE - 1;

const uint32_t NESSYS_PPU_MEM_SIZE = 0x800;  // 2KB ram, nametable space
const uint32_t NESSYS_PPU_NUM_SPRITES = 64;
const uint32_t NESSYS_PPU_SPRITE_SIZE = 4;
const uint32_t NESSYS_PPU_OAM_SIZE = NESSYS_PPU_NUM_SPRITES * NESSYS_PPU_SPRITE_SIZE;

const uint32_t NESSYS_CHR_ROM_WIN_MIN = 0x0;
const uint32_t NESSYS_CHR_ROM_WIN_SIZE = 0x2000;
const uint32_t NESSYS_CHR_ROM_WIN_MAX = NESSYS_CHR_ROM_WIN_MIN + NESSYS_CHR_ROM_WIN_SIZE - 1;

const uint32_t NESSYS_CHR_NTB_WIN_MIN = 0x2000;
const uint32_t NESSYS_CHR_NTB_WIN_SIZE = 0x2000;
const uint32_t NESSYS_CHR_NTB_WIN_MAX = NESSYS_CHR_NTB_WIN_MIN + NESSYS_CHR_NTB_WIN_SIZE - 1;

const uint32_t NESSYS_PPU_PAL_SIZE = 0x20;
const uint32_t NESSYS_PPU_PAL_MASK = NESSYS_PPU_PAL_SIZE - 1;
const uint32_t NESSYS_CHR_PAL_WIN_MIN = 0x3F00;
const uint32_t NESSYS_CHR_PAL_WIN_SIZE = 0x100;
const uint32_t NESSYS_CHR_PAL_WIN_MAX = NESSYS_CHR_PAL_WIN_MIN + NESSYS_CHR_PAL_WIN_SIZE - 1;

const uint32_t NESSYS_PRG_BANK_SIZE_LOG2 = 13;  // PRG ROM/RAM address mappers are at 8KB granularity
const uint32_t NESSYS_CHR_BANK_SIZE_LOG2 = 10;  // CHR ROM address mappers are at 1KB granularity
const uint32_t NESSYS_PRG_BANK_SIZE = (1 << NESSYS_PRG_BANK_SIZE_LOG2);
const uint32_t NESSYS_CHR_BANK_SIZE = (1 << NESSYS_CHR_BANK_SIZE_LOG2);
const uint32_t NESSYS_PRG_MEM_MASK = NESSYS_PRG_BANK_SIZE - 1;
const uint32_t NESSYS_CHR_MEM_MASK = NESSYS_CHR_BANK_SIZE - 1;

const uint32_t NESSYS_PRG_ADDR_SPACE = 0x10000;
const uint32_t NESSYS_CHR_ADDR_SPACE = 0x4000;
const uint32_t NESSYS_PRG_NUM_BANKS = NESSYS_PRG_ADDR_SPACE / NESSYS_PRG_BANK_SIZE;
const uint32_t NESSYS_CHR_NUM_BANKS = NESSYS_CHR_ADDR_SPACE / NESSYS_CHR_BANK_SIZE;

const uint32_t NESSYS_PRG_RAM_START = 0x6000;  // typical starting address of RAM
const uint32_t NESSYS_PRG_ROM_START = 0x8000;  // typical starting address of ROM

const uint32_t NESSYS_SYS_RAM_START_BANK = NESSYS_RAM_WIN_MIN / NESSYS_PRG_BANK_SIZE;
const uint32_t NESSYS_PPU_REG_START_BANK = NESSYS_PPU_WIN_MIN / NESSYS_PRG_BANK_SIZE;
const uint32_t NESSYS_APU_REG_START_BANK = NESSYS_APU_WIN_MIN / NESSYS_PRG_BANK_SIZE;
const uint32_t NESSYS_PRG_RAM_START_BANK = NESSYS_PRG_RAM_START / NESSYS_PRG_BANK_SIZE;
const uint32_t NESSYS_PRG_ROM_START_BANK = NESSYS_PRG_ROM_START / NESSYS_PRG_BANK_SIZE;

const uint32_t NESSYS_CHR_ROM_START_BANK = NESSYS_CHR_ROM_WIN_MIN / NESSYS_CHR_BANK_SIZE;
const uint32_t NESSYS_CHR_NTB_START_BANK = NESSYS_CHR_NTB_WIN_MIN / NESSYS_CHR_BANK_SIZE;

const uint32_t NESSYS_CHR_ROM_END_BANK = NESSYS_CHR_ROM_WIN_MAX / NESSYS_CHR_BANK_SIZE;
const uint32_t NESSYS_CHR_NTB_END_BANK = NESSYS_CHR_NTB_WIN_MAX / NESSYS_CHR_BANK_SIZE;

// interrupt table addresses
const uint16_t NESSYS_NMI_VECTOR = 0xFFFA;
const uint16_t NESSYS_RST_VECTOR = 0xFFFC;
const uint16_t NESSYS_IRQ_VECTOR = 0xFFFE;  // also used for BRK

// ppu timing - ntsc
const uint32_t NESSYS_PPU_PER_CPU_CLK = 3;
const uint32_t NESSYS_PPU_CLK_PER_SCANLINE = 341;
const uint32_t NESSYS_PPU_SCANLINES_RENDERED = 240;
const uint32_t NESSYS_PPU_SCANLINES_POST_RENDER = 1;
const uint32_t NESSYS_PPU_SCANLINES_VBLANK = 20;
const uint32_t NESSYS_PPU_SCANLINES_PRE_RENDER = 1;
const uint32_t NESSYS_PPU_SCANLINES_PER_FRAME = NESSYS_PPU_SCANLINES_RENDERED + NESSYS_PPU_SCANLINES_POST_RENDER + NESSYS_PPU_SCANLINES_VBLANK + NESSYS_PPU_SCANLINES_PRE_RENDER;

const uint32_t NESSYS_PPU_SCANLINES_RENDERED_CLKS = NESSYS_PPU_SCANLINES_RENDERED * NESSYS_PPU_CLK_PER_SCANLINE;
const uint32_t NESSYS_PPU_SCANLINES_POST_RENDER_CLKS = NESSYS_PPU_SCANLINES_POST_RENDER * NESSYS_PPU_CLK_PER_SCANLINE;
const uint32_t NESSYS_PPU_SCANLINES_VBLANK_CLKS = NESSYS_PPU_SCANLINES_VBLANK * NESSYS_PPU_CLK_PER_SCANLINE;
const uint32_t NESSYS_PPU_SCANLINES_PRE_RENDER_CLKS = NESSYS_PPU_SCANLINES_PRE_RENDER * NESSYS_PPU_CLK_PER_SCANLINE;
const uint32_t NESSYS_PPU_SCANLINES_PER_FRAME_CLKS = NESSYS_PPU_SCANLINES_PER_FRAME * NESSYS_PPU_CLK_PER_SCANLINE;

// 64x3 component entry palette, in float
const float NESSYS_PPU_PALETTE[] = {
	 84/255.0f,  84/255.0f,  84/255.0f,    0/255.0f,  30/255.0f, 116/255.0f,    8/255.0f,  16/255.0f, 144/255.0f,   48/255.0f,   0/255.0f, 136/255.0f,   68/255.0f,   0/255.0f, 100/255.0f,   92/255.0f,   0/255.0f,  48/255.0f,   84/255.0f,   4/255.0f,   0/255.0f,   60/255.0f,  24/255.0f,   0/255.0f,   32/255.0f,  42/255.0f,   0/255.0f,    8/255.0f,  58/255.0f,   0/255.0f,    0/255.0f,  64/255.0f,   0/255.0f,    0/255.0f,  60/255.0f,   0/255.0f,    0/255.0f,  50/255.0f,  60/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,
	152/255.0f, 150/255.0f, 152/255.0f,    8/255.0f,  76/255.0f, 196/255.0f,   48/255.0f,  50/255.0f, 236/255.0f,   92/255.0f,  30/255.0f, 228/255.0f,  136/255.0f,  20/255.0f, 176/255.0f,  160/255.0f,  20/255.0f, 100/255.0f,  152/255.0f,  34/255.0f,  32/255.0f,  120/255.0f,  60/255.0f,   0/255.0f,   84/255.0f,  90/255.0f,   0/255.0f,   40/255.0f, 114/255.0f,   0/255.0f,    8/255.0f, 124/255.0f,   0/255.0f,    0/255.0f, 118/255.0f,  40/255.0f,    0/255.0f, 102/255.0f, 120/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,
	236/255.0f, 238/255.0f, 236/255.0f,   76/255.0f, 154/255.0f, 236/255.0f,  120/255.0f, 124/255.0f, 236/255.0f,  176/255.0f,  98/255.0f, 236/255.0f,  228/255.0f,  84/255.0f, 236/255.0f,  236/255.0f,  88/255.0f, 180/255.0f,  236/255.0f, 106/255.0f, 100/255.0f,  212/255.0f, 136/255.0f,  32/255.0f,  160/255.0f, 170/255.0f,   0/255.0f,  116/255.0f, 196/255.0f,   0/255.0f,   76/255.0f, 208/255.0f,  32/255.0f,   56/255.0f, 204/255.0f, 108/255.0f,   56/255.0f, 180/255.0f, 204/255.0f,   60/255.0f,  60/255.0f,  60/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,
	236/255.0f, 238/255.0f, 236/255.0f,  168/255.0f, 204/255.0f, 236/255.0f,  188/255.0f, 188/255.0f, 236/255.0f,  212/255.0f, 178/255.0f, 236/255.0f,  236/255.0f, 174/255.0f, 236/255.0f,  236/255.0f, 174/255.0f, 212/255.0f,  236/255.0f, 180/255.0f, 176/255.0f,  228/255.0f, 196/255.0f, 144/255.0f,  204/255.0f, 210/255.0f, 120/255.0f,  180/255.0f, 222/255.0f, 120/255.0f,  168/255.0f, 226/255.0f, 144/255.0f,  152/255.0f, 226/255.0f, 180/255.0f,  160/255.0f, 214/255.0f, 228/255.0f,  160/255.0f, 162/255.0f, 160/255.0f,    0/255.0f,   0/255.0f,   0/255.0f,    0/255.0f,   0/255.0f,   0/255.0f
};

const uint32_t NESSYS_APU_JOYPAD0_OFFSET = 0x16;
const uint32_t NESSYS_APU_JOYPAD1_OFFSET = 0x17;
const uint32_t NESSYS_APU_FRAME_COUNTER_OFFSET = 0x17;

const uint32_t NESSYS_STD_CONTROLLER_BUTTON_A      = 0x0;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_B      = 0x1;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_SELECT = 0x2;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_START  = 0x3;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_UP     = 0x4;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_DOWN   = 0x5;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_LEFT   = 0x6;
const uint32_t NESSYS_STD_CONTROLLER_BUTTON_RIGHT  = 0x7;

const uint8_t NESSYS_STD_CONTROLLER_BUTTON_A_MASK      = 1 << NESSYS_STD_CONTROLLER_BUTTON_A;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_B_MASK      = 1 << NESSYS_STD_CONTROLLER_BUTTON_B;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_SELECT_MASK = 1 << NESSYS_STD_CONTROLLER_BUTTON_SELECT;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_START_MASK  = 1 << NESSYS_STD_CONTROLLER_BUTTON_START;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_UP_MASK     = 1 << NESSYS_STD_CONTROLLER_BUTTON_UP;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK   = 1 << NESSYS_STD_CONTROLLER_BUTTON_DOWN;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK   = 1 << NESSYS_STD_CONTROLLER_BUTTON_LEFT;
const uint8_t NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK  = 1 << NESSYS_STD_CONTROLLER_BUTTON_RIGHT;

// system structs
struct nessys_cpu_regs_t {
	uint8_t a;
	uint8_t x;
	uint8_t y;
	uint8_t s;
	uint16_t pc;
	uint8_t p;
	uint8_t pad0;
};

struct nessys_apu_regs_t {
	uint8_t reg[NESSYS_APU_SIZE];
	uint8_t joy_control;
	uint8_t frame_counter;
	uint8_t joypad[2];
	uint8_t latched_joypad[2];
	uint8_t pad[NESSYS_APU_MASK + 1 - NESSYS_APU_SIZE - 6];
};

struct nessys_ppu_t {
	uint32_t cycle;
	uint8_t reg[NESSYS_PPU_REG_SIZE];
	uint8_t mem[NESSYS_PPU_MEM_SIZE];
	uint8_t oam[NESSYS_PPU_OAM_SIZE];
	uint8_t pal[NESSYS_PPU_PAL_SIZE];
	bool name_tbl_vert_mirror;
	uint8_t status;
	uint8_t scroll[2];
	uint8_t scroll_toggle;
	uint16_t mem_addr;
	uint16_t mem_addr_mask;
	uint32_t chr_rom_size;
	uint32_t chr_ram_size;
	uint8_t* chr_rom_base;
	uint8_t* chr_ram_base;
	uint16_t chr_rom_bank_mask[NESSYS_CHR_NUM_BANKS];
	uint8_t* chr_rom_bank[NESSYS_CHR_NUM_BANKS];
};

struct nessys_t {
	uint32_t mapper_id;
	uint32_t cycle;
	int32_t cycles_remaining;
	uint32_t vblank_cycles; // cycles until next vblank
	nessys_cpu_regs_t reg;
	nessys_apu_regs_t apu;
	nessys_ppu_t ppu;
	uint32_t prg_rom_size;
	uint8_t sysmem[NESSYS_RAM_SIZE];
	uint8_t* prg_rom_base;
	uint16_t prg_rom_bank_mask[NESSYS_PRG_NUM_BANKS];
	uint8_t* prg_rom_bank[NESSYS_PRG_NUM_BANKS];
	// rendering data structures
	k2win* win;
	k2gfx* gfx;
	k2rendergroup* rg_win;
	k2blendstate* bs_normal;
	k2blendstate* bs_mask;
	k2depthstate* ds_none;
	k2rasterizerstate* rs_normal;
	k2shadergroup* sg_background;
	k2shadergroup* sg_sprite;
	k2vertexgroup* vg_fullscreen;
	k2vertexgroup* vg_sprite;
	k2constantbuffer* cb_nametable;
	k2constantbuffer* cb_pattern;
	k2constantbuffer* cb_palette;
	k2constantbuffer* cb_sprite;
	k2constantbuffer* cb_ppu;
	k2constantgroup* cg_background;
	k2constantgroup* cg_sprite;
};

#include "c6502.h"
#include "ines.h"

void nessys_init(nessys_t* nes);
void nessys_power_cycle(nessys_t* nes);
void nessys_reset(nessys_t* nes);
bool nessys_load_cart_filename(nessys_t* nes, const char* filename);
bool nessys_load_cart(nessys_t* nes, FILE* fh);
void nessys_default_memmap(nessys_t* nes);
uint32_t nessys_exec_cpu_cycles(nessys_t* nes, uint32_t num_cycles);
void nessys_unload_cart(nessys_t* nes);

inline uint8_t* nessys_mem(nessys_t* nes, uint16_t addr, uint16_t* bank, uint16_t* offset)
{
	uint16_t b = addr >> NESSYS_PRG_BANK_SIZE_LOG2;
	uint16_t o = addr & nes->prg_rom_bank_mask[b];
	*bank = b;
	*offset = o;
	return nes->prg_rom_bank[b] + o;
}

inline uint8_t* nessys_ppu_mem(nessys_t* nes, uint16_t addr)
{
	if (addr >= NESSYS_CHR_PAL_WIN_MIN) return nes->ppu.pal + (addr & NESSYS_PPU_PAL_MASK);
	uint16_t b = addr >> NESSYS_CHR_BANK_SIZE_LOG2;
	return nes->ppu.chr_rom_bank[b] + (addr & nes->ppu.chr_rom_bank_mask[b]);
}

// Project:     ArkNESS
// File:        mapper.h
// Author:      Kamal Pillai
// Date:        8/9/2021
// Description:	NES memory mappers

#pragma once
#include "nessys.h"

// return smallest mask that covers all btis of the input
inline uint8_t nes_get_mask(uint8_t n)
{
	uint8_t mask;
	for (mask = 0x80; mask != 0; mask = mask >> 1) {
		if (n & mask) {
			mask--;
			mask <<= 1;
			mask |= 1;
			break;
		}
	}
	return mask;
}

// ------------------------------------------------------------
// mapper 1 structs/constants

// mask to distinguish which register is being programmed
const uint16_t MAPPER1_ADDR_MASK = 0xE000;

const uint16_t MAPPER1_ADDR_CONTROL = 0x8000;
const uint16_t MAPPER1_ADDR_CHR_BANK0 = 0xA000;
const uint16_t MAPPER1_ADDR_CHR_BANK1 = 0xC000;
const uint16_t MAPPER1_ADDR_PRG_BANK = 0xE000;

const uint8_t MAPPER1_PRG_BANK_SIZE_LOG2 = 14;
const uint8_t MAPPER1_CHR_BANK_SIZE_LOG2 = 12;
const uint32_t MAPPER1_PRG_BANK_MASK = (1 << MAPPER1_PRG_BANK_SIZE_LOG2) - 1;
const uint32_t MAPPER1_CHR_BANK_MASK = (1 << MAPPER1_CHR_BANK_SIZE_LOG2) - 1;

const uint8_t MAPPER1_SHIFT_REG_RESET = 0x10;

struct mapper1_data {
	uint8_t shift_reg;
	uint8_t pad0[3];
	uint8_t control;
	uint8_t chr_bank0;
	uint8_t chr_bank1;
	uint8_t prg_bank;
	uint8_t prg_ram_bank;
};

// ------------------------------------------------------------
// mapper 2 structs/constants

const uint8_t MAPPER2_PRG_BANK_SIZE_LOG2 = 14;
const uint32_t MAPPER2_PRG_BANK_MASK = (1 << MAPPER2_PRG_BANK_SIZE_LOG2) - 1;

struct mapper2_data {
	uint8_t prg_bank;
};

// ------------------------------------------------------------
// mapper 3 structs/constants

const uint8_t MAPPER3_CHR_BANK_SIZE_LOG2 = 13;
const uint32_t MAPPER3_CHR_BANK_MASK = (1 << MAPPER3_CHR_BANK_SIZE_LOG2) - 1;

struct mapper3_data {
	uint8_t chr_bank;
};

// ------------------------------------------------------------
// mapper 4 struct/constants

// mask to distinguish which register is being programmed
const uint16_t MAPPER4_ADDR_MASK = 0xE001;

const uint16_t MAPPER4_ADDR_BANK_SELECT = 0x8000;
const uint16_t MAPPER4_ADDR_BANK_DATA = 0x8001;
const uint16_t MAPPER4_ADDR_MIRROR = 0xA000;
const uint16_t MAPPER4_ADDR_PRG_RAM_PROTECT = 0xA001;
const uint16_t MAPPER4_ADDR_IRQ_LATCH = 0xC000;
const uint16_t MAPPER4_ADDR_IRQ_RELOAD = 0xC001;
const uint16_t MAPPER4_ADDR_IRQ_DISABLE = 0xE000;
const uint16_t MAPPER4_ADDR_IRQ_ENABLE = 0xE001;

const uint8_t MAPPER4_PRG_BANK_SIZE_LOG2 = 13;
const uint8_t MAPPER4_CHR_BANK_SIZE_LOG2 = 10;
const uint32_t MAPPER4_PRG_BANK_MASK = (1 << MAPPER4_PRG_BANK_SIZE_LOG2) - 1;
const uint32_t MAPPER4_CHR_BANK_MASK = (1 << MAPPER4_CHR_BANK_SIZE_LOG2) - 1;

const uint8_t MAPPER4_REG_MASK[] = { 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x3f };

struct mapper4_data {
	uint8_t r[8];
	uint8_t bank_select;
	uint8_t prg_ram_protect;
	uint8_t irq_latch;
	uint8_t counter_write_pending;
	uint8_t irq_enable;
	uint8_t irq_counter;
	uint8_t last_scanline;
	uint8_t last_upper_ppu_addr;
};

// ------------------------------------------------------------
// mapper 5 struct/constants

// this is not a real register address, just an offset into the memory poool allocated
// for the fill data/color
const uint32_t MAPPER5_ADDR_FILL_DATA_OFFSET = 0x0c00;

// these are register offset from the bank base (same bank as apu registers)
const uint32_t MAPPER5_ADDR_PULSE0_CTRL_OFFSET = 0x1000;
const uint32_t MAPPER5_ADDR_PULSE0_SWEEP_OFFSET = 0x1001;
const uint32_t MAPPER5_ADDR_PULSE0_TIMER_OFFSET = 0x1002;
const uint32_t MAPPER5_ADDR_PULSE0_RELOAD_OFFSET = 0x1003;
const uint32_t MAPPER5_ADDR_PULSE1_CTRL_OFFSET = 0x1004;
const uint32_t MAPPER5_ADDR_PULSE1_SWEEP_OFFSET = 0x1005;
const uint32_t MAPPER5_ADDR_PULSE1_TIMER_OFFSET = 0x1006;
const uint32_t MAPPER5_ADDR_PULSE1_RELOAD_OFFSET = 0x1007;
const uint32_t MAPPER5_ADDR_PCM_CTRL_OFFSET = 0x1010;
const uint32_t MAPPER5_ADDR_PCM_DATA_OFFSET = 0x1011;
const uint32_t MAPPER5_ADDR_PRG_MODE_OFFSET = 0x1100;
const uint32_t MAPPER5_ADDR_CHR_MODE_OFFSET = 0x1101;
const uint32_t MAPPER5_ADDR_PRG_RAM_PROTECT1_OFFSET = 0x1102;
const uint32_t MAPPER5_ADDR_PRG_RAM_PROTECT2_OFFSET = 0x1103;
const uint32_t MAPPER5_ADDR_EXP_RAM_MODE_OFFSET = 0x1104;
const uint32_t MAPPER5_ADDR_NTB_MAP_OFFSET = 0x1105;
const uint32_t MAPPER5_ADDR_FILL_MODE_TILE_OFFSET = 0x1106;
const uint32_t MAPPER5_ADDR_FILL_MODE_COLOR_OFFSET = 0x1107;
const uint32_t MAPPER5_ADDR_PRG_BANK0_OFFSET = 0x1113;
const uint32_t MAPPER5_ADDR_PRG_BANK1_OFFSET = 0x1114;
const uint32_t MAPPER5_ADDR_PRG_BANK2_OFFSET = 0x1115;
const uint32_t MAPPER5_ADDR_PRG_BANK3_OFFSET = 0x1116;
const uint32_t MAPPER5_ADDR_PRG_BANK4_OFFSET = 0x1117;
const uint32_t MAPPER5_ADDR_CHR_BANK0_OFFSET = 0x1120;
const uint32_t MAPPER5_ADDR_CHR_BANK1_OFFSET = 0x1121;
const uint32_t MAPPER5_ADDR_CHR_BANK2_OFFSET = 0x1122;
const uint32_t MAPPER5_ADDR_CHR_BANK3_OFFSET = 0x1123;
const uint32_t MAPPER5_ADDR_CHR_BANK4_OFFSET = 0x1124;
const uint32_t MAPPER5_ADDR_CHR_BANK5_OFFSET = 0x1125;
const uint32_t MAPPER5_ADDR_CHR_BANK6_OFFSET = 0x1126;
const uint32_t MAPPER5_ADDR_CHR_BANK7_OFFSET = 0x1127;
const uint32_t MAPPER5_ADDR_CHR_BANK8_OFFSET = 0x1128;
const uint32_t MAPPER5_ADDR_CHR_BANK9_OFFSET = 0x1129;
const uint32_t MAPPER5_ADDR_CHR_BANKA_OFFSET = 0x112A;
const uint32_t MAPPER5_ADDR_CHR_BANKB_OFFSET = 0x112B;
const uint32_t MAPPER5_ADDR_UPPER_CHR_BANK_OFFSET = 0x1130;
const uint32_t MAPPER5_ADDR_VSPLIT_MODE_OFFSET = 0x1200;
const uint32_t MAPPER5_ADDR_VSPLIT_SCROLL_OFFSET = 0x1201;
const uint32_t MAPPER5_ADDR_VSPLIT_BANK_OFFSET = 0x1202;
const uint32_t MAPPER5_ADDR_SCANLINE_IRQ_CMP_OFFSET = 0x1203;
const uint32_t MAPPER5_ADDR_SCANLINE_IRQ_STATUS_OFFSET = 0x1204;
const uint32_t MAPPER5_ADDR_MULT0_OFFSET = 0x1205;
const uint32_t MAPPER5_ADDR_MULT1_OFFSET = 0x1206;
const uint32_t MAPPER5_ADDR_CL3_SL3_CTRL_OFFSET = 0x1207;
const uint32_t MAPPER5_ADDR_CL3_SL3_STATUS_OFFSET = 0x1208;
const uint32_t MAPPER5_ADDR_TIMER_IRQ_LSB_OFFSET = 0x1209;
const uint32_t MAPPER5_ADDR_TIMER_IRQ_MSB_OFFSET = 0x120a;
const uint32_t MAPPER5_ADDR_EXP_RAM_START_OFFSET = 0x1c00;

const uint32_t MAPPER5_ADDR_PULSE0_CTRL = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE0_CTRL_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE0_SWEEP = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE0_SWEEP_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE0_TIMER = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE0_TIMER_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE0_RELOAD = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE0_RELOAD_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE1_CTRL = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE1_CTRL_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE1_SWEEP = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE1_SWEEP_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE1_TIMER = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE1_TIMER_OFFSET;
const uint32_t MAPPER5_ADDR_PULSE1_RELOAD = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PULSE1_RELOAD_OFFSET;
const uint32_t MAPPER5_ADDR_PCM_CTRL = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PCM_CTRL_OFFSET;
const uint32_t MAPPER5_ADDR_PCM_DATA = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PCM_DATA_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_MODE = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_MODE_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_MODE = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_MODE_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_RAM_PROTECT1 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_RAM_PROTECT1_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_RAM_PROTECT2 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_RAM_PROTECT2_OFFSET;
const uint32_t MAPPER5_ADDR_EXP_RAM_MODE = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_EXP_RAM_MODE_OFFSET;
const uint32_t MAPPER5_ADDR_NTB_MAP = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_NTB_MAP_OFFSET;
const uint32_t MAPPER5_ADDR_FILL_MODE_TILE = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_FILL_MODE_TILE_OFFSET;
const uint32_t MAPPER5_ADDR_FILL_MODE_COLOR = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_FILL_MODE_COLOR_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_BANK0 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_BANK0_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_BANK1 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_BANK1_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_BANK2 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_BANK2_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_BANK3 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_BANK3_OFFSET;
const uint32_t MAPPER5_ADDR_PRG_BANK4 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_PRG_BANK4_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK0 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK0_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK1 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK1_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK2 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK2_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK3 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK3_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK4 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK4_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK5 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK5_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK6 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK6_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK7 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK7_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK8 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK8_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANK9 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANK9_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANKA = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANKA_OFFSET;
const uint32_t MAPPER5_ADDR_CHR_BANKB = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CHR_BANKB_OFFSET;
const uint32_t MAPPER5_ADDR_UPPER_CHR_BANK = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_UPPER_CHR_BANK_OFFSET;
const uint32_t MAPPER5_ADDR_VSPLIT_MODE = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_VSPLIT_MODE_OFFSET;
const uint32_t MAPPER5_ADDR_VSPLIT_SCROLL = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_VSPLIT_SCROLL_OFFSET;
const uint32_t MAPPER5_ADDR_VSPLIT_BANK = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_VSPLIT_BANK_OFFSET;
const uint32_t MAPPER5_ADDR_SCANLINE_IRQ_CMP = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_SCANLINE_IRQ_CMP_OFFSET;
const uint32_t MAPPER5_ADDR_SCANLINE_IRQ_STATUS = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_SCANLINE_IRQ_STATUS_OFFSET;
const uint32_t MAPPER5_ADDR_MULT0 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_MULT0_OFFSET;
const uint32_t MAPPER5_ADDR_MULT1 = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_MULT1_OFFSET;
const uint32_t MAPPER5_ADDR_CL3_SL3_CTRL = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CL3_SL3_CTRL_OFFSET;
const uint32_t MAPPER5_ADDR_CL3_SL3_STATUS = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_CL3_SL3_STATUS_OFFSET;
const uint32_t MAPPER5_ADDR_TIMER_IRQ_LSB = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_TIMER_IRQ_LSB_OFFSET;
const uint32_t MAPPER5_ADDR_TIMER_IRQ_MSB = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_TIMER_IRQ_MSB_OFFSET;
const uint32_t MAPPER5_ADDR_EXP_RAM_START = NESSYS_APU_WIN_MIN + MAPPER5_ADDR_EXP_RAM_START_OFFSET;

const uint32_t MAPPER5_EXP_RAM_SIZE = 0x400;

const uint32_t MAPPER5_MEM_SIZE_LOG2 = 13;  // 8 KB
const uint32_t MAPPER5_MEM_SIZE = 1 << MAPPER5_MEM_SIZE_LOG2;

const uint32_t MAPPER5_PRG_BANK_BASE_SIZE_LOG2 = 13;  // 8 KB
const uint32_t MAPPER5_CHR_BANK_BASE_SIZE_LOG2 = 10;  // 1 KB

const uint32_t MAPPER5_PRG_BANK_BASE_SIZE = 1 << MAPPER5_PRG_BANK_BASE_SIZE_LOG2;
const uint32_t MAPPER5_CHR_BANK_BASE_SIZE = 1 << MAPPER5_CHR_BANK_BASE_SIZE_LOG2;

const uint32_t MAPPER5_PRG_BANK_BASE_MASK = MAPPER5_PRG_BANK_BASE_SIZE - 1;
const uint32_t MAPPER5_CHR_BANK_BASE_MASK = MAPPER5_CHR_BANK_BASE_SIZE - 1;

struct mapper5_data {
	uint8_t mem[MAPPER5_MEM_SIZE];
	uint8_t prg_mode;
	uint8_t chr_mode;
	uint8_t prg_ram_protect1;
	uint8_t prg_ram_protect2;
	uint8_t exp_ram_mode;
	uint8_t ntb_map;
	uint8_t msb_chr_bank;
	uint8_t prg_bank[5];
	uint16_t chr_bank[12];
	uint8_t upper_reg_touched;
	uint8_t vsplit_mode;
	uint8_t vsplit_scroll;
	uint8_t vsplit_bank;
	uint8_t scanline_irq_cmp;
	uint8_t scanline_irq_status;
	uint8_t scanline_irq_pend_clear;
	uint8_t mult[2];
	uint8_t cl3_sl3_ctrl;
	uint8_t cl3_sl3_status;
	uint16_t timer_irq;
	int32_t last_scanline;
	uint8_t scroll_save[2];
	uint16_t scroll_save_y;
	nessys_apu_pulse_t pulse[2];
	bool exp_surf_dirty;
};

// ------------------------------------------------------------
// mapper 7 struct/constants

const uint8_t MAPPER7_PRG_BANK_SIZE_LOG2 = 15;
const uint32_t MAPPER7_PRG_BANK_MASK = (1 << MAPPER7_PRG_BANK_SIZE_LOG2) - 1;

const uint8_t MAPPER7_PRG_BANK_BITS = 0x7;
const uint8_t MAPPER7_NTB_SELECT = 0x10;

struct mapper7_data {
	uint8_t bank;
};

// ------------------------------------------------------------
// mapper 9 struct/constants

const uint16_t MAPPER9_ADDR_MASK = 0xF000;

const uint16_t MAPPER9_ADDR_PRG_ROM_BANK  = 0xA000;
const uint16_t MAPPER9_ADDR_CHR_ROM_BANK0 = 0xB000;
const uint16_t MAPPER9_ADDR_CHR_ROM_BANK1 = 0xC000;
const uint16_t MAPPER9_ADDR_CHR_ROM_BANK2 = 0xD000;
const uint16_t MAPPER9_ADDR_CHR_ROM_BANK3 = 0xE000;
const uint16_t MAPPER9_ADDR_MIRROR        = 0xF000;

const uint8_t MAPPER9_PRG_BANK_SIZE_LOG2 = 13;  // 8KB granularity
const uint32_t MAPPER9_PRG_BANK_MASK = (1 << MAPPER9_PRG_BANK_SIZE_LOG2) - 1;
const uint8_t MAPPER9_CHR_BANK_SIZE_LOG2 = 12;  // 4KB granularity
const uint32_t MAPPER9_CHR_BANK_MASK = (1 << MAPPER9_CHR_BANK_SIZE_LOG2) - 1;

const uint8_t MAPPER9_MIRROR_MODE_VERTICAL = 0x0;
const uint8_t MAPPER9_MIRROR_MODE_HORIZONTAL = 0x1;

const uint8_t MAPPER9_PRG_BANK_BITS = 0x0F;
const uint8_t MAPPER9_CHR_BANK_BITS = 0x1F;
const uint8_t MAPPER9_MIRROR_BITS = 0x1;

struct mapper9_data {
	uint8_t prg_bank;
	uint8_t chr_bank[4];
	uint8_t mirror;
};

// ------------------------------------------------------------
// mapper 69 struct/constants

const uint16_t MAPPER69_ADDR_MASK = 0xE000;

const uint16_t MAPPER69_ADDR_COMMAND = 0x8000;
const uint16_t MAPPER69_ADDR_PARAMETER = 0xA000;

const uint8_t MAPPER69_PRG_BANK_SIZE_LOG2 = 13;
const uint8_t MAPPER69_CHR_BANK_SIZE_LOG2 = 10;
const uint32_t MAPPER69_PRG_BANK_MASK = (1 << MAPPER69_PRG_BANK_SIZE_LOG2) - 1;
const uint32_t MAPPER69_CHR_BANK_MASK = (1 << MAPPER69_CHR_BANK_SIZE_LOG2) - 1;

const uint8_t MAPPER69_FLAGS_IRQ_ENABLE = 0x01;
const uint8_t MAPPER69_FLAGS_IRQ_COUNTER_ENABLE = 0x80;
const uint8_t MAPPER69_FLAGS_MIRROR_MODE = 0x30;
const uint8_t MAPPER69_MIRROR_MODE_SHIFT = 4;

const uint8_t MAPPER69_MIRROR_MODE_VERTICAL = 0x00;
const uint8_t MAPPER69_MIRROR_MODE_HORIZONTAL = 0x10;
const uint8_t MAPPER69_MIRROR_MODE_ONE_SCREEN_LOWER = 0x20;
const uint8_t MAPPER69_MIRROR_MODE_ONE_SCREEN_UPPER = 0x30;

const uint8_t MAPPER69_PRG_BANK0_RAM_SELECT = 0x40;

struct mapper69_data {
	uint8_t command;
	uint8_t flags;
	uint16_t irq_counter;
	uint8_t prg_bank[4];
	uint8_t chr_bank[8];
};

// ------------------------------------------------------------
// mapper 180 structs/constants

const uint8_t MAPPER180_PRG_BANK_SIZE_LOG2 = 14;
const uint32_t MAPPER180_PRG_BANK_MASK = (1 << MAPPER2_PRG_BANK_SIZE_LOG2) - 1;

struct mapper180_data {
	uint8_t prg_bank;
};


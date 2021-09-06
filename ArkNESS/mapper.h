// Project:     ArkNESS
// File:        mapper.h
// Author:      Kamal Pillai
// Date:        8/9/2021
// Description:	NES memory mappers

#pragma once
#include "nessys.h"

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
};

// ------------------------------------------------------------
// mapper 2 structs/constants

const uint8_t MAPPER2_PRG_BANK_SIZE_LOG2 = 14;
const uint32_t MAPPER2_PRG_BANK_MASK = (1 << MAPPER2_PRG_BANK_SIZE_LOG2) - 1;

struct mapper2_data {
	uint8_t prg_bank;
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
// mapper 7 struct/constants

const uint8_t MAPPER7_PRG_BANK_SIZE_LOG2 = 15;
const uint32_t MAPPER7_PRG_BANK_MASK = (1 << MAPPER7_PRG_BANK_SIZE_LOG2) - 1;

const uint8_t MAPPER7_PRG_BANK_BITS = 0x7;
const uint8_t MAPPER7_NTB_SELECT = 0x10;

struct mapper7_data {
	uint8_t bank;
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
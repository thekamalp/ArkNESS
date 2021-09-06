// Project:     ArkNESS
// File:        mapper.h
// Author:      Kamal Pillai
// Date:        8/9/2021
// Description:	NES memory mappers

#include "mapper.h"

bool mapper_write_null(nessys_t* nes, uint16_t addr, uint8_t data)
{
	return false;
}

bool mapper_update_null(nessys_t* nes)
{
	return false;
}

// mapper 1 functions
void mapper1_update_name_tbl_map(nessys_t* nes)
{
	mapper1_data* m1_data = (mapper1_data*)nes->mapper_data;
	nes->ppu.name_tbl_vert_mirror = (m1_data->control & 0x3) == 2;
	switch (m1_data->control & 0x3) {
	case 0:
		// one screen, lower bank
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem;
		break;
	case 1:
		// one screen, upper bank
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
		break;
	case 2:
		// vertical mirror
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
		break;
	case 3:
		// horizontal mirror
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
		break;
	}
	uint8_t b;
	for (b = NESSYS_CHR_NTB_START_BANK + 4; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_bank[b - 4];
	}
}

void mapper1_update_prg_map(nessys_t* nes)
{
	mapper1_data* m1_data = (mapper1_data*)nes->mapper_data;
	uint32_t offset;
	uint8_t b;
	const uint8_t HALF_BANK = NESSYS_PRG_ROM_START_BANK + (NESSYS_PRG_NUM_BANKS - NESSYS_PRG_ROM_START_BANK) / 2;
	switch (m1_data->control & 0x0c) {
	case 0x0:
	case 0x4:
		// single 32KB bank
		offset = (m1_data->prg_bank & 0xe) << MAPPER1_PRG_BANK_SIZE_LOG2;
		for(b = NESSYS_PRG_ROM_START_BANK; b < NESSYS_PRG_NUM_BANKS; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		break;
	case 0x8:
		// first 16KB of addr space fixed to first 16KB in cart
		// second 16KB programmable
		offset = 0x0;
		for (b = NESSYS_PRG_ROM_START_BANK; b < HALF_BANK; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		offset = (m1_data->prg_bank & 0xf) << MAPPER1_PRG_BANK_SIZE_LOG2;
		for (b = HALF_BANK; b < NESSYS_PRG_NUM_BANKS; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		break;
	case 0xc:
		// first 16KB programmable
		// second 16KB of addr space fixed to last 16KB in cart
		offset = (m1_data->prg_bank & 0xf) << MAPPER1_PRG_BANK_SIZE_LOG2;
		for (b = NESSYS_PRG_ROM_START_BANK; b < HALF_BANK; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		offset = nes->prg_rom_size - 0x4000;
		for (b = HALF_BANK; b < NESSYS_PRG_NUM_BANKS; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		break;
	}
}

void mapper1_update_chr_map(nessys_t* nes)
{
	mapper1_data* m1_data = (mapper1_data*)nes->mapper_data;
	uint32_t offset;
	uint8_t b;
	const uint8_t HALF_BANK = NESSYS_CHR_ROM_START_BANK + (NESSYS_CHR_ROM_END_BANK + 1 - NESSYS_CHR_ROM_START_BANK) / 2;
	uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
	switch (m1_data->control & 0x10) {
	case 0x00:
		// switch 8KB at a time
		offset = (m1_data->chr_bank0 & 0x1e) << MAPPER1_CHR_BANK_SIZE_LOG2;
		for (b = NESSYS_CHR_ROM_START_BANK; b <= NESSYS_CHR_ROM_END_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = base + offset;
			nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
			offset += NESSYS_CHR_BANK_SIZE;
		}
		break;
	case 0x10:
		// switch 2 independent 4KB at a time
		offset = (m1_data->chr_bank0 & 0x1f) << MAPPER1_CHR_BANK_SIZE_LOG2;
		for (b = NESSYS_CHR_ROM_START_BANK; b < HALF_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = base + offset;
			nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
			offset += NESSYS_CHR_BANK_SIZE;
		}
		offset = (m1_data->chr_bank1 & 0x1f) << MAPPER1_CHR_BANK_SIZE_LOG2;
		for (b = HALF_BANK; b <= NESSYS_CHR_ROM_END_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = base + offset;
			nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
			offset += NESSYS_CHR_BANK_SIZE;
		}
		break;
	}
}

void mapper1_reset(nessys_t* nes)
{
	mapper1_data* m1_data = (mapper1_data*)nes->mapper_data;
	m1_data->control = 0x1e;
	mapper1_update_name_tbl_map(nes);
	mapper1_update_prg_map(nes);
	mapper1_update_chr_map(nes);
}

bool mapper1_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER1_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER1_PRG_BANK_MASK) ? 1 : 0);
	uint16_t max_chr_rom_bank_offset = (nes->ppu.chr_rom_size >> MAPPER1_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_rom_size & MAPPER1_CHR_BANK_MASK) ? 1 : 0);
	if (max_chr_rom_bank_offset == 0) max_chr_rom_bank_offset = (nes->ppu.chr_ram_size >> MAPPER1_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_ram_size & MAPPER1_CHR_BANK_MASK) ? 1 : 0);
	uint8_t data_changed = 0x0;
	mapper1_data* m1_data = (mapper1_data*)nes->mapper_data;
	if (data & 0x80) {
		m1_data->shift_reg = MAPPER1_SHIFT_REG_RESET;
	} else {
		bool shift_reg_full = m1_data->shift_reg & 0x1;
		m1_data->shift_reg >>= 1;
		m1_data->shift_reg |= (data & 0x1) << 4;
		if (shift_reg_full) {
			switch (addr & MAPPER1_ADDR_MASK) {
			case MAPPER1_ADDR_CONTROL:
				data_changed = (m1_data->control ^ m1_data->shift_reg) & 0x13;
				m1_data->control = m1_data->shift_reg;
				mapper1_update_name_tbl_map(nes);
				mapper1_update_prg_map(nes);
				mapper1_update_chr_map(nes);
				break;
			case MAPPER1_ADDR_CHR_BANK0:
				data_changed = (m1_data->chr_bank0 ^ m1_data->shift_reg) & 0x1f;
				m1_data->chr_bank0 = m1_data->shift_reg % max_chr_rom_bank_offset;
				mapper1_update_chr_map(nes);
				break;
			case MAPPER1_ADDR_CHR_BANK1:
				data_changed = (m1_data->chr_bank1 ^ m1_data->shift_reg) & 0x1f;
				m1_data->chr_bank1 = m1_data->shift_reg % max_chr_rom_bank_offset;
				mapper1_update_chr_map(nes);
				break;
			case MAPPER1_ADDR_PRG_BANK:
				m1_data->prg_bank = m1_data->shift_reg % max_prg_rom_bank_offset;
				mapper1_update_prg_map(nes);
				break;
			}
			m1_data->shift_reg = MAPPER1_SHIFT_REG_RESET;
		}
	}
	return (data_changed) ? true : false;
}

// mapper 2 functions
bool mapper2_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER2_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER2_PRG_BANK_MASK) ? 1 : 0);
	mapper2_data* m2_data = (mapper2_data*)nes->mapper_data;
	uint8_t data_changed = m2_data->prg_bank;
	m2_data->prg_bank = (data & 0xf) % max_prg_rom_bank_offset;
	data_changed &= m2_data->prg_bank;

	uint32_t offset;
	uint8_t b;
	const uint8_t HALF_BANK = NESSYS_PRG_ROM_START_BANK + (NESSYS_PRG_NUM_BANKS - NESSYS_PRG_ROM_START_BANK) / 2;
	offset = (m2_data->prg_bank & 0xf) << MAPPER2_PRG_BANK_SIZE_LOG2;
	for (b = NESSYS_PRG_ROM_START_BANK; b < HALF_BANK; b++) {
		nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
		nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
		offset += NESSYS_PRG_BANK_SIZE;
	}

	return (data_changed) ? true : false;
}

// mapper 4 functions
void mapper4_update_memmap(nessys_t* nes)
{
	mapper4_data* m4_data = (mapper4_data*)nes->mapper_data;
	// chr map
	uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
	if (m4_data->bank_select & 0x80) {
		nes->ppu.chr_rom_bank[0] = base + (m4_data->r[2] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[1] = base + (m4_data->r[3] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[2] = base + (m4_data->r[4] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[3] = base + (m4_data->r[5] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[4] = base + (m4_data->r[0] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[5] = base + (m4_data->r[0] << MAPPER4_CHR_BANK_SIZE_LOG2) + 0x400;
		nes->ppu.chr_rom_bank[6] = base + (m4_data->r[1] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[7] = base + (m4_data->r[1] << MAPPER4_CHR_BANK_SIZE_LOG2) + 0x400;
	} else {
		nes->ppu.chr_rom_bank[0] = base + (m4_data->r[0] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[1] = base + (m4_data->r[0] << MAPPER4_CHR_BANK_SIZE_LOG2) + 0x400;
		nes->ppu.chr_rom_bank[2] = base + (m4_data->r[1] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[3] = base + (m4_data->r[1] << MAPPER4_CHR_BANK_SIZE_LOG2) + 0x400;
		nes->ppu.chr_rom_bank[4] = base + (m4_data->r[2] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[5] = base + (m4_data->r[3] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[6] = base + (m4_data->r[4] << MAPPER4_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[7] = base + (m4_data->r[5] << MAPPER4_CHR_BANK_SIZE_LOG2);
	}
	// prg map
	if (m4_data->bank_select & 0x40) {
		nes->prg_rom_bank[4] = nes->prg_rom_base + nes->prg_rom_size - 0x4000;
		nes->prg_rom_bank[5] = nes->prg_rom_base + (m4_data->r[7] << MAPPER4_PRG_BANK_SIZE_LOG2);
		nes->prg_rom_bank[6] = nes->prg_rom_base + (m4_data->r[6] << MAPPER4_PRG_BANK_SIZE_LOG2);
		nes->prg_rom_bank[7] = nes->prg_rom_base + nes->prg_rom_size - 0x2000;
	} else {
		nes->prg_rom_bank[4] = nes->prg_rom_base + (m4_data->r[6] << MAPPER4_PRG_BANK_SIZE_LOG2);
		nes->prg_rom_bank[5] = nes->prg_rom_base + (m4_data->r[7] << MAPPER4_PRG_BANK_SIZE_LOG2);
		nes->prg_rom_bank[6] = nes->prg_rom_base + nes->prg_rom_size - 0x4000;
		nes->prg_rom_bank[7] = nes->prg_rom_base + nes->prg_rom_size - 0x2000;
	}
}

void mapper4_eval_irq_timer(nessys_t* nes)
{
	mapper4_data* m4_data = (mapper4_data*)nes->mapper_data;
	nes->mapper_irq_cycles |= (m4_data->irq_enable && m4_data->irq_counter == 0) ? 1 : 0;
//	if (m4_data->irq_enable == 0) nes->mapper_irq_cycles = 0;
//	else if (m4_data->irq_counter == 0) nes->mapper_irq_cycles = 1;
//	else if (nes->scanline < 0 || nes->scanline + m4_data->irq_counter >= 240) nes->mapper_irq_cycles = 0;
//	else nes->mapper_irq_cycles = NESSYS_PPU_CLK_PER_SCANLINE * m4_data->irq_counter - nes->scanline_cycle;
}

bool mapper4_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	bool data_change = false;
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER4_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER4_PRG_BANK_MASK) ? 1 : 0);
	uint16_t max_chr_rom_bank_offset = (nes->ppu.chr_rom_size >> MAPPER4_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_rom_size & MAPPER4_CHR_BANK_MASK) ? 1 : 0);
	if (max_chr_rom_bank_offset == 0) max_chr_rom_bank_offset = (nes->ppu.chr_ram_size >> MAPPER4_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_ram_size & MAPPER4_CHR_BANK_MASK) ? 1 : 0);
	mapper4_data* m4_data = (mapper4_data*)nes->mapper_data;
	uint16_t max_bank_offset = ((m4_data->bank_select & 7) < 6) ? max_chr_rom_bank_offset : max_prg_rom_bank_offset;
	//if (nes->scanline >= 0) printf("rom write scanlien %d a: 0x%x d 0x%x\n", nes->scanline, addr, data);
	switch (addr & MAPPER4_ADDR_MASK) {
	case MAPPER4_ADDR_BANK_SELECT:
		data_change = ((m4_data->bank_select ^ data) & 0xf8)!= 0x0;
		m4_data->bank_select = data;
		mapper4_update_memmap(nes);
		return data_change;
	case MAPPER4_ADDR_BANK_DATA:
		data_change = (m4_data->r[m4_data->bank_select & 0x7] ^ (data & MAPPER4_REG_MASK[m4_data->bank_select & 0x7])) != 0x0;
		data_change = data_change && ((m4_data->bank_select & 0x7) < 6);
		m4_data->r[m4_data->bank_select & 0x7] = (data & MAPPER4_REG_MASK[m4_data->bank_select & 0x7]) % max_bank_offset;
		mapper4_update_memmap(nes);
		//printf("Setting r%d to 0x%x\n", m4_data->bank_select & 0x7, m4_data->r[m4_data->bank_select & 0x7]);
		return data_change;
	case MAPPER4_ADDR_MIRROR:
		if (nes->ppu.mem_4screen == NULL) {
			nes->ppu.name_tbl_vert_mirror = !(data & 0x1);
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
			if (nes->ppu.name_tbl_vert_mirror) {
				nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
				nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
			} else {
				nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
				nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
			}
			uint8_t b;
			for (b = NESSYS_CHR_NTB_START_BANK + 4; b <= NESSYS_CHR_NTB_END_BANK; b++) {
				nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_bank[b - 4];
			}
			return true;
		}
		break;
	case MAPPER4_ADDR_PRG_RAM_PROTECT:
		m4_data->prg_ram_protect = data;
		break;
	case MAPPER4_ADDR_IRQ_LATCH:
		//printf("mapper4 irq latch set from %d to %d\n", m4_data->irq_latch, data);
		m4_data->irq_latch = data;
		break;
	case MAPPER4_ADDR_IRQ_RELOAD:
		m4_data->counter_write_pending = 1;
		break;
	case MAPPER4_ADDR_IRQ_DISABLE:
		m4_data->irq_enable = 0;
		nes->mapper_irq_cycles = 0;
		//mapper4_eval_irq_timer(nes);
		break;
	case MAPPER4_ADDR_IRQ_ENABLE:
		m4_data->irq_enable = 1;
		//mapper4_eval_irq_timer(nes);
		break;
	}
	return false;
}

bool mapper4_update(nessys_t* nes)
{
	mapper4_data* m4_data = (mapper4_data*)nes->mapper_data;
	bool ppu_a12_toggle = (~m4_data->last_upper_ppu_addr & (nes->ppu.mem_addr >> 8) & 0x10);
	//if(ppu_a12_toggle) printf("a12 toggle: 0x%x last 0x%x new 0x%x\n", ppu_a12_toggle, m4_data->last_upper_ppu_addr, nes->ppu.mem_addr);
	bool eval_irq_timer = false;
	if (ppu_a12_toggle) {// || ((nes->ppu.reg[1] & 0x18) && nes->scanline >= 0 && (nes->scanline != m4_data->last_scanline))) {
		//if(ppu_a12_toggle) printf("ppu[1]: 0x%x wr_pend %d latch 0x%x counter 0x%x scanline: %d scan_cycle: %d\n", nes->ppu.reg[1], m4_data->counter_write_pending, m4_data->irq_latch, m4_data->irq_counter, nes->scanline, nes->scanline_cycle);
		//if (m4_data->irq_counter == 0) m4_data->irq_counter = m4_data->irq_latch;
		m4_data->irq_counter = (m4_data->irq_counter == 0 || m4_data->counter_write_pending) ? m4_data->irq_latch : m4_data->irq_counter - 1;
		//eval_irq_timer = (ppu_a12_toggle || m4_data->counter_write_pending || (m4_data->last_scanline < 0)) && (nes->scanline >= 0);
		m4_data->counter_write_pending = 0;
		//if (eval_irq_timer)
		mapper4_eval_irq_timer(nes);
	}
	m4_data->last_scanline = nes->scanline;
	m4_data->last_upper_ppu_addr = (uint8_t)(nes->ppu.mem_addr >> 8);
	return false;
}

// mapper7 functions
void mapper7_update_memmap(nessys_t* nes)
{
	mapper7_data* m7_data = (mapper7_data*)nes->mapper_data;
	// first do nametable
	uint8_t b;
	uint16_t ntb_offset = (m7_data->bank & MAPPER7_NTB_SELECT) ? 0x400 : 0x0;
	for (b = NESSYS_CHR_NTB_START_BANK; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank[b] = nes->ppu.mem + ntb_offset;
	}
	// prg rom mapping
	uint8_t* base = nes->prg_rom_base;
	uint32_t offset = (m7_data->bank & MAPPER7_PRG_BANK_BITS) << MAPPER7_PRG_BANK_SIZE_LOG2;
	for (b = 0; b < 4; b++) {
		nes->prg_rom_bank[4 + b] = base + offset;
		offset += NESSYS_PRG_BANK_SIZE;
	}
}

bool mapper7_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	mapper7_data* m7_data = (mapper7_data*)nes->mapper_data;
	uint8_t data_changed = m7_data->bank;
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER7_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER7_PRG_BANK_MASK) ? 1 : 0);
	m7_data->bank = (data & MAPPER7_PRG_BANK_BITS) % max_prg_rom_bank_offset;
	m7_data->bank |= (data & MAPPER7_NTB_SELECT);
	data_changed ^= m7_data->bank;
	data_changed &= MAPPER7_NTB_SELECT;  // only nametable mapping can affect rendering
	mapper7_update_memmap(nes);
	return (data_changed) ? true : false;
}

// mapper69 functions
void mapper69_update_nametable(nessys_t* nes)
{
	mapper69_data* m69_data = (mapper69_data*)nes->mapper_data;
	uint8_t mirror_mode = m69_data->flags & MAPPER69_FLAGS_MIRROR_MODE;
	nes->ppu.name_tbl_vert_mirror = (mirror_mode == MAPPER69_MIRROR_MODE_VERTICAL);
	switch (mirror_mode) {
	case MAPPER69_MIRROR_MODE_VERTICAL:
	case MAPPER69_MIRROR_MODE_HORIZONTAL:
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
		break;
	case MAPPER69_MIRROR_MODE_ONE_SCREEN_LOWER:
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem;
		break;
	case MAPPER69_MIRROR_MODE_ONE_SCREEN_UPPER:
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
		break;
	}
	if (nes->ppu.name_tbl_vert_mirror) {
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
	} else {
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
	}
	uint8_t b;
	for (b = NESSYS_CHR_NTB_START_BANK + 4; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_bank[b - 4];
	}
}

void mapper69_update_chr_map(nessys_t* nes)
{
	mapper69_data* m69_data = (mapper69_data*)nes->mapper_data;
	uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
	uint8_t b;
	for (b = 0; b < 8; b++) {
		nes->ppu.chr_rom_bank[b] = base + (m69_data->chr_bank[b] << MAPPER69_CHR_BANK_SIZE_LOG2);
	}
}

void mapper69_update_prg_map(nessys_t* nes)
{
	mapper69_data* m69_data = (mapper69_data*)nes->mapper_data;
	uint8_t* base = (m69_data->prg_bank[0] & MAPPER69_PRG_BANK0_RAM_SELECT) ? nes->prg_ram_base : nes->prg_rom_base;
	nes->prg_rom_bank[3] = base + ((m69_data->prg_bank[0] & 0x3f) << MAPPER69_PRG_BANK_SIZE_LOG2);
	base = nes->prg_rom_base;
	uint8_t b;
	for (b = 1; b < 4; b++) {
		nes->prg_rom_bank[3 + b] = base + (m69_data->prg_bank[b] << MAPPER69_PRG_BANK_SIZE_LOG2);
	}
}

bool mapper69_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	uint8_t data_change = 0;
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER69_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER69_PRG_BANK_MASK) ? 1 : 0);
	uint16_t max_chr_rom_bank_offset = (nes->ppu.chr_rom_size >> MAPPER69_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_rom_size & MAPPER69_CHR_BANK_MASK) ? 1 : 0);
	uint16_t max_chr_ram_bank_offset = (nes->ppu.chr_ram_size >> MAPPER69_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_ram_size & MAPPER69_CHR_BANK_MASK) ? 1 : 0);
	if (max_chr_rom_bank_offset == 0) max_chr_rom_bank_offset = max_chr_ram_bank_offset;
	mapper69_data* m69_data = (mapper69_data*)nes->mapper_data;
	switch (addr & MAPPER69_ADDR_MASK) {
	case MAPPER69_ADDR_COMMAND:
		m69_data->command = data & 0xf;
		break;
	case MAPPER69_ADDR_PARAMETER:
		switch (m69_data->command) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			data_change = m69_data->chr_bank[m69_data->command];
			m69_data->chr_bank[m69_data->command] = data % max_chr_rom_bank_offset;
			data_change ^= m69_data->chr_bank[m69_data->command];
			mapper69_update_chr_map(nes);
			break;
		case 8:
			m69_data->prg_bank[0] = (data & 0x3f) % max_prg_rom_bank_offset;
			m69_data->prg_bank[0] |= (data & 0xc0);
			mapper69_update_prg_map(nes);
			break;
		case 9:
		case 10:
		case 11:
			m69_data->prg_bank[m69_data->command - 8] = (data & 0x3f) % max_prg_rom_bank_offset;
			mapper69_update_prg_map(nes);
			break;
		case 12:
			m69_data->flags &= ~MAPPER69_FLAGS_MIRROR_MODE;
			m69_data->flags |= (data << MAPPER69_MIRROR_MODE_SHIFT) & MAPPER69_FLAGS_MIRROR_MODE;
			mapper69_update_nametable(nes);
			break;
		case 13:
			nes->mapper_irq_cycles = 0;
			m69_data->flags &= ~(MAPPER69_FLAGS_IRQ_COUNTER_ENABLE | MAPPER69_FLAGS_IRQ_ENABLE);
			m69_data->flags |= data & (MAPPER69_FLAGS_IRQ_COUNTER_ENABLE | MAPPER69_FLAGS_IRQ_ENABLE);
			break;
		case 14:
			m69_data->irq_counter &= 0xFF00;
			m69_data->irq_counter |= data;
			break;
		case 15:
			m69_data->irq_counter &= 0x00FF;
			m69_data->irq_counter |= ((uint16_t)data) << 8;
			break;
		}
		break;
	}
	return (data_change) ? true : false;
}

bool mapper69_update(nessys_t* nes)
{
	mapper69_data* m69_data = (mapper69_data*)nes->mapper_data;
	if (m69_data->flags & MAPPER69_FLAGS_IRQ_COUNTER_ENABLE) {
		if (m69_data->irq_counter < nes->cpu_cycle_inc && (m69_data->flags & MAPPER69_FLAGS_IRQ_ENABLE)) {
			nes->mapper_irq_cycles = 1;
		}
		m69_data->irq_counter -= nes->cpu_cycle_inc;
	}
	return false;
}

void mapper69_reset(nessys_t* nes)
{
	mapper69_data* m69_data = (mapper69_data*)nes->mapper_data;
	m69_data->prg_bank[0] = 0xc0;
	mapper69_update_chr_map(nes);
	mapper69_update_prg_map(nes);
	mapper69_update_nametable(nes);
}

bool nessys_init_mapper(nessys_t* nes)
{
	bool success = true;
	switch (nes->mapper_id) {
	case 0:
		nes->mapper_write = mapper_write_null;
		nes->mapper_update = mapper_update_null;
		nes->mapper_data = NULL;
		break;
	case 1:
		nes->mapper_write = mapper1_write;
		nes->mapper_update = mapper_update_null;
		nes->mapper_data = malloc(sizeof(mapper1_data));
		memset(nes->mapper_data, 0, sizeof(mapper1_data));
		mapper1_reset(nes);
		break;
	case 2:
		nes->mapper_write = mapper2_write;
		nes->mapper_update = mapper_update_null;
		nes->mapper_data = malloc(sizeof(mapper2_data));
		memset(nes->mapper_data, 0, sizeof(mapper2_data));
		break;
	case 4:
		nes->mapper_write = mapper4_write;
		nes->mapper_update = mapper4_update;
		nes->mapper_data = malloc(sizeof(mapper4_data));
		memset(nes->mapper_data, 0, sizeof(mapper4_data));
		break;
	case 7:
		nes->mapper_write = mapper7_write;
		nes->mapper_update = mapper_update_null;
		nes->mapper_data = malloc(sizeof(mapper7_data));
		memset(nes->mapper_data, 0, sizeof(mapper7_data));
		mapper7_update_memmap(nes);
		break;
	case 69:
		nes->mapper_write = mapper69_write;
		nes->mapper_update = mapper69_update;
		nes->mapper_data = malloc(sizeof(mapper69_data));
		memset(nes->mapper_data, 0, sizeof(mapper69_data));
		mapper69_reset(nes);
		break;
	default:
		printf("Uknown mapper id: %d\n", nes->mapper_id);
		success = false;
		break;
	}
	return success;
}

void nessys_cleanup_mapper(nessys_t* nes)
{
	nes->mapper_write = NULL;
	nes->mapper_update = NULL;
	if (nes->mapper_data) {
		free(nes->mapper_data);
		nes->mapper_data = NULL;
	}
}

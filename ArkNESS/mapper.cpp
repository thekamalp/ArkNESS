// Project:     ArkNESS
// File:        mapper.h
// Author:      Kamal Pillai
// Date:        8/9/2021
// Description:	NES memory mappers

#include "mapper.h"

// ------------------------------------------------------------
// default functions
// used by mappers with specific specializations
uint32_t mapper_null_setup(nessys_t* nes, uint32_t phase)
{
	return NESSYS_MAPPER_SETUP_DEFAULT;
}

void mapper_null_cpu_setup(nessys_t* nes)
{ }

void mapper_audio_tick_null(nessys_t* nes)
{ }

int16_t mapper_gen_sound_null(nessys_t* nes)
{
	return 0;
}

uint8_t* mapper_read_null(nessys_t* nes, uint16_t addr)
{
	return NULL;
}

bool mapper_write_null(nessys_t* nes, uint16_t addr, uint8_t data)
{
	return false;
}

bool mapper_update_null(nessys_t* nes)
{
	return false;
}

// ------------------------------------------------------------
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
		offset = (m1_data->prg_bank & 0x1e) << MAPPER1_PRG_BANK_SIZE_LOG2;
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
		if (nes->prg_rom_size >= 0x40000) {
			offset = (m1_data->prg_bank & 0x10) ? 0x40000 : 0x0;
		}
		for (b = NESSYS_PRG_ROM_START_BANK; b < HALF_BANK; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		offset = (m1_data->prg_bank & 0x1f) << MAPPER1_PRG_BANK_SIZE_LOG2;
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
		offset = (m1_data->prg_bank & 0x1f) << MAPPER1_PRG_BANK_SIZE_LOG2;
		for (b = NESSYS_PRG_ROM_START_BANK; b < HALF_BANK; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		offset = nes->prg_rom_size - 0x4000;
		if (nes->prg_rom_size >= 0x40000) {
			if ((m1_data->prg_bank & 0x10) == 0x0) offset = 0x40000 - 0x4000;
		}
		for (b = HALF_BANK; b < NESSYS_PRG_NUM_BANKS; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			offset += NESSYS_PRG_BANK_SIZE;
			if (offset >= nes->prg_rom_size) offset &= ~MAPPER1_PRG_BANK_MASK;
		}
		break;
	}
	offset = (m1_data->prg_ram_bank << MAPPER1_PRG_BANK_SIZE_LOG2);
	nes->prg_rom_bank[NESSYS_PRG_RAM_START_BANK] = nes->prg_ram_base + offset;
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
	nes->mapper_flags |= NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE;
}

bool mapper1_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER1_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER1_PRG_BANK_MASK) ? 1 : 0);
	uint16_t max_prg_ram_bank_offset = (nes->prg_ram_size >> MAPPER1_PRG_BANK_SIZE_LOG2) + ((nes->prg_ram_size & MAPPER1_PRG_BANK_MASK) ? 1 : 0);
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
				data_changed = m1_data->chr_bank0;
				m1_data->chr_bank0 = m1_data->shift_reg % max_chr_rom_bank_offset;
				if (nes->ppu.chr_ram_size == 0x2000) {
					if (m1_data->control & 0x10) {
						m1_data->chr_bank0 &= ~0x1;
						m1_data->chr_bank1 |= 0x1;
					}
					m1_data->prg_ram_bank = ((m1_data->shift_reg >> 2) & 0x3) % max_prg_ram_bank_offset;
					if (max_prg_rom_bank_offset >= 0x20) {
						m1_data->prg_bank &= ~0x10;
						m1_data->prg_bank |= m1_data->shift_reg & 0x10;
						mapper1_update_prg_map(nes);
					}
				}
				data_changed ^= m1_data->chr_bank0;
				mapper1_update_chr_map(nes);
				break;
			case MAPPER1_ADDR_CHR_BANK1:
				data_changed = m1_data->chr_bank1;
				m1_data->chr_bank1 = m1_data->shift_reg % max_chr_rom_bank_offset;
				if (nes->ppu.chr_ram_size == 0x2000) {
					if (m1_data->control & 0x10) {
						m1_data->chr_bank0 &= ~0x1;
						m1_data->chr_bank1 |= 0x1;
					} else {
						m1_data->prg_ram_bank = ((m1_data->shift_reg >> 2) & 0x3) % max_prg_ram_bank_offset;
						if (max_prg_rom_bank_offset >= 0x20) {
							m1_data->prg_bank &= ~0x10;
							m1_data->prg_bank |= m1_data->shift_reg & 0x10;
							mapper1_update_prg_map(nes);
						}
					}
				}
				data_changed ^= m1_data->chr_bank1;
				mapper1_update_chr_map(nes);
				break;
			case MAPPER1_ADDR_PRG_BANK:
				m1_data->prg_bank &= 0x10;
				m1_data->prg_bank |= (m1_data->shift_reg & 0xf) % max_prg_rom_bank_offset;
				mapper1_update_prg_map(nes);
				break;
			}
			m1_data->shift_reg = MAPPER1_SHIFT_REG_RESET;
		}
	}
	return (data_changed) ? true : false;
}

// ------------------------------------------------------------
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

// ------------------------------------------------------------
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
		m4_data->irq_counter = 0;
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

// ------------------------------------------------------------
// mapper5 functions
void mapper5_update_exp_chr_rom(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	if (m5_data->exp_ram_mode == 0x1) {
		uint8_t* base = NULL;
		uint32_t size = 0;
		if (nes->ppu.chr_rom_base) {
			base = nes->ppu.chr_rom_base;
			size = nes->ppu.chr_rom_size;
		} else {
			base = nes->ppu.chr_ram_base;
			size = nes->ppu.chr_ram_size;
		}
		uint32_t offset = (256 * 1024 * m5_data->msb_chr_bank) % size;
		if (offset + 256 * 1024 > size) {
			size -= offset;
			//uint8_t buffer[256 * 1024];
			//memcpy(buffer, base + offset, size);
			//nes->gfx->k2SurfUpdate(nes->st_exp_pattern, buffer);
		//} else {
			//nes->gfx->k2SurfUpdate(nes->st_exp_pattern, base + offset);
		}
		void* buffer = nes->surf_upload_exp_pattern[nes->surf_exp_cpu_version]->MapForWrite();
		memcpy(buffer, base + offset, size);
		nes->surf_upload_exp_pattern[nes->surf_exp_cpu_version]->Unmap();
		nes->cmd_buf->TransitionResource(nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
		nes->cmd_buf->UploadImage(nes->surf_upload_exp_pattern[nes->surf_exp_cpu_version], nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource());
		nes->cmd_buf->TransitionResource(nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource(), k3resourceState::SHADER_RESOURCE);
		nes->cmd_buf->SetShaderView(1, nes->surf_exp_pattern[nes->surf_exp_gpu_version]);
		nes->surf_exp_cpu_version++;
		nes->surf_exp_gpu_version++;
		if (nes->surf_exp_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->surf_exp_cpu_version = 0;
		if (nes->surf_exp_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->surf_exp_gpu_version = 0;
	}
	m5_data->exp_surf_dirty = false;
}

void mapper5_reset(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	m5_data->prg_mode = 0x3;
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER5_PRG_BANK_BASE_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER5_PRG_BANK_BASE_MASK) ? 1 : 0);
	m5_data->prg_bank[4] = max_prg_rom_bank_offset - 1;
	if (nes->prg_ram_size < 0x10000) {
		if (nes->prg_ram_size) free(nes->prg_ram_base);
		nes->prg_ram_size = 0x10000;
		nes->prg_ram_base = (uint8_t*) malloc(nes->prg_ram_size);
	}
	nes->apu.reg = m5_data->mem;
	nes->prg_rom_bank[NESSYS_APU_REG_START_BANK] = nes->apu.reg;
	nes->prg_rom_bank_mask[NESSYS_APU_REG_START_BANK] = NESSYS_PRG_MEM_MASK;

	nes->cmd_buf->Reset();
	mapper5_update_exp_chr_rom(nes);

	void* buffer = nes->surf_upload_exp_pattern[nes->surf_exp_cpu_version]->MapForWrite();
	memcpy(buffer, nes->prg_rom_base, 256 * 1024);
	nes->surf_upload_exp_pattern[nes->surf_exp_cpu_version]->Unmap();
	nes->cmd_buf->TransitionResource(nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->UploadImage(nes->surf_upload_exp_pattern[nes->surf_exp_cpu_version], nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource());
	//nes->cmd_buf->TransitionResource(nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource(), k3resourceState::SHADER_RESOURCE);
	//nes->cmd_buf->SetShaderView(1, nes->surf_exp_pattern[nes->surf_exp_gpu_version]);
	nes->surf_exp_cpu_version++;
	nes->surf_exp_gpu_version++;
	if (nes->surf_exp_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->surf_exp_cpu_version = 0;
	if (nes->surf_exp_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->surf_exp_gpu_version = 0;
	//nes->gfx->k2SurfUpdate(nes->st_exp_pattern, nes->prg_rom_base);
	nes->cmd_buf->Close();
	nes->gfx->SubmitCmdBuf(nes->cmd_buf);

}

void mapper5_audio_tick(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	nessys_apu_env_tick(&(m5_data->pulse[0].env));
	nessys_apu_env_tick(&(m5_data->pulse[1].env));
	nessys_apu_sweep_tick(&(m5_data->pulse[0]));
	nessys_apu_sweep_tick(&(m5_data->pulse[1]));
}

int16_t mapper5_gen_sound(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	uint8_t pulse_sound = nessys_apu_gen_pulse(&(m5_data->pulse[0])) +
		nessys_apu_gen_pulse(&(m5_data->pulse[1]));
	uint8_t pcm_sound = 0;// nessys_apu_gen_dmc(nes);
	float pulse_soundf = (pulse_sound) ?
		95.88f / ((8128.0f / pulse_sound) + 100.0f) : 0.0f;
	float other_soundf = (pcm_sound) ?
		159.79f / (1.0f / ((pcm_sound / 22638.0f))) : 0.0f;
	float soundf = pulse_soundf + other_soundf;
	int16_t sound = (int16_t)(16384.0f * (soundf - 0.5f));
	return sound;
}

void mapper5_update_nametable_map(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	uint32_t i;
	for (i = 0; i < 4; i++) {
		switch ((m5_data->ntb_map >> (2 * i)) & 0x3) {
		case 0:
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + i] = nes->ppu.mem;
			break;
		case 1:
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + i] = nes->ppu.mem + 0x400;
			break;
		case 2:
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + i] = m5_data->mem + MAPPER5_ADDR_EXP_RAM_START_OFFSET;
			break;
		case 3:
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + i] = m5_data->mem + MAPPER5_ADDR_FILL_DATA_OFFSET;
			break;
		}
	}
	uint8_t b;
	for (b = NESSYS_CHR_NTB_START_BANK + 4; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_bank[b - 4];
	}
}

void mapper5_update_prg_map(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER5_PRG_BANK_BASE_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER5_PRG_BANK_BASE_MASK) ? 1 : 0);
	uint16_t max_prg_ram_bank_offset = (nes->prg_ram_size >> MAPPER5_PRG_BANK_BASE_SIZE_LOG2) + ((nes->prg_ram_size & MAPPER5_PRG_BANK_BASE_MASK) ? 1 : 0);
	bool bank_uses_ram[5];
	uint16_t bank[5];
	uint32_t i;
	for (i = 0; i < 5; i++) {
		bank_uses_ram[i] = (m5_data->prg_bank[i] & 0x80) ? false : true;
		bank[i] = m5_data->prg_bank[i] & 0x7f;
	}
	bank_uses_ram[0] = true;  // 0x6000-0x7fff must use ram
	bank_uses_ram[4] = false; // 0xe000-0xffff muse use rom
	switch (m5_data->prg_mode) {
	case 0:
		bank[1] = bank[4] & ~0x3;
		bank[2] = bank[1] + 1;
		bank[3] = bank[1] + 2;
		bank[4] = bank[1] + 3;
		bank_uses_ram[1] = false;
		bank_uses_ram[2] = false;
		bank_uses_ram[3] = false;
		break;
	case 1:
		bank[3] = bank[4] & ~0x1;
		bank[4] = bank[3] + 1;
		bank_uses_ram[3] = false;
		// no break - on purpose; mode 1 and 2 do the same thing for bank[1]
	case 2:
		bank[1] = bank[2] & ~0x1;
		bank[2] = bank[1] + 1;
		bank_uses_ram[1] = bank_uses_ram[2];
		break;
	}
	uint8_t* base;
	uint16_t max_bank_offset;
	for (i = 0; i < 5; i++) {
		base = (bank_uses_ram[i]) ? nes->prg_ram_base : nes->prg_rom_base;
		max_bank_offset = (bank_uses_ram[i]) ? max_prg_ram_bank_offset : max_prg_rom_bank_offset;
		bank[i] %= max_bank_offset;
		nes->prg_rom_bank[3 + i] = base + (((uint32_t)bank[i]) << MAPPER5_PRG_BANK_BASE_SIZE_LOG2);
	}
}

void mapper5_update_chr_map(nessys_t* nes, bool upper_reg_touched)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	uint16_t max_chr_rom_bank_offset = (nes->ppu.chr_rom_size >> MAPPER5_CHR_BANK_BASE_SIZE_LOG2) + ((nes->ppu.chr_rom_size & MAPPER5_CHR_BANK_BASE_MASK) ? 1 : 0);
	if (max_chr_rom_bank_offset == 0) max_chr_rom_bank_offset = (nes->ppu.chr_ram_size >> MAPPER5_CHR_BANK_BASE_SIZE_LOG2) + ((nes->ppu.chr_ram_size & MAPPER5_CHR_BANK_BASE_MASK) ? 1 : 0);
	uint32_t bank[8];
	uint32_t i;
	uint32_t bank_shift = 0;
	uint32_t index_mask = 0x7;
	if ((nes->ppu.reg[0] & 0x20) && upper_reg_touched) {
		// in 8x16 sprite mdoe, if the upper regs were last touched, map in the upper regs
		bank_shift = 8;
		index_mask = 0x3;
	}
	for (i = 0; i < 8; i++) {
		bank[i] = m5_data->chr_bank[bank_shift + (i & index_mask)];
	}
	bank_shift = (3 - m5_data->chr_mode);
	index_mask = (1 << bank_shift) - 1;
	uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
	for (i = 0; i < 8; i++) {
		bank[i] = bank[i | index_mask] << bank_shift;
		bank[i] += (i & index_mask);
		bank[i] %= max_chr_rom_bank_offset;
		bank[i] <<= MAPPER5_CHR_BANK_BASE_SIZE_LOG2;
		nes->ppu.chr_rom_bank[i] = base + bank[i];
	}
}

uint32_t mapper5_bg_setup(nessys_t* nes, uint32_t phase)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	uint32_t mapper_setup = NESSYS_MAPPER_SETUP_DEFAULT;

	if(m5_data->exp_surf_dirty) mapper5_update_exp_chr_rom(nes);

	int i, c, index;
	float palette[4 * NESSYS_PPU_PAL_SIZE];
	for (i = 0; i < NESSYS_PPU_PAL_SIZE; i++) {
		for (c = 0; c < 3; c++) {
			index = ((i & 0x3) == 0) ? 0 : i;
			palette[4 * i + c] = NESSYS_PPU_PALETTE[3 * nes->ppu.pal[index] + c];
		}
		palette[4 * i + 3] = ((i & 0x3) == 0) ? 0.0f : 1.0f;
	}

	nessys_cbuffer_exp_t* cb_exp_data = static_cast<nessys_cbuffer_exp_t*>(nes->cb_upload[nes->cb_main_cpu_version]->MapForWrite(sizeof(nessys_cbuffer_exp_t)));

	if (m5_data->vsplit_mode & 0x80) {
		// vertical split is enabled
		int32_t split_pos = (m5_data->vsplit_mode & 0x1f) << 3;
		if (split_pos) split_pos -= nes->ppu.scroll[0] & 0x7;
		split_pos <<= 1;
		split_pos += 64;
		if (phase == 0) {
			nes->scissor_right_x = split_pos;
			// save away the scroll information
			m5_data->scroll_save[0] = nes->ppu.scroll[0];
			m5_data->scroll_save[1] = nes->ppu.scroll[1];
			m5_data->scroll_save_y = nes->ppu.scroll_y;
		} else {
			nes->scissor_left_x = split_pos;
			nes->scissor_right_x = 576;
		}
		uint32_t vsplit_is_right = (m5_data->vsplit_mode >> 6) & 0x1;
		if (phase == vsplit_is_right) {
			// map vsplit chr banks
			uint16_t max_chr_rom_bank_offset = (nes->ppu.chr_rom_size >> MAPPER5_CHR_BANK_BASE_SIZE_LOG2) + ((nes->ppu.chr_rom_size & MAPPER5_CHR_BANK_BASE_MASK) ? 1 : 0);
			if (max_chr_rom_bank_offset == 0) max_chr_rom_bank_offset = (nes->ppu.chr_ram_size >> MAPPER5_CHR_BANK_BASE_SIZE_LOG2) + ((nes->ppu.chr_ram_size & MAPPER5_CHR_BANK_BASE_MASK) ? 1 : 0);
			uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
			uint32_t i, bank;
			for (i = 0; i < 8; i++) {
				bank = m5_data->vsplit_bank;
				bank <<= 2;
				bank &= ~0x3;
				bank += (i & 0x3);
				bank %= max_chr_rom_bank_offset;
				bank <<= MAPPER5_CHR_BANK_BASE_SIZE_LOG2;
				nes->ppu.chr_rom_bank[i] = base + bank;
			}
			// map expanded ram to name table
			for (i = 0; i < 4; i++) {
				nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + i] = m5_data->mem + MAPPER5_ADDR_EXP_RAM_START_OFFSET;
			}
			// use vscroll, and there is no hscroll
			nes->ppu.scroll[0] = 0;
			nes->ppu.scroll[1] = m5_data->vsplit_scroll;
			nes->ppu.scroll_y = m5_data->vsplit_scroll;
		} else {
			// use default chr banks
			mapper5_update_chr_map(nes, true);
			// restore nametable
			mapper5_update_nametable_map(nes);
			// restore scroll
			nes->ppu.scroll[0] = m5_data->scroll_save[0];
			nes->ppu.scroll[1] = m5_data->scroll_save[1];
			nes->ppu.scroll_y = m5_data->scroll_save_y;
		}
		// if phase is one, we're done; otherwise we're not
		mapper_setup |= (phase) ? NESSYS_MAPPER_SETUP_DEFAULT : NESSYS_MAPPER_SETUP_DRAW_INCOMPLETE;
	} else {
		mapper5_update_chr_map(nes, true);
	}
	if (m5_data->exp_ram_mode == 0x01) {
		mapper_setup |= NESSYS_MAPPER_SETUP_CUSTOM;

		for (i = 0; i < 4; i++) {
			memcpy(cb_exp_data->exp_nametable + (i << 8), m5_data->mem + MAPPER5_ADDR_EXP_RAM_START_OFFSET, 1024);
			index += 1024;
		}
		//nes->gfx->k2CBUpdate(nes->cb_exp_nametable, buffer);

		k3rect scissor;
		scissor.x = nes->scissor_left_x;
		scissor.y = nes->scissor_top_y;
		scissor.width = nes->scissor_right_x - nes->scissor_left_x;
		scissor.height = nes->scissor_bottom_y - nes->scissor_top_y;
		nes->cmd_buf->SetScissor(&scissor);
		nes->cmd_buf->SetGfxState(nes->st_exp_background);

		//nes->gfx->k2SetScissorRect(nes->scissor_left_x, nes->scissor_top_y, nes->scissor_right_x, nes->scissor_bottom_y);
		//nes->gfx->k2AttachBlendState(nes->bs_mask);
		//nes->gfx->k2AttachDepthState(nes->ds_normal);
		//nes->gfx->k2AttachShaderGroup(nes->sg_exp_background);
		//nes->gfx->k2AttachConstantGroup(nes->cg_exp_background);
		//nes->gfx->k2AttachTextureGroup(nes->tg_exp_background);
	}
	memcpy(cb_exp_data->cbuf.ppu, nes->ppu.reg, 4 * sizeof(uint32_t));
	memcpy(cb_exp_data->cbuf.sprite, nes->ppu.oam, 4 * 16 * sizeof(uint32_t));
	index = NESSYS_CHR_ROM_WIN_MIN;
	for (i = 0; i < 8; i++) {
		memcpy(cb_exp_data->cbuf.pattern + (i << 8), nessys_ppu_mem(nes, index), 1024);
		index += 1024;
	}
	memcpy(cb_exp_data->cbuf.palette, palette, 4 * 32 * sizeof(float));
	//nes->gfx->k2CBUpdate(nes->cb_ppu, &(nes->ppu.reg));
	// enable background rendering
	// update constants
	// nametable update
	index = NESSYS_CHR_NTB_WIN_MIN;
	for (i = 0; i < 4; i++) {
		memcpy(cb_exp_data->cbuf.nametable + (i << 8), nessys_ppu_mem(nes, index), 1024);
		index += 1024;
	}
	//nes->gfx->k2CBUpdate(nes->cb_nametable, buffer);
	nes->cb_upload[nes->cb_main_cpu_version]->Unmap();

	nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->UploadBuffer(nes->cb_upload[nes->cb_main_cpu_version], nes->cb_main[nes->cb_main_gpu_version]->GetResource());
	nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::SHADER_BUFFER);
	nes->cmd_buf->SetConstantBuffer(0, nes->cb_main[nes->cb_main_gpu_version]);
	//nes->cmd_buf->TransitionResource(nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource(), k3resourceState::SHADER_RESOURCE);
	//nes->cmd_buf->SetShaderView(1, nes->surf_exp_pattern[nes->surf_exp_gpu_version]);
	nes->cb_main_cpu_version++;
	nes->cb_main_gpu_version++;
	if (nes->cb_main_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->cb_main_cpu_version = 0;
	if (nes->cb_main_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->cb_main_gpu_version = 0;

	return mapper_setup;
}

uint32_t mapper5_sprite_setup(nessys_t* nes, uint32_t phase)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	if (m5_data->vsplit_mode & 0x80) {
		// restore chr banks
		nes->ppu.scroll[0] = m5_data->scroll_save[0];
		nes->ppu.scroll[1] = m5_data->scroll_save[1];
		nes->ppu.scroll_y = m5_data->scroll_save_y;
		// restore nametable
		mapper5_update_nametable_map(nes);
	}
	mapper5_update_chr_map(nes, false);

	int i, c, index;
	float palette[4 * NESSYS_PPU_PAL_SIZE];
	for (i = 0; i < NESSYS_PPU_PAL_SIZE; i++) {
		for (c = 0; c < 3; c++) {
			index = ((i & 0x3) == 0) ? 0 : i;
			palette[4 * i + c] = NESSYS_PPU_PALETTE[3 * nes->ppu.pal[index] + c];
		}
		palette[4 * i + 3] = ((i & 0x3) == 0) ? 0.0f : 1.0f;
	}

	nessys_cbuffer_t* cb_data = static_cast<nessys_cbuffer_t*>(nes->cb_upload[nes->cb_main_cpu_version]->MapForWrite(sizeof(nessys_cbuffer_exp_t)));
	memcpy(cb_data->ppu, nes->ppu.reg, 4 * sizeof(uint32_t));
	memcpy(cb_data->sprite, nes->ppu.oam, 4 * 16 * sizeof(uint32_t));
	index = NESSYS_CHR_ROM_WIN_MIN;
	for (i = 0; i < 8; i++) {
		memcpy(cb_data->pattern + (i << 8), nessys_ppu_mem(nes, index), 1024);
		index += 1024;
	}
	memcpy(cb_data->palette, palette, 4 * 32 * sizeof(float));
	index = NESSYS_CHR_NTB_WIN_MIN;
	for (i = 0; i < 4; i++) {
		memcpy(cb_data->nametable + (i << 8), nessys_ppu_mem(nes, index), 1024);
		index += 1024;
	}
	nes->cb_upload[nes->cb_main_cpu_version]->Unmap();

	nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->UploadBuffer(nes->cb_upload[nes->cb_main_cpu_version], nes->cb_main[nes->cb_main_gpu_version]->GetResource());
	nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::SHADER_BUFFER);
	nes->cmd_buf->SetConstantBuffer(0, nes->cb_main[nes->cb_main_gpu_version]);
	nes->cb_main_cpu_version++;
	nes->cb_main_gpu_version++;
	if (nes->cb_main_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->cb_main_cpu_version = 0;
	if (nes->cb_main_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->cb_main_gpu_version = 0;

	return NESSYS_MAPPER_SETUP_DEFAULT;
}

void mapper5_cpu_setup(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	mapper5_update_chr_map(nes, m5_data->upper_reg_touched);
}

uint8_t* mapper5_read(nessys_t* nes, uint16_t addr)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	switch (addr) {
	case MAPPER5_ADDR_SCANLINE_IRQ_STATUS:
		m5_data->scanline_irq_pend_clear = 0x80;
		break;
	}
	return NULL;
}

bool mapper5_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	uint16_t data_change = 0x0;
	uint16_t result;
	if (addr >= NESSYS_PRG_RAM_START) {
		uint8_t bank = (addr >> MAPPER5_PRG_BANK_BASE_SIZE_LOG2) - 3;
		bool is_ram = false;
		switch(m5_data->prg_mode) {
		case 3:
			is_ram = (bank == 0) || (bank <= 3 && !(m5_data->prg_bank[bank] & 0x80));
			break;
		case 2:
			is_ram = (bank == 3 && !(m5_data->prg_bank[3] & 0x80));
		case 1:
			is_ram |= (bank == 0 || (bank <= 2 && !(m5_data->prg_bank[2] & 0x80)));
		case 0:
			is_ram |= (bank == 0);
			break;
		}
		if (is_ram) {
			uint16_t b, o;
			*nessys_mem(nes, addr, &b, &o) = data;
		}
	} else if (addr >= MAPPER5_ADDR_EXP_RAM_START && addr < MAPPER5_ADDR_EXP_RAM_START + MAPPER5_EXP_RAM_SIZE && m5_data->exp_ram_mode < 0x3) {
		// access expanded ram
		uint16_t mem_addr = addr - NESSYS_APU_WIN_MIN;
		if (m5_data->exp_ram_mode < 0x2) {
			data_change = m5_data->mem[mem_addr] ^ data;
		}
		m5_data->mem[mem_addr] = data;
	} else {
		if (addr < MAPPER5_ADDR_PRG_MODE) {
			// audio register
			nessys_gen_sound(nes);
		}
		// access other registers
		switch (addr) {
		case MAPPER5_ADDR_PULSE0_CTRL:
			m5_data->pulse[0].env.volume = data & 0xf;
			m5_data->pulse[0].env.flags &= ~(NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
			m5_data->pulse[0].env.flags |= data & (NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
			m5_data->pulse[0].duty = NESSYS_APU_PULSE_DUTY_TABLE[(data >> 6) & 0x3];
			break;
		case MAPPER5_ADDR_PULSE0_SWEEP:
			// no sweeper units in MMC5
			break;
		case MAPPER5_ADDR_PULSE0_TIMER:
			m5_data->pulse[0].period &= 0xFF00;
			m5_data->pulse[0].period |= data;
			m5_data->pulse[0].sweep_orig_period = m5_data->pulse[0].period;
			break;
		case MAPPER5_ADDR_PULSE0_RELOAD:
			m5_data->pulse[0].period &= 0x00FF;
			m5_data->pulse[0].period |= ((uint16_t)data << 8) & 0x700;
			m5_data->pulse[0].sweep_orig_period = m5_data->pulse[0].period;
			m5_data->pulse[0].length = NESSYS_APU_PULSE_LENGTH_TABLE[(data >> 3) & 0x1f];
			m5_data->pulse[0].duty_phase = 0;
			m5_data->pulse[0].cur_time_frac = 0;
			m5_data->pulse[0].env.flags |= NESSYS_APU_PULSE_FLAG_ENV_START;
			break;
		case MAPPER5_ADDR_PULSE1_CTRL:
			m5_data->pulse[1].env.volume = data & 0xf;
			m5_data->pulse[1].env.flags &= ~(NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
			m5_data->pulse[1].env.flags |= data & (NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
			m5_data->pulse[1].duty = NESSYS_APU_PULSE_DUTY_TABLE[(data >> 6) & 0x3];
			break;
		case MAPPER5_ADDR_PULSE1_SWEEP:
			// no sweeper units in MMC5
			break;
		case MAPPER5_ADDR_PULSE1_TIMER:
			m5_data->pulse[1].period &= 0xFF00;
			m5_data->pulse[1].period |= data;
			m5_data->pulse[1].sweep_orig_period = m5_data->pulse[1].period;
			break;
		case MAPPER5_ADDR_PULSE1_RELOAD:
			m5_data->pulse[1].period &= 0x00FF;
			m5_data->pulse[1].period |= ((uint16_t)data << 8) & 0x700;
			m5_data->pulse[1].sweep_orig_period = m5_data->pulse[1].period;
			m5_data->pulse[1].length = NESSYS_APU_PULSE_LENGTH_TABLE[(data >> 3) & 0x1f];
			m5_data->pulse[1].duty_phase = 0;
			m5_data->pulse[1].cur_time_frac = 0;
			m5_data->pulse[1].env.flags |= NESSYS_APU_PULSE_FLAG_ENV_START;
		case MAPPER5_ADDR_PCM_CTRL:
			// TODO
			break;
		case MAPPER5_ADDR_PCM_DATA:
			// TODO
			break;
		case MAPPER5_ADDR_PRG_MODE:
			m5_data->prg_mode = data & 0x3;
			mapper5_update_prg_map(nes);
			break;
		case MAPPER5_ADDR_CHR_MODE:
			data_change = m5_data->chr_mode;
			m5_data->chr_mode = data & 0x3;
			data_change ^= m5_data->chr_mode;
			mapper5_update_chr_map(nes, m5_data->upper_reg_touched);
			break;
		case MAPPER5_ADDR_PRG_RAM_PROTECT1:
			m5_data->prg_ram_protect1 = data & 0x3;
			break;
		case MAPPER5_ADDR_PRG_RAM_PROTECT2:
			m5_data->prg_ram_protect2 = data & 0x3;
			break;
		case MAPPER5_ADDR_EXP_RAM_MODE:
			m5_data->exp_surf_dirty = true;
			m5_data->exp_ram_mode = data & 0x3;
			break;
		case MAPPER5_ADDR_NTB_MAP:
			data_change = m5_data->ntb_map;
			m5_data->ntb_map = data;
			data_change ^= m5_data->ntb_map;
			mapper5_update_nametable_map(nes);
			break;
		case MAPPER5_ADDR_FILL_MODE_TILE:
			data_change = m5_data->mem[MAPPER5_ADDR_FILL_DATA_OFFSET];
			data_change ^= data;
			memset(m5_data->mem + MAPPER5_ADDR_FILL_DATA_OFFSET, data, 960);
			break;
		case MAPPER5_ADDR_FILL_MODE_COLOR:
			// replicate bottom 2 bits up through the byte
			data &= 0x3;
			data |= data << 2;
			data |= data << 4;
			data_change = m5_data->mem[MAPPER5_ADDR_FILL_DATA_OFFSET + 960];
			data_change ^= data;
			memset(m5_data->mem + MAPPER5_ADDR_FILL_DATA_OFFSET + 960, data, 64);
			break;
		case MAPPER5_ADDR_PRG_BANK0:
		case MAPPER5_ADDR_PRG_BANK1:
		case MAPPER5_ADDR_PRG_BANK2:
		case MAPPER5_ADDR_PRG_BANK3:
		case MAPPER5_ADDR_PRG_BANK4:
			m5_data->prg_bank[addr - MAPPER5_ADDR_PRG_BANK0] = data;
			mapper5_update_prg_map(nes);
			break;
		case MAPPER5_ADDR_CHR_BANK0:
		case MAPPER5_ADDR_CHR_BANK1:
		case MAPPER5_ADDR_CHR_BANK2:
		case MAPPER5_ADDR_CHR_BANK3:
		case MAPPER5_ADDR_CHR_BANK4:
		case MAPPER5_ADDR_CHR_BANK5:
		case MAPPER5_ADDR_CHR_BANK6:
		case MAPPER5_ADDR_CHR_BANK7:
		case MAPPER5_ADDR_CHR_BANK8:
		case MAPPER5_ADDR_CHR_BANK9:
		case MAPPER5_ADDR_CHR_BANKA:
		case MAPPER5_ADDR_CHR_BANKB:
			data_change = m5_data->chr_bank[addr - MAPPER5_ADDR_CHR_BANK0];
			data_change ^= data | (((uint16_t)m5_data->msb_chr_bank) << 8);
			m5_data->chr_bank[addr - MAPPER5_ADDR_CHR_BANK0] = data;
			m5_data->chr_bank[addr - MAPPER5_ADDR_CHR_BANK0] |= ((uint16_t)m5_data->msb_chr_bank) << 8;
			m5_data->upper_reg_touched = (addr >= MAPPER5_ADDR_CHR_BANK8);
			mapper5_update_chr_map(nes, m5_data->upper_reg_touched);
			break;
		case MAPPER5_ADDR_UPPER_CHR_BANK:
			data_change = m5_data->msb_chr_bank;
			m5_data->msb_chr_bank = data & 0x3;
			data_change ^= m5_data->msb_chr_bank;
			if (m5_data->exp_ram_mode == 0x01 && data_change != 0)
				m5_data->exp_surf_dirty = true;
			else data_change = 0x0;
			break;
		case MAPPER5_ADDR_VSPLIT_MODE:
			data_change = m5_data->vsplit_mode;
			data_change ^= data;
			m5_data->vsplit_mode = data;
			break;
		case MAPPER5_ADDR_VSPLIT_SCROLL:
			data_change = m5_data->vsplit_scroll;
			data_change ^= data;
			m5_data->vsplit_scroll = data;
			break;
		case MAPPER5_ADDR_VSPLIT_BANK:
			data_change = m5_data->vsplit_bank;
			data_change ^= data;
			m5_data->vsplit_bank = data;
			break;
		case MAPPER5_ADDR_SCANLINE_IRQ_CMP:
			m5_data->scanline_irq_cmp = data;
			break;
		case MAPPER5_ADDR_SCANLINE_IRQ_STATUS:
			m5_data->scanline_irq_status = data;
			break;
		case MAPPER5_ADDR_MULT0:
		case MAPPER5_ADDR_MULT1:
			m5_data->mult[addr - MAPPER5_ADDR_MULT0] = data;
			result = (uint16_t)m5_data->mult[0] * (uint16_t)m5_data->mult[1];
			m5_data->mem[MAPPER5_ADDR_MULT0_OFFSET] = (uint8_t)result;
			m5_data->mem[MAPPER5_ADDR_MULT1_OFFSET] = (uint8_t)(result >> 8);
			break;
		case MAPPER5_ADDR_CL3_SL3_CTRL:
			m5_data->cl3_sl3_ctrl = data;
			break;
		case MAPPER5_ADDR_CL3_SL3_STATUS:
			m5_data->cl3_sl3_status = data;
			break;
		case MAPPER5_ADDR_TIMER_IRQ_LSB:
			m5_data->timer_irq &= 0xFF00;
			m5_data->timer_irq |= data;
			break;
		case MAPPER5_ADDR_TIMER_IRQ_MSB:
			m5_data->timer_irq &= 0x00FF;
			m5_data->timer_irq |= ((uint16_t)data) << 8;
			break;
		}
	}
	return (data_change) ? true : false;
}

bool mapper5_update(nessys_t* nes)
{
	mapper5_data* m5_data = (mapper5_data*)nes->mapper_data;
	nes->mapper_irq_cycles = 0;
	if (nes->scanline != m5_data->last_scanline) {
		m5_data->mem[MAPPER5_ADDR_SCANLINE_IRQ_STATUS_OFFSET] = ((nes->scanline >= 0) << 6);
		m5_data->mem[MAPPER5_ADDR_SCANLINE_IRQ_STATUS_OFFSET] |= 
			((m5_data->last_scanline < m5_data->scanline_irq_cmp) &&
				(m5_data->scanline_irq_cmp != 0) &&
				(nes->scanline >= m5_data->scanline_irq_cmp)) << 7;
		m5_data->last_scanline = nes->scanline;
	}
	nes->mapper_irq_cycles |= ((m5_data->scanline_irq_status &
		m5_data->mem[MAPPER5_ADDR_SCANLINE_IRQ_STATUS_OFFSET]) >> 7) & 0x1;
	m5_data->mem[MAPPER5_ADDR_SCANLINE_IRQ_STATUS_OFFSET] &= ~(m5_data->scanline_irq_pend_clear);
	m5_data->scanline_irq_pend_clear = 0x0;

	return false;
}

// ------------------------------------------------------------
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

// ------------------------------------------------------------
// mapper9 functions
void mapper9_update_chr_map(nessys_t* nes, uint32_t latch0, uint32_t latch1)
{
	mapper9_data* m9_data = (mapper9_data*)nes->mapper_data;
	uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
	uint8_t b;
	for (b = 0; b < 4; b++) {
		nes->ppu.chr_rom_bank[b]     = base + (m9_data->chr_bank[0 | latch0] << MAPPER9_CHR_BANK_SIZE_LOG2) + (b << NESSYS_CHR_BANK_SIZE_LOG2);
		nes->ppu.chr_rom_bank[b + 4] = base + (m9_data->chr_bank[2 | latch1] << MAPPER9_CHR_BANK_SIZE_LOG2) + (b << NESSYS_CHR_BANK_SIZE_LOG2);
	}
}

void mapper9_update_prg_map(nessys_t* nes)
{
	mapper9_data* m9_data = (mapper9_data*)nes->mapper_data;
	uint8_t* base = nes->prg_rom_base;
	uint32_t offset = (m9_data->prg_bank & MAPPER9_PRG_BANK_BITS) << MAPPER9_PRG_BANK_SIZE_LOG2;
	nes->prg_rom_bank[4] = base + offset;
}

void mapper9_update_nametable(nessys_t* nes)
{
	mapper9_data* m9_data = (mapper9_data*)nes->mapper_data;
	nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
	nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
	if (m9_data->mirror == MAPPER9_MIRROR_MODE_VERTICAL) {
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

bool mapper9_write(nessys_t* nes, uint16_t addr, uint8_t data)
{
	uint8_t data_change = 0;
	uint16_t max_prg_rom_bank_offset = (nes->prg_rom_size >> MAPPER9_PRG_BANK_SIZE_LOG2) + ((nes->prg_rom_size & MAPPER9_PRG_BANK_MASK) ? 1 : 0);
	uint16_t max_chr_rom_bank_offset = (nes->ppu.chr_rom_size >> MAPPER9_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_rom_size & MAPPER9_CHR_BANK_MASK) ? 1 : 0);
	uint16_t max_chr_ram_bank_offset = (nes->ppu.chr_ram_size >> MAPPER9_CHR_BANK_SIZE_LOG2) + ((nes->ppu.chr_ram_size & MAPPER9_CHR_BANK_MASK) ? 1 : 0);
	if (max_chr_rom_bank_offset == 0) max_chr_rom_bank_offset = max_chr_ram_bank_offset;
	uint8_t chr_bank_num = ((addr & MAPPER9_ADDR_MASK) - MAPPER9_ADDR_CHR_ROM_BANK0) >> 12;
	mapper9_data* m9_data = (mapper9_data*)nes->mapper_data;
	switch (addr & MAPPER9_ADDR_MASK) {
	case MAPPER9_ADDR_PRG_ROM_BANK:
		m9_data->prg_bank = (data & MAPPER9_PRG_BANK_BITS) % max_prg_rom_bank_offset;
		mapper9_update_prg_map(nes);
		break;
	case MAPPER9_ADDR_CHR_ROM_BANK0:
	case MAPPER9_ADDR_CHR_ROM_BANK1:
	case MAPPER9_ADDR_CHR_ROM_BANK2:
	case MAPPER9_ADDR_CHR_ROM_BANK3:
		data_change = m9_data->chr_bank[chr_bank_num];
		m9_data->chr_bank[chr_bank_num] = (data & MAPPER9_CHR_BANK_BITS) % max_chr_rom_bank_offset;
		data_change ^= m9_data->chr_bank[chr_bank_num];
		mapper9_update_chr_map(nes, 0, 0);
		break;
	case MAPPER9_ADDR_MIRROR:
		data_change = m9_data->mirror;
		m9_data->mirror = data & MAPPER9_MIRROR_BITS;
		data_change ^= m9_data->mirror;
		mapper9_update_nametable(nes);
		break;
	}
	return (data_change) ? true : false;
}

uint32_t mapper9_bg_setup(nessys_t* nes, uint32_t phase)
{
	mapper9_data* m9_data = (mapper9_data*)nes->mapper_data;
	mapper9_update_chr_map(nes, 1, 1);
	nes->cb_main_gpu_version = (nes->cb_main_gpu_version == 0) ? nessys_t::NUM_GPU_VERSIONS - 1 : nes->cb_main_gpu_version - 1;
	nessys_cbuffer_m9_t* cb_data = static_cast<nessys_cbuffer_m9_t*>(nes->cb_upload[nes->cb_main_cpu_version]->MapForWrite(sizeof(nessys_cbuffer_m9_t)));
	uint32_t i, j, index = NESSYS_CHR_ROM_WIN_MIN;
	for (i = 0; i < 8; i++) {
		memcpy(cb_data->alt_pattern + (i << 8), nessys_ppu_mem(nes, index), 1024);
		index += 1024;
	}
	uint32_t r, c;
	uint32_t latch = 0;
	uint8_t tile_num;
	uint32_t background_latch = 0;
	memset(cb_data->nametable_msb, 0, 4 * 128);
	for (j = 0; j < 31; j++) {  // 31 rows
		r = (nes->ppu.reg[0] & 0x2) << 4;
		r |= nes->ppu.scroll_y >> 3;
		r += j;
		if ((r & 0x1f) >= 30) r += 2;
		r &= 0x3f;
		c = (nes->ppu.reg[0] & 0x1) << 5;
		c |= nes->ppu.scroll[0] >> 3;
		for (i = 0; i < 33; i++, c++) {  // 33 columns
			c &= 0x3f;
			index = ((r & 0x20) << 6) | ((c & 0x20) << 5) | ((r & 0x1f) << 5) | (c & 0x1f);
			cb_data->nametable_msb[index >> 5] |= latch << (index & 0x1f);
			index |= NESSYS_CHR_NTB_WIN_MIN;
			tile_num = *nessys_ppu_mem(nes, index);
			if (i == (nes->ppu.oam[3] >> 3) && j == (nes->ppu.oam[0] >> 3)) {
				background_latch = latch;
			}
			if (tile_num == 0xFE) latch = 0x1;
			else if (tile_num == 0xFD) latch = 0x0;
		}
	}

	memset(cb_data->sprite_msb, 0, 32 * 240);
	uint32_t num_pattern_start = 0;
	uint32_t insert;
	uint32_t sprite_pattern_start[NESSYS_PPU_NUM_SPRITES];
	for (i = 0; i < NESSYS_PPU_NUM_SPRITES; i++) {
		// check if the sprite is visiable, and uses tile index 0xFD or 0xFE
		if (nes->ppu.oam[4 * i + 0] < 240 && (nes->ppu.oam[4 * i + 1] == 0xFD || nes->ppu.oam[4 * i + 1] == 0xFE)) {
			sprite_pattern_start[num_pattern_start] = nes->ppu.oam[4 * i + 0];
			sprite_pattern_start[num_pattern_start] <<= 8;
			sprite_pattern_start[num_pattern_start] |= nes->ppu.oam[4 * i + 3];
			sprite_pattern_start[num_pattern_start] |= (nes->ppu.oam[4 * i + 1] == 0xFE) ? 0x80000000 : 0x0;
			num_pattern_start++;
			for (j = num_pattern_start - 1; j > 0; j--) {
				if ((sprite_pattern_start[j] & 0xFFFF) < (sprite_pattern_start[j - 1] & 0xFFFF)) {
					insert = sprite_pattern_start[j - 1];
					sprite_pattern_start[j - 1] = sprite_pattern_start[j];
					sprite_pattern_start[j] = insert;
				} else {
					break;
				}
			}
		}
	}
	uint32_t sprite0_index = nes->ppu.oam[0];
	sprite0_index <<= 8;
	sprite0_index |= nes->ppu.oam[3];
	uint32_t sprite0_latch = 0;
	for (i = 0; i < num_pattern_start; i++) {
		uint32_t start_pix = sprite_pattern_start[i];
		uint32_t end_pix = (i == num_pattern_start - 1) ? (239 << 8) | 255 : (sprite_pattern_start[i + 1] & 0xFFFF);
		if (start_pix & 0x80000000) {
			start_pix &= 0xFFFF;
			if (sprite0_index >= start_pix && sprite0_index < end_pix) sprite0_latch = 1;
			insert = static_cast<uint32_t>(~0x0) << (start_pix & 0x1f);
			for (j = (start_pix >> 5); j < (end_pix >> 5); j++) {
				cb_data->sprite_msb[j] |= insert;
				insert = ~0x0;
			}
			insert &= ~(static_cast<uint32_t>(~0x0) << (end_pix & 0x1f));
			cb_data->sprite_msb[j] |= insert;
		}
	}
	nes->cb_upload[nes->cb_main_cpu_version]->Unmap();

	nes->cmd_buf->SetGfxState(nes->st_m9_background);
	nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->UploadBuffer(nes->cb_upload[nes->cb_main_cpu_version], nes->cb_main[nes->cb_main_gpu_version]->GetResource(), sizeof(nessys_cbuffer_t));
	nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::SHADER_BUFFER);
	nes->cmd_buf->SetConstantBuffer(0, nes->cb_main[nes->cb_main_gpu_version]);
	k3rect scissor;
	scissor.x = nes->scissor_left_x;
	scissor.y = nes->scissor_top_y;
	scissor.width = nes->scissor_right_x - nes->scissor_left_x;
	scissor.height = nes->scissor_bottom_y - nes->scissor_top_y;
	nes->cmd_buf->SetScissor(&scissor);

	nes->cb_main_cpu_version++;
	nes->cb_main_gpu_version++;
	if (nes->cb_main_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->cb_main_cpu_version = 0;
	if (nes->cb_main_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->cb_main_gpu_version = 0;
	mapper9_update_chr_map(nes, sprite0_latch, background_latch);

	return NESSYS_MAPPER_SETUP_CUSTOM;
}

uint32_t mapper9_sprite_setup(nessys_t* nes, uint32_t phase)
{
	mapper9_data* m9_data = (mapper9_data*)nes->mapper_data;

	nes->cmd_buf->SetGfxState(nes->st_m9_sprite);

	k3rect scissor;
	scissor.x = nes->scissor_left_x;
	scissor.y = nes->scissor_top_y;
	scissor.width = nes->scissor_right_x - nes->scissor_left_x;
	scissor.height = nes->scissor_bottom_y - nes->scissor_top_y;
	nes->cmd_buf->SetStencilRef(8);
	nes->cmd_buf->SetScissor(&scissor);
	nes->cmd_buf->SetVertexBuffer(0, nes->vb_sprite);

	return NESSYS_MAPPER_SETUP_CUSTOM;

}

void mapper9_cpu_setup(nessys_t* nes)
{
	mapper9_update_chr_map(nes, 0, 0);
}


// ------------------------------------------------------------
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

// ------------------------------------------------------------
// general mapper load and unload functions
bool nessys_init_mapper(nessys_t* nes)
{
	bool success = true;
	nes->mapper_flags = 0;
	nes->mapper_bg_setup = mapper_null_setup;
	nes->mapper_sprite_setup = mapper_null_setup;
	nes->mapper_cpu_setup = mapper_null_cpu_setup;
	nes->mapper_audio_tick = mapper_audio_tick_null;
	nes->mapper_gen_sound = mapper_gen_sound_null;
	nes->mapper_read = mapper_read_null;
	nes->mapper_write = mapper_write_null;
	nes->mapper_update = mapper_update_null;
	nes->mapper_data = NULL;
	switch (nes->mapper_id) {
	case 0:
		break;
	case 1:
		nes->mapper_write = mapper1_write;
		nes->mapper_data = malloc(sizeof(mapper1_data));
		memset(nes->mapper_data, 0, sizeof(mapper1_data));
		mapper1_reset(nes);
		break;
	case 2:
		nes->mapper_write = mapper2_write;
		nes->mapper_data = malloc(sizeof(mapper2_data));
		memset(nes->mapper_data, 0, sizeof(mapper2_data));
		break;
	case 4:
		nes->mapper_write = mapper4_write;
		nes->mapper_update = mapper4_update;
		nes->mapper_data = malloc(sizeof(mapper4_data));
		memset(nes->mapper_data, 0, sizeof(mapper4_data));
		break;
	case 5:
		nes->mapper_bg_setup = mapper5_bg_setup;
		nes->mapper_sprite_setup = mapper5_sprite_setup;
		nes->mapper_cpu_setup = mapper5_cpu_setup;
		nes->mapper_audio_tick = mapper5_audio_tick;
		nes->mapper_gen_sound = mapper5_gen_sound;
		nes->mapper_read = mapper5_read;
		nes->mapper_write = mapper5_write;
		nes->mapper_update = mapper5_update;
		nes->mapper_data = malloc(sizeof(mapper5_data));
		memset(nes->mapper_data, 0, sizeof(mapper5_data));
		mapper5_reset(nes);
		break;
	case 7:
		nes->mapper_write = mapper7_write;
		nes->mapper_data = malloc(sizeof(mapper7_data));
		memset(nes->mapper_data, 0, sizeof(mapper7_data));
		mapper7_update_memmap(nes);
		break;
	case 9:
		nes->mapper_bg_setup = mapper9_bg_setup;
		nes->mapper_sprite_setup = mapper9_sprite_setup;
		nes->mapper_cpu_setup = mapper9_cpu_setup;
		nes->mapper_write = mapper9_write;
		nes->mapper_data = malloc(sizeof(mapper9_data));
		memset(nes->mapper_data, 0, sizeof(mapper9_data));
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
	if (nes->apu.reg != nes->apu.reg_mem) {
		// if mapper allocated new space for apu registers, then copy back that data
		// to default space, and point to that
		memcpy(nes->apu.reg_mem, nes->apu.reg, NESSYS_APU_SIZE);
		nes->apu.reg = nes->apu.reg_mem;
		nes->prg_rom_bank[NESSYS_APU_REG_START_BANK] = nes->apu.reg;
		nes->prg_rom_bank_mask[NESSYS_APU_REG_START_BANK] = NESSYS_APU_MASK;
	}
	nes->mapper_write = NULL;
	nes->mapper_update = NULL;
	if (nes->mapper_data) {
		free(nes->mapper_data);
		nes->mapper_data = NULL;
	}
}

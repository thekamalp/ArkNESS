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

// mapper 4 functions
void mapper4_update_memmap(nessys_t* nes)
{
	mapper4_data* m4_data = (mapper4_data*)nes->mapper_data;
	// chr map
	if (m4_data->bank_select & 0x80) {
		nes->ppu.chr_rom_bank[0] = nes->ppu.chr_rom_base + (m4_data->r[2] << 10);
		nes->ppu.chr_rom_bank[1] = nes->ppu.chr_rom_base + (m4_data->r[3] << 10);
		nes->ppu.chr_rom_bank[2] = nes->ppu.chr_rom_base + (m4_data->r[4] << 10);
		nes->ppu.chr_rom_bank[3] = nes->ppu.chr_rom_base + (m4_data->r[5] << 10);
		nes->ppu.chr_rom_bank[4] = nes->ppu.chr_rom_base + (m4_data->r[0] << 10);
		nes->ppu.chr_rom_bank[5] = nes->ppu.chr_rom_base + (m4_data->r[0] << 10) + 0x400;
		nes->ppu.chr_rom_bank[6] = nes->ppu.chr_rom_base + (m4_data->r[1] << 10);
		nes->ppu.chr_rom_bank[7] = nes->ppu.chr_rom_base + (m4_data->r[1] << 10) + 0x400;
	} else {
		nes->ppu.chr_rom_bank[0] = nes->ppu.chr_rom_base + (m4_data->r[0] << 10);
		nes->ppu.chr_rom_bank[1] = nes->ppu.chr_rom_base + (m4_data->r[0] << 10) + 0x400;
		nes->ppu.chr_rom_bank[2] = nes->ppu.chr_rom_base + (m4_data->r[1] << 10);
		nes->ppu.chr_rom_bank[3] = nes->ppu.chr_rom_base + (m4_data->r[1] << 10) + 0x400;
		nes->ppu.chr_rom_bank[4] = nes->ppu.chr_rom_base + (m4_data->r[2] << 10);
		nes->ppu.chr_rom_bank[5] = nes->ppu.chr_rom_base + (m4_data->r[3] << 10);
		nes->ppu.chr_rom_bank[6] = nes->ppu.chr_rom_base + (m4_data->r[4] << 10);
		nes->ppu.chr_rom_bank[7] = nes->ppu.chr_rom_base + (m4_data->r[5] << 10);
	}
	// prg map
	if (m4_data->bank_select & 0x40) {
		nes->prg_rom_bank[4] = nes->prg_rom_base + nes->prg_rom_size - 0x4000;
		nes->prg_rom_bank[5] = nes->prg_rom_base + (m4_data->r[7] << 13);
		nes->prg_rom_bank[6] = nes->prg_rom_base + (m4_data->r[6] << 13);
		nes->prg_rom_bank[7] = nes->prg_rom_base + nes->prg_rom_size - 0x2000;
	} else {
		nes->prg_rom_bank[4] = nes->prg_rom_base + (m4_data->r[6] << 13);
		nes->prg_rom_bank[5] = nes->prg_rom_base + (m4_data->r[7] << 13);
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
	mapper4_data* m4_data = (mapper4_data*)nes->mapper_data;
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
		m4_data->r[m4_data->bank_select & 0x7] = data & MAPPER4_REG_MASK[m4_data->bank_select & 0x7];
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

bool nessys_init_mapper(nessys_t* nes)
{
	bool success = true;
	switch (nes->mapper_id) {
	case 0:
		nes->mapper_write = mapper_write_null;
		nes->mapper_update = mapper_update_null;
		nes->mapper_data = NULL;
		break;
	case 4:
		nes->mapper_write = mapper4_write;
		nes->mapper_update = mapper4_update;
		nes->mapper_data = malloc(sizeof(mapper4_data));
		memset(nes->mapper_data, 0, sizeof(mapper4_data));
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

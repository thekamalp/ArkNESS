// Project:     ArkNES
// File:        nessys.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description:	NES system functions

#include "nessys.h"

void nessys_init(nessys_t* nes)
{
	memset(nes, 0, sizeof(nessys_t));
	nes->ppu.mem_addr_mask = 0xFF00;
	k2image::k2Initialize();
	k2error_SetHandler(k2error_StdOutHandler);
	k2win::k2SetGfxType(K2GFX_DIRECTX_11);
	nes->win = k2win::CreateWindowedWithFormat("ArkNES", 0, 0, 640, 480, K2FMT_RGBA8_UNORM, K2FMT_UNKNOWN);
	nes->win->k2SetDataPtr(nes);
	nes->gfx = nes->win->k2GetGfx();
	nes->rg_win = nes->win->k2GetRenderGroup();

	// initialize render states
	k2blendstate_desc bs_desc = { 0 };
	k2depthstate_desc ds_desc = { 0 };
	k2rasterizerstate_desc rs_desc = { 0 };

	bs_desc.alphaToCoverageEnable = false;
	bs_desc.independentBlendEnable = false;
	bs_desc.rt[0].blendEnable = false;
	bs_desc.rt[0].writemask = 0x0f;
	nes->bs_normal = nes->gfx->k2CreateBlendState(&bs_desc);

	bs_desc.alphaToCoverageEnable = true;
	nes->bs_mask = nes->gfx->k2CreateBlendState(&bs_desc);

	ds_desc.zEnable = false;
	ds_desc.zWriteEnable = false;
	ds_desc.zFunc = K2COMPARE_ALWAYS;
	ds_desc.stencilEnable = false;
	nes->ds_none = nes->gfx->k2CreateDepthState(&ds_desc);

	rs_desc.fillMode = K2FILL_SOLID;
	rs_desc.cullMode = K2CULL_BACK;
	rs_desc.frontCCW = false;
	rs_desc.zClipEnable = false;
	rs_desc.scissorEnable = false;
	rs_desc.multisampleEnable = true;
	rs_desc.aaLineEnable = true;
	nes->rs_normal = nes->gfx->k2CreateRasterizerState(&rs_desc);

	// initialize shaders
	k2vertexshader* vs;
	k2pixelshader* ps;
	//vs = nes->gfx->k2CreateVertexShaderFromFile("..\\ArkNES\\screen_vs.hlsl", "main");
	//ps = nes->gfx->k2CreatePixelShaderFromFile("..\\ArkNES\\background_ps.hlsl", "main");
	vs = nes->gfx->k2CreateVertexShaderFromObj("..\\Debug\\screen_vs.cso");
	ps = nes->gfx->k2CreatePixelShaderFromObj("..\\Debug\\background_ps.cso");
	nes->sg_background = nes->gfx->k2CreateShaderGroup(vs, ps);
	//vs = nes->gfx->k2CreateVertexShaderFromFile("..\\ArkNES\\sprite_vs.hlsl", "main");
	//ps = nes->gfx->k2CreatePixelShaderFromFile("..\\ArkNES\\sprite_ps.hlsl", "main");
	vs = nes->gfx->k2CreateVertexShaderFromObj("..\\Debug\\sprite_vs.cso");
	ps = nes->gfx->k2CreatePixelShaderFromObj("..\\Debug\\sprite_ps.cso");
	nes->sg_sprite = nes->gfx->k2CreateShaderGroup(vs, ps);

	// vertex group
	k2inputelement inelm[1];
	inelm[0].semantic_name = "POSITION";
	inelm[0].semantic_index = 0;
	inelm[0].format = K2FMT_RG32_FLOAT;
	inelm[0].slot = 0;
	inelm[0].byte_offset = 0;

	k2inputformat* infmt = nes->gfx->k2CreateInputFormat(1, inelm);

	float vb_data[] = { 0.0f, 0.0f,
						256.0f, 0.0f,
						0.0f, 240.0f,
						256.0f, 240.0f };
	k2vertexbuffer* vb = nes->gfx->k2CreateVertexBuffer(8, 4, 0, vb_data);
	nes->vg_fullscreen = nes->gfx->k2CreateVertexGroup(K2TRIANGLESTRIP, 1, infmt, NULL, &vb);

	infmt = nes->gfx->k2CreateInputFormat(1, inelm);
	vb_data[2] = 8.0f;
	vb_data[5] = 8.0f;
	vb_data[6] = 8.0f;
	vb_data[7] = 8.0f;
	vb = nes->gfx->k2CreateVertexBuffer(8, 4, 0, vb_data);
	nes->vg_sprite = nes->gfx->k2CreateVertexGroup(K2TRIANGLESTRIP, 1, infmt, NULL, &vb);

	nes->cb_nametable = nes->gfx->k2CreateConstantBuffer(1024, K2ACCESS_UPDATE, NULL);
	nes->cb_pattern = nes->gfx->k2CreateConstantBuffer(512 * 16, K2ACCESS_UPDATE, NULL);
	nes->cb_palette = nes->gfx->k2CreateConstantBuffer(256, K2ACCESS_UPDATE, NULL);
	nes->cb_ppu = nes->gfx->k2CreateConstantBuffer(16, K2ACCESS_UPDATE, NULL);
	nes->cb_sprite = nes->gfx->k2CreateConstantBuffer(256, K2ACCESS_UPDATE, NULL);

	k2constantelement celem[] = { 
		{"nametable", nes->cb_nametable},
		{"pattern", nes->cb_pattern},
		{"palette", nes->cb_palette},
		{"sprite", nes->cb_sprite} };
	nes->cg_background = nes->gfx->k2CreateConstantGroup(nes->sg_background, 3, celem);

	celem[0].slot_name = "ppu";
	celem[0].constant_buffer = nes->cb_ppu;
	nes->cg_sprite = nes->gfx->k2CreateConstantGroup(nes->sg_sprite, 4, celem);

}

void nessys_power_cycle(nessys_t* nes)
{
	nes->reg.p = 0x34;
	nes->reg.a = 0;
	nes->reg.x = 0;
	nes->reg.y = 0;
	nes->reg.s = 0xfd;
	nes->apu.reg[0x17] = 0x0;
	nes->apu.reg[0x15] = 0x0;
	memset(nes->apu.reg, 0, 14); // regs 0x0 to 0x13
	memset(nes->ppu.reg, 0, 8);  // clear all 8 regs
	memset(nes->sysmem, 0, NESSYS_RAM_SIZE);
	uint16_t bank, offset;
	nes->reg.pc = *((uint16_t*)nessys_mem(nes, NESSYS_RST_VECTOR, &bank, &offset));
}

void nessys_reset(nessys_t* nes)
{
	nes->reg.s -= 3;
	nes->reg.p |= 0x04;
	nes->apu.reg[0x15] = 0x0;
	nes->ppu.reg[0x0] = 0x0;
	nes->ppu.reg[0x1] = 0x0;
	nes->ppu.reg[0x5] = 0x0;
	nes->ppu.reg[0x6] = 0x0;
	nes->ppu.reg[0x7] = 0x0;
	uint16_t bank, offset;
	nes->reg.pc = *((uint16_t*)nessys_mem(nes, NESSYS_RST_VECTOR, &bank, &offset));
}

bool nessys_load_cart_filename(nessys_t* nes, const char* filename)
{
	FILE* fh;
	fopen_s(&fh, filename, "rb");
	if (fh == NULL) {
		return false;
	}
	bool success = nessys_load_cart(nes, fh);
	fclose(fh);
	return success;
}

void K2CALLBACK nessys_display(void* ptr)
{
	float clear_color[] = { 0.0f, 1.0f, 0.0f, 1.0f };
	float palette[4 * NESSYS_PPU_PAL_SIZE ];
	uint8_t pattern[8 * 1024];
	nessys_t* nes = (nessys_t*)ptr;
	uint32_t cycles;
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_RENDERED_CLKS + NESSYS_PPU_SCANLINES_POST_RENDER_CLKS);  // 241 scanlines
	printf("executed %d cycles\n", cycles);
	nes->vblank_cycles = (nes->cycles_remaining <= 0) ? 1 : nes->cycles_remaining;
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_VBLANK_CLKS);  // 20 vblank lines
	printf("executed %d cycles\n", cycles);

	// render image
	printf("------------------------------------------------------>Rendering pal[0] = 0x%X\n", nes->ppu.pal[0]);
	nes->gfx->k2AttachRenderGroup(nes->rg_win);
	nes->gfx->k2Clear(K2CLEAR_COLOR0, clear_color, 1.0f, 0);

	int i, c, index;
	if (nes->ppu.reg[0x1] & 0x18) {
		// initialize palette if rendering background or sprites
		for (i = 0; i < NESSYS_PPU_PAL_SIZE; i++) {
			for (c = 0; c < 3; c++) {
				index = ((i & 0x3) == 0) ? 0 : i;
				palette[4 * i + c] = NESSYS_PPU_PALETTE[3 * nes->ppu.pal[index] + c];
			}
			palette[4 * i + 3] = ((i & 0x3) == 0) ? 0.0f : 1.0f;
		}
	}

	if (nes->ppu.reg[0x1] & 0x08) {
		// enable background rendering
		nes->gfx->k2AttachBlendState(nes->bs_normal);
		nes->gfx->k2AttachDepthState(nes->ds_none);
		nes->gfx->k2AttachRasterizerState(nes->rs_normal);

		nes->gfx->k2AttachVertexGroup(nes->vg_fullscreen, 4);
		nes->gfx->k2AttachShaderGroup(nes->sg_background);

		// update constants
		// nametable update
		nes->gfx->k2CBUpdate(nes->cb_nametable, nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN));

		// pattern update
		// copy pattern table at address 0x0, or 0x1000, if bit 4 of ppureg[0] is set, 1KB ata time
		index = NESSYS_CHR_ROM_WIN_MIN + ((((uint32_t)nes->ppu.reg[0]) & 0x10) << 8);
		for (i = 0; i < 4; i++) {
			memcpy(pattern + (i << 10), nessys_ppu_mem(nes, index), 1024);
			index += 1024;
		}
		nes->gfx->k2CBUpdate(nes->cb_pattern, pattern);

		// update palette
		nes->gfx->k2CBUpdate(nes->cb_palette, palette);
		nes->gfx->k2AttachConstantGroup(nes->cg_background);

		nes->gfx->k2Draw();
	}

	if (nes->ppu.reg[0x1] & 0x10) {
		// enable sprite rendering
		nes->gfx->k2AttachBlendState(nes->bs_mask);
		nes->gfx->k2AttachDepthState(nes->ds_none);
		nes->gfx->k2AttachRasterizerState(nes->rs_normal);

		// update constants that are same for all sprites
		index = NESSYS_CHR_ROM_WIN_MIN;
		for (i = 0; i < 8; i++) {
			memcpy(pattern + (i << 10), nessys_ppu_mem(nes, index), 1024);
			index += 1024;
		}
		nes->gfx->k2CBUpdate(nes->cb_pattern, pattern);
		nes->gfx->k2CBUpdate(nes->cb_palette, palette + 2 * NESSYS_PPU_PAL_SIZE);
		nes->gfx->k2CBUpdate(nes->cb_ppu, &(nes->ppu.reg));
		nes->gfx->k2CBUpdate(nes->cb_sprite, nes->ppu.oam);
		nes->gfx->k2AttachConstantGroup(nes->cg_sprite);

		nes->gfx->k2AttachInstancedVertexGroup(nes->vg_sprite, 4, 64);
		nes->gfx->k2AttachShaderGroup(nes->sg_sprite);

		nes->gfx->k2Draw();

		//for (i = 0; i < 64; i++) {
		//	if (nes->ppu.oam[4 * i] < 0xef) {
		//
		//		// update constant buffers
		//		nes->gfx->k2CBUpdate(nes->cb_sprite, &(nes->ppu.oam[4 * i]));
		//		nes->gfx->k2AttachConstantGroup(nes->cg_sprite);
		//
		//		nes->gfx->k2Draw();
		//	}
		//}
	}

	nes->win->k2SwapBuffer();

	nes->ppu.status &= ~0x80;
	nes->ppu.reg[2] &= ~0x80;
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_PRE_RENDER_CLKS);   // pre-renderline
	printf("executed %d cycles\n", cycles);
}

void K2CALLBACK nessys_keyboard(void* ptr, k2key k, k2char c, k2keystate state)
{
	nessys_t* nes = (nessys_t*)ptr;
	uint8_t key_mask = 0x0;
	switch (k) {
	case K2KEY_ESCAPE:
		if (state == K2KEYSTATE_PRESSED) k2win::k2ExitLoop();
		break;
	case K2KEY_UP:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_UP_MASK;
		break;
	case K2KEY_DOWN:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK;
		break;
	case K2KEY_LEFT:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK;
		break;
	case K2KEY_RIGHT:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK;
		break;
	case 'A':
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_B_MASK;
		break;
	case 'S':
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_A_MASK;
		break;
	case 'Q':
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_SELECT_MASK;
		break;
	case 'W':
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_START_MASK;
		break;
	}

	if (key_mask) {
		if (state == K2KEYSTATE_PRESSED) {
			nes->apu.joypad[0] |= key_mask;
		} else {
			nes->apu.joypad[0] &= ~key_mask;
		}
		if (nes->apu.joy_control & 0x1) {
			nes->apu.latched_joypad[0] = nes->apu.joypad[0];
			nes->apu.latched_joypad[1] = nes->apu.joypad[1];
		}
	}
}

bool nessys_load_cart(nessys_t* nes, FILE* fh)
{
	bool success;
	nessys_unload_cart(nes);
	success = ines_load_cart(nes, fh);
	if (success) {
		nessys_default_memmap(nes);
		nes->win->SetKeyboardFunc(nessys_keyboard);
		nes->win->SetDisplayFunc(nessys_display);
		nes->win->SetIdleFunc(nessys_display);
		nessys_power_cycle(nes);
	}

	return success;
}

void nessys_default_memmap(nessys_t* nes)
{
	nes->prg_rom_bank[NESSYS_SYS_RAM_START_BANK] = nes->sysmem;
	nes->prg_rom_bank_mask[NESSYS_SYS_RAM_START_BANK] = NESSYS_RAM_MASK;

	nes->prg_rom_bank[NESSYS_PPU_REG_START_BANK] = nes->ppu.reg;
	nes->prg_rom_bank_mask[NESSYS_PPU_REG_START_BANK] = NESSYS_PPU_REG_MASK;

	nes->prg_rom_bank[NESSYS_APU_REG_START_BANK] = nes->apu.reg;
	nes->prg_rom_bank_mask[NESSYS_APU_REG_START_BANK] = NESSYS_APU_MASK;

	if (nes->ppu.chr_ram_base) {
		nes->prg_rom_bank[NESSYS_PRG_RAM_START_BANK] = nes->ppu.chr_ram_base;
		nes->prg_rom_bank_mask[NESSYS_PRG_RAM_START_BANK] = NESSYS_PRG_MEM_MASK;
	} else {
		// point to some junk location
		nes->prg_rom_bank[NESSYS_PRG_RAM_START_BANK] = &nes->reg.pad0;
		nes->prg_rom_bank_mask[NESSYS_PRG_RAM_START_BANK] = 0x0;
	}

	int b;
	uint32_t mem_offset = 0;
	if (nes->prg_rom_base) {
		for (b = NESSYS_PRG_ROM_START_BANK; b < NESSYS_PRG_NUM_BANKS; b++) {
			nes->prg_rom_bank[b] = nes->prg_rom_base + mem_offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			mem_offset += NESSYS_PRG_BANK_SIZE;
			if (mem_offset >= nes->prg_rom_size) mem_offset -= nes->prg_rom_size;
		}
	} else {
		for (b = NESSYS_PRG_ROM_START_BANK; b < NESSYS_PRG_NUM_BANKS; b++) {
			nes->prg_rom_bank[b] = &nes->reg.pad0;
			nes->prg_rom_bank_mask[b] = 0x0;
		}
	}
	if (nes->ppu.chr_rom_base) {
		mem_offset = 0;
		for (b = 0; b <= NESSYS_CHR_ROM_END_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_base + mem_offset;
			nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
			mem_offset += NESSYS_CHR_BANK_SIZE;
			if (mem_offset >= nes->ppu.chr_rom_size) mem_offset -= nes->ppu.chr_rom_size;
		}
	} else {
		for (b = 0; b <= NESSYS_CHR_ROM_END_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = &nes->reg.pad0;
			nes->ppu.chr_rom_bank_mask[b] = 0x0;
		}
	}
	// map name table
	nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
	nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
	if (nes->ppu.name_tbl_vert_mirror) {
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
	} else {
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
	}
	for (b = NESSYS_CHR_NTB_START_BANK + 4; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_bank[b - 4];
	}
	for (b = NESSYS_CHR_NTB_START_BANK; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
	}
}

uint32_t nessys_exec_cpu_cycles(nessys_t* nes, uint32_t num_cycles)
{
	uint32_t cycle_count = 0;
	nes->cycles_remaining += num_cycles; // cycles to execute
	bool done = false;
	const c6502_op_code_t* op = NULL;
	uint16_t addr = nes->reg.pc, indirect_addr = 0x0;
	uint8_t* operand = &nes->reg.a;
	uint16_t bank, offset;
	uint16_t result;
	uint8_t* pc_ptr = NULL;
	uint16_t penalty_cycles;
	uint8_t overflow;
	bool vblank_interrupt;
	uint8_t skip_print = 0;
	bool ppu_write, apu_write, rom_write;
	while (!done) {
		pc_ptr = nessys_mem(nes, nes->reg.pc, &bank, &offset);
		op = &C6502_OP_CODE[*pc_ptr];
		ppu_write = false;
		apu_write = false;
		rom_write = false;
		penalty_cycles = 0;  // indicates cycles of page cross or branch taken penalty
		addr = 0x0;  // just a defulat address, if not used
		// perform addressing operation to get operand
		bank = 0;
		switch (op->addr) {
		case C6502_ADDR_NONE:
			break;
		case C6502_ADDR_ACCUM:
			operand = &nes->reg.a;
			break;
		case C6502_ADDR_IMMED:
			operand = nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			break;
		case C6502_ADDR_ZEROPAGE:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_ABSOLUTE:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			addr |= ((uint16_t) *nessys_mem(nes, nes->reg.pc + 2, &bank, &offset)) << 8;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_RELATIVE:
			indirect_addr = nes->reg.pc + op->num_bytes;
			addr = indirect_addr + ((int8_t) *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset));
			// branch penalty of 2 cycles if page changes, otherwise just 1 cycle
			penalty_cycles += 1;
			penalty_cycles += ((addr & 0xFF00) != (indirect_addr & 0xFF00));
			break;
		case C6502_ADDR_INDIRECT:
			indirect_addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			indirect_addr |= ((uint16_t)*nessys_mem(nes, nes->reg.pc + 2, &bank, &offset)) << 8;
			addr = *nessys_mem(nes, indirect_addr, &bank, &offset);
			((uint8_t&)indirect_addr)++;
			addr |= ((uint16_t)*nessys_mem(nes, indirect_addr, &bank, &offset)) << 8;
			break;
		case C6502_ADDR_ZEROPAGE_X:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			addr += nes->reg.x;
			addr &= 0xFF;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_ZEROPAGE_Y:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			addr += nes->reg.y;
			addr &= 0xFF;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_ABSOLUTE_X:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			penalty_cycles += (addr + nes->reg.x >= 0x100);
			addr |= ((uint16_t)*nessys_mem(nes, nes->reg.pc + 2, &bank, &offset)) << 8;
			addr += nes->reg.x;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_ABSOLUTE_Y:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			penalty_cycles += (addr + nes->reg.y >= 0x100);
			addr |= ((uint16_t)*nessys_mem(nes, nes->reg.pc + 2, &bank, &offset)) << 8;
			addr += nes->reg.y;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_INDIRECT_X:
			indirect_addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			indirect_addr += nes->reg.x;
			indirect_addr &= 0xFF;
			addr = *nessys_mem(nes, indirect_addr, &bank, &offset);
			indirect_addr++;
			indirect_addr &= 0xFF;
			addr |= ((uint16_t)*nessys_mem(nes, indirect_addr, &bank, &offset)) << 8;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_INDIRECT_Y:
			indirect_addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			addr = *nessys_mem(nes, indirect_addr, &bank, &offset);
			penalty_cycles += (addr + nes->reg.y >= 0x100);
			indirect_addr++;
			indirect_addr &= 0xFF;
			addr |= ((uint16_t)*nessys_mem(nes, indirect_addr, &bank, &offset)) << 8;
			addr += nes->reg.y;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		}

		vblank_interrupt = false;
		if (nes->vblank_cycles > 0 && nes->vblank_cycles < NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles)) {
			nes->ppu.status |= 0x80;
			nes->ppu.reg[2] |= 0x80;
			vblank_interrupt = (nes->ppu.reg[0] & 0x80) ? true : false;
			nes->vblank_cycles = 0;
		}
		if (nes->cycles_remaining < ((int32_t) (NESSYS_PPU_PER_CPU_CLK*(op->num_cycles + penalty_cycles))) || vblank_interrupt) {
			done = !vblank_interrupt;
			if (vblank_interrupt) {
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = nes->reg.pc >> 8;        nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.pc & 0xFF;      nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.p & ~C6502_P_B; nes->reg.s--;
				nes->reg.pc = *((uint16_t*) nessys_mem(nes, NESSYS_NMI_VECTOR, &bank, &offset));

				printf("Vblank interrupt\n");
				cycle_count += NESSYS_PPU_PER_CPU_CLK * (7);
				// cycles remaining may go negative if we don't have enough to cover the NMI
				nes->cycles_remaining -= NESSYS_PPU_PER_CPU_CLK * (7);
				nes->cycle += 7;
			}
		} else {
			skip_print = 1;
			if (skip_print == 0) {
				printf("0x%x: %s(0x%x) a:0x%x d:0x%x", nes->reg.pc, op->ins_name, *pc_ptr, addr, *operand);
				if (op->flags & (C6502_FL_ILLEGAL | C6502_FL_UNDOCUMENTED)) {
					printf(" - warning: illegal or undocumented instruction\n");
				}
				printf("\n");
			} else {
				if (skip_print < op->num_bytes) skip_print = 0;
				else skip_print -= op->num_bytes;
			}

			// move up the program counter
			nes->reg.pc += op->num_bytes;

			// check if we read the PPU, or APU
			switch (bank) {
			case NESSYS_PPU_REG_START_BANK:
				switch (offset) {
				case 2:
					nes->ppu.status &= ~0x80;
					break;
				case 4:
					nes->ppu.reg[4] = nes->ppu.oam[nes->ppu.reg[3]];
					break;
				case 7:
					nes->ppu.reg[7] = *nessys_ppu_mem(nes, nes->ppu.mem_addr);
					break;
				}
				break;
			case NESSYS_APU_REG_START_BANK:
				if (offset >= NESSYS_APU_JOYPAD0_OFFSET && offset <= NESSYS_APU_JOYPAD1_OFFSET) {
					uint8_t j = offset - NESSYS_APU_JOYPAD0_OFFSET;
					nes->apu.reg[offset] = nes->apu.latched_joypad[j] & 0x1;
					if ((nes->apu.joy_control & 0x1) == 0x0) {
						nes->apu.latched_joypad[j] >>= 1;
					}
				}
				break;
			}

			// perform the instruction's operation
			switch (op->ins) {
			case C6502_INS_ADC:
			case C6502_INS_SBC:
				result = *operand;
				//if (skip_print == 0 && op->ins == C6502_INS_ADC) printf("------------------------------------------------> ADC: op=0x%x a=0x%x p=0x%x, ", *operand, nes->reg.a, nes->reg.p);
				if (op->ins == C6502_INS_SBC) result = ~result;
				// overflow possible if bit 7 of two operands are the same
				overflow = (result & 0x80) == (nes->reg.a & 0x80);
				result += nes->reg.a + ((nes->reg.p & C6502_P_C) >> C6502_P_C_SHIFT);
				overflow &= (result & 0x80) != (nes->reg.a & 0x80);
				nes->reg.a = (uint8_t)result;
				// clear N/V/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_V | C6502_P_Z | C6502_P_C);
				nes->reg.p |= (nes->reg.a == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= overflow << C6502_P_V_SHIFT;
				overflow = (result & 0x100) >> 8;  // carry bit
				if (op->ins == C6502_INS_SBC) overflow = !overflow;
				nes->reg.p |= overflow << C6502_P_C_SHIFT;
				nes->reg.p |= (nes->reg.a & 0x80) >> (7 - C6502_P_N_SHIFT);
				//if (skip_print == 0 && op->ins == C6502_INS_ADC) printf("fin=0x%x p=0x%x\n", nes->reg.a, nes->reg.p);
				break;
			case C6502_INS_AND:
				nes->reg.a &= *operand;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= (nes->reg.a == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (nes->reg.a & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_ASL:
				overflow = ((*operand & 0x80) != 0x00);
				result = *operand << 1;
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= overflow << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t) result);
				}
				break;
			case C6502_INS_BCC:
				if ((nes->reg.p & C6502_P_C) == 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if(indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BCS:
				if ((nes->reg.p & C6502_P_C) != 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BEQ:
				if ((nes->reg.p & C6502_P_Z) != 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BIT:
				// clear N/V/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_V | C6502_P_Z);
				result = *operand;
				nes->reg.p |= (result & 0xC0);  // bit 7 & 6 go into the N/V bits respectively
				result &= nes->reg.a;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				break;
			case C6502_INS_BMI:
				if ((nes->reg.p & C6502_P_N) != 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BNE:
				if ((nes->reg.p & C6502_P_Z) == 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BPL:
				if ((nes->reg.p & C6502_P_N) == 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BRK:
				printf("----------------------------------> BRK\n");
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = nes->reg.pc >> 8;   nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.pc & 0xFF; nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.p;         nes->reg.s--;
				nes->reg.pc = *((uint16_t*) nessys_mem(nes, NESSYS_IRQ_VECTOR, &bank, &offset));
				break;
			case C6502_INS_BVC:
				if ((nes->reg.p & C6502_P_V) == 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_BVS:
				if ((nes->reg.p & C6502_P_V) != 0x00) {
					// take the branch
					nes->reg.pc = addr;
					if (indirect_addr >= addr) skip_print = indirect_addr - addr + 1;
				} else {
					// if we don't take the branch, there is no penalty
					penalty_cycles = 0;
					skip_print = 0;
				}
				break;
			case C6502_INS_CLC:
				nes->reg.p &= ~C6502_P_C;
				break;
			case C6502_INS_CLD:
				nes->reg.p &= ~C6502_P_D;
				break;
			case C6502_INS_CLI:
				nes->reg.p &= ~C6502_P_I;
				break;
			case C6502_INS_CLV:
				nes->reg.p &= ~C6502_P_V;
				break;
			case C6502_INS_CMP:
				overflow = (nes->reg.a >= *operand);
				result = nes->reg.a - *operand;
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= overflow << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_CPX:
				overflow = (nes->reg.x >= *operand);
				result = nes->reg.x - *operand;
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= overflow << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_CPY:
				overflow = (nes->reg.y >= *operand);
				result = nes->reg.y - *operand;
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= overflow << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_DEC:
				result = *operand - 1;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
				}
				break;
			case C6502_INS_DEX:
				nes->reg.x--;
				result = nes->reg.x;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_DEY:
				nes->reg.y--;
				result = nes->reg.y;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_EOR:
				nes->reg.a ^= *operand;
				result = nes->reg.a;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_INC:
				result = *operand + 1;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
				}
				break;
			case C6502_INS_INX:
				nes->reg.x++;
				result = nes->reg.x;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_INY:
				nes->reg.y++;
				result = nes->reg.y;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_JMP:
				if (op->addr == C6502_ADDR_INDIRECT)   printf("----------------------> JMP ():  iaddr = 0x%x addr = 0x%x\n", indirect_addr, addr);
				nes->reg.pc = addr;
				break;
			case C6502_INS_JSR:
				result = nes->reg.pc - 1;
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = result >> 8;   nes->reg.s--;
				*(operand + nes->reg.s) = result & 0xFF; nes->reg.s--;
				nes->reg.pc = addr;
				break;
			case C6502_INS_LDA:
				nes->reg.a = *operand;
				result = nes->reg.a;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_LDX:
				nes->reg.x = *operand;
				result = nes->reg.x;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_LDY:
				nes->reg.y = *operand;
				result = nes->reg.y;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_LSR:
				overflow = *operand & 0x01;
				result = *operand >> 1;
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= overflow << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
				}
				break;
			case C6502_INS_ORA:
				nes->reg.a |= *operand;
				result = nes->reg.a;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_PHA:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = nes->reg.a;   nes->reg.s--;
				break;
			case C6502_INS_PHP:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = nes->reg.p;   nes->reg.s--;
				break;
			case C6502_INS_PLA:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->reg.s++; nes->reg.a = *(operand + nes->reg.s);
				result = nes->reg.a;
				// clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_PLP:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->reg.s++; nes->reg.p = *(operand + nes->reg.s) | C6502_P_U | C6502_P_B;
				break;
			case C6502_INS_ROL:
				result = (*operand << 1) | ((nes->reg.p & C6502_P_C) >> C6502_P_C_SHIFT);
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= (result & 0x100) >> (8 - C6502_P_C_SHIFT);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
				}
				break;
			case C6502_INS_ROR:
				result = (*operand >> 1) | ((nes->reg.p & C6502_P_C) << (7 - C6502_P_C_SHIFT));
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= (*operand & 0x1) << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
				}
				break;
			case C6502_INS_RTI:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->reg.s++; nes->reg.p =  *(operand + nes->reg.s) | C6502_P_U | C6502_P_B;
				nes->reg.s++; nes->reg.pc = *(operand + nes->reg.s);
				nes->reg.s++; nes->reg.pc |= ((uint16_t) *(operand + nes->reg.s)) << 8;
				break;
			case C6502_INS_RTS:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->reg.s++; result = *(operand + nes->reg.s);
				nes->reg.s++; result |= ((uint16_t) * (operand + nes->reg.s)) << 8;
				nes->reg.pc = result + 1;
				break;
/*
			case C6502_INS_SBC:
				result = *operand;
				if (skip_print == 0) printf("------------------------------------------------------> SBC: a=0x%x op=0x%x ", nes->reg.a, result);
				// overflow possible if bit 7 of two operands are the same
				overflow = (result & 0x80) != (nes->reg.a & 0x80);
				result = nes->reg.a - result - !((nes->reg.p & C6502_P_C) >> C6502_P_C_SHIFT);
				overflow &= (result & 0x80) != (nes->reg.a & 0x80);
				nes->reg.a = (uint8_t)result;
				// clear N/V/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_V | C6502_P_Z | C6502_P_C);
				nes->reg.p |= (result & 0x8000) >> (15 - C6502_P_C_SHIFT);
				nes->reg.p |= (nes->reg.a == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= overflow << C6502_P_V_SHIFT;
				nes->reg.p |= (nes->reg.a & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (skip_print == 0) printf("result=0x%x p=0x%x\n", nes->reg.a, nes->reg.p);
				break;
*/
			case C6502_INS_SEC:
				nes->reg.p |= C6502_P_C;
				break;
			case C6502_INS_SED:
				nes->reg.p |= C6502_P_D;
				break;
			case C6502_INS_SEI:
				nes->reg.p |= C6502_P_I;
				break;
			case C6502_INS_STA:
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = nes->reg.a;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = nes->reg.a;
							break;
						}
					} else {
						*operand = nes->reg.a;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, nes->reg.a);
				}
				break;
			case C6502_INS_STX:
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = nes->reg.x;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = nes->reg.x;
							break;
						}
					} else {
						*operand = nes->reg.x;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, nes->reg.x);
				}
				break;
			case C6502_INS_STY:
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = nes->reg.y;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = nes->reg.y;
							break;
						}
					} else {
						*operand = nes->reg.y;
					}
				} else {
					rom_write = true;
					printf("writing to rom area a=0x%x d=0x%x\n", addr, nes->reg.y);
				}
				break;
			case C6502_INS_TAX:
				nes->reg.x = nes->reg.a;
				//clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= (nes->reg.x == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (nes->reg.x & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_TAY:
				nes->reg.y = nes->reg.a;
				//clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= (nes->reg.y == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (nes->reg.y & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_TSX:
				nes->reg.x = nes->reg.s;
				//clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= (nes->reg.x == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (nes->reg.x & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_TXA:
				nes->reg.a = nes->reg.x;
				//clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= (nes->reg.a == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (nes->reg.a & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			case C6502_INS_TXS:
				nes->reg.s = nes->reg.x;
				break;
			case C6502_INS_TYA:
				nes->reg.a = nes->reg.y;
				//clear N/Z
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z);
				nes->reg.p |= (nes->reg.a == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (nes->reg.a & 0x80) >> (7 - C6502_P_N_SHIFT);
				break;
			default:
				// everything not decoded is a NOP
				// TODO: undocumented instructions
				// TODO: handle writing to memory mappers (writing to rom space)
				break;
			}

			skip_print = 1;
			// if we read or write ppu status, update it's value from the master status reg
			if (bank == NESSYS_PPU_REG_START_BANK && offset == 2) {
				nes->ppu.reg[2] &= 0x1F;
				nes->ppu.reg[2] |= nes->ppu.status;
				nes->ppu.scroll_toggle = 0;
				nes->ppu.mem_addr_mask = 0xFF00;
			}

			if (apu_write) {
				if (offset == NESSYS_APU_JOYPAD0_OFFSET && (nes->apu.joy_control & 0x1)) {
					nes->apu.latched_joypad[0] = nes->apu.joypad[0];
					nes->apu.latched_joypad[1] = nes->apu.joypad[1];
				}
			}
			if (ppu_write) {
				if (skip_print == 0) printf("ppu_write offset %d\n", offset);
				if (bank == NESSYS_PPU_REG_START_BANK) {
					nes->ppu.reg[2] = (nes->ppu.status & 0xE) | (nes->ppu.reg[offset & 0x7] & 0x1F);
				}
				switch (offset) {
				case 4:
					nes->ppu.oam[nes->ppu.reg[3]] = nes->ppu.reg[4];
					nes->ppu.reg[3]++;
					break;
				case 5:
					nes->ppu.scroll[nes->ppu.scroll_toggle] = nes->ppu.reg[5];
					nes->ppu.scroll_toggle = !nes->ppu.scroll_toggle;
					break;
				case 6:
					if (skip_print == 0) printf("addr_mask = 0x%x data=0x%x\n", nes->ppu.mem_addr_mask, (((uint16_t)nes->ppu.reg[6] << 8) | nes->ppu.reg[6]));
					nes->ppu.mem_addr &= ~(nes->ppu.mem_addr_mask);
					nes->ppu.mem_addr |= (((uint16_t) nes->ppu.reg[6] << 8) | nes->ppu.reg[6]) & (nes->ppu.mem_addr_mask);
					nes->ppu.mem_addr_mask = ~(nes->ppu.mem_addr_mask);
					break;
				case 7:
					if (skip_print == 0) printf("write to vram addr 0x%x data 0x%x", nes->ppu.mem_addr, nes->ppu.reg[7]);
					*nessys_ppu_mem(nes, nes->ppu.mem_addr) = nes->ppu.reg[7];
					if (nes->ppu.mem_addr >= NESSYS_CHR_PAL_WIN_MIN) {
						// alias 3f10, 3f14, 3f18 and 3f1c to corresponding 3f0x
						// and vice versa
						if ((nes->ppu.mem_addr & 0x3) == 0x0) {
							nes->ppu.pal[(nes->ppu.mem_addr & NESSYS_PPU_PAL_MASK) ^ 0x10] = nes->ppu.reg[7];
						}
					}
					// if bit 2 is 0, increment address by 1 (one step horizontal), otherwise, increment by 32 (one step vertical)
					nes->ppu.mem_addr += !((nes->ppu.reg[0] & 0x4) >> 1) + ((nes->ppu.reg[0] & 0x4) << 3);
					if (skip_print == 0) printf(" vram addr nest 0x%x\n", nes->ppu.mem_addr);
					break;
				case 0x14:
					addr = nes->apu.reg[0x14] << 8;
					operand = nessys_mem(nes, addr, &bank, &offset);
					memcpy(nes->ppu.oam, operand, NESSYS_PPU_OAM_SIZE);
					penalty_cycles += 514;
					break;
				}
			}
			cycle_count += NESSYS_PPU_PER_CPU_CLK*(op->num_cycles + penalty_cycles);
			// cycles remaining may go negative if we have a oam dma transfer
			// the cycle penalty is accounted for until after th decision has already been bade to execute it
			nes->cycles_remaining -= NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles);
			nes->cycle += op->num_cycles + penalty_cycles;
			if (nes->vblank_cycles > 0) {
				if (nes->vblank_cycles < NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles)) {
					nes->vblank_cycles = 1;
				} else {
					nes->vblank_cycles -= NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles);
				}
			}
		}
	}
	return cycle_count;
}


void nessys_unload_cart(nessys_t* nes)
{
	nes->win->SetDisplayFunc(NULL);
	nes->win->SetDisplayFunc(NULL);
	nes->win->SetIdleFunc(NULL);
	if (nes->prg_rom_base) {
		free(nes->prg_rom_base);
		nes->prg_rom_base = NULL;
	}
	if (nes->ppu.chr_rom_base) {
		free(nes->ppu.chr_rom_base);
		nes->ppu.chr_rom_base = NULL;
	}
	if (nes->ppu.chr_ram_base) {
		free(nes->ppu.chr_ram_base);
		nes->ppu.chr_ram_base = NULL;
	}
}

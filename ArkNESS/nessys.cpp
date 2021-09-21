// Project:     ArkNES
// File:        nessys.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description:	NES system functions

#include "nessys.h"
#include <math.h>

void nessys_init(nessys_t* nes)
{
	memset(nes, 0, sizeof(nessys_t));
	nes->apu.pulse[0].env.flags = NESSYS_APU_PULSE_FLAG_SWEEP_ONES_COMP;
	nes->apu.noise.shift_reg = 0x01;
	k2image::k2Initialize();
	k2error_SetHandler(k2error_StdOutHandler);
	k2win::k2SetGfxType(K2GFX_DIRECTX_11);
	nes->win = k2win::CreateWindowedWithFormat("ArkNES", 0, 0, 640, 480, K2FMT_RGBA8_UNORM, K2FMT_D24_UNORM_S8_UINT);
	nes->win->k2SetDataPtr(nes);
	nes->gfx = nes->win->k2GetGfx();
	nes->rg_win = nes->win->k2GetRenderGroup();
	nes->win->k2SetVsyncInterval(1);

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

	ds_desc.zEnable = true;
	ds_desc.zWriteEnable = true;
	ds_desc.zFunc = K2COMPARE_LEQUAL;
	ds_desc.stencilEnable = false;
	nes->ds_normal = nes->gfx->k2CreateDepthState(&ds_desc);

	ds_desc.zWriteEnable = false;
	ds_desc.stencilEnable = true;
	ds_desc.stencilTestMask = 0xff;
	ds_desc.stencilWriteMask = 0xff;
	ds_desc.backFace.stencilFunc = K2COMPARE_NEVER;
	ds_desc.backFace.stencilFailOp = K2STENCILOP_INCR_SAT;
	ds_desc.backFace.zFailOp = K2STENCILOP_INCR_SAT;
	ds_desc.backFace.passOp = K2STENCILOP_INCR_SAT;
	ds_desc.frontFace.stencilFunc = K2COMPARE_GREATER;
	ds_desc.frontFace.stencilFailOp = K2STENCILOP_KEEP;
	ds_desc.frontFace.zFailOp = K2STENCILOP_REPLACE;
	ds_desc.frontFace.passOp = K2STENCILOP_REPLACE;
	nes->ds_sprite = nes->gfx->k2CreateDepthState(&ds_desc);

	rs_desc.fillMode = K2FILL_SOLID;
	rs_desc.cullMode = K2CULL_NONE;
	rs_desc.frontCCW = false;
	rs_desc.zClipEnable = false;
	rs_desc.scissorEnable = true;
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
	ps = nes->gfx->k2CreatePixelShaderFromObj("..\\Debug\\fill_ps.cso");
	nes->sg_fill = nes->gfx->k2CreateShaderGroup(vs, ps);
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

	nes->cb_nametable = nes->gfx->k2CreateConstantBuffer(4*1024, K2ACCESS_UPDATE, NULL);
	nes->cb_pattern = nes->gfx->k2CreateConstantBuffer(512 * 16, K2ACCESS_UPDATE, NULL);
	nes->cb_palette = nes->gfx->k2CreateConstantBuffer(256, K2ACCESS_UPDATE, NULL);
	nes->cb_ppu = nes->gfx->k2CreateConstantBuffer(16, K2ACCESS_UPDATE, NULL);
	nes->cb_sprite = nes->gfx->k2CreateConstantBuffer(256, K2ACCESS_UPDATE, NULL);

	k2constantelement celem[] = { 
		{"ppu", nes->cb_ppu},
		{"palette", nes->cb_palette},
		{"pattern", nes->cb_pattern},
		{"nametable", nes->cb_nametable} };
	nes->cg_fill = nes->gfx->k2CreateConstantGroup(nes->sg_fill, 2, celem);
	nes->cg_background = nes->gfx->k2CreateConstantGroup(nes->sg_background, 4, celem);

	celem[3].slot_name = "sprite";
	celem[3].constant_buffer = nes->cb_sprite;
	nes->cg_sprite = nes->gfx->k2CreateConstantGroup(nes->sg_sprite, 4, celem);

	nes->sb_main = nes->win->k2CreateSBuffer(NESSYS_SND_SAMPLES_PER_SECOND, NESSYS_SND_BITS_PER_SAMPLE, NESSYS_SND_SAMPLES);
	void* sbuf_ptr = nes->sb_main->k2MapSBufForWrite(0, 2 * NESSYS_SND_SAMPLES);
	memset(sbuf_ptr, 0, 2 * NESSYS_SND_SAMPLES);
	nes->sb_main->k2UnmapSBuf(sbuf_ptr, 0, 2 * NESSYS_SND_SAMPLES);

	// reset write pointer
	nes->sbuf_offset = NESSYS_SND_START_POSITION;
	sbuf_ptr = nes->sb_main->k2MapSBufForWrite(nes->sbuf_offset, NESSYS_SND_BYTES_PER_BUFFER);
	nes->sb_main->k2UnmapSBuf(sbuf_ptr, nes->sbuf_offset, NESSYS_SND_BYTES_PER_BUFFER);

	nes->timer = nes->win->k2CreateTimer();
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
	nes->apu.noise.shift_reg = 0x01;
	nes->sb_main->k2StopSBuffer();
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
	nes->apu.noise.shift_reg = 0x01;
	nes->sb_main->k2StopSBuffer();
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

void nesssys_set_scanline(nessys_t* nes, int32_t scanline)
{
	nes->scanline = scanline;
	//nes->scanline_cycle = 0;
	if (nes->cycles_remaining > 0) {
		nes->scanline_cycle = -(nes->cycles_remaining);
	} else {
		nes->scanline += (-nes->cycles_remaining) / NESSYS_PPU_CLK_PER_SCANLINE;
		nes->scanline_cycle = (-nes->cycles_remaining) % NESSYS_PPU_CLK_PER_SCANLINE;
	}
}

void nessys_apu_env_tick(nessys_apu_envelope_t* envelope)
{
	if (envelope->flags & NESSYS_APU_PULSE_FLAG_ENV_START) {
		envelope->decay = 15;
		envelope->divider = envelope->volume;
		envelope->flags &= ~NESSYS_APU_PULSE_FLAG_ENV_START;
	} else {
		if (envelope->divider) {
			envelope->divider--;
		} else {
			envelope->divider = envelope->volume;
			if (envelope->decay) {
				envelope->decay--;
			} else if(envelope->flags & NESSYS_APU_PULSE_FLAG_HALT_LENGTH) {
				// halt flag is also used to loop env
				envelope->decay = 15;
			}
		}
	}
}

void nessys_apu_tri_linear_tick(nessys_apu_triangle_t* triangle)
{
	if (triangle->flags & NESSYS_APU_TRIANGLE_FLAG_RELOAD) {
		triangle->linear = triangle->reload;
	} else if (triangle->linear) {
		triangle->linear--;
	}
	if ((triangle->flags & NESSYS_APU_TRIANGLE_FLAG_CONTROL) == 0) {
		triangle->flags &= ~NESSYS_APU_TRIANGLE_FLAG_RELOAD;
	}
}

void nessys_apu_tri_length_tick(nessys_apu_triangle_t* triangle)
{
	if (triangle->length && !(triangle->flags & NESSYS_APU_TRIANGLE_FLAG_CONTROL)) {
		triangle->length--;
	}
}

void nessys_apu_noise_length_tick(nessys_apu_noise_t* noise)
{
	if (noise->length && !(noise->env.flags & NESSYS_APU_PULSE_FLAG_HALT_LENGTH)) {
		noise->length--;
	}
}

uint16_t nessys_apu_sweep_get_target_period(nessys_apu_pulse_t* pulse)
{
	uint16_t delta = pulse->period >> pulse->sweep_shift;
	uint16_t target_period = pulse->period;
	if (pulse->env.flags & NESSYS_APU_PULSE_FLAG_SWEEP_NEGATE) {
		target_period -= delta + ((pulse->env.flags & NESSYS_APU_PULSE_FLAG_SWEEP_ONES_COMP) && (pulse->sweep_shift != 0));
	} else {
		target_period += delta;
	}
	return target_period;
}

void nessys_apu_sweep_tick(nessys_apu_pulse_t* pulse)
{
	uint16_t target_period = nessys_apu_sweep_get_target_period(pulse);
	bool mute = (pulse->period < 8) || (target_period > 0x7ff) || (pulse->length == 0);
	if (pulse->sweep_divider == 0 && (pulse->env.flags & NESSYS_APU_PULSE_FLAG_SWEEP_EN) && pulse->sweep_shift && !mute) {
		pulse->period = target_period;
	}
	if (pulse->sweep_divider == 0 || (pulse->env.flags & NESSYS_APU_PULSE_FLAG_SWEEP_RELOAD)) {
		pulse->sweep_divider = pulse->sweep_period;
		pulse->env.flags &= ~NESSYS_APU_PULSE_FLAG_SWEEP_RELOAD;
	} else {
		pulse->sweep_divider--;
	}
	// also handle length
	if (pulse->length && !(pulse->env.flags & NESSYS_APU_PULSE_FLAG_HALT_LENGTH)) {
		pulse->length--;
	}
}

uint8_t nessys_apu_gen_pulse(nessys_apu_pulse_t* pulse)
{
	if (pulse->period) {
		pulse->cur_time_frac += NESSYS_SND_APU_FRAC_PER_SAMPLE;
		while ((pulse->cur_time_frac >> NESSYS_SND_APU_FRAC_LOG2) > pulse->period) {
			pulse->duty_phase++;
			pulse->duty_phase &= 0x7;
			pulse->cur_time_frac -= (pulse->period << NESSYS_SND_APU_FRAC_LOG2);
		}
	}
	uint8_t sound = (pulse->env.flags & NESSYS_APU_PULSE_FLAG_CONST_VOLUME) ? pulse->env.volume : pulse->env.decay;
	uint16_t target_period = nessys_apu_sweep_get_target_period(pulse);
	bool mute = (pulse->period < 8) || (target_period > 0x7ff);
	mute = mute || (((pulse->duty >> pulse->duty_phase) & 0x1) == 0);
	mute = mute || (pulse->length == 0);// && !(pulse->env.flags & NESSYS_APU_PULSE_FLAG_HALT_LENGTH));
	sound = (mute) ? 0 : sound;
	return sound;
}

uint8_t nessys_apu_gen_triangle(nessys_apu_triangle_t* triangle)
{
	if (triangle->period) {
		triangle->cur_time_frac += NESSYS_SND_CPU_FRAC_PER_SAMPLE;
		while ((triangle->cur_time_frac >> NESSYS_SND_APU_FRAC_LOG2) > triangle->period) {
			triangle->sequence++;
			triangle->sequence &= 0x1f;
			triangle->cur_time_frac -= (triangle->period << NESSYS_SND_APU_FRAC_LOG2);
		}
	}
	uint8_t sound = triangle->sequence ^ ((triangle->sequence & 0x10) ? 0x1f : 0x00);
	if (triangle->linear == 0 || triangle->length == 0) sound = 0;
	return sound;
}

uint8_t nessys_apu_gen_noise(nessys_apu_noise_t* noise)
{
	uint16_t feedback = 0;
	if (noise->period) {
		noise->cur_time_frac += NESSYS_SND_APU_FRAC_PER_SAMPLE;
		uint8_t xor_bit = 1 + ((noise->env.flags & NESSYS_APU_NOISE_FLAG_MODE) ? 5 : 0);
		while ((noise->cur_time_frac >> NESSYS_SND_APU_FRAC_LOG2) > noise->period) {
			feedback = ((noise->shift_reg ^ (noise->shift_reg >> xor_bit)) & 0x1) << 14;
			noise->shift_reg = (noise->shift_reg >> 1) | feedback;
			noise->cur_time_frac -= (noise->period << NESSYS_SND_APU_FRAC_LOG2);
		}
	}
	uint8_t sound = (noise->env.flags & NESSYS_APU_PULSE_FLAG_CONST_VOLUME) ? noise->env.volume : noise->env.decay;
	if (noise->length == 0 || (noise->shift_reg & 0x1) == 0) {
		sound = 0;
	}
	return sound;
}

uint8_t nessys_apu_gen_dmc(nessys_t* nes)
{
	nessys_apu_dmc_t* dmc = &(nes->apu.dmc);
	if (dmc->bytes_remaining == 0 && (dmc->flags & NESSYS_APU_DMC_FLAG_DMA_ENABLE)) {
		dmc->cur_addr = dmc->start_addr;
		dmc->bytes_remaining = dmc->length;
	}
	if (dmc->bits_remaining == 0 && dmc->bytes_remaining != 0) {
		uint16_t bank, offset;
		dmc->delta_buffer = *nessys_mem(nes, dmc->cur_addr, &bank, &offset);
		dmc->bits_remaining = 8;
		dmc->cur_addr++;
		dmc->cur_addr |= 0x8000;
		dmc->bytes_remaining--;
		if (dmc->bytes_remaining == 0 && !(dmc->flags & NESSYS_APU_DMC_FLAG_LOOP)) {
			dmc->flags &= ~NESSYS_APU_DMC_FLAG_DMA_ENABLE;
		}
		if (dmc->cur_addr == dmc->start_addr) {
			dmc->cur_time_frac = 0;
		}
	}
	if (dmc->bits_remaining != 0 && dmc->period != 0) {
		dmc->cur_time_frac += NESSYS_SND_CPU_FRAC_PER_SAMPLE;
		while ((dmc->cur_time_frac >> NESSYS_SND_APU_FRAC_LOG2) > dmc->period) {
			if (dmc->delta_buffer & 0x1) {
				if (dmc->output < 126) dmc->output += 2;
			} else {
				if (dmc->output > 2) dmc->output -= 2;
			}
			dmc->delta_buffer >>= 1;
			dmc->bits_remaining--;
			dmc->cur_time_frac -= (dmc->period << NESSYS_SND_APU_FRAC_LOG2);
		}
	}
	return dmc->output;
}

int16_t nessys_gen_sound_sample(nessys_t* nes)
{
	// first increment frame conter, and see if it ticks
	uint32_t last_frame_tick = nes->apu.frame_frac_counter >> NESSYS_SND_FRAME_FRAC_LOG2;
	nes->apu.frame_frac_counter += NESSYS_SND_FRAME_FRAC_PER_SAMPLE;
	uint32_t cur_frame_tick = nes->apu.frame_frac_counter >> NESSYS_SND_FRAME_FRAC_LOG2;
	if (cur_frame_tick > last_frame_tick) {
		uint32_t max_frame_tick = (nes->apu.frame_counter & 0x80) ? 4 : 3;
		if (cur_frame_tick > max_frame_tick) {
			cur_frame_tick = 0;
			nes->apu.frame_frac_counter &= NESSYS_SND_FRAME_FRAC_MASK;
		}
		if (last_frame_tick < 4) {
			nessys_apu_env_tick(&(nes->apu.pulse[0].env));
			nessys_apu_env_tick(&(nes->apu.pulse[1].env));
			nessys_apu_env_tick(&(nes->apu.noise.env));
			nessys_apu_tri_linear_tick(&(nes->apu.triangle));
			if (!(last_frame_tick & 1)) {
				nessys_apu_sweep_tick(&(nes->apu.pulse[0]));
				nessys_apu_sweep_tick(&(nes->apu.pulse[1]));
				nessys_apu_noise_length_tick(&(nes->apu.noise));
				nessys_apu_tri_length_tick(&(nes->apu.triangle));
			}
		}
	}
	uint8_t pulse_sound = nessys_apu_gen_pulse(&(nes->apu.pulse[0])) + 
		nessys_apu_gen_pulse(&(nes->apu.pulse[1]));
	uint8_t triangle_sound = nessys_apu_gen_triangle(&(nes->apu.triangle));
	uint8_t noise_sound = nessys_apu_gen_noise(&(nes->apu.noise));
	uint8_t dmc_sound = nessys_apu_gen_dmc(nes);
	float pulse_soundf = (pulse_sound) ?
		95.88f / ((8128.0f / pulse_sound) + 100.0f) : 0.0f;
	float other_soundf = (triangle_sound || noise_sound || dmc_sound) ?
		159.79f / (1.0f / ((triangle_sound / 8227.0f) + (noise_sound / 12241.0f) + (dmc_sound / 22638.0f))) : 0.0f;
	float soundf = pulse_soundf + other_soundf;
	int16_t sound = (int16_t)(16384.0f * (soundf - 0.5f));
	return sound;
}

void nessys_gen_sound(nessys_t* nes)
{
	const float PI = 3.141592654f;
	const float t_inc = 2.0f * PI * 440 / NESSYS_SND_SAMPLES_PER_SECOND;
	static float t = 0.0f;
	static bool playing = false;
	uint32_t s;
	uint32_t max_wr_size = NESSYS_SND_BYTES_PER_BUFFER - 20;
	uint32_t cur_wr_size = (nes->apu.sample_frac_generated >> NESSYS_SND_SAMPLES_FRAC_LOG2) * (NESSYS_SND_BITS_PER_SAMPLE / 8);
	max_wr_size = (cur_wr_size > max_wr_size) ? 0 : max_wr_size -= cur_wr_size;

	uint32_t end_sample_frac = nes->cycle * NESSYS_SND_SAMPLES_FRAC_PER_CYCLE;
	uint32_t wr_sample_frac_size = end_sample_frac - nes->apu.sample_frac_generated;
	uint32_t wr_size = (wr_sample_frac_size >> NESSYS_SND_SAMPLES_FRAC_LOG2) * (NESSYS_SND_BITS_PER_SAMPLE / 8);
	if (wr_size > max_wr_size) {
		wr_size = max_wr_size;
		uint32_t play_pos = nes->sb_main->GetPlayPosition();
		uint32_t wr_pos = nes->sbuf_frame_start;
		if (wr_pos < play_pos) wr_pos += NESSYS_SND_BYTES;
		if (wr_pos - play_pos < 3 * NESSYS_SND_BYTES_PER_BUFFER) wr_size += 40;// else wr_size -= 20;
	}
	nes->apu.sample_frac_generated = end_sample_frac;

	if (wr_size) {
		void* p0;
		void* p1;
		uint32_t size0, size1;
		p0 = nes->sb_main->MapForWrite(nes->sbuf_offset, wr_size, &p1, &size1);
		if (p0 == NULL) {
			printf("Could not lock!!!\n");
			return;
		}
		size0 = wr_size - size1;
		int16_t* buf0 = (int16_t*)p0;
		int16_t* buf1 = (int16_t*)p1;
		for (s = 0; s < size0 / 2; s++) {
			//buf0[s] = (int16_t)(8192.0f * sinf(t));
			buf0[s] = nessys_gen_sound_sample(nes);
			t += t_inc;
		}
		for (s = 0; s < size1 / 2; s++) {
			//buf1[s] = (int16_t)(8192.0f * sinf(t));
			buf1[s] = nessys_gen_sound_sample(nes);
			t += t_inc;
		}
		//uint32_t wr_pos = nes->sb_main->GetWritePosition();
		//uint32_t end_wr_pos = wr_pos + NESSYS_SND_SAMPLES / 2 - 1;
		//uint32_t cur_pos = nes->sb_main->GetPlayPosition();
		//uint32_t wr_pos = (2*NESSYS_SND_SAMPLES / 3) * (((cur_pos / (2*NESSYS_SND_SAMPLES / 3)) + 2) % 3);
		//// if play position is within the region we want to write, wait until it's out
		//while (cur_pos >= wr_pos && cur_pos <= end_wr_pos && playing) {
		//	cur_pos = nes->sb_main->GetPlayPosition();
		//}
		//printf("committing buffer play_pos: %f wr_pos: %f time: %d\n", nes->sb_main->GetPlayPosition() / (float) (NESSYS_SND_BYTES_PER_BUFFER), nes->sb_main->GetWritePosition() / (float)(NESSYS_SND_BYTES_PER_BUFFER), nes->timer->k2GetTime());
		//void* sbuf_ptr = nes->sb_main->k2MapSBufForWrite(wr_pos, 2*NESSYS_SND_SAMPLES / 3);
		//memcpy(sbuf_ptr, buffer, 2 * NESSYS_SND_SAMPLES / 3);
		//nes->sb_main->k2UnmapSBuf(sbuf_ptr, wr_pos, 2*NESSYS_SND_SAMPLES / 3);

		nes->sb_main->k2UnmapSBuf(p0, nes->sbuf_offset, wr_size);

		nes->sbuf_offset += wr_size;
		nes->sbuf_offset = nes->sbuf_offset % NESSYS_SND_BYTES;

		//nes->sb_main->k2UpdateSBuffer(buffer, 2*NESSYS_SND_SAMPLES/4);
		nes->sb_main->k2PlaySBuffer();
		playing = true;

		if ((nes->apu.reg[NESSYS_APU_FRAME_COUNTER_OFFSET] & 0xc0) == 0) {
			// if were 4-step mode, and IRQ disable is 0, then we can produce an interrupt
			uint32_t frac_until_frame_irq = (0x4 << NESSYS_SND_FRAME_FRAC_LOG2) - nes->apu.frame_frac_counter;
			uint32_t cycles_until_frame_irq = frac_until_frame_irq / NESSYS_SND_SAMPLES_FRAC_PER_CYCLE;
			if (nes->frame_irq_cycles == 0 || nes->frame_irq_cycles > cycles_until_frame_irq) nes->frame_irq_cycles = cycles_until_frame_irq;
		} else {
			nes->frame_irq_cycles = 0;
			nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x40;
		}
		if ((nes->apu.reg[0x10] & 0x80 && nes->apu.dmc.length != 0)) {
			// generate a dmc interrupt
			uint32_t cyces_until_dmc_interrupt = ((uint32_t)nes->apu.dmc.bytes_remaining * 8) + nes->apu.dmc.bits_remaining;
			cyces_until_dmc_interrupt *= nes->apu.dmc.period;
			if (nes->dmc_irq_cycles == 0 || nes->dmc_irq_cycles > cyces_until_dmc_interrupt) nes->dmc_irq_cycles = cyces_until_dmc_interrupt;
		} else {
			nes->dmc_irq_cycles = 0;
			nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x80;
		}
	}
}

void nessys_gen_scanline_sprite_map(nessys_ppu_t* ppu)
{
	uint32_t sp, row_start, row_offset, row;
	uint32_t sprite_height = (ppu->reg[0] & 0x20) ? 16 : 8;
	memset(ppu->scanline_num_sprites, 0, NESSYS_PPU_SCANLINES_RENDERED);
	for (sp = 0; sp < NESSYS_PPU_SPRITE_SIZE * NESSYS_PPU_NUM_SPRITES; sp += NESSYS_PPU_SPRITE_SIZE) {
		row_start = ppu->oam[sp + 0];
		for (row_offset = 0; row_offset < sprite_height; row_offset++) {
			row = row_start + row_offset;
			if (ppu->scanline_num_sprites[row] < NESSYS_PPU_MAX_SPRITES_PER_SCALINE && row < NESSYS_PPU_SCANLINES_RENDERED) {
				ppu->scanline_sprite_id[row][ppu->scanline_num_sprites[row]] = sp;
				ppu->scanline_num_sprites[row]++;
			}
		}
	}
}

void K2CALLBACK nessys_display(void* ptr)
{
	nessys_t* nes = (nessys_t*)ptr;
	float clear_color[] = { NESSYS_PPU_PALETTE[0],
							NESSYS_PPU_PALETTE[1],
							NESSYS_PPU_PALETTE[2],
							1.0f };
	float palette[4 * NESSYS_PPU_PAL_SIZE ];
	uint8_t buffer[8 * 1024];
	uint32_t cycles;
	nesssys_set_scanline(nes, -21);
	//printf("cycles remaining: %d\n", nes->cycles_remaining);
	nes->cycle = 0;
	nes->apu.sample_frac_generated = 0;
	nes->sbuf_frame_start = nes->sbuf_offset;
	nes->cycles_remaining = 0;
	nes->vblank_cycles = (nes->cycles_remaining <= 0) ? 1 : nes->cycles_remaining;
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_VBLANK_CLKS);  // 20 vblank lines
	//printf("executed %d cycles\n", cycles);

	// render image
	nes->gfx->k2AttachRenderGroup(nes->rg_win);
	nes->gfx->k2SetScissorRect(0, 0, 640, 480);
	nes->gfx->k2Clear(K2CLEAR_COLOR0, clear_color, 1.0f, 0);

	nes->ppu.status &= ~0xE0;
	nes->ppu.reg[2] &= ~0xE0;
	//nesssys_set_scanline(nes, -1);
	//printf("------------------------------------------------------>Rendering frame scanline: %d scroll: %d %d\n", nes->scanline, nes->ppu.scroll[0], nes->ppu.scroll[1]);
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_PRE_RENDER_CLKS);   // pre-renderline
	//printf("executed %d cycles\n", cycles);

	nesssys_set_scanline(nes, 0);
	nes->ppu.scroll_y = ((nes->ppu.reg[0] & 0x2) << 7) | nes->ppu.scroll[1];
	nes->ppu.max_y = 240 + (nes->ppu.scroll_y & 0x100);// +256 * ((nes->ppu.scroll_y & 0xFF) >= 240);
	nes->ppu.scroll_y_changed = true;
	nes->cycles_remaining += NESSYS_PPU_SCANLINES_RENDERED_CLKS + NESSYS_PPU_SCANLINES_POST_RENDER_CLKS; // 241 scanlines
	int i, c, index;
	do {
		if (nes->ppu.scroll_y_changed) {
			uint16_t old_scroll_y = nes->ppu.scroll_y;
			nes->ppu.scroll_y -= nes->scanline;
			if ((nes->ppu.scroll_y & 0x100) != (old_scroll_y & 0x100) && ((old_scroll_y & 0xff) < 240)) nes->ppu.scroll_y -= 16;
			nes->ppu.scroll_y_changed = false;
			//nes->ppu.scroll_y = 237;
		}
		nes->gfx->k2Clear(K2CLEAR_DEPTH | K2CLEAR_STENCIL, clear_color, 1.0f, 0);
		// initialize palette if rendering background or sprites
		for (i = 0; i < NESSYS_PPU_PAL_SIZE; i++) {
			for (c = 0; c < 3; c++) {
				index = ((i & 0x3) == 0) ? 0 : i;
				palette[4 * i + c] = NESSYS_PPU_PALETTE[3 * nes->ppu.pal[index] + c];
			}
			palette[4 * i + 3] = ((i & 0x3) == 0) ? 0.0f : 1.0f;
		}
		nes->gfx->k2CBUpdate(nes->cb_ppu, &(nes->ppu.reg));

		//if (nes->scanline) 
		//printf("scanline %d ppureg[0]: 0x%x scroll: %d %d %d ppu addr 0x%x\n",
		//	nes->scanline, nes->ppu.reg[0], nes->ppu.scroll[0], nes->ppu.scroll[1], nes->ppu.scroll_y, nes->ppu.mem_addr);

		nes->gfx->k2AttachRasterizerState(nes->rs_normal);
		nes->gfx->k2AttachBlendState(nes->bs_normal);
		nes->gfx->k2AttachDepthState(nes->ds_none);

		nes->gfx->k2SetScissorRect(64, 2*nes->scanline, 576, 480);

		// update palette
		nes->gfx->k2CBUpdate(nes->cb_palette, palette);
		nes->gfx->k2AttachShaderGroup(nes->sg_fill);
		nes->gfx->k2AttachConstantGroup(nes->cg_fill);
		nes->gfx->k2AttachVertexGroup(nes->vg_fullscreen, 4);

		nes->gfx->k2Draw();

		if (nes->ppu.reg[0x1] & 0x08) {
			// enable background rendering
			// update constants
			// nametable update
			index = NESSYS_CHR_NTB_WIN_MIN;
			for (i = 0; i < 4; i++) {
				memcpy(buffer + (i << 10), nessys_ppu_mem(nes, index), 1024);
				index += 1024;
			}
			nes->gfx->k2CBUpdate(nes->cb_nametable, buffer);

			// pattern update
			// copy pattern table at address 0x0, or 0x1000, if bit 4 of ppureg[0] is set, 1KB ata time
			index = NESSYS_CHR_ROM_WIN_MIN + ((((uint32_t)nes->ppu.reg[0]) & 0x10) << 8);
			for (i = 0; i < 4; i++) {
				memcpy(buffer + (i << 10), nessys_ppu_mem(nes, index), 1024);
				index += 1024;
			}
			nes->gfx->k2CBUpdate(nes->cb_pattern, buffer);

			int32_t left_x = 80 - ((nes->ppu.reg[1] << 3) & 0x10);
			nes->gfx->k2SetScissorRect(left_x, 2 * nes->scanline, 576, 480);
			nes->gfx->k2AttachBlendState(nes->bs_mask);
			nes->gfx->k2AttachDepthState(nes->ds_normal);
			nes->gfx->k2AttachShaderGroup(nes->sg_background);
			nes->gfx->k2AttachConstantGroup(nes->cg_background);

			nes->gfx->k2Draw();
		}

		if (nes->ppu.reg[0x1] & 0x10) {
			// enable sprite rendering
			nes->gfx->k2AttachBlendState(nes->bs_mask);
			nes->gfx->k2AttachDepthStateWithRef(nes->ds_sprite, 8);
			nes->gfx->k2AttachRasterizerState(nes->rs_normal);

			int32_t left_x = 80 - ((nes->ppu.reg[1] << 2) & 0x10);
			nes->gfx->k2SetScissorRect(left_x, 2*nes->scanline, 576, 480);

			// update constants that are same for all sprites
			index = NESSYS_CHR_ROM_WIN_MIN;
			for (i = 0; i < 8; i++) {
				memcpy(buffer + (i << 10), nessys_ppu_mem(nes, index), 1024);
				index += 1024;
			}
			nes->gfx->k2CBUpdate(nes->cb_pattern, buffer);
			nes->gfx->k2CBUpdate(nes->cb_palette, palette + 2 * NESSYS_PPU_PAL_SIZE);
			nes->gfx->k2CBUpdate(nes->cb_sprite, nes->ppu.oam);
			nes->gfx->k2AttachConstantGroup(nes->cg_sprite);

			nes->gfx->k2AttachInstancedVertexGroup(nes->vg_sprite, 4, 128);
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

		// compute sprite0 hit
		if ((nes->ppu.reg[0x1] & 0x18) && ((nes->ppu.reg[2] & 0x40)==0x0)) {
			// sprite and background rendering must be enabled
			uint32_t sprite_x = nes->ppu.oam[3];
			uint32_t sprite_y = nes->ppu.oam[0] + 1;
			if (sprite_y < 240) {
				uint32_t global_x = nes->ppu.scroll[0] + ((nes->ppu.reg[0] & 0x01) << 8) + sprite_x;
				uint32_t global_y = nes->ppu.scroll_y                                    + sprite_y;
				if (((nes->ppu.scroll_y & 0xFF) < 240) && ((nes->ppu.scroll_y & 0xFF) + sprite_y >= 240)) global_y += 16;
				if (((nes->ppu.scroll_y & 0xFF) >=240) && ((nes->ppu.scroll_y & 0xFF) + sprite_y >= 256)) global_y -= 256;
				uint32_t ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
				uint32_t pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
				//printf("gx/y: 0x%x 0x%x ntb_addr: 0x%x pat_addr: 0x%x ", global_x, global_y, ntb_addr, pat_addr);
				uint64_t bg_pattern[6];
				bg_pattern[0] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
				bg_pattern[0] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
				global_x += 8;
				ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
				pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
				bg_pattern[1] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
				bg_pattern[1] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
				if (((global_y & 0xFF) < 240) && ((global_y & 0xFF) + 8) >= 240) global_y += 16;
				if (((global_y & 0xFF) >=240) && ((global_y & 0xFF) + 8) >= 256) global_y -= 256;
				global_y += 8;
				ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
				pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
				bg_pattern[3] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
				bg_pattern[3] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
				global_x -= 8;
				ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
				pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
				bg_pattern[2] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
				bg_pattern[2] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
				if (((global_y & 0xFF) < 240) && ((global_y & 0xFF) + 8) >= 240) global_y += 16;
				if (((global_y & 0xFF) >=240) && ((global_y & 0xFF) + 8) >= 256) global_y -= 256;
				global_y += 8;
				ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
				pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
				bg_pattern[4] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
				bg_pattern[4] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
				global_x += 8;
				ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
				pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
				bg_pattern[5] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
				bg_pattern[5] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
				uint64_t sp_pattern[2];
				uint32_t sp_addr = (nes->ppu.reg[0] & 0x20) ? ((nes->ppu.oam[1] & 0x1) << 8) : ((nes->ppu.reg[0] & 0x08) << 5) | (nes->ppu.oam[1] & 0x1);
				sp_addr |= nes->ppu.oam[1] & 0xFE;
				sp_addr <<= 4;
				sp_pattern[0] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + sp_addr));
				sp_pattern[0] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + sp_addr + 8));
				sp_addr += 16;
				sp_pattern[1] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + sp_addr));
				sp_pattern[1] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + sp_addr + 8));
				uint32_t local_x = global_x & 0x7;
				uint32_t local_y = global_y & 0x7;
				uint64_t x_mask = (0xFF << local_x) & 0xFF;
				x_mask |= (x_mask << 8);
				x_mask |= (x_mask << 16);
				x_mask |= (x_mask << 32);
				uint64_t y_mask = 0;
				y_mask = ~y_mask >> (8 * local_y);
				bg_pattern[0] = ((bg_pattern[0] << local_x) >> (8 * local_y)) & x_mask & y_mask;
				bg_pattern[0] |= ((bg_pattern[1] >> (8 - local_x)) >> (8 * local_y)) & ~x_mask & y_mask;
				bg_pattern[0] |= ((bg_pattern[2] << local_x) << (8 * (8 - local_y))) & x_mask & ~y_mask;
				bg_pattern[0] |= ((bg_pattern[3] >> (8 - local_x)) << (8 * (8 - local_y))) & ~x_mask & ~y_mask;
				bg_pattern[1] = ((bg_pattern[2] << local_x) >> (8 * local_y)) & x_mask & y_mask;
				bg_pattern[1] |= ((bg_pattern[3] >> (8 - local_x)) >> (8 * local_y)) & ~x_mask & y_mask;
				bg_pattern[1] |= ((bg_pattern[4] << local_x) << (8 * (8 - local_y))) & x_mask & ~y_mask;
				bg_pattern[1] |= ((bg_pattern[5] >> (8 - local_x)) << (8 * (8 - local_y))) & ~x_mask & ~y_mask;

				// slip sprite, if needed
				uint64_t hit_mask;
				uint32_t s, max_s = (nes->ppu.reg[0] & 0x20) ? 2 : 1;
				if (nes->ppu.oam[2] & 0x40) {
					// flip horizontally
					for (s = 0; s < max_s; s++) {
						sp_pattern[s] = ((sp_pattern[s] & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((sp_pattern[s] & 0x0F0F0F0F0F0F0F0FULL) << 4);
						sp_pattern[s] = ((sp_pattern[s] & 0xCCCCCCCCCCCCCCCCULL) >> 2) | ((sp_pattern[s] & 0x3333333333333333ULL) << 2);
						sp_pattern[s] = ((sp_pattern[s] & 0xAAAAAAAAAAAAAAAAULL) >> 1) | ((sp_pattern[s] & 0x5555555555555555ULL) << 1);
					}
				}
				if (nes->ppu.oam[2] & 0x80) {
					// flip vertically
					if (max_s == 2) {
						// swap the upper and lower sprites
						hit_mask = sp_pattern[0];
						sp_pattern[0] = sp_pattern[1];
						sp_pattern[1] = hit_mask;
					}
					for (s = 0; s < max_s; s++) {
						sp_pattern[s] = ((sp_pattern[s] & 0xFFFFFFFF00000000ULL) >> 32) | ((sp_pattern[s] & 0x00000000FFFFFFFFULL) << 32);
						sp_pattern[s] = ((sp_pattern[s] & 0xFFFF0000FFFF0000ULL) >> 16) | ((sp_pattern[s] & 0x0000FFFF0000FFFFULL) << 16);
						sp_pattern[s] = ((sp_pattern[s] & 0xFF00FF00FF00FF00ULL) >> 8) | ((sp_pattern[s] & 0x00FF00FF00FF00FFULL) << 8);
					}
				}

				// check if sprite is partially outside the view
				x_mask = 0xFF;
				if (((nes->ppu.reg[1] & 0x06) != 0x06) && sprite_x < 8) {
					// if either background or tile clipping is enabled, don't test pixels coordinates below 8
					x_mask >>= 8 - sprite_x;
				} else if (sprite_x > 247) {
					x_mask <<= sprite_x - 247;
					x_mask &= 0xFF;
				}
				x_mask |= (x_mask << 8);
				x_mask |= (x_mask << 16);
				x_mask |= (x_mask << 32);

				y_mask = 0;
				y_mask = ~y_mask;
				if (sprite_y > 232) {
					y_mask >>= 8 * (sprite_y - 232);
					sp_pattern[0] &= y_mask;
					sp_pattern[1] = 0;
				}
				else if (sprite_y > 224) {
					y_mask >>= 8 * (sprite_y - 224);
					sp_pattern[1] &= y_mask;
				}

				bool sprite0_hit = false;
				uint32_t hit_x = 0, hit_y = 0;
				uint32_t x, y;
				for (s = 0; s < max_s; s++) {
					hit_mask = sp_pattern[s] & bg_pattern[s] & x_mask;
					if (hit_mask) {
						for (y = 0; y < 8 && !sprite0_hit; y++) {
							if (hit_mask & 0xFF) {
								for (x = 0; x < 8 && !sprite0_hit; x++) {
									if (hit_mask & 0x80) {
										sprite0_hit = true;
										hit_x = sprite_x + x;
										hit_y = sprite_y + 8 * s + y;
									}
									else {
										hit_mask <<= 1;
									}
								}
							}
							else {
								hit_mask >>= 8;
							}
						}
					}
				}
				//printf("sprite0 pos: %d %d ", sprite_x, sprite_y);
				if (sprite0_hit) {
					if ((int32_t)hit_y < nes->scanline) {
						nes->sprite0_hit_cycles = 1;
					} else {
						nes->sprite0_hit_cycles = NESSYS_PPU_CLK_PER_SCANLINE * (hit_y - nes->scanline) + hit_x - nes->scanline_cycle;
					}
					//printf(" - hit\n");
				} else {
					//printf(" - miss\n");

				}
			}
		}

		// need to only do this if sprtes have changed position
		nessys_gen_scanline_sprite_map(&(nes->ppu));

		cycles = nessys_exec_cpu_cycles(nes, 0);
		//printf("executed %d cycles; cycles remaining %d\n", cycles, nes->cycles_remaining);
	} while (nes->cycles_remaining > 100);

	nessys_gen_sound(nes);
	nes->win->k2SwapBuffer();
	//printf("end frame: %d\n", nes->timer->k2GetTime());

}

uint8_t nessys_masked_joypad(uint8_t joypad)
{
	// this function takes the current joypad state, and masks off the up/down keys if they are both pressed
	// and also does the same for the left/right keys
	uint8_t result = joypad;
	if ((result & (NESSYS_STD_CONTROLLER_BUTTON_UP_MASK | NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK)) ==
		(NESSYS_STD_CONTROLLER_BUTTON_UP_MASK | NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK)) {
		result &= ~(NESSYS_STD_CONTROLLER_BUTTON_UP_MASK | NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK);
	}
	if ((result & (NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK | NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK)) ==
		(NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK | NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK)) {
		result &= ~(NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK | NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK);
	}
	return result;
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
		if (state == K2KEYSTATE_RELEASED) {
			nes->apu.joypad[0] &= ~key_mask;
		} else {
			nes->apu.joypad[0] |= key_mask;
		}
		if (nes->apu.joy_control & 0x1) {
			nes->apu.latched_joypad[0] = nessys_masked_joypad(nes->apu.joypad[0]);
			nes->apu.latched_joypad[1] = nessys_masked_joypad(nes->apu.joypad[1]);
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
		success = nessys_init_mapper(nes);
	}
	if(success) {
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
		// map the last 32KB of rom data
		// if rom is less than 32KB, then map from the beggining, and wrap the addresses
		mem_offset = (nes->prg_rom_size <= NESSYS_PRG_ADDR_SPACE - NESSYS_PRG_ROM_START) ?
			0 : (nes->prg_rom_size - (NESSYS_PRG_ADDR_SPACE - NESSYS_PRG_ROM_START));
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
	if (nes->prg_ram_base) {
		mem_offset = 0;
		for (b = NESSYS_PRG_RAM_START_BANK; b <= NESSYS_PRM_RAM_END_BANK; b++) {
			nes->prg_rom_bank[b] = nes->prg_ram_base + mem_offset;
			nes->prg_rom_bank_mask[b] = NESSYS_PRG_MEM_MASK;
			mem_offset += NESSYS_PRG_BANK_SIZE;
		}
	} else {
		for (b = NESSYS_PRG_RAM_START_BANK; b <= NESSYS_PRM_RAM_END_BANK; b++) {
			nes->prg_rom_bank[b] = &nes->reg.pad0;
			nes->prg_rom_bank_mask[b] = 0x0;
		}
	}
	if (nes->ppu.chr_rom_base || nes->ppu.chr_ram_base) {
		mem_offset = 0;
		uint8_t* base = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_base : nes->ppu.chr_ram_base;
		uint32_t size = (nes->ppu.chr_rom_base) ? nes->ppu.chr_rom_size : nes->ppu.chr_ram_size;
		for (b = 0; b <= NESSYS_CHR_ROM_END_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = base + mem_offset;
			nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
			mem_offset += NESSYS_CHR_BANK_SIZE;
			if (mem_offset >= size) mem_offset -= size;
		}
	} else {
		for (b = 0; b <= NESSYS_CHR_ROM_END_BANK; b++) {
			nes->ppu.chr_rom_bank[b] = &nes->reg.pad0;
			nes->ppu.chr_rom_bank_mask[b] = 0x0;
		}
	}
	// map name table
	if (nes->ppu.mem_4screen) {
		// if we allocated space for 4 screens, then directly map address space
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.mem + 0x400;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.mem_4screen;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem_4screen + 0x400;
	} else {
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0] = nes->ppu.mem;
		nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3] = nes->ppu.mem + 0x400;
		if (nes->ppu.name_tbl_vert_mirror) {
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
		} else {
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 1] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 0];
			nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 2] = nes->ppu.chr_rom_bank[NESSYS_CHR_NTB_START_BANK + 3];
		}
	}
	for (b = NESSYS_CHR_NTB_START_BANK + 4; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank[b] = nes->ppu.chr_rom_bank[b - 4];
	}
	for (b = NESSYS_CHR_NTB_START_BANK; b <= NESSYS_CHR_NTB_END_BANK; b++) {
		nes->ppu.chr_rom_bank_mask[b] = NESSYS_CHR_MEM_MASK;
	}
}

void nessys_add_backtrace(nessys_t* nes)
{
	nessys_cpu_backtrace_t* bt = &(nes->backtrace[nes->backtrace_entry]);
	bt->scanline = nes->scanline;
	bt->scanline_cycle = nes->scanline_cycle;
	bt->reg = nes->reg;
	nes->backtrace_entry++;
	nes->backtrace_entry %= NESSYS_NUM_CPU_BACKTRACE_ENTRIES;
}

void nessys_dump_backtrace(nessys_t* nes)
{
	printf("backtrace\n");
	nessys_cpu_backtrace_t* bt;
	uint32_t entry = nes->backtrace_entry;
	uint32_t i;
	for (i = 0; i < NESSYS_NUM_CPU_BACKTRACE_ENTRIES; i++) {
		bt = &(nes->backtrace[entry]);
		printf("%d: scan/cycle: %d/%d pc: 0x%x a: 0x%x x: 0x%x y: 0x%x s: 0x%X p: 0x%x\n",
			i, bt->scanline, bt->scanline_cycle, bt->reg.pc, bt->reg.a, bt->reg.x, bt->reg.y, bt->reg.s, bt->reg.p);
		entry++;
		entry %= NESSYS_NUM_CPU_BACKTRACE_ENTRIES;
	}
}

uint32_t nessys_exec_cpu_cycles(nessys_t* nes, uint32_t num_cycles)
{
	uint32_t cycle_count = 0;
	nes->cycles_remaining += num_cycles; // cycles to execute
	bool done = false;
	bool ppu_ever_written = false;
	const c6502_op_code_t* op = NULL;
	uint16_t addr = nes->reg.pc, indirect_addr = 0x0;
	uint8_t* operand = &nes->reg.a;
	uint16_t bank, offset;
	uint16_t result;
	uint16_t mem_addr_mask;
	uint8_t* pc_ptr = NULL;
	uint16_t penalty_cycles;
	uint8_t overflow;
	bool vblank_interrupt;
	uint8_t skip_print = 0;
	bool ppu_write, apu_write, rom_write;
	uint8_t data_change;  // when writing to addressable memory, indicates which bits changed
	uint8_t reset_ppu_status_after_nmi = 0;
	int32_t next_scanline_cycle = 0;
	nes->ppu.reg[2] &= 0x1F;
	nes->ppu.reg[2] |= nes->ppu.status;
	while (!done) {
		pc_ptr = nessys_mem(nes, nes->reg.pc, &bank, &offset);
		op = &C6502_OP_CODE[*pc_ptr];
		ppu_write = false;
		data_change = 0x0;
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
			penalty_cycles += (addr + nes->reg.x >= 0x100) * op->penalty_cycles;
			addr |= ((uint16_t)*nessys_mem(nes, nes->reg.pc + 2, &bank, &offset)) << 8;
			addr += nes->reg.x;
			operand = nessys_mem(nes, addr, &bank, &offset);
			break;
		case C6502_ADDR_ABSOLUTE_Y:
			addr = *nessys_mem(nes, nes->reg.pc + 1, &bank, &offset);
			penalty_cycles += (addr + nes->reg.y >= 0x100) * op->penalty_cycles;
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
			penalty_cycles += (addr + nes->reg.y >= 0x100) * op->penalty_cycles;
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
		if (nes->sprite0_hit_cycles > 0 && nes->sprite0_hit_cycles < NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles)) {
			nes->ppu.status |= 0x40;
			nes->ppu.reg[2] |= 0x40;
			nes->sprite0_hit_cycles = 0;
		}

		uint32_t instruction_cycles = (NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles));
		bool mapper_irq = (nes->mapper_irq_cycles > 0 && nes->mapper_irq_cycles < instruction_cycles);
		bool frame_irq = false;// (nes->frame_irq_cycles > 0 && nes->frame_irq_cycles < instruction_cycles);
		bool dmc_irq = false;// (nes->dmc_irq_cycles > 0 && nes->dmc_irq_cycles < instruction_cycles);
		bool irq_interrupt = !(nes->reg.p & C6502_P_I) && mapper_irq;// || frame_irq || dmc_irq;
		nes->cpu_cycle_inc = 0;
		if (nes->cycles_remaining < (int32_t)instruction_cycles || vblank_interrupt) {
			done = !vblank_interrupt;
			if (vblank_interrupt) {
				// get the stack base
				//printf("-----------------------------------> Vblank interrupt from 0x%x, stack 0x%x\n", nes->reg.pc, nes->reg.s);
				//if (nes->in_nmi) {
				//	printf("NMI trapped during NMI!!\n");
				//}
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = nes->reg.pc >> 8;        nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.pc & 0xFF;      nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.p & ~C6502_P_B; nes->reg.s--;
				nes->reg.pc = *((uint16_t*) nessys_mem(nes, NESSYS_NMI_VECTOR, &bank, &offset));
				nes->reg.p |= C6502_P_I;
				nes->in_nmi++;
				nes->ppu.old_status  = (nes->ppu.status & 0xe0);
				nes->ppu.old_status |= (nes->ppu.reg[2] & 0x1f);

				nes->cpu_cycle_inc = 7;
				//cycle_count += NESSYS_PPU_PER_CPU_CLK * (7);
				//// cycles remaining may go negative if we don't have enough to cover the NMI
				//nes->cycles_remaining -= NESSYS_PPU_PER_CPU_CLK * (7);
				//nes->cycle += NESSYS_PPU_PER_CPU_CLK * (7);
			}
		} else if( irq_interrupt) {
			// mapper interrupt
			// get the stack base
			operand = nessys_mem(nes, 0x100, &bank, &offset);
			*(operand + nes->reg.s) = nes->reg.pc >> 8;        nes->reg.s--;
			*(operand + nes->reg.s) = nes->reg.pc & 0xFF;      nes->reg.s--;
			*(operand + nes->reg.s) = nes->reg.p & ~C6502_P_B; nes->reg.s--;
			nes->reg.pc = *((uint16_t*)nessys_mem(nes, NESSYS_IRQ_VECTOR, &bank, &offset));
			nes->reg.p |= C6502_P_I;
			if (mapper_irq) nes->mapper_irq_cycles = 1;
			if (frame_irq) {
				nes->frame_irq_cycles = 1;
				nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= 0x40;
			}
			if (dmc_irq) {
				nes->dmc_irq_cycles = 1;
				nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= 0x80;
			}

			//printf("-----------------------------------> Mapper interrupt scanline: %d\n", nes->scanline);
			nes->cpu_cycle_inc = 7;
			//cycle_count += NESSYS_PPU_PER_CPU_CLK * (7);
			//// cycles remaining may go negative if we don't have enough to cover the NMI
			//nes->cycles_remaining -= NESSYS_PPU_PER_CPU_CLK * (7);
			//nes->cycle += NESSYS_PPU_PER_CPU_CLK* (7);
		} else {
			skip_print = 1;
			if (skip_print == 0) {
				printf("0x%x: %s(0x%x) a:0x%x d:0x%x c: %d scan: %d", nes->reg.pc, op->ins_name, *pc_ptr, addr, *operand, nes->cycle, nes->scanline);
				if (op->flags & (C6502_FL_ILLEGAL | C6502_FL_UNDOCUMENTED)) {
					printf(" - warning: illegal or undocumented instruction\n");
				}
				printf("\n");
			} else {
				if (skip_print < op->num_bytes) skip_print = 0;
				else skip_print -= op->num_bytes;
			}

			// add debug info
			nessys_add_backtrace(nes);

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
					// immediately update data if reading from palette data; otherwise defer until after this instruction
					if((nes->ppu.mem_addr & 0x3f00) == 0x3f00) nes->ppu.reg[7] = *nessys_ppu_mem(nes, nes->ppu.mem_addr);
					break;
				}
				break;
			case NESSYS_APU_REG_START_BANK:
				if (offset >= NESSYS_APU_JOYPAD0_OFFSET && offset <= NESSYS_APU_JOYPAD1_OFFSET) {
					uint8_t j = offset - NESSYS_APU_JOYPAD0_OFFSET;
					nes->apu.reg[offset] = nes->apu.latched_joypad[j] & 0x1;
					//if ((nes->apu.joy_control & 0x1) == 0x0) {
					//	nes->apu.latched_joypad[j] >>= 1;
					//}
				} else if (offset == NESSYS_APU_STATUS_OFFSET) {
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= 0xc0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.pulse[0].length) ? 0x1 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.pulse[1].length) ? 0x2 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.triangle.length) ? 0x4 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.noise.length)    ? 0x8 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.dmc.bytes_remaining) ? 0x10 : 0x0;
				}
				break;
			}

			//if (nes->reg.s == 0xfe) printf("------------------> stack about to overflow: 0x%x!!!\n", nes->reg.s);;
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = (uint8_t)result;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						data_change = (*operand ^ (uint8_t)result);
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					if (nes->mapper_flags & NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE) result = *operand;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t) result);
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
				//printf("----------------------------------> BRK\n");
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = (uint8_t)result;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						data_change = (*operand ^ (uint8_t)result);
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					if (nes->mapper_flags & NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE) result = *operand;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = (uint8_t)result;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						data_change = (*operand ^ (uint8_t)result);
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					if (nes->mapper_flags & NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE) result = *operand;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
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
				//if (op->addr == C6502_ADDR_INDIRECT)   printf("----------------------> JMP ():  iaddr = 0x%x addr = 0x%x\n", indirect_addr, addr);
				if (nes->reg.pc - op->num_bytes == addr) skip_print = 1;
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = (uint8_t)result;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						data_change = (*operand ^ (uint8_t)result);
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					if (nes->mapper_flags & NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE) result = *operand;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = (uint8_t)result;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						data_change = (*operand ^ (uint8_t)result);
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					if (nes->mapper_flags & NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE) result = *operand;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = (uint8_t)result;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = (uint8_t)result;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = (uint8_t)result;
							break;
						}
					} else {
						data_change = (*operand ^ (uint8_t)result);
						*operand = (uint8_t)result;
					}
				} else {
					rom_write = true;
					if (nes->mapper_flags & NESSYS_MAPPER_FLAG_DATA_FROM_SOURCE) result = *operand;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, (uint8_t)result);
				}
				break;
			case C6502_INS_RTI:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->reg.s++; nes->reg.p =  *(operand + nes->reg.s) | C6502_P_U | C6502_P_B;
				nes->reg.s++; nes->reg.pc = *(operand + nes->reg.s);
				nes->reg.s++; nes->reg.pc |= ((uint16_t) *(operand + nes->reg.s)) << 8;
				if (nes->in_nmi) {
					nes->in_nmi--;
					nes->ppu.reg[2] = nes->ppu.old_status;
					reset_ppu_status_after_nmi = 3;
				}
				//printf("-----------------------------------> Return Interrupt to 0x%x, stack 0x%x\n", nes->reg.pc, nes->reg.s);
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
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							//if ((nes->apu.status & 0x10) != (nes->reg.a & 0x10)) {
							//	printf("changing dmc dmc\n");
							//}
							nes->apu.status = nes->reg.a;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = nes->reg.a;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = nes->reg.a;
							break;
						}
					} else {
						data_change = (*operand ^ nes->reg.a);
						*operand = nes->reg.a;
					}
				} else {
					rom_write = true;
					result = nes->reg.a;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, nes->reg.a);
				}
				break;
			case C6502_INS_STX:
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = nes->reg.x;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = nes->reg.x;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = nes->reg.x;
							break;
						}
					} else {
						data_change = (*operand ^ nes->reg.x);
						*operand = nes->reg.x;
					}
				} else {
					rom_write = true;
					result = nes->reg.x;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, nes->reg.x);
				}
				break;
			case C6502_INS_STY:
				if (bank < NESSYS_PRG_ROM_START_BANK) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_JOYPAD0_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
							nes->apu.status = nes->reg.y;
							break;
						case NESSYS_APU_JOYPAD0_OFFSET:
							nes->apu.joy_control = nes->reg.y;
							break;
						case NESSYS_APU_FRAME_COUNTER_OFFSET:
							nes->apu.frame_counter = nes->reg.y;
							break;
						}
					} else {
						data_change = (*operand ^ nes->reg.y);
						*operand = nes->reg.y;
					}
				} else {
					rom_write = true;
					result = nes->reg.y;
					//printf("writing to rom area a=0x%x d=0x%x\n", addr, nes->reg.y);
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

			if (reset_ppu_status_after_nmi) {
				reset_ppu_status_after_nmi--;
				if (reset_ppu_status_after_nmi == 0) {
					nes->ppu.reg[2] &= 0x1F;
					nes->ppu.reg[2] |= nes->ppu.status;
				}
			}

			skip_print = 1;
			if (bank == NESSYS_PPU_REG_START_BANK) {
				switch (offset) {
				case 2:
					// if we read or write ppu status, update it's value from the master status reg
					nes->ppu.reg[2] &= 0x1F;
					nes->ppu.reg[2] |= nes->ppu.status;
					nes->ppu.addr_toggle = 0;
					break;
				case 7:
					// latch read data after the instruction
					if (!ppu_write) {
						nes->ppu.reg[7] = *nessys_ppu_mem(nes, nes->ppu.mem_addr);
						//printf("PPU read: 0x%x 0x%x\n", nes->ppu.mem_addr, nes->ppu.reg[7]);
						// if bit 2 is 0, increment address by 1 (one step horizontal), otherwise, increment by 32 (one step vertical)
						//printf("reading ppu addr: 0x%x\n", nes->ppu.mem_4screen);
						nes->ppu.mem_addr += !((nes->ppu.reg[0] & 0x4) >> 1) + ((nes->ppu.reg[0] & 0x4) << 3);
						nes->ppu.mem_addr &= NESSYS_PPU_WIN_MAX;
					}
					break;
				}
			}

			if (apu_write) {
				if (offset != NESSYS_APU_JOYPAD0_OFFSET) {
					nessys_gen_sound(nes);
				}
				uint8_t pulse_sel = (offset >> 2) & 0x1;
				switch (offset) {
				case 0x0:
				case 0x4:
					nes->apu.pulse[pulse_sel].env.volume = nes->apu.reg[offset] & 0xf;// (nes->apu.reg[NESSYS_APU_STATUS_OFFSET] & (1 << pulse_sel)) ? nes->apu.reg[offset] & 0xf : 0;
					nes->apu.pulse[pulse_sel].env.flags &= ~(NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
					nes->apu.pulse[pulse_sel].env.flags |= nes->apu.reg[offset] & (NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
					nes->apu.pulse[pulse_sel].duty = NESSYS_APU_PULSE_DUTY_TABLE[(nes->apu.reg[offset] >> 6) & 0x3];
					break;
				case 0x1:
				case 0x5:
					nes->apu.pulse[pulse_sel].sweep_shift = nes->apu.reg[offset] & 0x7;
					nes->apu.pulse[pulse_sel].sweep_period = (nes->apu.reg[offset] >> 4) & 0x7;
					nes->apu.pulse[pulse_sel].env.flags &= ~(NESSYS_APU_PULSE_FLAG_SWEEP_EN | NESSYS_APU_PULSE_FLAG_SWEEP_NEGATE);
					nes->apu.pulse[pulse_sel].env.flags|= nes->apu.reg[offset] & (NESSYS_APU_PULSE_FLAG_SWEEP_EN | NESSYS_APU_PULSE_FLAG_SWEEP_NEGATE);
					nes->apu.pulse[pulse_sel].env.flags |= NESSYS_APU_PULSE_FLAG_SWEEP_RELOAD;
					break;
				case 0x2:
				case 0x6:
					nes->apu.pulse[pulse_sel].period &= 0xFF00;
					nes->apu.pulse[pulse_sel].period |= nes->apu.reg[offset];
					nes->apu.pulse[pulse_sel].sweep_orig_period = nes->apu.pulse[pulse_sel].period;
					break;
				case 0x3:
				case 0x7:
					nes->apu.pulse[pulse_sel].period &= 0x00FF;
					nes->apu.pulse[pulse_sel].period |= ((uint16_t)nes->apu.reg[offset] << 8) & 0x700;
					nes->apu.pulse[pulse_sel].sweep_orig_period = nes->apu.pulse[pulse_sel].period;
					if (nes->apu.status & (1 << pulse_sel)) {
						nes->apu.pulse[pulse_sel].length = NESSYS_APU_PULSE_LENGTH_TABLE[(nes->apu.reg[offset] >> 3) & 0x1f];
					} else {
						nes->apu.pulse[pulse_sel].length = 0;
					}
					nes->apu.pulse[pulse_sel].duty_phase = 0;
					nes->apu.pulse[pulse_sel].cur_time_frac = 0;
					nes->apu.pulse[pulse_sel].env.flags |= NESSYS_APU_PULSE_FLAG_ENV_START;
					break;
				case 0x8:
					nes->apu.triangle.reload = nes->apu.reg[0x8] & 0x7f;
					nes->apu.triangle.flags &= ~(NESSYS_APU_TRIANGLE_FLAG_CONTROL);
					nes->apu.triangle.flags |= nes->apu.reg[0x8] & NESSYS_APU_TRIANGLE_FLAG_CONTROL;
					break;
				case 0xa:
					nes->apu.triangle.period &= 0xFF00;
					nes->apu.triangle.period |= nes->apu.reg[0xa];
					break;
				case 0xb:
					nes->apu.triangle.period &= 0x00FF;
					nes->apu.triangle.period |= ((uint16_t)nes->apu.reg[0xb] << 8) & 0x700;
					if (nes->apu.status & 0x4) {
						nes->apu.triangle.length = NESSYS_APU_PULSE_LENGTH_TABLE[(nes->apu.reg[0xb] >> 3) & 0x1f];
					} else {
						nes->apu.triangle.length = 0;
					}
					nes->apu.triangle.sequence = 0x10;
					nes->apu.triangle.flags |= NESSYS_APU_TRIANGLE_FLAG_RELOAD;
					break;
				case 0xc:
					nes->apu.noise.env.volume = nes->apu.reg[0xc] & 0xf;// (nes->apu.reg[NESSYS_APU_STATUS_OFFSET] & 0x4) ? nes->apu.reg[0xc] & 0xf : 0;
					nes->apu.noise.env.flags &= ~(NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
					nes->apu.noise.env.flags |= nes->apu.reg[0xc] & (NESSYS_APU_PULSE_FLAG_HALT_LENGTH | NESSYS_APU_PULSE_FLAG_CONST_VOLUME);
					break;
				case 0xe:
					nes->apu.noise.env.flags &= ~(NESSYS_APU_NOISE_FLAG_MODE);
					nes->apu.noise.env.flags |= nes->apu.reg[0xe] & NESSYS_APU_NOISE_FLAG_MODE;
					nes->apu.noise.period = NESSYS_APU_NOISE_PERIOD_TABLE[nes->apu.reg[0xe] & 0xf];
					break;
				case 0xf:
					if (nes->apu.status & 0x8) {
						nes->apu.noise.length = NESSYS_APU_PULSE_LENGTH_TABLE[(nes->apu.reg[0xf] >> 3) & 0x1f];
					} else {
						nes->apu.noise.length = 0;
					}
					nes->apu.noise.env.flags |= NESSYS_APU_PULSE_FLAG_ENV_START;
					break;
				case 0x10:
					nes->apu.dmc.flags &= ~(NESSYS_APU_DMC_FLAG_IRQ_ENABLE | NESSYS_APU_DMC_FLAG_LOOP);
					nes->apu.dmc.flags |= nes->apu.reg[0x10] & (NESSYS_APU_DMC_FLAG_IRQ_ENABLE | NESSYS_APU_DMC_FLAG_LOOP);
					nes->apu.dmc.period = NESSYS_APU_DMC_PERIOD_TABLE[nes->apu.reg[0x10] & 0xf];
					break;
				case 0x11:
					nes->apu.dmc.output = nes->apu.reg[0x11] & 0x7f;
					break;
				case 0x12:
					nes->apu.dmc.start_addr = 0xc000 + (((uint16_t)nes->apu.reg[0x12]) << 6);
					break;
				case 0x13:
					nes->apu.dmc.length = (((uint16_t)nes->apu.reg[0x13]) << 4) + 1;
					if ((nes->apu.reg[0x13] & 0x80) == 0) {
						nes->dmc_irq_cycles = 0;
						nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x80;
					}
					break;
				case NESSYS_APU_STATUS_OFFSET:
					if (!(nes->apu.status & 0x1)) {
						nes->apu.pulse[0].length = 0;
					}
					if (!(nes->apu.status & 0x2)) {
						nes->apu.pulse[1].length = 0;
					}
					if (!(nes->apu.status & 0x4)) {
						nes->apu.triangle.length = 0;
					}
					if (!(nes->apu.status & 0x8)) {
						nes->apu.noise.length = 0;
					}
					if (!(nes->apu.status & 0x10)) {
						nes->apu.dmc.bytes_remaining = 0;
						nes->apu.dmc.flags &= ~NESSYS_APU_DMC_FLAG_DMA_ENABLE;
					} else {
						if (nes->apu.dmc.bytes_remaining == 0) {
							nes->apu.dmc.cur_addr = nes->apu.dmc.start_addr;
							nes->apu.dmc.bytes_remaining = nes->apu.dmc.length;
						}
						nes->apu.dmc.flags |= NESSYS_APU_DMC_FLAG_DMA_ENABLE;
					}
					break;
				case NESSYS_APU_JOYPAD0_OFFSET:
					if (nes->apu.joy_control & 0x1) {
						nes->apu.latched_joypad[0] = nessys_masked_joypad(nes->apu.joypad[0]);
						nes->apu.latched_joypad[1] = nessys_masked_joypad(nes->apu.joypad[1]);
					}
					break;
				case NESSYS_APU_FRAME_COUNTER_OFFSET:
					//nes->apu.frame_frac_counter = 1 << NESSYS_SND_FRAME_FRAC_LOG2;
					nes->apu.frame_frac_counter &= ~NESSYS_SND_FRAME_FRAC_MASK;
					nes->apu.frame_frac_counter |= (nes->apu.frame_counter & 0x80) ? NESSYS_SND_FRAME_FRAC_MASK : 0;
					break;
				}
			} else {
				if (bank == NESSYS_APU_REG_START_BANK) {
					switch (offset) {
					case NESSYS_APU_STATUS_OFFSET:
						if (nes->frame_irq_cycles == 1) {
							nes->frame_irq_cycles = 0;
							nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x40;
						}
						break;
					case NESSYS_APU_JOYPAD0_OFFSET:
					case NESSYS_APU_JOYPAD1_OFFSET:
						uint8_t j = offset - NESSYS_APU_JOYPAD0_OFFSET;
						if ((nes->apu.joy_control & 0x1) == 0x0) {
							nes->apu.latched_joypad[j] >>= 1;
						}
						break;
					}
				}
			}
			if (ppu_write) {
				//printf("ppu_write offset 0x%x addr: 0x%x data: 0x%x scanline: %d scan cycle: %d data_change 0x%x %d\n", 
				//	offset, nes->ppu.mem_addr, (offset == 0x14) ? nes->apu.reg[0x14] : nes->ppu.reg[offset], nes->scanline, nes->scanline_cycle, data_change, ppu_ever_written);
				if (bank == NESSYS_PPU_REG_START_BANK) {
					nes->ppu.reg[2] = (nes->ppu.status & 0xE0) | (nes->ppu.reg[offset & 0x7] & 0x1F);
				}
				switch (offset) {
				case 0x0:
					// we only re-render frame if bits that change rendering changes
					ppu_ever_written = ppu_ever_written || (data_change & 0x39);
					break;
				case 0x1:
					// we only re-render the frame if the register actually changed value
					ppu_ever_written = ppu_ever_written || data_change;
					break;
				case 0x3:
					// writing this reg during rendering may have odd effects...not implemented, but re-render anyway
					ppu_ever_written = true;
					break;
				case 0x4:
					ppu_ever_written = true;
					nes->ppu.oam[nes->ppu.reg[3]] = nes->ppu.reg[4];
					nes->ppu.reg[3]++;
					break;
				case 0x5:
					// only update if the scroll value actually changed, and only in x direction (y updates at next frame)
					ppu_ever_written = ppu_ever_written || ((nes->ppu.addr_toggle == 0) && (nes->ppu.scroll[0] != nes->ppu.reg[5]));
					nes->ppu.scroll[nes->ppu.addr_toggle] = nes->ppu.reg[5];
					// update the vram address as well
					//printf("update ppu addr for scroll %d %d %d addr: 0x%x\n", nes->ppu.scroll[0], nes->ppu.scroll[1], nes->ppu.scroll_y, nes->ppu.mem_addr);
					//printf("ppu wr 2005: toggle %d 0x%x\n", nes->ppu.addr_toggle, nes->ppu.reg[5]);
					if (nes->ppu.addr_toggle) {
						nes->ppu.t_mem_addr &= 0x0c1f;
						nes->ppu.t_mem_addr |= ((uint16_t)nes->ppu.reg[5] << 2) & 0x03e0;
						nes->ppu.t_mem_addr |= ((uint16_t)nes->ppu.reg[5] << 12) & 0x7000;
					} else {
						nes->ppu.t_mem_addr &= 0xffe0;
						nes->ppu.t_mem_addr |= (nes->ppu.reg[5] >> 3) & 0x1f;
					}
					nes->ppu.addr_toggle = !nes->ppu.addr_toggle;
					break;
				case 0x6:
					ppu_ever_written = true;  // any access to this register during rendering can effect rendering
					//printf("writing ppu addr: 0x%x\n", nes->ppu.mem_addr);
					mem_addr_mask = 0xFF00 >> 8 * (nes->ppu.addr_toggle);
					//printf("ppu wr 2006: toggle %d 0x%x\n", nes->ppu.addr_toggle, nes->ppu.reg[6]);
					//printf("update vram from addr= 0x%x ", nes->ppu.mem_addr);
					nes->ppu.t_mem_addr &= ~mem_addr_mask;
					nes->ppu.t_mem_addr |= (((uint16_t) nes->ppu.reg[6] << 8) | nes->ppu.reg[6]) & mem_addr_mask;
					// update scroll and nametable select signals
					if (nes->ppu.addr_toggle) {
						nes->ppu.scroll[0] &= 0x07;
						nes->ppu.scroll[0] |= (nes->ppu.reg[6] << 3) & 0xf8;
						nes->ppu.scroll[1] &= 0xc7;
						nes->ppu.scroll[1] |= (nes->ppu.reg[6] >> 2) & 0x38;
					} else {
						nes->ppu.scroll[1] &= 0x38;
						nes->ppu.scroll[1] |= (nes->ppu.reg[6] << 6) & 0xc0;
						nes->ppu.scroll[1] |= (nes->ppu.reg[6] >> 4) & 0x03;
						nes->ppu.reg[0] &= 0xfc;
						nes->ppu.reg[0] |= (nes->ppu.reg[6] >> 2) & 0x3;
					}
					if (nes->ppu.addr_toggle) {
						nes->ppu.mem_addr = nes->ppu.t_mem_addr;
						nes->ppu.mem_addr &= NESSYS_PPU_WIN_MAX;
						nes->ppu.scroll_y = ((nes->ppu.reg[0] & 0x2) << 7) | nes->ppu.scroll[1];
						nes->ppu.max_y = 240 + (nes->ppu.scroll_y & 0x100);// +256 * ((nes->ppu.scroll_y & 0xFF) >= 240);
						nes->ppu.scroll_y_changed = true;
					}
					//printf("to addr= 0x%x scroll: %d %d %d\n", nes->ppu.mem_addr, nes->ppu.scroll[0], nes->ppu.scroll[1], nes->ppu.scroll_y);
					nes->ppu.addr_toggle = !nes->ppu.addr_toggle;
					break;
				case 0x7:
					ppu_ever_written = true;
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
					//printf("writing ppu  addr: 0x%x\n", nes->ppu.mem_addr);
					nes->ppu.mem_addr += !((nes->ppu.reg[0] & 0x4) >> 1) + ((nes->ppu.reg[0] & 0x4) << 3);
					nes->ppu.mem_addr &= NESSYS_PPU_WIN_MAX;
					if (skip_print == 0) printf(" vram addr nest 0x%x\n", nes->ppu.mem_addr);
					break;
				case 0x14:
					ppu_ever_written = true;
					addr = nes->apu.reg[0x14] << 8;
					operand = nessys_mem(nes, addr, &bank, &offset);
					memcpy(nes->ppu.oam, operand, NESSYS_PPU_OAM_SIZE);
					penalty_cycles += 514;
					break;
				}
			}
			if (rom_write) {
				if(skip_print == 0) printf("writing to rom a: 0x%x d: 0x%x\n", addr, result);
				ppu_ever_written |= nes->mapper_write(nes, addr, (uint8_t)result);
			}
			nes->cpu_cycle_inc = op->num_cycles + penalty_cycles;
		}
		cycle_count += NESSYS_PPU_PER_CPU_CLK*(nes->cpu_cycle_inc);
		// cycles remaining may go negative if we have a oam dma transfer
		// the cycle penalty is accounted for until after th decision has already been bade to execute it
		nes->cycles_remaining -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
		nes->cycle += NESSYS_PPU_PER_CPU_CLK*(nes->cpu_cycle_inc);
		next_scanline_cycle = nes->scanline_cycle;
		next_scanline_cycle += NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
		if ((nes->ppu.reg[1] & 0x18) && (nes->scanline >= -1)) {// && (nes->scanline < NESSYS_PPU_SCANLINES_RENDERED)) {
			//uint32_t a12_toggle_cycle;
			//switch (nes->ppu.reg[0] & 0x18) {
			//case 0x08:
			//	a12_toggle_cycle = 260;
			//	break;
			//case 0x10:
			//	a12_toggle_cycle = 324;
			//	break;
			//default:
			//	a12_toggle_cycle = 1000;// that is, never
			//	break;
			//}
			//if (nes->scanline_cycle < (int32_t) a12_toggle_cycle) {
			//	nes->ppu.mem_addr &= ~0x1000;
			//} else {
			//	//if((nes->ppu.mem_addr & 0x1000) == 0) printf("a12_toggled: %d\n", nes->scanline_cycle);
			//	nes->ppu.mem_addr |= 0x1000;
			//}
			nes->ppu.mem_addr &= ~0x1000;
			if (nes->ppu.reg[0] & 0x20) {
				// 8x16 sprites are more complex
				if (next_scanline_cycle < 260) {
					nes->ppu.mem_addr |= (nes->ppu.reg[0] & 0x10) << 8;
				} else {
					int32_t sp_cycle = (nes->scanline_cycle < 260) ? 260 : nes->scanline_cycle;
					sp_cycle &= ~0x3;
					sp_cycle |= next_scanline_cycle & 0x3;
					int32_t sp_index;
					while (sp_cycle <= next_scanline_cycle) {
						sp_index = (sp_cycle - 260) / 4;
						sp_cycle += 4;
						if (nes->scanline >= 0 && nes->scanline < NESSYS_PPU_SCANLINES_RENDERED && sp_index < nes->ppu.scanline_num_sprites[nes->scanline]) {
							uint8_t tile_index = nes->ppu.oam[nes->ppu.scanline_sprite_id[nes->scanline][sp_index] + 1];
							nes->ppu.mem_addr |= (tile_index & 0x1) << 12;
						} else {
							// if there are fewer than 8 sprites, they fetch the last sprite
							nes->ppu.mem_addr |= 0x1000;
						}
						if (sp_cycle < next_scanline_cycle) {
							ppu_ever_written |= nes->mapper_update(nes);
						}
					}
				}
			} else {
				// 8x8 sprites generally toggle once per scanline
				if (next_scanline_cycle < 260 || next_scanline_cycle >= 324) {
					nes->ppu.mem_addr |= (nes->ppu.reg[0] & 0x10) << 8;
				} else {
					nes->ppu.mem_addr |= (nes->ppu.reg[0] & 0x08) << 9;
				}
			}
		}
		nes->scanline_cycle = next_scanline_cycle;
		ppu_ever_written |= nes->mapper_update(nes);
		if (nes->scanline_cycle >= (int32_t)NESSYS_PPU_CLK_PER_SCANLINE) {
			//if (nes->scanline_cycle > (int32_t)2 * NESSYS_PPU_CLK_PER_SCANLINE)
			//	printf("Large scanline cycle: %d, scanline %d cycles remaining %d\n", nes->scanline_cycle, nes->scanline, nes->cycles_remaining);
			if (nes->scanline_cycle >= (int32_t)NESSYS_PPU_CLK_PER_SCANLINE) {
				nes->scanline_cycle -= NESSYS_PPU_CLK_PER_SCANLINE;
				nes->scanline++;
			}
			//if (nes->scanline > 240) printf("scanline exceeded: %d %d %d\n", nes->scanline, nes->scanline_cycle, nes->cycles_remaining);
			// if we are on positive scanline, the we are in the display portion of the frame
			// if so, we are done if ppu had been written, and rnedering is enabled
			done = (nes->scanline > 0) && ppu_ever_written && ((nes->ppu.reg[1] & 0x18) != 0x0) && (nes->scanline < NESSYS_PPU_SCANLINES_RENDERED);
		}
		if (nes->vblank_cycles > 0) {
			if (nes->vblank_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->vblank_cycles = 1;
			} else {
				nes->vblank_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->sprite0_hit_cycles > 0) {
			if (nes->sprite0_hit_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->sprite0_hit_cycles = 1;
			} else {
				nes->sprite0_hit_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->mapper_irq_cycles > 0) {
			if (nes->mapper_irq_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->mapper_irq_cycles = 1;
			} else {
				nes->mapper_irq_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->frame_irq_cycles > 0) {
			if (nes->frame_irq_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->frame_irq_cycles = 1;
			} else {
				nes->frame_irq_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->dmc_irq_cycles > 0) {
			if (nes->dmc_irq_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->dmc_irq_cycles = 1;
			} else {
				nes->dmc_irq_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
	}
	return cycle_count;
}


void nessys_unload_cart(nessys_t* nes)
{
	nessys_cleanup_mapper(nes);
	nes->win->SetDisplayFunc(NULL);
	nes->win->SetDisplayFunc(NULL);
	nes->win->SetIdleFunc(NULL);
	if (nes->ppu.mem_4screen) {
		free(nes->ppu.mem_4screen);
		nes->ppu.mem_4screen = NULL;
	}
	if (nes->prg_ram_base) {
		free(nes->prg_ram_base);
		nes->prg_ram_base = NULL;
	}
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

void nessys_cleanup(nessys_t* nes)
{
	// clean up all gfx resources
	// TODO - k2 is not working with destroy objects
}


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
	nes->apu.reg = nes->apu.reg_mem;
	nes->apu.pulse[0].env.flags = NESSYS_APU_PULSE_FLAG_SWEEP_ONES_COMP;
	nes->apu.noise.shift_reg = 0x01;
	nes->apu.dmc.period = NESSYS_APU_DMC_PERIOD_TABLE[0];
	nes->win = k3winObj::CreateWindowedWithFormat("ArkNESS", 0, 0, 512, 480, k3fmt::RGBA8_UNORM, 2 * nessys_t::NUM_GPU_VERSIONS + 1 + 2 + 2, 0);
	nes->win->SetVisible(true);
	nes->win->SetCursorVisible(true);
	nes->win->SetDataPtr(nes);
	nes->gfx = nes->win->GetGfx();
	nes->win->SetVsyncInterval(1);
	nes->cmd_buf = nes->gfx->CreateCmdBuf();
	nes->fence = nes->gfx->CreateFence();

	// Create depth/stencil surface
	k3resourceDesc rdesc = { 0 };
	rdesc.width = 256;
	rdesc.height = 240;
	rdesc.depth = 1;
	rdesc.mip_levels = 1;
	rdesc.num_samples = 1;
	rdesc.format = k3fmt::RGBA8_UNORM;
	k3viewDesc vdesc = { 0 };
	nes->surf_render = nes->gfx->CreateSurface(&rdesc, &vdesc, &vdesc, NULL);

	rdesc.format = k3fmt::D24_UNORM_S8_UINT;
	nes->surf_depth = nes->gfx->CreateSurface(&rdesc, &vdesc, NULL, NULL);

	// create shader bindings
	k3bindingParam bind_params[2];
	bind_params[0].type = k3bindingType::VIEW_SET;
	bind_params[0].view_set_desc.type = k3shaderBindType::CBV;
	bind_params[0].view_set_desc.num_views = 1;
	bind_params[0].view_set_desc.reg = 0;
	bind_params[0].view_set_desc.space = 0;
	bind_params[1].type = k3bindingType::VIEW_SET;
	bind_params[1].view_set_desc.type = k3shaderBindType::SRV;
	bind_params[1].view_set_desc.num_views = 1;
	bind_params[1].view_set_desc.reg = 0;
	bind_params[1].view_set_desc.space = 0;
	k3shaderBinding shader_binding = nes->gfx->CreateShaderBinding(2, bind_params, 0, NULL);

	k3samplerDesc samp_desc = { 0 };
	samp_desc.filter = k3texFilter::MIN_MAG_MIP_POINT;
	samp_desc.addr_u = k3texAddr::CLAMP;
	samp_desc.addr_v = k3texAddr::CLAMP;
	samp_desc.addr_w = k3texAddr::CLAMP;
	samp_desc.sampler_index = 0;
	k3shaderBinding copy_binding = nes->gfx->CreateShaderBinding(2, bind_params, 1, &samp_desc);

	// initialize render states
	k3blendState bs_normal = { 0 };
	bs_normal.alpha_to_mask = false;
	bs_normal.independent_blend = false;
	bs_normal.blend_op[0].blend_enable = false;
	bs_normal.blend_op[0].rop_enable = false;
	bs_normal.blend_op[0].rt_write_mask = 0xf;

	k3blendState bs_mask = bs_normal;
	bs_mask.alpha_to_mask = true;

	k3blendState bs_blend = bs_normal;
	bs_blend.blend_op[0].blend_enable = true;
	bs_blend.blend_op[0].src_blend = k3blend::SRC_ALPHA;
	bs_blend.blend_op[0].dst_blend = k3blend::INV_SRC_ALPHA;
	bs_blend.blend_op[0].blend_op = k3blendOp::ADD;
	bs_blend.blend_op[0].alpha_src_blend = k3blend::SRC_ALPHA;
	bs_blend.blend_op[0].alpha_dst_blend = k3blend::INV_SRC_ALPHA;
	bs_blend.blend_op[0].alpha_blend_op = k3blendOp::ADD;

	k3depthState ds_none = { 0 };
	ds_none.depth_enable = false;
	ds_none.depth_write_enable = false;
	ds_none.depth_test = k3testFunc::ALWAYS;
	ds_none.stencil_enable = false;

	k3depthState ds_normal = ds_none;
	ds_normal.depth_enable = true;
	ds_normal.depth_write_enable = true;
	ds_normal.depth_test = k3testFunc::LESS_EQUAL;
	ds_normal.stencil_enable = false;

	k3depthState ds_sprite = ds_normal;
	ds_sprite.depth_write_enable = false;
	ds_sprite.stencil_enable = true;
	ds_sprite.stencil_read_mask = 0xff;
	ds_sprite.stencil_write_mask = 0xff;
	ds_sprite.back.stencil_test = k3testFunc::NEVER;
	ds_sprite.back.fail_op = k3stencilOp::INCR_SAT;
	ds_sprite.back.z_fail_op = k3stencilOp::INCR_SAT;
	ds_sprite.back.pass_op = k3stencilOp::INCR_SAT;
	ds_sprite.front.stencil_test = k3testFunc::GREATER;
	ds_sprite.front.fail_op = k3stencilOp::KEEP;
	ds_sprite.front.z_fail_op = k3stencilOp::REPLACE;
	ds_sprite.front.pass_op = k3stencilOp::REPLACE;

	// Load shaders
	k3shader screen_vs = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\screen_vs.cso");
	k3shader sprite_vs = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\sprite_vs.cso");
	k3shader copy_vs = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\copy_vs.cso");
	k3shader fill_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\fill_ps.cso");
	k3shader sprite_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\sprite_ps.cso");
	k3shader m9_sprite_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\m9_sprite_ps.cso");
	k3shader background_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\background_ps.cso");
	k3shader exp_background_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\exp_background_ps.cso");
	k3shader m9_background_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\m9_background_ps.cso");
	k3shader copy_ps = nes->gfx->CreateShaderFromCompiledFile("..\\Debug\\copy_ps.cso");

	// setup input format
	k3inputElement in_elem[1];
	in_elem[0].name = "POSITION";
	in_elem[0].index = 0;
	in_elem[0].format = k3fmt::RG32_FLOAT;
	in_elem[0].slot = 0;
	in_elem[0].offset = 0;
	in_elem[0].instance_step = 0;
	in_elem[0].in_type = k3inputType::VERTEX;

	// create gfx states
	k3gfxStateDesc gfx_state_desc = { 0 };
	gfx_state_desc.num_input_elements = 1;
	gfx_state_desc.input_elements = in_elem;
	gfx_state_desc.shader_binding = shader_binding;
	gfx_state_desc.sample_mask = ~0;
	gfx_state_desc.rast_state.fill_mode = k3fill::SOLID;
	gfx_state_desc.rast_state.cull_mode = k3cull::BACK;
	gfx_state_desc.rast_state.front_counter_clockwise = false;
	gfx_state_desc.rast_state.depth_clip_enable = false;
	gfx_state_desc.rast_state.msaa_enable = true;
	gfx_state_desc.rast_state.aa_line_enable = true;
	gfx_state_desc.prim_type = k3primType::TRIANGLE;
	gfx_state_desc.num_render_targets = 1;
	gfx_state_desc.msaa_samples = 1;
	gfx_state_desc.rtv_format[0] = k3fmt::RGBA8_UNORM;
	gfx_state_desc.dsv_format = k3fmt::D24_UNORM_S8_UINT;

	gfx_state_desc.vertex_shader = screen_vs;
	gfx_state_desc.pixel_shader = background_ps;
	gfx_state_desc.blend_state = bs_mask;
	gfx_state_desc.depth_state = ds_normal;
	nes->st_background = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.pixel_shader = exp_background_ps;
	nes->st_exp_background = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.pixel_shader = m9_background_ps;
	nes->st_m9_background = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.pixel_shader = fill_ps;
	gfx_state_desc.blend_state = bs_normal;
	gfx_state_desc.depth_state = ds_none;
	nes->st_fill = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.blend_state = bs_blend;
	nes->st_blend_fill = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.vertex_shader = sprite_vs;
	gfx_state_desc.pixel_shader = sprite_ps;
	gfx_state_desc.blend_state = bs_mask;
	gfx_state_desc.depth_state = ds_sprite;
	nes->st_sprite_max = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.rast_state.cull_mode = k3cull::NONE;
	nes->st_sprite_8 = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.pixel_shader = m9_sprite_ps;
	nes->st_m9_sprite = nes->gfx->CreateGfxState(&gfx_state_desc);

	gfx_state_desc.shader_binding = copy_binding;
	gfx_state_desc.vertex_shader = copy_vs;
	gfx_state_desc.pixel_shader = copy_ps;
	gfx_state_desc.blend_state = bs_normal;
	gfx_state_desc.depth_state = ds_none;
	gfx_state_desc.rast_state.cull_mode = k3cull::BACK;
	nes->st_copy = nes->gfx->CreateGfxState(&gfx_state_desc);

	// Create buffers
	uint32_t i;
	for (i = 0; i < nessys_t::NUM_CPU_VERSIONS; i++) {
		nes->cb_upload[i] = nes->gfx->CreateUploadBuffer();
		nes->surf_upload_exp_pattern[i] = nes->gfx->CreateUploadImage();
		nes->surf_upload_exp_pattern[i]->SetDimensions(256, 64, 1, k3fmt::RGBA32_UINT);
	}

	float* buf_data = static_cast<float*>(nes->cb_upload[0]->MapForWrite(8 * sizeof(float)));
	buf_data[0] = 0.0f; buf_data[1] = 0.0f;
	buf_data[2] = 256.0f; buf_data[3] = 0.0f;
	buf_data[4] = 0.0f; buf_data[5] = 240.0f;
	buf_data[6] = 256.0f; buf_data[7] = 240.0f;
	nes->cb_upload[0]->Unmap();

	buf_data = static_cast<float*>(nes->cb_upload[1]->MapForWrite(8 * sizeof(float)));
	buf_data[0] = 0.0f; buf_data[1] = 0.0f;
	buf_data[2] = 8.0f; buf_data[3] = 0.0f;
	buf_data[4] = 0.0f; buf_data[5] = 8.0f;
	buf_data[6] = 8.0f; buf_data[7] = 8.0f;
	nes->cb_upload[1]->Unmap();

	buf_data = static_cast<float*>(nes->cb_upload[2]->MapForWrite(16 * sizeof(float)));
	buf_data[ 0] = 1.0f; buf_data[ 1] = 0.0f; buf_data[ 2] = 0.0f; buf_data[ 3] = 0.0f;
	buf_data[ 4] = 0.0f; buf_data[ 5] = 1.0f; buf_data[ 6] = 0.0f; buf_data[ 7] = 0.0f;
	buf_data[ 8] = 0.0f; buf_data[ 9] = 0.0f; buf_data[10] = 1.0f; buf_data[11] = 0.0f;
	buf_data[12] = 0.0f; buf_data[13] = 0.0f; buf_data[14] = 0.0f; buf_data[15] = 1.0f;
	nes->cb_upload[2]->Unmap();

	buf_data = static_cast<float*>(nes->cb_upload[3]->MapForWrite(16 * sizeof(float)));
	buf_data[ 0] = 0.1f; buf_data[ 1] = 0.2f; buf_data[ 2] = 0.03f; buf_data[ 3] = 0.0f;
	buf_data[ 4] = 0.1f; buf_data[ 5] = 0.2f; buf_data[ 6] = 0.03f; buf_data[ 7] = 0.0f;
	buf_data[ 8] = 0.1f; buf_data[ 9] = 0.2f; buf_data[10] = 0.03f; buf_data[11] = 0.0f;
	buf_data[12] = 0.0f; buf_data[13] = 0.0f; buf_data[14] = 0.00f; buf_data[15] = 1.0f;
	nes->cb_upload[3]->Unmap();

	k3bufferDesc bdesc = { 0 };
	bdesc.size = 8 * sizeof(float);
	bdesc.stride = 2 * sizeof(float);
	nes->vb_fullscreen = nes->gfx->CreateBuffer(&bdesc);
	nes->vb_sprite = nes->gfx->CreateBuffer(&bdesc);

	bdesc.size = 16 * sizeof(float);
	bdesc.stride = 0;
	bdesc.view_index = 1;

	nes->cb_copy_normal = nes->gfx->CreateBuffer(&bdesc);
	bdesc.view_index++;

	nes->cb_copy_menu = nes->gfx->CreateBuffer(&bdesc);
	bdesc.view_index++;

	nes->cmd_buf->Reset();
	nes->cmd_buf->TransitionResource(nes->vb_fullscreen->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->TransitionResource(nes->vb_sprite->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->TransitionResource(nes->cb_copy_normal->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->TransitionResource(nes->cb_copy_menu->GetResource(), k3resourceState::COPY_DEST);
	nes->cmd_buf->TransitionResource(nes->surf_depth->GetResource(), k3resourceState::RENDER_TARGET);
	nes->cmd_buf->UploadBuffer(nes->cb_upload[0], nes->vb_fullscreen->GetResource());
	nes->cmd_buf->UploadBuffer(nes->cb_upload[1], nes->vb_sprite->GetResource());
	nes->cmd_buf->UploadBuffer(nes->cb_upload[2], nes->cb_copy_normal->GetResource());
	nes->cmd_buf->UploadBuffer(nes->cb_upload[3], nes->cb_copy_menu->GetResource());
	nes->cmd_buf->TransitionResource(nes->cb_copy_normal->GetResource(), k3resourceState::SHADER_BUFFER);
	nes->cmd_buf->TransitionResource(nes->cb_copy_menu->GetResource(), k3resourceState::SHADER_BUFFER);
	nes->cmd_buf->Close();
	nes->gfx->SubmitCmdBuf(nes->cmd_buf);

	bdesc.size = NESSYS_MAX_CBUFFER_SIZE;
	for (i = 0; i < nessys_t::NUM_GPU_VERSIONS; i++) {
		nes->cb_main[i] = nes->gfx->CreateBuffer(&bdesc);
		bdesc.view_index++;
	}

	rdesc.width = 256;
	rdesc.height = 64;
	rdesc.depth = 1;
	rdesc.mip_levels = 1;
	rdesc.num_samples = 1;
	rdesc.format = k3fmt::RGBA32_UINT;
	vdesc.view_index = bdesc.view_index;
	for (i = 0; i < nessys_t::NUM_GPU_VERSIONS; i++) {
		nes->surf_exp_pattern[i] = nes->gfx->CreateSurface(&rdesc, NULL, &vdesc, NULL);
		vdesc.view_index++;
	}

	nes->sb_main = nes->win->CreateSoundBuffer(1, NESSYS_SND_SAMPLES_PER_SECOND, NESSYS_SND_BITS_PER_SAMPLE, NESSYS_SND_SAMPLES);
	void* sbuf_ptr = nes->sb_main->MapForWrite(0, 2 * NESSYS_SND_SAMPLES, NULL, NULL);
	memset(sbuf_ptr, 0, 2 * NESSYS_SND_SAMPLES);
	nes->sb_main->Unmap(sbuf_ptr, 0, 2 * NESSYS_SND_SAMPLES);

	// reset write pointer
	nes->sbuf_offset = NESSYS_SND_START_POSITION;
	sbuf_ptr = nes->sb_main->MapForWrite(nes->sbuf_offset, NESSYS_SND_BYTES_PER_BUFFER, NULL, NULL);
	nes->sb_main->Unmap(sbuf_ptr, nes->sbuf_offset, NESSYS_SND_BYTES_PER_BUFFER);

	nes->timer = nes->win->CreateTimer();

	k3fontDesc fdesc = { 0 };
	fdesc.view_index = vdesc.view_index;
	fdesc.name = "..\\assets\\LapsusPro-Bold.otf";
	fdesc.point_size = 48.0f;
	fdesc.style = k3fontStyle::NORMAL;
	fdesc.weight = k3fontWeight::NORMAL;
	fdesc.format = k3fmt::RGBA8_UNORM;
	fdesc.cmd_buf = nes->cmd_buf;
	fdesc.transparent = true;
	nes->main_font = nes->gfx->CreateFont(&fdesc);

	nesmenu_init(nes);
	nes->num_joy = 0;
	memset(nes->joy_data, 0, 2 * sizeof(nesjoy_data));

	nes->win->SetKeyboardFunc(nessys_keyboard);
	nes->win->SetJoystickFunc(nessys_joystick_added, nessys_joystick_removed, nessys_joystick_move, nessys_joystick_button);
	nes->win->SetDisplayFunc(nessys_display);
	nes->win->SetIdleFunc(nessys_display);
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
	nes->sb_main->StopSBuffer();
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
	nes->sb_main->StopSBuffer();
}

bool nessys_load_cart_filename(nessys_t* nes, const char* filename)
{
	FILE* fh;
	nessys_unload_cart(nes);
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
	if (nes->cycles_remaining >= 0) {
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
		dmc->bits_remaining = 0;
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

void nessys_apu_frame_tick(nessys_t* nes)
{
	uint32_t cur_frame_tick = nes->apu.frame_frac_counter >> NESSYS_SND_FRAME_FRAC_LOG2;
	uint32_t max_frame_tick = (nes->apu.frame_counter & 0x80) ? 4 : 3;
	uint32_t last_frame_tick = (cur_frame_tick == 0) ? max_frame_tick : cur_frame_tick - 1;
	if (cur_frame_tick > max_frame_tick) {
		cur_frame_tick = 0;
		nes->apu.frame_frac_counter &= NESSYS_SND_FRAME_FRAC_MASK;
	}
	nes->mapper_audio_tick(nes);
	if (last_frame_tick != 3 || max_frame_tick == 3) {
		nessys_apu_env_tick(&(nes->apu.pulse[0].env));
		nessys_apu_env_tick(&(nes->apu.pulse[1].env));
		nessys_apu_env_tick(&(nes->apu.noise.env));
		nessys_apu_tri_linear_tick(&(nes->apu.triangle));
		if (last_frame_tick == 1 || last_frame_tick == max_frame_tick) {
			nessys_apu_sweep_tick(&(nes->apu.pulse[0]));
			nessys_apu_sweep_tick(&(nes->apu.pulse[1]));
			nessys_apu_noise_length_tick(&(nes->apu.noise));
			nessys_apu_tri_length_tick(&(nes->apu.triangle));
			if ((nes->apu.frame_counter & 0xc0) == 0 && last_frame_tick == max_frame_tick) {
				// if were 4-step mode, and IRQ disable is 0, then we can produce an interrupt
				nes->frame_irq = true;
			}
		}
	}
}

int16_t nessys_gen_sound_sample(nessys_t* nes)
{
	// first increment frame conter, and see if it ticks
	uint32_t last_frame_tick = nes->apu.frame_frac_counter >> NESSYS_SND_FRAME_FRAC_LOG2;
	nes->apu.frame_frac_counter += NESSYS_SND_FRAME_FRAC_PER_SAMPLE;
	uint32_t cur_frame_tick = nes->apu.frame_frac_counter >> NESSYS_SND_FRAME_FRAC_LOG2;
	if (cur_frame_tick > last_frame_tick) {
		nessys_apu_frame_tick(nes);
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
	sound += nes->mapper_gen_sound(nes);
	return sound;
}

void nessys_gen_sound(nessys_t* nes)
{
	const float PI = 3.141592654f;
	const float t_inc = 2.0f * PI * 440 / NESSYS_SND_SAMPLES_PER_SECOND;
	static float t = 0.0f;
	static bool playing = false;
	uint32_t s;
	uint32_t max_wr_size = NESSYS_SND_BYTES_PER_BUFFER - NESSYS_SND_BYTES_SKEW;
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
		if (wr_pos - play_pos < (NESSYS_SND_BUFFERS / 2) * NESSYS_SND_BYTES_PER_BUFFER) wr_size += 2 * NESSYS_SND_BYTES_SKEW;
	}

	if (wr_size) {
		nes->apu.sample_frac_generated = end_sample_frac;
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

		nes->sb_main->Unmap(p0, nes->sbuf_offset, wr_size);

		nes->sbuf_offset += wr_size;
		nes->sbuf_offset = nes->sbuf_offset % NESSYS_SND_BYTES;

		//nes->sb_main->k2UpdateSBuffer(buffer, 2*NESSYS_SND_SAMPLES/4);
		nes->sb_main->PlaySBuffer();
		playing = true;

		//if ((nes->apu.frame_counter & 0xc0) == 0) {
		//	// if were 4-step mode, and IRQ disable is 0, then we can produce an interrupt
		//	uint32_t frac_until_frame_irq = ((0x4* NESSYS_SND_SAMPLES_PER_BUFFER) << NESSYS_SND_FRAME_FRAC_LOG2) - (NESSYS_SND_SAMPLES_PER_BUFFER*nes->apu.frame_frac_counter);
		//	uint32_t cycles_until_frame_irq = frac_until_frame_irq / (0x4*NESSYS_SND_SAMPLES_FRAC_PER_CYCLE);
		//	if (nes->frame_irq_cycles == 0 /* || nes->frame_irq_cycles > cycles_until_frame_irq*/) nes->frame_irq_cycles = cycles_until_frame_irq;
		//} else {
		//	nes->frame_irq_cycles = 0;
		//	nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x40;
		//}
	}
}

void nessys_gen_scanline_sprite_map(nessys_t* nes)
{
	nessys_ppu_t* ppu = &(nes->ppu);
	uint32_t sp, row_start, row_offset, row;
	uint32_t sprite_height = (ppu->reg[0] & 0x20) ? 16 : 8;
	memset(ppu->scanline_num_sprites, 0, NESSYS_PPU_SCANLINES_RENDERED);
	nes->sprite_overflow_cycles = 0;
	for (sp = 0; sp < NESSYS_PPU_SPRITE_SIZE * NESSYS_PPU_NUM_SPRITES; sp += NESSYS_PPU_SPRITE_SIZE) {
		row_start = ppu->oam[sp + 0];
		for (row_offset = 0; row_offset < sprite_height; row_offset++) {
			row = row_start + row_offset;
			if (row < NESSYS_PPU_SCANLINES_RENDERED) {
				if (ppu->scanline_num_sprites[row] < NESSYS_PPU_MAX_SPRITES_PER_SCALINE) {
					ppu->scanline_sprite_id[row][ppu->scanline_num_sprites[row]] = sp;
					ppu->scanline_num_sprites[row]++;
				} else {
					int32_t overflow_scanline_cycle = 64 + 48 + sp / 2;
					int32_t cycles_until_overflow = (row - nes->scanline) * NESSYS_PPU_CLK_PER_SCANLINE + overflow_scanline_cycle - nes->scanline_cycle;
					if (cycles_until_overflow < 0) cycles_until_overflow = 1;
					if (nes->sprite_overflow_cycles == 0 || (uint32_t)cycles_until_overflow < nes->sprite_overflow_cycles) {
						nes->sprite_overflow_cycles = cycles_until_overflow;
					}
				}
			}
		}
	}
	if ((ppu->reg[1] & 0x18) == 0x0) {
		// if rendering is disabled, then we don't set the sprite overflow bit
		nes->sprite_overflow_cycles = 0;
	}
}

bool nessys_sprite_max_check(nessys_t* nes)
{
	// returns true if we can use unlimited sprites per scanline
	uint8_t i;
	uint32_t index;
	bool use_sprite_max = true;
	for (i = 0; i < NESSYS_PPU_NUM_SPRITES && use_sprite_max; i++) {
		if (nes->ppu.oam[NESSYS_PPU_SPRITE_SIZE * i + 0] < 240) {
			index = nes->ppu.oam[NESSYS_PPU_SPRITE_SIZE * i + 0];
			index <<= 8;
			index |= nes->ppu.oam[NESSYS_PPU_SPRITE_SIZE * i + 3];
			nes->coord_scoreboard[index]++;
			if (nes->coord_scoreboard[index] >= 8) use_sprite_max = false;
		}
	}
	// clear scoreboard for next time
	for (i = 0; i < NESSYS_PPU_NUM_SPRITES; i++) {
		if (nes->ppu.oam[NESSYS_PPU_SPRITE_SIZE * i + 0] < 240) {
			index = nes->ppu.oam[NESSYS_PPU_SPRITE_SIZE * i + 0];
			index <<= 8;
			index |= nes->ppu.oam[NESSYS_PPU_SPRITE_SIZE * i + 3];
			nes->coord_scoreboard[index] = 0;
		}
	}
	return use_sprite_max;
}

void nessys_scale_to_back_buffer(nessys_t* nes)
{
	k3renderTargets rt = { NULL };
	k3rect scissor;
	float clear_color[] = { NESSYS_PPU_PALETTE[0],
							NESSYS_PPU_PALETTE[1],
							NESSYS_PPU_PALETTE[2],
							1.0f };
	rt.render_targets[0] = nes->win->GetBackBuffer();
	uint32_t win_width = nes->win->GetWidth();
	uint32_t win_height = nes->win->GetHeight();
	uint32_t win_width_pad = 0;
	uint32_t win_height_pad = 0;
	if (15 * win_width >= 16 * win_height) {
		win_width_pad = win_width - ((win_height * 256) / 240);
	} else {
		win_height_pad = win_height - ((win_width * 240) / 256);
	}
	scissor.x = win_width_pad / 2;
	scissor.y = win_height_pad / 2;
	scissor.width = win_width - win_width_pad;
	scissor.height = win_height - win_height_pad;
	nes->cmd_buf->Reset();
	nes->cmd_buf->TransitionResource(nes->win->GetBackBuffer()->GetResource(), k3resourceState::RENDER_TARGET);
	nes->cmd_buf->ClearRenderTarget(nes->win->GetBackBuffer(), clear_color, NULL);
	nes->cmd_buf->SetRenderTargets(&rt);
	nes->cmd_buf->SetViewport(&scissor);
	nes->cmd_buf->SetScissor(&scissor);
	nes->cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLESTRIP);
	nes->cmd_buf->TransitionResource(nes->surf_render->GetResource(), k3resourceState::SHADER_RESOURCE);
	nes->cmd_buf->SetGfxState(nes->st_copy);
	if (nes->menu.pane == nesmenu_pane_t::NONE) {
		nes->cmd_buf->SetConstantBuffer(0, nes->cb_copy_normal);
	} else {
		nes->cmd_buf->SetConstantBuffer(0, nes->cb_copy_menu);
	}
	nes->cmd_buf->SetShaderView(1, nes->surf_render);
	nes->cmd_buf->SetVertexBuffer(0, nes->vb_fullscreen);
	nes->cmd_buf->Draw(4);
	nes->cmd_buf->TransitionResource(rt.render_targets[0]->GetResource(), k3resourceState::COMMON);
	nes->cmd_buf->Close();
	nes->gfx->SubmitCmdBuf(nes->cmd_buf);

}

void K3CALLBACK nessys_display(void* ptr)
{
	nessys_t* nes = (nessys_t*)ptr;

	// Decrement wait time each time we enter this function
	// if we have more time to wait, then exit out
	uint32_t delta_time = nes->timer->GetDeltaTime();
	delta_time = (delta_time > 100) ? 100 : delta_time;  // First call may have a large number, so cap it
	nes->frame_wait_time -= delta_time;
	if (nes->frame_wait_time > 0) {
		nes->timer->Sleep(nes->frame_wait_time);
		return;
	}

	// if we are done waiting, then execute/render the frame
	// and increment wait time by 1/60 of second,
	// this works out to 16.6667 ms.  Since we have to enter wait time in integer
	// values, add 16 ms every 3rd frame, and 17 ms on all other frames
	int32_t wait_time = 16 + (nes->frame % 3 != 0);
	nes->frame_wait_time += wait_time;

	if (nes->menu.pane == nesmenu_pane_t::NONE &&
		(((nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_START_MASK) && 
			(nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_SELECT_MASK)) ||
		 (nes->prg_rom_base == NULL)) ) {
		nes->menu.pane = nesmenu_pane_t::MAIN;
		nesmenu_update_list(nes);
		nes->apu.joypad[0] = 0;
	}

	if (nes->menu.pane != nesmenu_pane_t::NONE) {
		nesmenu_display(nes);
		return;
	}

	float palette[4 * NESSYS_PPU_PAL_SIZE ];
	//uint8_t buffer[8 * 1024];
	uint32_t cycles;

	// render image
	k3renderTargets rt = { NULL };
	rt.render_targets[0] = nes->surf_render;
	rt.depth_target = nes->surf_depth;

	nesssys_set_scanline(nes, 0);
	nes->ppu.scroll_y = ((nes->ppu.reg[0] & 0x2) << 7) | nes->ppu.scroll[1];
	nes->ppu.max_y = 240 + (nes->ppu.scroll_y & 0x100);
	nes->ppu.scroll_y_changed = true;
	// 241 scanlines
	nes->cycles_remaining += NESSYS_PPU_SCANLINES_RENDERED_CLKS;
	nes->last_num_mid_scan_ntb_bank_changes = 0;
	nes->prev_last_num_mid_scan_ntb_bank_changes = 0;
	nessys_cbuffer_t* cb_data;
	k3rect scissor;
	int i, c, index;
	do {
		while (nes->scanline_cycle > (int32_t)NESSYS_PPU_CLK_PER_SCANLINE) {
			nes->scanline_cycle -= (int32_t)NESSYS_PPU_CLK_PER_SCANLINE;
			nes->scanline++;
		}

		nes->cmd_buf->Reset();
		nes->cmd_buf->TransitionResource(nes->win->GetBackBuffer()->GetResource(), k3resourceState::RENDER_TARGET);
		nes->cmd_buf->TransitionResource(rt.render_targets[0]->GetResource(), k3resourceState::RENDER_TARGET);
		nes->cmd_buf->SetRenderTargets(&rt);
		nes->cmd_buf->SetViewToSurface(rt.render_targets[0]->GetResource());
		nes->cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLESTRIP);
		nes->cmd_buf->ClearDepthTarget(nes->surf_depth, k3depthSelect::DEPTH_STENCIL, 1.0f, 0, NULL);

		// initialize palette if rendering background or sprites
		uint8_t palette_index_mask = (nes->ppu.reg[1] & 0x1) ? 0x30 : 0x3f;
		for (i = 0; i < NESSYS_PPU_PAL_SIZE; i++) {
			for (c = 0; c < 3; c++) {
				index = ((i & 0x3) == 0) ? 0 : i;
				palette[4 * i + c] = NESSYS_PPU_PALETTE[3 * (nes->ppu.pal[index] & palette_index_mask)+ c];
			}
			palette[4 * i + 3] = ((i & 0x3) == 0) ? 0.0f : 1.0f;
		}
		cb_data = static_cast<nessys_cbuffer_t*>(nes->cb_upload[nes->cb_main_cpu_version]->MapForWrite(sizeof(nessys_cbuffer_exp_t)));
		if (nes->scroll_x_scanline <= 0) {
			if (nes->ppu.scroll_y_changed) {
				uint16_t old_scroll_y = nes->ppu.scroll_y;
				nes->ppu.scroll_y -= nes->scanline;
				if ((nes->ppu.scroll_y & 0x100) != (old_scroll_y & 0x100) && ((old_scroll_y & 0xff) < 240)) nes->ppu.scroll_y -= 16;
				nes->ppu.scroll_y_changed = false;
				//nes->ppu.scroll_y = 237;
			}

			memcpy(cb_data->ppu, nes->ppu.reg, 4 * sizeof(uint32_t) + 240);
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
			scissor.x = 0;
			scissor.y = nes->scanline;
			scissor.width = 256;
			scissor.height = 240 - scissor.y;
		} else {
			uint32_t prev_cb_main_version = (nes->cb_main_cpu_version == 0) ? nessys_t::NUM_CPU_VERSIONS - 1 : nes->cb_main_cpu_version - 1;
			nessys_cbuffer_t* cb_src_data = static_cast<nessys_cbuffer_t*>(nes->cb_upload[prev_cb_main_version]->MapForWrite(sizeof(nessys_cbuffer_exp_t)));
			memcpy(cb_data, cb_src_data, sizeof(nessys_cbuffer_t));
			nes->cb_upload[prev_cb_main_version]->Unmap();
			memcpy(cb_data->scroll_x, nes->ppu.scroll_x, 240);
			scissor.x = 0;
			scissor.y = nes->scroll_x_scanline;
			scissor.width = 256;
			scissor.height = 240 - scissor.y;
		}
		nes->cb_upload[nes->cb_main_cpu_version]->Unmap();

		nes->cmd_buf->SetGfxState(nes->st_fill);
		nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
		nes->cmd_buf->UploadBuffer(nes->cb_upload[nes->cb_main_cpu_version], nes->cb_main[nes->cb_main_gpu_version]->GetResource());
		nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::SHADER_BUFFER);
		nes->cmd_buf->SetConstantBuffer(0, nes->cb_main[nes->cb_main_gpu_version]);
		nes->cmd_buf->TransitionResource(nes->surf_exp_pattern[nes->surf_exp_gpu_version]->GetResource(), k3resourceState::SHADER_RESOURCE);
		nes->cmd_buf->SetShaderView(1, nes->surf_exp_pattern[nes->surf_exp_gpu_version]);
		nes->cb_main_cpu_version++;
		nes->cb_main_gpu_version++;
		if (nes->cb_main_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->cb_main_cpu_version = 0;
		if (nes->cb_main_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->cb_main_gpu_version = 0;

		nes->cmd_buf->SetScissor(&scissor);

		nes->cmd_buf->SetVertexBuffer(0, nes->vb_fullscreen);
		nes->cmd_buf->Draw(4);

		if (nes->ppu.reg[0x1] & 0x08) {
			uint32_t mapper_setup = NESSYS_MAPPER_SETUP_DRAW_INCOMPLETE;
			uint32_t phase = 0;
			while (mapper_setup & NESSYS_MAPPER_SETUP_DRAW_INCOMPLETE) {
				nes->scissor_left_x = 8 - ((nes->ppu.reg[1] << 2) & 0x8);
				nes->scissor_top_y = (nes->scroll_x_scanline > 0) ? nes->scroll_x_scanline : nes->scanline;
				nes->scissor_right_x = 256;
				nes->scissor_bottom_y = 240;

				mapper_setup = nes->mapper_bg_setup(nes, phase);
				nes->mapper_bg_setup_type = mapper_setup;
				if (!(mapper_setup & NESSYS_MAPPER_SETUP_CUSTOM)) {
					nes->cmd_buf->SetGfxState(nes->st_background);
					if (nes->last_num_mid_scan_ntb_bank_changes != nes->prev_last_num_mid_scan_ntb_bank_changes && nes->scissor_top_y > 0) {
						nes->scissor_top_y--;
					}

					uint8_t m;
					uint16_t scan_right;
					for (m = 0; m < nes->last_num_mid_scan_ntb_bank_changes; m++) {
						scan_right = nes->mid_scan_ntb_bank_change_position[m];
						scan_right |= (scan_right == 0) << 8;  // make 0 into 256
						cb_data = static_cast<nessys_cbuffer_t*>(nes->cb_upload[nes->cb_main_cpu_version]->MapForWrite(sizeof(nessys_cbuffer_exp_t)));
						memcpy(cb_data->ppu, nes->ppu.reg, 4 * sizeof(uint32_t) + 240);
						memcpy(cb_data->sprite, nes->ppu.oam, 4 * 16 * sizeof(uint32_t));
						index = NESSYS_CHR_ROM_WIN_MIN;
						for (i = 0; i < 8; i++) {
							memcpy(cb_data->pattern + (i << 8), nes->mid_scan_ntb_banks[4 * m + (i & 0x3)], 1024);
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
					//	memcpy(buffer + 0 * 1024, nes->mid_scan_ntb_banks[4 * m + 0], 1024);
					//	memcpy(buffer + 1 * 1024, nes->mid_scan_ntb_banks[4 * m + 1], 1024);
					//	memcpy(buffer + 2 * 1024, nes->mid_scan_ntb_banks[4 * m + 2], 1024);
					//	memcpy(buffer + 3 * 1024, nes->mid_scan_ntb_banks[4 * m + 3], 1024);
					//	nes->gfx->k2CBUpdate(nes->cb_pattern, buffer);
						scissor.x = nes->scissor_left_x;
						scissor.y = nes->scissor_top_y;
						scissor.width = scan_right - nes->scissor_left_x;
						scissor.height = nes->scissor_bottom_y - nes->scissor_top_y;
						nes->cmd_buf->SetScissor(&scissor);
						nes->cmd_buf->Draw(4);
						nes->scissor_left_x = scan_right;
					}
					if (nes->last_num_mid_scan_ntb_bank_changes) {
						// go back to original data
						uint32_t upload_version = ((nes->last_num_mid_scan_ntb_bank_changes >= nes->cb_main_cpu_version) ? nessys_t::NUM_CPU_VERSIONS : 0);
						upload_version += nes->cb_main_cpu_version - nes->last_num_mid_scan_ntb_bank_changes - 1;

						nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
						nes->cmd_buf->UploadBuffer(nes->cb_upload[upload_version], nes->cb_main[nes->cb_main_gpu_version]->GetResource());
						nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::SHADER_BUFFER);
						nes->cmd_buf->SetConstantBuffer(0, nes->cb_main[nes->cb_main_gpu_version]);
						nes->cb_main_gpu_version++;
						if (nes->cb_main_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->cb_main_gpu_version = 0;

					}

					scissor.x = nes->scissor_left_x;
					scissor.y = nes->scissor_top_y;
					scissor.width = nes->scissor_right_x - nes->scissor_left_x;
					scissor.height = nes->scissor_bottom_y - nes->scissor_top_y;
					nes->cmd_buf->SetScissor(&scissor);
				}

				nes->cmd_buf->Draw(4);
				phase++;
			}
		}

		if (nes->ppu.reg[0x1] & 0x10) {
			uint32_t mapper_setup = NESSYS_MAPPER_SETUP_DRAW_INCOMPLETE;
			uint32_t phase = 0;
			while (mapper_setup & NESSYS_MAPPER_SETUP_DRAW_INCOMPLETE) {
				nes->scissor_left_x = 8 - ((nes->ppu.reg[1] << 1) & 0x8);
				nes->scissor_top_y = (nes->scroll_x_scanline > 0) ? nes->scroll_x_scanline : nes->scanline;
				nes->scissor_right_x = 256;
				nes->scissor_bottom_y = 240;

				mapper_setup = nes->mapper_sprite_setup(nes, phase);

				if (!(mapper_setup & NESSYS_MAPPER_SETUP_CUSTOM)) {
					// enable sprite rendering
					bool use_sprite_max = nessys_sprite_max_check(nes);
					nes->cmd_buf->SetGfxState( (use_sprite_max) ? nes->st_sprite_max : nes->st_sprite_8 );
					nes->cmd_buf->SetStencilRef(8);
					scissor.x = nes->scissor_left_x;
					scissor.y = nes->scissor_top_y;
					scissor.width = nes->scissor_right_x - nes->scissor_left_x;
					scissor.height = nes->scissor_bottom_y - nes->scissor_top_y;
					nes->cmd_buf->SetScissor(&scissor);
					nes->cmd_buf->SetVertexBuffer(0, nes->vb_sprite);

				}
				nes->cmd_buf->Draw(4, 0, 128, 0);

				phase++;
			}
		}
		nes->cmd_buf->Close();
		nes->gfx->SubmitCmdBuf(nes->cmd_buf);

		if (nes->scroll_x_scanline <= 0) {
			// compute sprite0 hit
			if ((nes->ppu.reg[0x1] & 0x18)) {// && ((nes->ppu.reg[2] & 0x40) == 0x0)) {
				// sprite and background rendering must be enabled
				uint32_t sprite_x = nes->ppu.oam[3];
				uint32_t sprite_y = nes->ppu.oam[0] + 1;
				if (sprite_y < 240) {
					uint32_t global_x = nes->ppu.scroll[0] + ((nes->ppu.reg[0] & 0x01) << 8) + sprite_x;
					uint32_t global_y = nes->ppu.scroll_y + sprite_y;
					if (((nes->ppu.scroll_y & 0xFF) < 240) && ((nes->ppu.scroll_y & 0xFF) + sprite_y >= 240)) global_y += 16;
					if (((nes->ppu.scroll_y & 0xFF) >= 240) && ((nes->ppu.scroll_y & 0xFF) + sprite_y >= 256)) global_y -= 256;
					uint32_t ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
					uint32_t pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
					uint64_t bg_pattern[6];
					bg_pattern[0] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
					bg_pattern[0] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
					global_x += 8;
					ntb_addr = ((global_x & 0xF8) >> 3) | ((global_y & 0xF8) << 2) | ((global_x & 0x100) << 2) | ((global_y & 0x100) << 3);
					pat_addr = (*nessys_ppu_mem(nes, NESSYS_CHR_NTB_WIN_MIN + ntb_addr) << 4) | ((nes->ppu.reg[0] & 0x10) << 8);
					bg_pattern[1] = *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr));
					bg_pattern[1] |= *((uint64_t*)nessys_ppu_mem(nes, NESSYS_CHR_ROM_WIN_MIN + pat_addr + 8));
					if (((global_y & 0xFF) >= 240) && ((global_y & 0xFF) + 8) >= 256) global_y -= 256;
					if (((global_y & 0xFF) < 240) && ((global_y & 0xFF) + 8) >= 240) global_y += 16;
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
					if (((global_y & 0xFF) >= 240) && ((global_y & 0xFF) + 8) >= 256) global_y -= 256;
					if (((global_y & 0xFF) < 240) && ((global_y & 0xFF) + 8) >= 240) global_y += 16;
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
					} else if (sprite_y > 224) {
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
										} else {
											hit_mask <<= 1;
										}
									}
								} else {
									hit_mask >>= 8;
								}
							}
						}
					}

					if (sprite0_hit) {
						if ((int32_t)hit_y < nes->scanline) {
							nes->sprite0_hit_cycles = 1;
						} else {
							nes->sprite0_hit_cycles = NESSYS_PPU_CLK_PER_SCANLINE * (hit_y - nes->scanline) + hit_x - nes->scanline_cycle;
						}
						//printf(" - hit %d\n", nes->sprite0_hit_cycles);
					} else {
						nes->sprite0_hit_cycles = 0;
					}
				}
			}

			// need to only do this if sprtes have changed position
			nessys_gen_scanline_sprite_map(nes);

			nes->mapper_cpu_setup(nes);
			cycles = nessys_exec_cpu_cycles(nes, 0);
		} else {
			nes->scroll_x_scanline = 0;
		}
	} while ((nes->cycles_remaining > 20) || (nes->scroll_x_scanline > 0));

	nessys_scale_to_back_buffer(nes);

	nesssys_set_scanline(nes, -22);
	//printf("cycles remaining: %d\n", nes->cycles_remaining);
	uint32_t cycle_dec = nes->apu.sample_frac_generated / NESSYS_SND_SAMPLES_FRAC_PER_CYCLE;
	nes->cycle -= cycle_dec;
	nes->apu.sample_frac_generated -= cycle_dec * NESSYS_SND_SAMPLES_FRAC_PER_CYCLE;
	nes->sbuf_frame_start = nes->sbuf_offset;
	nes->vblank_cycles = NESSYS_PPU_SCANLINES_POST_RENDER_CLKS + nes->cycles_remaining;
	nes->vblank_clear_cycles = nes->vblank_cycles + NESSYS_PPU_SCANLINES_VBLANK_CLKS + 1;
	// 1 post render line + 20 vblank lines
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_VBLANK_CLKS + NESSYS_PPU_SCANLINES_POST_RENDER_CLKS);
	uint32_t skip_cycle = ((nes->frame & 1) && (nes->ppu.reg[1] & 0x18)) ? 1 : 0;
	// pre-render line
	cycles = nessys_exec_cpu_cycles(nes, NESSYS_PPU_SCANLINES_PRE_RENDER_CLKS - skip_cycle);
	nes->scanline_cycle += skip_cycle;

	nessys_gen_sound(nes);
	nes->win->SwapBuffer();
	nes->frame++;

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

void K3CALLBACK nessys_keyboard(void* ptr, k3key k, char c, k3keyState state)
{
	nessys_t* nes = (nessys_t*)ptr;
	uint8_t key_mask = 0x0;
	switch (k) {
	case k3key::ESCAPE:
		if (state == k3keyState::PRESSED) {
			if (nes->menu.message_box != "") {
				nes->menu.message_box = "";
			} else {
				nes->menu.pane = (nes->menu.pane == nesmenu_pane_t::MAIN) ? nesmenu_pane_t::NONE : nesmenu_pane_t::MAIN;
				nesmenu_update_list(nes);
			}
		}
		break;
	case k3key::UP:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_UP_MASK;
		break;
	case k3key::DOWN:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK;
		break;
	case k3key::LEFT:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK;
		break;
	case k3key::RIGHT:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK;
		break;
	case k3key::A:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_B_MASK;
		break;
	case k3key::S:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_A_MASK;
		break;
	case k3key::Q:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_SELECT_MASK;
		break;
	case k3key::W:
		key_mask = NESSYS_STD_CONTROLLER_BUTTON_START_MASK;
		break;
	}

	if (key_mask && nes->num_joy < 2) {
		if (state == k3keyState::RELEASED) {
			nes->apu.joypad[nes->num_joy] &= ~key_mask;
		} else {
			nes->apu.joypad[nes->num_joy] |= key_mask;
		}
		if (nes->apu.joy_control & 0x1) {
			nes->apu.latched_joypad[0] = nessys_masked_joypad(nes->apu.joypad[0]);
			nes->apu.latched_joypad[1] = nessys_masked_joypad(nes->apu.joypad[1]);
		}
	}
}

void K3CALLBACK nessys_joystick_added(void* ptr, uint32_t joystick, const k3joyInfo* joy_info, const k3joyState* joy_state)
{
	nessys_t* nes = (nessys_t*)ptr;
	if (nes->num_joy < 2) {
		nes->joy_data[nes->num_joy].dev_id = joystick;
		uint32_t a;
		for (a = 0; a < joy_info->num_axes; a++) {
			switch (joy_info->axis[a]) {
			case k3joyAxis::X: nes->joy_data->x_axis = a; break;
			case k3joyAxis::Y: nes->joy_data->y_axis = a; break;
			case k3joyAxis::POV: nes->joy_data->pov_axis = a; break;
			}
		}
		nes->joy_data[nes->num_joy].button_b = 0;
		nes->joy_data[nes->num_joy].button_a = 1;
		nes->joy_data[nes->num_joy].button_start = joy_info->num_buttons - 2;
		nes->joy_data[nes->num_joy].button_select = joy_info->num_buttons - 1;
		nes->num_joy++;
	}
}

void K3CALLBACK nessys_joystick_removed(void* ptr, uint32_t joystick)
{
	nessys_t* nes = (nessys_t*)ptr;
	bool remove = false;
	uint32_t j;
	for (j = 0; j < nes->num_joy; j++) {
		if (nes->joy_data[j].dev_id == joystick) remove = true;
		if (remove && j < nes->num_joy - 1) {
			nes->joy_data[j] = nes->joy_data[j + 1];
		}
	}
	if (remove) nes->num_joy--;
}

void K3CALLBACK nessys_joystick_move(void* ptr, uint32_t joystick, uint32_t axis_num, k3joyAxis axis, uint32_t ordinal, float position)
{
	nessys_t* nes = (nessys_t*)ptr;
	uint32_t j;
	uint8_t key_mask = 0, key_pressed = 0;
	for (j = 0; j < nes->num_joy; j++) {
		if (nes->joy_data[j].dev_id == joystick) {
			if (nes->joy_data[j].x_axis == axis_num) {
				key_mask = NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK | NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK;
				if (position > 0.75f) key_pressed = NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK;
				else if (position < 0.25f) key_pressed = NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK;
			} else if (nes->joy_data[j].y_axis == axis_num) {
				key_mask = NESSYS_STD_CONTROLLER_BUTTON_UP_MASK | NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK;
				if (position > 0.75f) key_pressed = NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK;
				else if (position < 0.25f) key_pressed = NESSYS_STD_CONTROLLER_BUTTON_UP_MASK;
			} else if (nes->joy_data[j].pov_axis == axis_num) {
				key_mask = NESSYS_STD_CONTROLLER_BUTTON_UP_MASK | NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK |
					NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK | NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK;
				if (position <= 0.125f || position >= 0.875f) key_pressed |= NESSYS_STD_CONTROLLER_BUTTON_UP_MASK;
				else if (position >= 0.375f && position <= 0.625f) key_pressed |= NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK;
				if (position >= 0.125f && position <= 0.375f) key_pressed |= NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK;
				else if (position >= 0.625f && position <= 0.875f) key_pressed |= NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK;
				if (position >= 1.0f) key_pressed = 0;
			}
			nes->apu.joypad[j] &= ~key_mask;
			nes->apu.joypad[j] |= key_pressed;
			if (nes->apu.joy_control & 0x1) {
				nes->apu.latched_joypad[0] = nessys_masked_joypad(nes->apu.joypad[0]);
				nes->apu.latched_joypad[1] = nessys_masked_joypad(nes->apu.joypad[1]);
			}
		}
	}
}

void K3CALLBACK nessys_joystick_button(void* ptr, uint32_t joystick, uint32_t button, k3keyState state)
{
	nessys_t* nes = (nessys_t*)ptr;
	uint32_t j;
	uint8_t key_mask = 0;
	for (j = 0; j < nes->num_joy; j++) {
		if (nes->joy_data[j].dev_id == joystick) {
			if (nes->joy_data[j].button_a == button) key_mask = NESSYS_STD_CONTROLLER_BUTTON_A_MASK;
			else if (nes->joy_data[j].button_b == button) key_mask = NESSYS_STD_CONTROLLER_BUTTON_B_MASK;
			else if (nes->joy_data[j].button_start == button) key_mask = NESSYS_STD_CONTROLLER_BUTTON_START_MASK;
			else if (nes->joy_data[j].button_select == button) key_mask = NESSYS_STD_CONTROLLER_BUTTON_SELECT_MASK;
		}
		if (key_mask) {
			if (state == k3keyState::RELEASED) {
				nes->apu.joypad[j] &= ~key_mask;
			} else {
				nes->apu.joypad[j] |= key_mask;
			}
			if (nes->apu.joy_control & 0x1) {
				nes->apu.latched_joypad[0] = nessys_masked_joypad(nes->apu.joypad[0]);
				nes->apu.latched_joypad[1] = nessys_masked_joypad(nes->apu.joypad[1]);
			}
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
		nessys_power_cycle(nes);
	} else {
		nessys_unload_cart(nes);
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
	bt->sprite0_hit_cycles = nes->sprite0_hit_cycles;
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

bool nessys_add_mid_scan_bank_change(nessys_t* nes)
{
	bool ppu_ever_written = false;
	uint8_t scan_position = nessys_get_scan_position(nes);
	if (scan_position == 0 && nes->num_mid_scan_ntb_bank_changes == 0) {
		ppu_ever_written = true;
	} else {
		uint8_t chr_bank = (nes->ppu.reg[0] & 0x10) ? 4 : 0;
		if (nes->num_mid_scan_ntb_bank_changes >= NESSYS_MAX_MID_SCAN_NTB_BANK_CHANGES)
			nes->num_mid_scan_ntb_bank_changes = NESSYS_MAX_MID_SCAN_NTB_BANK_CHANGES - 1;
		ppu_ever_written = (nes->mid_scan_ntb_bank_change_position[nes->num_mid_scan_ntb_bank_changes] != scan_position);
		nes->mid_scan_ntb_bank_change_position[nes->num_mid_scan_ntb_bank_changes] = scan_position;
		nes->mid_scan_ntb_banks[4 * nes->num_mid_scan_ntb_bank_changes + 0] = nes->ppu.chr_rom_bank[chr_bank + 0];
		nes->mid_scan_ntb_banks[4 * nes->num_mid_scan_ntb_bank_changes + 1] = nes->ppu.chr_rom_bank[chr_bank + 1];
		nes->mid_scan_ntb_banks[4 * nes->num_mid_scan_ntb_bank_changes + 2] = nes->ppu.chr_rom_bank[chr_bank + 2];
		nes->mid_scan_ntb_banks[4 * nes->num_mid_scan_ntb_bank_changes + 3] = nes->ppu.chr_rom_bank[chr_bank + 3];
		nes->num_mid_scan_ntb_bank_changes++;
	}
	return ppu_ever_written;
}

uint32_t nessys_exec_cpu_cycles(nessys_t* nes, uint32_t num_cycles)
{
	uint32_t cycle_count = 0;
	nes->cycles_remaining += num_cycles; // cycles to execute
	bool done = false;
	bool ppu_ever_written = false;
	const c6502_op_code_t* op = NULL;
	const c6502_op_code_t* op_next = NULL;
	uint16_t addr = nes->reg.pc, indirect_addr = 0x0;
	uint8_t* operand = &nes->reg.a;
	uint16_t bank, offset;
	uint16_t result;
	uint16_t mem_addr_mask;
	uint8_t* pc_ptr = NULL;
	uint8_t* pc_ptr_next = NULL;
	uint16_t penalty_cycles;
	uint8_t overflow;
	bool vblank_interrupt_taken;
	uint8_t skip_print = 0;
	bool ppu_write, apu_write, rom_write;
	uint8_t data_change;  // when writing to addressable memory, indicates which bits changed
	uint8_t reset_ppu_status_after_nmi = 0;
	int32_t next_scanline_cycle = 0;
	nes->ppu.reg[2] &= 0x1F;
	nes->ppu.reg[2] |= nes->ppu.status;
	pc_ptr_next = nessys_mem(nes, nes->reg.pc, &bank, &offset);
	op_next = &C6502_OP_CODE[*pc_ptr_next];
	while (!done) {
		pc_ptr = pc_ptr_next;
		op = op_next;
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

		vblank_interrupt_taken = false;
		if(nes->vblank_clear_cycles == 1) {
			nes->ppu.status &= ~0xE0;
			nes->ppu.reg[2] &= ~0xE0;
			nes->vblank_clear_cycles = 0;
		}
		if(nes->vblank_irq) {
			nes->ppu.status |= 0x80;
			nes->ppu.reg[2] |= 0x80;
			vblank_interrupt_taken = (nes->ppu.reg[0] & 0x80) ? true : false;
			nes->vblank_irq = false;
		}
		if(nes->sprite0_hit_cycles == 1) {
			nes->ppu.status |= 0x40;
			nes->ppu.reg[2] |= 0x40;
			nes->sprite0_hit_cycles = 0;
		}
		if (nes->sprite_overflow_cycles == 1) {
			nes->ppu.status |= 0x20;
			nes->ppu.reg[2] |= 0x20;
			nes->sprite_overflow_cycles = 0;
		}

		uint32_t instruction_cycles = (NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles));
		bool irq_interrupt = !((nes->reg.p ^ nes->iflag_delay) & C6502_P_I) && (nes->mapper_irq || nes->frame_irq || nes->dmc_irq);
		nes->iflag_delay = 0;
		nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->frame_irq) ? 0x40 : 0x0;
		nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->dmc_irq) ? 0x80 : 0x0;
		nes->cpu_cycle_inc = 0;
		if (nes->cycles_remaining < (int32_t)instruction_cycles || vblank_interrupt_taken) {
			done = !vblank_interrupt_taken;
			if (vblank_interrupt_taken) {
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = nes->reg.pc >> 8;        nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.pc & 0xFF;      nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.p & ~C6502_P_B; nes->reg.s--;
				nes->stack_trace[nes->stack_trace_entry].scanline = nes->scanline;
				nes->stack_trace[nes->stack_trace_entry].scanline_cycle = nes->scanline_cycle;
				nes->stack_trace[nes->stack_trace_entry].frame = nes->frame;
				nes->stack_trace[nes->stack_trace_entry].return_addr = nes->reg.pc;
				nes->reg.pc = *((uint16_t*) nessys_mem(nes, NESSYS_NMI_VECTOR, &bank, &offset));
				nes->reg.p |= C6502_P_I;
				nes->in_nmi++;
				nes->ppu.old_status  = (nes->ppu.status & 0xe0);
				nes->ppu.old_status |= (nes->ppu.reg[2] & 0x1f);
				nes->stack_trace[nes->stack_trace_entry].jump_addr = nes->reg.pc;
				nes->irq_trace[nes->irq_trace_entry] = nes->stack_trace[nes->stack_trace_entry];
				nes->stack_trace_entry++;
				if (nes->stack_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->stack_trace_entry = 0;
				nes->irq_trace_entry++;
				if (nes->irq_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->irq_trace_entry = 0;

				nes->cpu_cycle_inc = 7;
			}
		} else if( irq_interrupt) {
			// mapper interrupt
			// get the stack base
			operand = nessys_mem(nes, 0x100, &bank, &offset);
			nes->stack_trace[nes->stack_trace_entry].scanline = nes->scanline;
			nes->stack_trace[nes->stack_trace_entry].scanline_cycle = nes->scanline_cycle;
			nes->stack_trace[nes->stack_trace_entry].frame = nes->frame;
			nes->stack_trace[nes->stack_trace_entry].return_addr = nes->reg.pc;
			*(operand + nes->reg.s) = nes->reg.pc >> 8;        nes->reg.s--;
			*(operand + nes->reg.s) = nes->reg.pc & 0xFF;      nes->reg.s--;
			*(operand + nes->reg.s) = nes->reg.p & ~C6502_P_B; nes->reg.s--;
			nes->reg.pc = *((uint16_t*)nessys_mem(nes, NESSYS_IRQ_VECTOR, &bank, &offset));
			nes->reg.p |= C6502_P_I;
			nes->stack_trace[nes->stack_trace_entry].jump_addr = nes->reg.pc;
			nes->irq_trace[nes->irq_trace_entry] = nes->stack_trace[nes->stack_trace_entry];
			nes->stack_trace_entry++;
			if (nes->stack_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->stack_trace_entry = 0;
			nes->irq_trace_entry++;
			if (nes->irq_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->irq_trace_entry = 0;

			nes->cpu_cycle_inc = 7;
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
			bool clear_frame_irq = false;
			//bool clear_dmc_irq = false;
			switch (bank) {
			case NESSYS_PPU_REG_START_BANK:
				switch (offset) {
				case 2:
					nes->ppu.status &= ~0x80;
					if (nes->vblank_cycles == 1) nes->vblank_cycles = 0;
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
					nes->apu.reg[offset] = (nes->apu.latched_joypad[j] & 0x1) | (0x40);
				} else if (offset == NESSYS_APU_STATUS_OFFSET) {
					nessys_gen_sound(nes);
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= 0xc0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.pulse[0].length) ? 0x1 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.pulse[1].length) ? 0x2 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.triangle.length) ? 0x4 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->apu.noise.length) ? 0x8 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->dmc_bits_to_play >= 8) ? 0x10 : 0x0;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] |= (nes->frame_irq) ? 0x40 : 0x0;
					clear_frame_irq = true;
				} else if (offset >= NESSYS_APU_SIZE) {
					uint8_t* op = nes->mapper_read(nes, addr);
					if (op) operand = op;
				}
				break;
			}

			//if (nes->reg.s == 0xfe) printf("------------------> stack about to overflow: 0x%x!!!\n", nes->reg.s);;
			// perform the instruction's operation
			switch (op->ins) {
			case C6502_INS_ADC:
			case C6502_INS_SBC:
				result = *operand;
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
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->stack_trace[nes->stack_trace_entry].scanline = nes->scanline;
				nes->stack_trace[nes->stack_trace_entry].scanline_cycle = nes->scanline_cycle;
				nes->stack_trace[nes->stack_trace_entry].frame = nes->frame;
				nes->stack_trace[nes->stack_trace_entry].return_addr = nes->reg.pc;
				*(operand + nes->reg.s) = nes->reg.pc >> 8;   nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.pc & 0xFF; nes->reg.s--;
				*(operand + nes->reg.s) = nes->reg.p;         nes->reg.s--;
				nes->reg.pc = *((uint16_t*) nessys_mem(nes, NESSYS_IRQ_VECTOR, &bank, &offset));
				nes->stack_trace[nes->stack_trace_entry].jump_addr = nes->reg.pc;
				nes->irq_trace[nes->irq_trace_entry] = nes->stack_trace[nes->stack_trace_entry];
				nes->stack_trace_entry++;
				if (nes->stack_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->stack_trace_entry = 0;
				nes->irq_trace_entry++;
				if (nes->irq_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->irq_trace_entry = 0;
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
				nes->iflag_delay = nes->reg.p;
				nes->reg.p &= ~C6502_P_I;
				nes->iflag_delay ^= nes->reg.p;
				nes->iflag_delay &= C6502_P_I;
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
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				if (nes->reg.pc - op->num_bytes == addr) skip_print = 1;
				nes->reg.pc = addr;
				break;
			case C6502_INS_JSR:
				result = nes->reg.pc - 1;
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				*(operand + nes->reg.s) = result >> 8;   nes->reg.s--;
				*(operand + nes->reg.s) = result & 0xFF; nes->reg.s--;
				nes->stack_trace[nes->stack_trace_entry].scanline = nes->scanline;
				nes->stack_trace[nes->stack_trace_entry].scanline_cycle = nes->scanline_cycle;
				nes->stack_trace[nes->stack_trace_entry].frame = nes->frame;
				nes->stack_trace[nes->stack_trace_entry].return_addr = nes->reg.pc;
				nes->stack_trace[nes->stack_trace_entry].jump_addr = addr;
				nes->stack_trace_entry++;
				if (nes->stack_trace_entry >= NESSYS_STACK_TRACE_ENTRIES) nes->stack_trace_entry = 0;
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
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				nes->iflag_delay = nes->reg.p;
				nes->reg.s++; nes->reg.p = *(operand + nes->reg.s) | C6502_P_U | C6502_P_B;
				nes->iflag_delay ^= nes->reg.p;
				nes->iflag_delay &= C6502_P_I;
				break;
			case C6502_INS_ROL:
				result = (*operand << 1) | ((nes->reg.p & C6502_P_C) >> C6502_P_C_SHIFT);
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= (result & 0x100) >> (8 - C6502_P_C_SHIFT);
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				}
				break;
			case C6502_INS_ROR:
				result = (*operand >> 1) | ((nes->reg.p & C6502_P_C) << (7 - C6502_P_C_SHIFT));
				// clear N/Z/C
				nes->reg.p &= ~(C6502_P_N | C6502_P_Z | C6502_P_C);
				nes->reg.p |= (*operand & 0x1) << C6502_P_C_SHIFT;
				nes->reg.p |= ((result & 0xFF) == 0x00) << C6502_P_Z_SHIFT;
				nes->reg.p |= (result & 0x80) >> (7 - C6502_P_N_SHIFT);
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				if (nes->stack_trace_entry == 0) nes->stack_trace_entry = NESSYS_STACK_TRACE_ENTRIES;
				nes->stack_trace_entry--;
				if (nes->irq_trace_entry == 0) nes->irq_trace_entry = NESSYS_STACK_TRACE_ENTRIES;
				nes->irq_trace_entry--;
				break;
			case C6502_INS_RTS:
				// get the stack base
				operand = nessys_mem(nes, 0x100, &bank, &offset);
				nes->reg.s++; result = *(operand + nes->reg.s);
				nes->reg.s++; result |= ((uint16_t) * (operand + nes->reg.s)) << 8;
				nes->reg.pc = result + 1;
				if (nes->stack_trace_entry == 0) nes->stack_trace_entry = NESSYS_STACK_TRACE_ENTRIES;
				nes->stack_trace_entry--;
				break;
			case C6502_INS_SEC:
				nes->reg.p |= C6502_P_C;
				break;
			case C6502_INS_SED:
				nes->reg.p |= C6502_P_D;
				break;
			case C6502_INS_SEI:
				nes->iflag_delay = nes->reg.p;
				nes->reg.p |= C6502_P_I;
				nes->iflag_delay ^= nes->reg.p;
				nes->iflag_delay &= C6502_P_I;
				break;
			case C6502_INS_STA:
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
					ppu_write = (bank == NESSYS_PPU_REG_START_BANK) || (bank == NESSYS_APU_REG_START_BANK && offset == 0x14);
					apu_write = (bank == NESSYS_APU_REG_START_BANK);
					if (apu_write && offset >= NESSYS_APU_STATUS_OFFSET) {
						switch (offset) {
						case NESSYS_APU_STATUS_OFFSET:
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
				}
				break;
			case C6502_INS_STX:
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
				}
				break;
			case C6502_INS_STY:
				if (bank < NESSYS_PRG_ROM_START_BANK && !(bank == NESSYS_APU_REG_START_BANK && offset >= NESSYS_APU_SIZE)) {
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
						// if bit 2 is 0, increment address by 1 (one step horizontal), otherwise, increment by 32 (one step vertical)
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
					nes->apu.pulse[pulse_sel].env.volume = nes->apu.reg[offset] & 0xf;
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
					nes->apu.noise.env.volume = nes->apu.reg[0xc] & 0xf;
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
					if (!(nes->apu.dmc.flags & NESSYS_APU_DMC_FLAG_IRQ_ENABLE)) {
						nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x80;
						nes->dmc_irq = false;
					}
					break;
				case 0x11:
					nes->apu.dmc.output = nes->apu.reg[0x11] & 0x7f;
					break;
				case 0x12:
					nes->apu.dmc.start_addr = 0xc000 + (((uint16_t)nes->apu.reg[0x12]) << 6);
					break;
				case 0x13:
					nes->apu.dmc.length = (((uint16_t)nes->apu.reg[0x13]) << 4) + 1;
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
						nes->dmc_bits_to_play &= 0x7;
						nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x80;
						nes->dmc_irq = false;
					} else {
						if (nes->apu.dmc.bytes_remaining == 0) {
							nes->apu.dmc.cur_addr = nes->apu.dmc.start_addr;
							nes->apu.dmc.bytes_remaining = nes->apu.dmc.length;
							nes->apu.dmc.bits_remaining = 0;
						}
						nes->apu.dmc.flags |= NESSYS_APU_DMC_FLAG_DMA_ENABLE;
						if ((nes->dmc_bits_to_play & ~0x7) == 0) {
							// a byte is immediately loaded if buffer is empty
							uint32_t length = nes->apu.dmc.length - !nes->dmc_buffer_full;
							//penalty_cycles += 4 * !nes->dmc_buffer_full;
							nes->dmc_buffer_full = true;
							nes->dmc_bits_to_play &= 0x7;
							nes->dmc_bits_to_play |= (length << 3);
							if (nes->dmc_bits_to_play < 8 && ((nes->apu.dmc.flags & (NESSYS_APU_DMC_FLAG_IRQ_ENABLE | NESSYS_APU_DMC_FLAG_LOOP)) == NESSYS_APU_DMC_FLAG_IRQ_ENABLE)) {
								nes->dmc_irq = true;
								//nes->dmc_bits_to_play--;
							} else {
								nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x80;
								nes->dmc_irq = false;
							}
						} else {
							nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x80;
							nes->dmc_irq = false;
						}
					}
					break;
				case NESSYS_APU_JOYPAD0_OFFSET:
					if (nes->apu.joy_control & 0x1) {
						nes->apu.latched_joypad[0] = nessys_masked_joypad(nes->apu.joypad[0]);
						nes->apu.latched_joypad[1] = nessys_masked_joypad(nes->apu.joypad[1]);
					}
					break;
				case NESSYS_APU_FRAME_COUNTER_OFFSET:
					nes->apu.frame_frac_counter = 0;
					if (nes->apu.frame_counter & 0x80) {
						nessys_apu_frame_tick(nes);
					}
					if ((nes->apu.frame_counter & 0x40) != 0) {
						nes->frame_irq = false;
						nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x40;
					}
					break;
				}
			} else {
				if (bank == NESSYS_APU_REG_START_BANK) {
					switch (offset) {
					case NESSYS_APU_STATUS_OFFSET:
						clear_frame_irq = true;
						break;
					case NESSYS_APU_JOYPAD0_OFFSET:
					case NESSYS_APU_JOYPAD1_OFFSET:
						uint8_t j = offset - NESSYS_APU_JOYPAD0_OFFSET;
						if ((nes->apu.joy_control & 0x1) == 0x0) {
							nes->apu.latched_joypad[j] >>= 1;
							nes->apu.latched_joypad[j] |= 0x80;
						}
						break;
					}
				}
			}
			if (ppu_write) {
				bool custom_bg_setup = (nes->mapper_bg_setup_type & NESSYS_MAPPER_SETUP_CUSTOM) ? true : false;
				int32_t scroll_start = (nes->scanline < 0) ? 0 : nes->scanline;// +1;
				bool scroll_x_changed = false;
				if (bank == NESSYS_PPU_REG_START_BANK) {
					nes->ppu.reg[2] = (nes->ppu.status & 0xE0) | (nes->ppu.reg[offset & 0x7] & 0x1F);
				}
				switch (offset) {
				case 0x0:
					// if we toggle vblank enable from 0 to 1 while in vblank, retrigger a vblank
					if ((nes->ppu.reg[2] & 0x80) && (data_change & 0x80) && (nes->ppu.reg[0] & 0x80)) {
						nes->vblank_cycles = NESSYS_PPU_PER_CPU_CLK * (op->num_cycles + penalty_cycles) + 1;
					}
					// we only re-render frame if bits that change rendering changes
					ppu_ever_written = ppu_ever_written || (data_change & 0x29);
					if ((data_change & 0x10) && (nes->ppu.reg[1] & 0x08)) {
						if (nes->scanline_cycle < 256) {
							nes->ppu.reg[0] ^= 0x10;
							ppu_ever_written = nessys_add_mid_scan_bank_change(nes) || ppu_ever_written;
							nes->ppu.reg[0] ^= 0x10;
						} else {
							ppu_ever_written = true;
						}
					}
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
					scroll_x_changed = (nes->ppu.scroll_x[scroll_start] != nes->ppu.reg[5]);
					// only update if the scroll value actually changed, and only in x direction (y updates at next frame)
					ppu_ever_written = ppu_ever_written || ((nes->ppu.addr_toggle == 0) && scroll_x_changed && (custom_bg_setup || (nes->num_mid_scan_ntb_bank_changes != 0)));
					// update the vram address as well
					if (nes->ppu.addr_toggle) {
						nes->ppu.t_mem_addr &= 0x0c1f;
						nes->ppu.t_mem_addr |= ((uint16_t)nes->ppu.reg[5] << 2) & 0x03e0;
						nes->ppu.t_mem_addr |= ((uint16_t)nes->ppu.reg[5] << 12) & 0x7000;
					} else {
						nes->ppu.t_mem_addr &= 0xffe0;
						nes->ppu.t_mem_addr |= (nes->ppu.reg[5] >> 3) & 0x1f;
						if (nes->scanline < -1) {
							memset(nes->ppu.scroll_x, nes->ppu.reg[5], 240);
						} else if(nes->scanline < 239) {
							if (nes->scanline_cycle < 256) {
								nes->ppu.scroll_x[scroll_start] = nes->ppu.reg[5];
							} else {
								nes->ppu.scroll_x[scroll_start] &= 0xF8;
								nes->ppu.scroll_x[scroll_start] |= nes->ppu.reg[5] & 0x7;
							}
							memset(&(nes->ppu.scroll_x[scroll_start + 1]), nes->ppu.reg[5], 240 - 1 - scroll_start);
							if (!ppu_ever_written && nes->scroll_x_scanline <= 0 && scroll_x_changed) {
								nes->scroll_x_scanline = scroll_start;
							}
						}
					}
					nes->ppu.scroll[nes->ppu.addr_toggle] = nes->ppu.reg[5];
					nes->ppu.addr_toggle = !nes->ppu.addr_toggle;
					break;
				case 0x6:
					ppu_ever_written = true;  // any access to this register during rendering can effect rendering
					mem_addr_mask = 0xFF00 >> 8 * (nes->ppu.addr_toggle);
					nes->ppu.t_mem_addr &= ~mem_addr_mask;
					nes->ppu.t_mem_addr |= (((uint16_t) nes->ppu.reg[6] << 8) | nes->ppu.reg[6]) & mem_addr_mask;
					// update scroll and nametable select signals
					if (nes->ppu.addr_toggle) {
						nes->ppu.scroll[0] &= 0x07;
						nes->ppu.scroll[0] |= (nes->ppu.reg[6] << 3) & 0xf8;
						nes->ppu.scroll[1] &= 0xc7;
						nes->ppu.scroll[1] |= (nes->ppu.reg[6] >> 2) & 0x38;
						memset(&(nes->ppu.scroll_x[scroll_start]), nes->ppu.scroll[0], 240 - scroll_start);
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
			if (clear_frame_irq) {
				if (nes->apu.reg[NESSYS_APU_STATUS_OFFSET] & 0x40) {
					nes->frame_irq = false;
					nes->apu.reg[NESSYS_APU_STATUS_OFFSET] &= ~0x40;
				}
			}
			nes->cpu_cycle_inc = op->num_cycles + penalty_cycles;
		}

		pc_ptr_next = nessys_mem(nes, nes->reg.pc, &bank, &offset);
		op_next = &C6502_OP_CODE[*pc_ptr_next];

		while (nes->dmc_bit_timer < (nes->cpu_cycle_inc)) {
			if (nes->dmc_bits_to_play == 8 && (nes->apu.dmc.flags & NESSYS_APU_DMC_FLAG_LOOP)) {
				nes->dmc_bits_to_play += ((uint32_t)nes->apu.dmc.length << 3);
			}
			if (nes->dmc_bits_to_play == 8 && (nes->apu.dmc.flags & NESSYS_APU_DMC_FLAG_IRQ_ENABLE)) {
				nes->dmc_irq = true;
			}
			//if (nes->dmc_bits_to_play == 0) {
			//	if (nes->apu.dmc.flags & NESSYS_APU_DMC_FLAG_LOOP) {
			//		nes->dmc_bits_to_play += ((uint32_t)nes->apu.dmc.length << 3);
			//	} else {
			//		nes->dmc_buffer_full = false;
			//	}
			//}

			//if ((nes->dmc_bits_to_play & 0x7) == 0 && nes->dmc_buffer_full)
			//	nes->cpu_cycle_inc += (nes->cpu_cycle_inc > 500) ? 2 : 4;

			if (nes->dmc_bits_to_play == 0) {
				nes->dmc_bits_to_play = 8;
				nes->dmc_buffer_full = false;
			}
			nes->dmc_bits_to_play--;
			//if (nes->dmc_bits_to_play) nes->dmc_bits_to_play--;
			nes->dmc_bit_timer += nes->apu.dmc.period;
		}
		nes->dmc_bit_timer -= nes->cpu_cycle_inc;

		cycle_count += NESSYS_PPU_PER_CPU_CLK*(nes->cpu_cycle_inc);
		// cycles remaining may go negative if we have a oam dma transfer
		// the cycle penalty is accounted for until after th decision has already been bade to execute it
		nes->cycles_remaining -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
		nes->cycle += NESSYS_PPU_PER_CPU_CLK*(nes->cpu_cycle_inc);
		next_scanline_cycle = nes->scanline_cycle;
		next_scanline_cycle += NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
		if ((nes->ppu.reg[1] & 0x18) && (nes->scanline >= -1) && !(ppu_write && offset == 0x06)) {
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
					uint32_t saved_cpu_cycle_inc = nes->cpu_cycle_inc;
					// if we call mapper_update several times, we don't want it to update cpu cycle count every time; just the last time, which happens outside this loop
					nes->cpu_cycle_inc = 0;
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
					nes->cpu_cycle_inc = saved_cpu_cycle_inc;
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
		if ((nes->scanline_cycle + (NESSYS_PPU_PER_CPU_CLK * op_next->num_cycles)) >= (int32_t)NESSYS_PPU_CLK_PER_SCANLINE) {
			nes->scanline_cycle -= NESSYS_PPU_CLK_PER_SCANLINE;

			nes->scanline++;
			if (nes->scanline >= (int32_t)NESSYS_PPU_SCANLINES_RENDERED) {
				nes->scanline -= NESSYS_PPU_SCANLINES_PER_FRAME;
			}

			if (nes->num_mid_scan_ntb_bank_changes != nes->last_num_mid_scan_ntb_bank_changes) {
				ppu_ever_written = true;
				nes->prev_last_num_mid_scan_ntb_bank_changes = nes->last_num_mid_scan_ntb_bank_changes;
				nes->last_num_mid_scan_ntb_bank_changes = nes->num_mid_scan_ntb_bank_changes;
			}
			nes->num_mid_scan_ntb_bank_changes = 0;

			// if we are on positive scanline, the we are in the display portion of the frame
			// if so, we are done if ppu had been written, and rnedering is enabled
			done |= (nes->scanline > 0) && ppu_ever_written && ((nes->ppu.reg[1] & 0x18) != 0x0) && (nes->scanline < NESSYS_PPU_SCANLINES_RENDERED);
		}
		if (nes->vblank_cycles > 0) {
			if (nes->vblank_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->vblank_cycles = 0;
				nes->vblank_irq = true;
			} else {
				nes->vblank_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->vblank_clear_cycles > 0) {
			if (nes->vblank_clear_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->vblank_clear_cycles = 1;
			} else {
				nes->vblank_clear_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->sprite0_hit_cycles > 0) {
			if (nes->sprite0_hit_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->sprite0_hit_cycles = 1;
			} else {
				nes->sprite0_hit_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
		if (nes->sprite_overflow_cycles > 0) {
			if (nes->sprite_overflow_cycles <= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc)) {
				nes->sprite_overflow_cycles = 1;
			} else {
				nes->sprite_overflow_cycles -= NESSYS_PPU_PER_CPU_CLK * (nes->cpu_cycle_inc);
			}
		}
	}
	return cycle_count;
}


void nessys_unload_cart(nessys_t* nes)
{
	nessys_cleanup_mapper(nes);
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
	nes->ppu.chr_rom_size = 0;
	nes->ppu.chr_ram_size = 0;
}

void nessys_cleanup(nessys_t* nes)
{
	nes->win->SetDisplayFunc(NULL);
	nes->win->SetDisplayFunc(NULL);
	nes->win->SetIdleFunc(NULL);
	nesmenu_cleanup(nes);
}


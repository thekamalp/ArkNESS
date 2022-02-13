// Project:     ArkNES
// File:        nessys.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description:	NES menu functions

#include "nessys.h"
#include <filesystem>

void nesmenu_init(nessys_t* nes)
{
	nes->menu.pane = nesmenu_pane_t::MAIN;
	nes->menu.select = 0;
	nes->menu.list_start = 0;
	nes->menu.cur_list = NULL;
	nes->menu.cur_list_size = 0;
	nes->menu.cur_list_alloc_size = 0;
	nes->menu.message_box = "";
	nes->menu.sprite_line_limit = 1;  // smart check by default
	char* home_dir = NULL;
#ifdef _WIN32
	size_t str_size;
	_dupenv_s(&home_dir, &str_size, "USERPROFILE");
#endif
	if (home_dir) {
		nes->menu.opt_file = std::string(home_dir);
		nes->menu.cur_dir = std::string(home_dir);
		nes->menu.opt_file += NESMENU_DIR_SEPARATOR;
		nes->menu.opt_file += ".arkness";
		const std::filesystem::path opt_path = nes->menu.opt_file;
		if (!std::filesystem::exists(opt_path)) {
			std::filesystem::create_directory(opt_path);
		}
		nes->menu.opt_file += NESMENU_DIR_SEPARATOR;
		nes->menu.opt_file += "options.txt";
		nesmenu_load_options(nes);
	} else {
		nes->menu.opt_file = "";  // can't save options - no home directory found
#ifdef _WIN32
		nes->menu.cur_dir = "C:\\";
#else
		nes->menu.cur_dir = "/";
#endif
	}
#ifdef _WIN32
	free(home_dir);
#endif
	nesmenu_update_list(nes);
	nes->menu.fence_val = 0;
	nes->menu.last_joypad_state[0] = 0;
	nes->menu.last_joypad_state[1] = 0;
}

void nesmenu_resize_list(nesmenu_data* menu)
{
	if (menu->cur_list_size > menu->cur_list_alloc_size) {
		if (menu->cur_list) delete[] menu->cur_list;
		menu->cur_list_alloc_size = menu->cur_list_size;
		menu->cur_list = new nesmenu_list_item[menu->cur_list_alloc_size];
	}
}

void nesmenu_update_list(nessys_t* nes)
{
	uint32_t index = 0;
	std::string item;
	nes->menu.select = 0;
	switch (nes->menu.pane) {
	case nesmenu_pane_t::NONE:
		nes->menu.cur_list_size = 0;
		nesmenu_resize_list(&(nes->menu));
		break;
	case nesmenu_pane_t::MAIN:
		nes->menu.cur_list_size = nesmenu_main_items;
		nesmenu_resize_list(&(nes->menu));
		for (index = 0; index < nesmenu_main_items; index++) {
			nes->menu.cur_list[index].item = nesmenu_main[index];
			nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_NONE;
		}
		break;
	case nesmenu_pane_t::OPTIONS:
		nes->menu.cur_list_size = nesmenu_options_items;
		nesmenu_resize_list(&(nes->menu));
		for (index = 0; index < nesmenu_options_items; index++) {
			nes->menu.cur_list[index].item = nesmenu_options[index];
			nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_NONE;
		}
		switch (nes->menu.sprite_line_limit) {
		case 0: nes->menu.cur_list[nesmenu_options_item_sprite_line_limit].item += "Off"; break;
		case 1: nes->menu.cur_list[nesmenu_options_item_sprite_line_limit].item += "Smart"; break;
		case 2: nes->menu.cur_list[nesmenu_options_item_sprite_line_limit].item += "On"; break;
		}
		switch (nes->menu.upscale_type) {
		case 0: nes->menu.cur_list[nesmenu_options_item_upscale_type].item += "Copy"; break;
		case 1: nes->menu.cur_list[nesmenu_options_item_upscale_type].item += "xBR"; break;
		}
		nes->menu.cur_list[nesmenu_options_item_vsync].item += (nes->win->GetVsyncInterval()) ? "On" : "Off";
		break;
	case nesmenu_pane_t::OPEN:
		index = 0;
		for (const auto& entry : std::filesystem::directory_iterator(nes->menu.cur_dir)) {
			index++;
		}
		nes->menu.cur_list_size = index + 1;  // one extra for ".."
		nesmenu_resize_list(&(nes->menu));
		index = 0;
		nes->menu.cur_list[index].item = "<..>";
		nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_DIRECTORY | NESMENU_ITEM_FLAG_UP_DIR;
		index++;
		for (const auto& entry : std::filesystem::directory_iterator(nes->menu.cur_dir)) {
			item = entry.path().filename().string();
			if (entry.is_directory()) {
				item = "<" + item + ">";
				nes->menu.cur_list[index].item = item;
				nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_DIRECTORY;
				index++;
			}
		}
		for (const auto& entry : std::filesystem::directory_iterator(nes->menu.cur_dir)) {
			item = entry.path().filename().string();
			if (!entry.is_directory()) {
				nes->menu.cur_list[index].item = item;
				nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_FILE;
				index++;
			}
		}
		break;
	}
}

void nesmenu_display(nessys_t* nes)
{
	std::string item;
	float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float fg_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float sel_color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float bg_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	uint32_t i, row;
	k3renderTargets rt = { NULL };
	uint32_t cur_menu_items = nes->menu.cur_list_size;
	nes->sb_main->StopSBuffer();
	std::string pane_title = "ArkNESS";

	uint32_t displayable_menu_items = (nes->win->GetHeight() / NESMENU_FONT_HEIGHT) - 1;
	if (displayable_menu_items > 30) displayable_menu_items = 30;

	if (nes->menu.last_joypad_state[0] != nes->apu.joypad[0]) {
		if (nes->menu.message_box != "") {
			if (nes->apu.joypad[0]) {
				nes->menu.message_box = "";
			}
		} else {
			if (nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK) {
				nes->menu.select++;
				if (nes->menu.select >= cur_menu_items) nes->menu.select = 0;
			}
			if (nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_UP_MASK) {
				if (nes->menu.select == 0) nes->menu.select = cur_menu_items - 1;
				else nes->menu.select--;
			}
			if (nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK) {
				if (nes->menu.select == cur_menu_items - 1) nes->menu.select = 0;
				else {
					nes->menu.select += displayable_menu_items;
					if (nes->menu.select >= cur_menu_items) nes->menu.select = cur_menu_items - 1;
				}
			}
			if (nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK) {
				if (nes->menu.select == 0) nes->menu.select = cur_menu_items - 1;
				else if (nes->menu.select < displayable_menu_items) nes->menu.select = 0;
				else nes->menu.select -= displayable_menu_items;
			}
			if (nes->apu.joypad[0] & (NESSYS_STD_CONTROLLER_BUTTON_A_MASK | NESSYS_STD_CONTROLLER_BUTTON_START_MASK)) {
				switch (nes->menu.pane) {
				case nesmenu_pane_t::MAIN:
					switch (nes->menu.select) {
					case nesmenu_main_item_open:
						nes->menu.pane = nesmenu_pane_t::OPEN;
						nesmenu_update_list(nes);
						break;
					case nesmenu_main_item_options:
						nes->menu.pane = nesmenu_pane_t::OPTIONS;
						nesmenu_update_list(nes);
						break;
					case nesmenu_main_item_exit:
						k3winObj::ExitLoop();
						return;
					}
					break;
				case nesmenu_pane_t::OPTIONS:
					switch (nes->menu.select) {
					case nesmenu_options_item_sprite_line_limit:
						nes->menu.sprite_line_limit++;
						if (nes->menu.sprite_line_limit > 2) nes->menu.sprite_line_limit = 0;
						nesmenu_update_list(nes);
						nes->menu.select = nesmenu_options_item_sprite_line_limit;
						break;
					case nesmenu_options_item_upscale_type:
						nes->menu.upscale_type++;
						if (nes->menu.upscale_type > 1) nes->menu.upscale_type = 0;
						nesmenu_update_list(nes);
						nes->menu.select = nesmenu_options_item_upscale_type;
						break;
					case nesmenu_options_item_vsync:
						nes->win->SetVsyncInterval(!nes->win->GetVsyncInterval());
						nesmenu_update_list(nes);
						nes->menu.select = nesmenu_options_item_vsync;
						break;
					}
					break;
				case nesmenu_pane_t::OPEN:
					item = nes->menu.cur_list[nes->menu.select].item;
					if (nes->menu.cur_list[nes->menu.select].flag & NESMENU_ITEM_FLAG_DIRECTORY) {
						if (nes->menu.cur_list[nes->menu.select].flag & NESMENU_ITEM_FLAG_UP_DIR) {
							size_t pos = nes->menu.cur_dir.rfind(NESMENU_DIR_SEPARATOR);
							size_t lpos = nes->menu.cur_dir.find(NESMENU_DIR_SEPARATOR);
							if (lpos == pos) pos++;
							if (pos != 0 && pos != std::string::npos) {
								nes->menu.cur_dir = nes->menu.cur_dir.substr(0, pos);
							}
						} else {
							nes->menu.cur_dir += NESMENU_DIR_SEPARATOR;
							nes->menu.cur_dir += item.substr(1, item.size() - 2);
						}
						nesmenu_update_list(nes);
					} else if (nes->menu.cur_list[nes->menu.select].flag & NESMENU_ITEM_FLAG_FILE) {
						item = nes->menu.cur_dir + NESMENU_DIR_SEPARATOR + item;
						bool success = nessys_load_cart_filename(nes, item.c_str());
						if (success && (nes->prg_rom_base != NULL)) {
							// successfully loaded cart
							nes->menu.pane = nesmenu_pane_t::NONE;
							nesmenu_update_list(nes);
						} else {
							nes->menu.message_box = "Could not load rom file";
						}
					}
					break;
				}
			}
			if (nes->apu.joypad[0] & NESSYS_STD_CONTROLLER_BUTTON_B_MASK) {
				switch (nes->menu.pane) {
				case nesmenu_pane_t::MAIN:
					// if a cart is loaded, then we can resume it
					nes->menu.pane = (nes->prg_rom_base) ? nesmenu_pane_t::NONE : nesmenu_pane_t::MAIN;
					nesmenu_update_list(nes);
					break;
				case nesmenu_pane_t::OPEN:
					nes->menu.pane = nesmenu_pane_t::MAIN;
					nesmenu_update_list(nes);
					break;
				case nesmenu_pane_t::OPTIONS:
					nes->menu.pane = nesmenu_pane_t::MAIN;
					nesmenu_update_list(nes);
					break;
				}
			}
		}
		nes->menu.last_joypad_state[0] = nes->apu.joypad[0];
	}

	cur_menu_items = nes->menu.cur_list_size;
	switch (nes->menu.pane) {
	case nesmenu_pane_t::NONE:
		pane_title = "Playing...";
		break;
	case nesmenu_pane_t::MAIN:
		pane_title = "ArkNESS";
		break;
	case nesmenu_pane_t::OPEN:
		pane_title = "Open";
		break;
	}

	if (nes->menu.select < nes->menu.list_start) nes->menu.list_start = nes->menu.select;
	if (nes->menu.select >= nes->menu.list_start + displayable_menu_items) nes->menu.list_start = nes->menu.select - displayable_menu_items + 1;

	displayable_menu_items += nes->menu.list_start;
	if (displayable_menu_items > cur_menu_items) displayable_menu_items = cur_menu_items;

	// render the last frame as a background
	nessys_scale_to_back_buffer(nes);

	nes->fence->WaitCpuFence(nes->menu.fence_val);
	rt.render_targets[0] = nes->win->GetBackBuffer();
	nes->cmd_buf->Reset();
	nes->cmd_buf->TransitionResource(rt.render_targets[0]->GetResource(), k3resourceState::RENDER_TARGET);
	//nes->cmd_buf->ClearRenderTarget(rt.render_targets[0], clear_color, NULL);
	nes->cmd_buf->SetRenderTargets(&rt);
	nes->cmd_buf->SetViewToSurface(rt.render_targets[0]->GetResource());
	nes->cmd_buf->SetDrawPrim(k3drawPrimType::TRIANGLESTRIP);
	nes->cmd_buf->DrawText(pane_title.c_str(), nes->main_font, fg_color, bg_color, 0, 0, k3fontAlignment::TOP_CENTER);
	for (i = nes->menu.list_start, row = 0; i < displayable_menu_items; i++, row++) {
		float* color = (nes->menu.select == i) ? sel_color : fg_color;
		nes->cmd_buf->DrawText(nes->menu.cur_list[i].item.c_str(), nes->main_font, color, bg_color, 0, (row + 1) * NESMENU_FONT_HEIGHT, k3fontAlignment::TOP_LEFT);
	}
	if (nes->menu.message_box != "") {
		uint32_t win_width = nes->win->GetWidth();
		uint32_t win_height = nes->win->GetHeight();
		k3rect rect;
		rect.width = 500;
		rect.height = 2 * NESMENU_FONT_HEIGHT;
		rect.x = (rect.width > win_width) ? 0 : (win_width - rect.width) / 2;
		rect.y = (rect.height > win_height) ? 0 : (win_height - rect.height) / 2;
		nessys_cbuffer_t* cb_data = static_cast<nessys_cbuffer_t*>(nes->cb_upload[nes->cb_main_cpu_version]->MapForWrite(sizeof(nessys_cbuffer_exp_t)));
		cb_data->palette[0] = 0.0f;
		cb_data->palette[1] = 0.0f;
		cb_data->palette[2] = 0.0f;
		cb_data->palette[3] = 0.75f;
		nes->cb_upload[nes->cb_main_cpu_version]->Unmap();

		nes->cmd_buf->SetGfxState(nes->st_blend_fill);
		nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::COPY_DEST);
		nes->cmd_buf->UploadBuffer(nes->cb_upload[nes->cb_main_cpu_version], nes->cb_main[nes->cb_main_gpu_version]->GetResource());
		nes->cmd_buf->TransitionResource(nes->cb_main[nes->cb_main_gpu_version]->GetResource(), k3resourceState::SHADER_BUFFER);
		nes->cmd_buf->SetConstantBuffer(0, nes->cb_main[nes->cb_main_gpu_version]);
		nes->cmd_buf->SetScissor(&rect);
		nes->cmd_buf->SetVertexBuffer(0, nes->vb_fullscreen);
		nes->cmd_buf->Draw(4);
		nes->cmd_buf->DrawText(nes->menu.message_box.c_str(), nes->main_font, fg_color, bg_color, 0, 0, k3fontAlignment::MID_CENTER);
		nes->cb_main_cpu_version++;
		nes->cb_main_gpu_version++;
		if (nes->cb_main_cpu_version >= nessys_t::NUM_CPU_VERSIONS) nes->cb_main_cpu_version = 0;
		if (nes->cb_main_gpu_version >= nessys_t::NUM_GPU_VERSIONS) nes->cb_main_gpu_version = 0;
	}
	nes->cmd_buf->TransitionResource(rt.render_targets[0]->GetResource(), k3resourceState::COMMON);
	nes->cmd_buf->Close();
	nes->gfx->SubmitCmdBuf(nes->cmd_buf);
	nes->menu.fence_val = nes->fence->SetGpuFence(k3gpuQueue::GRAPHICS);
	nes->win->SwapBuffer();
}

void nesmenu_load_options(nessys_t* nes)
{
	if (nes->menu.opt_file != "") {
		FILE* opt_fh = NULL;
		char buffer[256];
		fopen_s(&opt_fh, nes->menu.opt_file.c_str(), "r");
		if (opt_fh) {
			while (fgets(buffer, 256, opt_fh) != NULL) {
				if (!strncmp(buffer, "[CUR_DIR]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					size_t len = strlen(buffer);
					if (len > 1) buffer[len - 1] = '\0';
					nes->menu.cur_dir = buffer;
				} else if (!strncmp(buffer, "[VSYNC]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					uint32_t vsync = (buffer[0] == '0' || buffer[0] == '\n') ? 0 : 1;
					nes->win->SetVsyncInterval(vsync);
				} else if(!strncmp(buffer, "[UPSCALE_TYPE]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->menu.upscale_type = (buffer[0] == '0') ? 0 : 1;
				} else if (!strncmp(buffer, "[SPRITE_LINE_LIMIT]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->menu.sprite_line_limit = (buffer[0] == '0') ? 0 : ((buffer[0] == '1') ? 1 : 2);
				}
			}
			fclose(opt_fh);
		}
	}
}

void nesmenu_save_options(nessys_t* nes)
{
	if (nes->menu.opt_file != "") {
		FILE* opt_fh = NULL;
		fopen_s(&opt_fh, nes->menu.opt_file.c_str(), "w");
		if (opt_fh) {
			fprintf(opt_fh, "[CUR_DIR]\n%s\n\n", nes->menu.cur_dir.c_str());
			fprintf(opt_fh, "[VSYNC]\n%d\n\n", (nes->win->GetVsyncInterval() != 0) ? 1 : 0);
			fprintf(opt_fh, "# Upscale values: 0 - Copy, 1 - xBR\n");
			fprintf(opt_fh, "[UPSCALE_TYPE]\n%d\n\n", nes->menu.upscale_type);
			fprintf(opt_fh, "# Sprite line limit values: 0 - off, 1 - smart limit, 2 - on (true nes emulation)\n");
			fprintf(opt_fh, "[SPRITE_LINE_LIMIT]\n%d\n\n", nes->menu.sprite_line_limit);
			fclose(opt_fh);
		}
	}
}

void nesmenu_cleanup(nessys_t* nes)
{
	if (nes->menu.cur_list) {
		delete[] nes->menu.cur_list;
		nes->menu.cur_list = NULL;
	}
}

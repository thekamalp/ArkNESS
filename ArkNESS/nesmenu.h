// Project:     ArkNES
// File:        nessys.h
// Author:      Kamal Pillai
// Date:        7/12/2021
// Description:	NES menu functions

#pragma once

#include <vector>
#include <string>

struct nessys_t;

enum class nesmenu_pane_t {
	NONE,  // emulation is running
	MAIN,
	OPEN,
	OPTIONS
};

#ifdef _WIN32
const char NESMENU_DIR_SEPARATOR = '\\';
#else
const char NESMENU_DIR_SEPARATOR = '/';
#endif

const uint32_t NESMENU_FONT_HEIGHT = 40;

const uint32_t NESMENU_ITEM_FLAG_NONE = 0x0;
const uint32_t NESMENU_ITEM_FLAG_DIRECTORY = 0x1;
const uint32_t NESMENU_ITEM_FLAG_UP_DIR = 0x2;
const uint32_t NESMENU_ITEM_FLAG_FILE = 0x4;

struct nesmenu_list_item {
	std::string item;
	uint32_t flag;
};

struct nesmenu_data {
	nesmenu_pane_t pane;
	uint64_t fence_val;
	uint32_t select;
	uint32_t list_start;
	std::string cur_dir;
	std::string opt_file;
	nesmenu_list_item* cur_list;
	uint32_t cur_list_size;
	uint32_t cur_list_alloc_size;
	std::string message_box;
	uint8_t last_joypad_state[2];
	uint8_t sprite_line_limit;
};

const uint32_t nesmenu_main_items = 3;
const uint8_t nesmenu_main_item_open = 0;
const uint8_t nesmenu_main_item_options = 1;
const uint8_t nesmenu_main_item_exit = 2;
const char nesmenu_main[nesmenu_main_items][32] = { "Open", "Options", "Exit" };

const uint32_t nesmenu_options_items = 2;
const uint8_t nesmenu_options_item_sprite_line_limit = 0;
const uint8_t nesmenu_options_item_vsync = 1;
const char nesmenu_options[nesmenu_options_items][32] = { "Sprite line limit: ", "VSync: " };

void nesmenu_init(nessys_t* nes);
void nesmenu_update_list(nessys_t* nes);
void nesmenu_display(nessys_t* nes);
void nesmenu_load_options(nessys_t* nes);
void nesmenu_save_options(nessys_t* nes);
void nesmenu_cleanup(nessys_t* nes);

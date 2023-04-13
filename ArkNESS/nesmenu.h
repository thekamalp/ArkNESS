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
	OPTIONS,
	KEYCONFIG,
	JOYCONFIG,
	EXIT
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
	uint8_t last_num_joy;
	uint8_t last_joypad_state[2];
	uint8_t sprite_line_limit;
	uint8_t upscale_type;
	uint8_t key_sel;
	k3key last_key;
	k3key up_key;
	k3key down_key;
	k3key left_key;
	k3key right_key;
	k3key a_key;
	k3key b_key;
	k3key start_key;
	k3key select_key;
#ifdef _WIN32
	void* h_thread;
	//void* hwnd;
#endif
};

const uint32_t nesmenu_main_items = 3;
const uint8_t nesmenu_main_item_open = 0;
const uint8_t nesmenu_main_item_options = 1;
const uint8_t nesmenu_main_item_exit = 2;
const char nesmenu_main[nesmenu_main_items][32] = { "Open", "Options", "Exit" };

const uint32_t nesmenu_options_items = 5;
const uint8_t nesmenu_options_item_sprite_line_limit = 0;
const uint8_t nesmenu_options_item_upscale_type = 1;
const uint8_t nesmenu_options_item_vsync = 2;
const uint8_t nesmenu_options_item_key_config = 3;
const uint8_t nesmenu_options_item_joy_config = 4;
const char nesmenu_options[nesmenu_options_items][32] = { "Sprite line limit: ", "Upscale type: ", "VSync: ", "Keyboard config", "Joystick config" };

const uint32_t nesmenu_keyconfig_items = 8;
const uint8_t nesmenu_keyconfig_item_up = 0;
const uint8_t nesmenu_keyconfig_item_down = 1;
const uint8_t nesmenu_keyconfig_item_left = 2;
const uint8_t nesmenu_keyconfig_item_right = 3;
const uint8_t nesmenu_keyconfig_item_a = 4;
const uint8_t nesmenu_keyconfig_item_b = 5;
const uint8_t nesmenu_keyconfig_item_start = 6;
const uint8_t nesmenu_keyconfig_item_select = 7;
const char nesmenu_keyconfig[nesmenu_keyconfig_items][32] = { "Up: ", "Down: ", "Left: ", "Right: ", "A: ", "B: ", "Start: ", "Select: " };

const uint32_t nesmenu_joyconfig_items = 8;
const uint8_t nesmenu_joyconfig_item_joy0_a = 0;
const uint8_t nesmenu_joyconfig_item_joy0_b = 1;
const uint8_t nesmenu_joyconfig_item_joy0_start = 2;
const uint8_t nesmenu_joyconfig_item_joy0_select = 3;
const uint8_t nesmenu_joyconfig_item_joy1_a = 4;
const uint8_t nesmenu_joyconfig_item_joy1_b = 5;
const uint8_t nesmenu_joyconfig_item_joy1_start = 6;
const uint8_t nesmenu_joyconfig_item_joy1_select = 7;
const char nesmenu_joyconfig[nesmenu_joyconfig_items][32] = { "Joypad0 A: ", "Joypad0 B: ", "Joypad0 Start: ", "Joypad0 Select: ", "Joypad1 A: ", "Joypad1 B: ", "Joypad1 Start: ", "Joypad1 Select: " };

#ifdef _WIN32
void nesmenu_win32_init(nessys_t* nes);
void nesmenu_win32_cleanup(nessys_t* nes);
void nesmenu_win32_open(nessys_t* nes);
#endif

void nesmenu_init(nessys_t* nes);
void nesmenu_update_list(nessys_t* nes);
void nesmenu_display(nessys_t* nes);
void nesmenu_load_options(nessys_t* nes);
void nesmenu_save_options(nessys_t* nes);
void nesmenu_cleanup(nessys_t* nes);

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
	// default keys
	nes->menu.key_sel = 0xFF;
	nes->menu.last_key = k3key::NONE;
	nes->menu.up_key = k3key::UP;
	nes->menu.down_key = k3key::DOWN;
	nes->menu.left_key = k3key::LEFT;
	nes->menu.right_key = k3key::RIGHT;
	nes->menu.a_key = k3key::S;
	nes->menu.b_key = k3key::A;
	nes->menu.start_key = k3key::W;
	nes->menu.select_key = k3key::Q;
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

#define NESKEY_ENCODE(a,b,c,d) ( ((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

k3key code2key(uint32_t code)
{
	k3key k;

	switch (code) {
	case NESKEY_ENCODE('N', 'O', 'N', 'E'): k = k3key::NONE; break;
	case NESKEY_ENCODE('E', 's', 'c', '\0'): k = k3key::ESCAPE; break;
	case NESKEY_ENCODE('F', '1', '\0', '\0'): k = k3key::F1; break;
	case NESKEY_ENCODE('F', '2', '\0', '\0'): k = k3key::F2; break;
	case NESKEY_ENCODE('F', '3', '\0', '\0'): k = k3key::F3; break;
	case NESKEY_ENCODE('F', '4', '\0', '\0'): k = k3key::F4; break;
	case NESKEY_ENCODE('F', '5', '\0', '\0'): k = k3key::F5; break;
	case NESKEY_ENCODE('F', '6', '\0', '\0'): k = k3key::F6; break;
	case NESKEY_ENCODE('F', '7', '\0', '\0'): k = k3key::F7; break;
	case NESKEY_ENCODE('F', '8', '\0', '\0'): k = k3key::F8; break;
	case NESKEY_ENCODE('F', '9', '\0', '\0'): k = k3key::F9; break;
	case NESKEY_ENCODE('F', '1', '0', '\0'): k = k3key::F10; break;
	case NESKEY_ENCODE('F', '1', '1', '\0'): k = k3key::F11; break;
	case NESKEY_ENCODE('F', '1', '2', '\0'): k = k3key::F12; break;
	case NESKEY_ENCODE('L', 'S', 'h', 'f'): k = k3key::LSHIFT; break;
	case NESKEY_ENCODE('R', 'S', 'h', 'f'): k = k3key::RSHIFT; break;
	case NESKEY_ENCODE('L', 'C', 't', 'l'): k = k3key::LCONTROL; break;
	case NESKEY_ENCODE('R', 'C', 't', 'l'): k = k3key::RCONTROL; break;
	case NESKEY_ENCODE('L', 'A', 'l', 't'): k = k3key::LALT; break;
	case NESKEY_ENCODE('R', 'A', 'l', 't'): k = k3key::RALT; break;
	case NESKEY_ENCODE('L', 'M', 'e', 't'): k = k3key::LMETA; break;
	case NESKEY_ENCODE('R', 'M', 'e', 't'): k = k3key::RMETA; break;
	case NESKEY_ENCODE('L', 'W', 'i', 'n'): k = k3key::LWIN; break;
	case NESKEY_ENCODE('R', 'W', 'i', 'n'): k = k3key::RWIN; break;
	case NESKEY_ENCODE('M', 'e', 'n', 'u'): k = k3key::MENU; break;
	case NESKEY_ENCODE('B', 'k', 'S', 'p'): k = k3key::BACKSPACE; break;
	case NESKEY_ENCODE('T', 'a', 'b', '\0'): k = k3key::TAB; break;
	case NESKEY_ENCODE('C', 'p', 'L', 'k'): k = k3key::CAPS_LOCK; break;
	case NESKEY_ENCODE('E', 'n', 't', '\0'): k = k3key::ENTER; break;
	case NESKEY_ENCODE('S', 'p', 'c', '\0'): k = k3key::SPACE; break;
	case NESKEY_ENCODE('S', 'y', 'R', 'q'): k = k3key::SYS_REQ; break;
	case NESKEY_ENCODE('S', 'c', 'L', 'k'): k = k3key::SCROLL_LOCK; break;
	case NESKEY_ENCODE('P', 'a', 'u', 's'): k = k3key::PAUSE; break;
	case NESKEY_ENCODE('I', 'n', 's', '\0'): k = k3key::INSERT; break;
	case NESKEY_ENCODE('D', 'e', 'l', '\0'): k = k3key::DEL; break;
	case NESKEY_ENCODE('H', 'o', 'm', 'e'): k = k3key::HOME; break;
	case NESKEY_ENCODE('E', 'n', 'd', '\0'): k = k3key::END; break;
	case NESKEY_ENCODE('P', 'g', 'U', 'p'): k = k3key::PAGE_UP; break;
	case NESKEY_ENCODE('P', 'g', 'D', 'n'): k = k3key::PAGE_DOWN; break;
	case NESKEY_ENCODE('U', 'p', '\0', '\0'): k = k3key::UP; break;
	case NESKEY_ENCODE('D', 'o', 'w', 'n'): k = k3key::DOWN; break;
	case NESKEY_ENCODE('L', 'e', 'f', 't'): k = k3key::LEFT; break;
	case NESKEY_ENCODE('R', 'g', 'h', 't'): k = k3key::RIGHT; break;
	case NESKEY_ENCODE('N', 'm', 'L', 'k'): k = k3key::NUM_LOCK; break;
	case NESKEY_ENCODE('N', 'm', '0', '\0'): k = k3key::NUM_0; break;
	case NESKEY_ENCODE('N', 'm', '1', '\0'): k = k3key::NUM_1; break;
	case NESKEY_ENCODE('N', 'm', '2', '\0'): k = k3key::NUM_2; break;
	case NESKEY_ENCODE('N', 'm', '3', '\0'): k = k3key::NUM_3; break;
	case NESKEY_ENCODE('N', 'm', '4', '\0'): k = k3key::NUM_4; break;
	case NESKEY_ENCODE('N', 'm', '5', '\0'): k = k3key::NUM_5; break;
	case NESKEY_ENCODE('N', 'm', '6', '\0'): k = k3key::NUM_6; break;
	case NESKEY_ENCODE('N', 'm', '7', '\0'): k = k3key::NUM_7; break;
	case NESKEY_ENCODE('N', 'm', '8', '\0'): k = k3key::NUM_8; break;
	case NESKEY_ENCODE('N', 'm', '9', '\0'): k = k3key::NUM_9; break;
	case NESKEY_ENCODE('N', 'm', '.', '\0'): k = k3key::NUM_DECIMAL; break;
	case NESKEY_ENCODE('N', 'm', '+', '\0'): k = k3key::NUM_PLUS; break;
	case NESKEY_ENCODE('N', 'm', '-', '\0'): k = k3key::NUM_MINUS; break;
	case NESKEY_ENCODE('N', 'm', '*', '\0'): k = k3key::NUM_TIMES; break;
	case NESKEY_ENCODE('N', 'm', '/', '\0'): k = k3key::NUM_DIVIDE; break;
	case NESKEY_ENCODE('N', 'm', 'E', 'n'): k = k3key::NUM_ENTER; break;
	case NESKEY_ENCODE('-', '\0', '\0', '\0'): k = k3key::MINUS; break;
	case NESKEY_ENCODE('+', '\0', '\0', '\0'): k = k3key::PLUS; break;
	case NESKEY_ENCODE('[', '\0', '\0', '\0'): k = k3key::LBRACKET; break;
	case NESKEY_ENCODE(']', '\0', '\0', '\0'): k = k3key::RBRACKET; break;
	case NESKEY_ENCODE('\\', '\0', '\0', '\0'): k = k3key::BACKSLASH; break;
	case NESKEY_ENCODE(';', '\0', '\0', '\0'): k = k3key::SEMICOLON; break;
	case NESKEY_ENCODE('\'', '\0', '\0', '\0'): k = k3key::TICK; break;
	case NESKEY_ENCODE('`', '\0', '\0', '\0'): k = k3key::BACKTICK; break;
	case NESKEY_ENCODE('A', '\0', '\0', '\0'): k = k3key::A; break;
	case NESKEY_ENCODE('B', '\0', '\0', '\0'): k = k3key::B; break;
	case NESKEY_ENCODE('C', '\0', '\0', '\0'): k = k3key::C; break;
	case NESKEY_ENCODE('D', '\0', '\0', '\0'): k = k3key::D; break;
	case NESKEY_ENCODE('E', '\0', '\0', '\0'): k = k3key::E; break;
	case NESKEY_ENCODE('F', '\0', '\0', '\0'): k = k3key::F; break;
	case NESKEY_ENCODE('G', '\0', '\0', '\0'): k = k3key::G; break;
	case NESKEY_ENCODE('H', '\0', '\0', '\0'): k = k3key::H; break;
	case NESKEY_ENCODE('I', '\0', '\0', '\0'): k = k3key::I; break;
	case NESKEY_ENCODE('J', '\0', '\0', '\0'): k = k3key::J; break;
	case NESKEY_ENCODE('K', '\0', '\0', '\0'): k = k3key::K; break;
	case NESKEY_ENCODE('L', '\0', '\0', '\0'): k = k3key::L; break;
	case NESKEY_ENCODE('M', '\0', '\0', '\0'): k = k3key::M; break;
	case NESKEY_ENCODE('N', '\0', '\0', '\0'): k = k3key::N; break;
	case NESKEY_ENCODE('O', '\0', '\0', '\0'): k = k3key::O; break;
	case NESKEY_ENCODE('P', '\0', '\0', '\0'): k = k3key::P; break;
	case NESKEY_ENCODE('Q', '\0', '\0', '\0'): k = k3key::Q; break;
	case NESKEY_ENCODE('R', '\0', '\0', '\0'): k = k3key::R; break;
	case NESKEY_ENCODE('S', '\0', '\0', '\0'): k = k3key::S; break;
	case NESKEY_ENCODE('T', '\0', '\0', '\0'): k = k3key::T; break;
	case NESKEY_ENCODE('U', '\0', '\0', '\0'): k = k3key::U; break;
	case NESKEY_ENCODE('V', '\0', '\0', '\0'): k = k3key::V; break;
	case NESKEY_ENCODE('W', '\0', '\0', '\0'): k = k3key::W; break;
	case NESKEY_ENCODE('X', '\0', '\0', '\0'): k = k3key::X; break;
	case NESKEY_ENCODE('Y', '\0', '\0', '\0'): k = k3key::Y; break;
	case NESKEY_ENCODE('Z', '\0', '\0', '\0'): k = k3key::Z; break;
	case NESKEY_ENCODE('0', '\0', '\0', '\0'): k = k3key::KEY_0; break;
	case NESKEY_ENCODE('1', '\0', '\0', '\0'): k = k3key::KEY_1; break;
	case NESKEY_ENCODE('2', '\0', '\0', '\0'): k = k3key::KEY_2; break;
	case NESKEY_ENCODE('3', '\0', '\0', '\0'): k = k3key::KEY_3; break;
	case NESKEY_ENCODE('4', '\0', '\0', '\0'): k = k3key::KEY_4; break;
	case NESKEY_ENCODE('5', '\0', '\0', '\0'): k = k3key::KEY_5; break;
	case NESKEY_ENCODE('6', '\0', '\0', '\0'): k = k3key::KEY_6; break;
	case NESKEY_ENCODE('7', '\0', '\0', '\0'): k = k3key::KEY_7; break;
	case NESKEY_ENCODE('8', '\0', '\0', '\0'): k = k3key::KEY_8; break;
	case NESKEY_ENCODE('9', '\0', '\0', '\0'): k = k3key::KEY_9; break;
	default: k = k3key::NONE; break;
	}
	return k;
}

uint32_t key2code(k3key k)
{
	uint32_t code;

	switch (k) {
	case k3key::NONE: code = NESKEY_ENCODE('N', 'O', 'N', 'E'); break;
	case k3key::ESCAPE: code = NESKEY_ENCODE('E', 's', 'c', '\0'); break;
	case k3key::F1:code = NESKEY_ENCODE('F', '1', '\0', '\0'); break;
	case k3key::F2:code = NESKEY_ENCODE('F', '2', '\0', '\0'); break;
	case k3key::F3:code = NESKEY_ENCODE('F', '3', '\0', '\0'); break;
	case k3key::F4:code = NESKEY_ENCODE('F', '4', '\0', '\0'); break;
	case k3key::F5:code = NESKEY_ENCODE('F', '5', '\0', '\0'); break;
	case k3key::F6:code = NESKEY_ENCODE('F', '6', '\0', '\0'); break;
	case k3key::F7:code = NESKEY_ENCODE('F', '7', '\0', '\0'); break;
	case k3key::F8:code = NESKEY_ENCODE('F', '8', '\0', '\0'); break;
	case k3key::F9:code = NESKEY_ENCODE('F', '9', '\0', '\0'); break;
	case k3key::F10: code = NESKEY_ENCODE('F', '1', '0', '\0'); break;
	case k3key::F11: code = NESKEY_ENCODE('F', '1', '1', '\0'); break;
	case k3key::F12: code = NESKEY_ENCODE('F', '1', '2', '\0'); break;
	case k3key::LSHIFT: code = NESKEY_ENCODE('L', 'S', 'h', 'f'); break;
	case k3key::RSHIFT: code = NESKEY_ENCODE('R', 'S', 'h', 'f'); break;
	case k3key::LCONTROL: code = NESKEY_ENCODE('L', 'C', 't', 'l'); break;
	case k3key::RCONTROL: code = NESKEY_ENCODE('R', 'C', 't', 'l'); break;
	case k3key::LALT: code = NESKEY_ENCODE('L', 'A', 'l', 't'); break;
	case k3key::RALT: code = NESKEY_ENCODE('R', 'A', 'l', 't'); break;
	case k3key::LMETA: code = NESKEY_ENCODE('L', 'M', 'e', 't'); break;
	case k3key::RMETA: code = NESKEY_ENCODE('R', 'M', 'e', 't'); break;
	case k3key::LWIN: code = NESKEY_ENCODE('L', 'W', 'i', 'n'); break;
	case k3key::RWIN: code = NESKEY_ENCODE('R', 'W', 'i', 'n'); break;
	case k3key::MENU: code = NESKEY_ENCODE('M', 'e', 'n', 'u'); break;
	case k3key::BACKSPACE: code = NESKEY_ENCODE('B', 'k', 'S', 'p'); break;
	case k3key::TAB: code = NESKEY_ENCODE('T', 'a', 'b', '\0'); break;
	case k3key::CAPS_LOCK: code = NESKEY_ENCODE('C', 'p', 'L', 'k'); break;
	case k3key::ENTER: code = NESKEY_ENCODE('E', 'n', 't', '\0'); break;
	case k3key::SPACE: code = NESKEY_ENCODE('S', 'p', 'c', '\0'); break;
	case k3key::SYS_REQ: code = NESKEY_ENCODE('S', 'y', 'R', 'q'); break;
	case k3key::SCROLL_LOCK: code = NESKEY_ENCODE('S', 'c', 'L', 'k'); break;
	case k3key::PAUSE: code = NESKEY_ENCODE('P', 'a', 'u', 's'); break;
	case k3key::INSERT: code = NESKEY_ENCODE('I', 'n', 's', '\0'); break;
	case k3key::DEL: code = NESKEY_ENCODE('D', 'e', 'l', '\0'); break;
	case k3key::HOME: code = NESKEY_ENCODE('H', 'o', 'm', 'e'); break;
	case k3key::END: code = NESKEY_ENCODE('E', 'n', 'd', '\0'); break;
	case k3key::PAGE_UP: code = NESKEY_ENCODE('P', 'g', 'U', 'p'); break;
	case k3key::PAGE_DOWN: code = NESKEY_ENCODE('P', 'g', 'D', 'n'); break;
	case k3key::UP: code = NESKEY_ENCODE('U', 'p', '\0', '\0'); break;
	case k3key::DOWN: code = NESKEY_ENCODE('D', 'o', 'w', 'n'); break;
	case k3key::LEFT: code = NESKEY_ENCODE('L', 'e', 'f', 't'); break;
	case k3key::RIGHT: code = NESKEY_ENCODE('R', 'g', 'h', 't'); break;
	case k3key::NUM_LOCK: code = NESKEY_ENCODE('N', 'm', 'L', 'k'); break;
	case k3key::NUM_0: code = NESKEY_ENCODE('N', 'm', '0', '\0'); break;
	case k3key::NUM_1: code = NESKEY_ENCODE('N', 'm', '1', '\0'); break;
	case k3key::NUM_2: code = NESKEY_ENCODE('N', 'm', '2', '\0'); break;
	case k3key::NUM_3: code = NESKEY_ENCODE('N', 'm', '3', '\0'); break;
	case k3key::NUM_4: code = NESKEY_ENCODE('N', 'm', '4', '\0'); break;
	case k3key::NUM_5: code = NESKEY_ENCODE('N', 'm', '5', '\0'); break;
	case k3key::NUM_6: code = NESKEY_ENCODE('N', 'm', '6', '\0'); break;
	case k3key::NUM_7: code = NESKEY_ENCODE('N', 'm', '7', '\0'); break;
	case k3key::NUM_8: code = NESKEY_ENCODE('N', 'm', '8', '\0'); break;
	case k3key::NUM_9: code = NESKEY_ENCODE('N', 'm', '9', '\0'); break;
	case k3key::NUM_DECIMAL: code = NESKEY_ENCODE('N', 'm', '.', '\0'); break;
	case k3key::NUM_PLUS: code = NESKEY_ENCODE('N', 'm', '+', '\0'); break;
	case k3key::NUM_MINUS: code = NESKEY_ENCODE('N', 'm', '-', '\0'); break;
	case k3key::NUM_TIMES: code = NESKEY_ENCODE('N', 'm', '*', '\0'); break;
	case k3key::NUM_DIVIDE: code = NESKEY_ENCODE('N', 'm', '/', '\0'); break;
	case k3key::NUM_ENTER: code = NESKEY_ENCODE('N', 'm', 'E', 'n'); break;
	case k3key::MINUS: code = NESKEY_ENCODE('-', '\0', '\0', '\0'); break;
	case k3key::PLUS: code = NESKEY_ENCODE('+', '\0', '\0', '\0'); break;
	case k3key::LBRACKET: code = NESKEY_ENCODE('[', '\0', '\0', '\0'); break;
	case k3key::RBRACKET: code = NESKEY_ENCODE(']', '\0', '\0', '\0'); break;
	case k3key::BACKSLASH: code = NESKEY_ENCODE('\\', '\0', '\0', '\0'); break;
	case k3key::SEMICOLON: code = NESKEY_ENCODE(';', '\0', '\0', '\0'); break;
	case k3key::TICK: code = NESKEY_ENCODE('\'', '\0', '\0', '\0'); break;
	case k3key::BACKTICK: code = NESKEY_ENCODE('`', '\0', '\0', '\0'); break;
	case k3key::A: code = NESKEY_ENCODE('A', '\0', '\0', '\0'); break;
	case k3key::B: code = NESKEY_ENCODE('B', '\0', '\0', '\0'); break;
	case k3key::C: code = NESKEY_ENCODE('C', '\0', '\0', '\0'); break;
	case k3key::D: code = NESKEY_ENCODE('D', '\0', '\0', '\0'); break;
	case k3key::E: code = NESKEY_ENCODE('E', '\0', '\0', '\0'); break;
	case k3key::F: code = NESKEY_ENCODE('F', '\0', '\0', '\0'); break;
	case k3key::G: code = NESKEY_ENCODE('G', '\0', '\0', '\0'); break;
	case k3key::H: code = NESKEY_ENCODE('H', '\0', '\0', '\0'); break;
	case k3key::I: code = NESKEY_ENCODE('I', '\0', '\0', '\0'); break;
	case k3key::J: code = NESKEY_ENCODE('J', '\0', '\0', '\0'); break;
	case k3key::K: code = NESKEY_ENCODE('K', '\0', '\0', '\0'); break;
	case k3key::L: code = NESKEY_ENCODE('L', '\0', '\0', '\0'); break;
	case k3key::M: code = NESKEY_ENCODE('M', '\0', '\0', '\0'); break;
	case k3key::N: code = NESKEY_ENCODE('N', '\0', '\0', '\0'); break;
	case k3key::O: code = NESKEY_ENCODE('O', '\0', '\0', '\0'); break;
	case k3key::P: code = NESKEY_ENCODE('P', '\0', '\0', '\0'); break;
	case k3key::Q: code = NESKEY_ENCODE('Q', '\0', '\0', '\0'); break;
	case k3key::R: code = NESKEY_ENCODE('R', '\0', '\0', '\0'); break;
	case k3key::S: code = NESKEY_ENCODE('S', '\0', '\0', '\0'); break;
	case k3key::T: code = NESKEY_ENCODE('T', '\0', '\0', '\0'); break;
	case k3key::U: code = NESKEY_ENCODE('U', '\0', '\0', '\0'); break;
	case k3key::V: code = NESKEY_ENCODE('V', '\0', '\0', '\0'); break;
	case k3key::W: code = NESKEY_ENCODE('W', '\0', '\0', '\0'); break;
	case k3key::X: code = NESKEY_ENCODE('X', '\0', '\0', '\0'); break;
	case k3key::Y: code = NESKEY_ENCODE('Y', '\0', '\0', '\0'); break;
	case k3key::Z: code = NESKEY_ENCODE('Z', '\0', '\0', '\0'); break;
	case k3key::KEY_0: code = NESKEY_ENCODE('0', '\0', '\0', '\0'); break;
	case k3key::KEY_1: code = NESKEY_ENCODE('1', '\0', '\0', '\0'); break;
	case k3key::KEY_2: code = NESKEY_ENCODE('2', '\0', '\0', '\0'); break;
	case k3key::KEY_3: code = NESKEY_ENCODE('3', '\0', '\0', '\0'); break;
	case k3key::KEY_4: code = NESKEY_ENCODE('4', '\0', '\0', '\0'); break;
	case k3key::KEY_5: code = NESKEY_ENCODE('5', '\0', '\0', '\0'); break;
	case k3key::KEY_6: code = NESKEY_ENCODE('6', '\0', '\0', '\0'); break;
	case k3key::KEY_7: code = NESKEY_ENCODE('7', '\0', '\0', '\0'); break;
	case k3key::KEY_8: code = NESKEY_ENCODE('8', '\0', '\0', '\0'); break;
	case k3key::KEY_9: code = NESKEY_ENCODE('9', '\0', '\0', '\0'); break;
	default: code = NESKEY_ENCODE('N', 'U', 'L', 'L'); break;
	}
	return code;
}

void nesmenu_update_list(nessys_t* nes)
{
	uint32_t index = 0;
	std::string item;
	nes->menu.select = 0;
	char key_str[5];
	key_str[4] = '\0';
	uint32_t* key_code = (uint32_t*)key_str;
	const char* select_msg = "Press a key";
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
	case nesmenu_pane_t::KEYCONFIG:
		nes->menu.cur_list_size = nesmenu_keyconfig_items;
		nesmenu_resize_list(&(nes->menu));
		for (index = 0; index < nesmenu_keyconfig_items; index++) {
			nes->menu.cur_list[index].item = nesmenu_keyconfig[index];
			nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_NONE;
		}
		*key_code = key2code(nes->menu.up_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_up].item += (nes->menu.key_sel == nesmenu_keyconfig_item_up) ? select_msg : key_str;
		*key_code = key2code(nes->menu.down_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_down].item += (nes->menu.key_sel == nesmenu_keyconfig_item_down) ? select_msg : key_str;
		*key_code = key2code(nes->menu.left_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_left].item += (nes->menu.key_sel == nesmenu_keyconfig_item_left) ? select_msg : key_str;
		*key_code = key2code(nes->menu.right_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_right].item += (nes->menu.key_sel == nesmenu_keyconfig_item_right) ? select_msg : key_str;
		*key_code = key2code(nes->menu.a_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_a].item += (nes->menu.key_sel == nesmenu_keyconfig_item_a) ? select_msg : key_str;
		*key_code = key2code(nes->menu.b_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_b].item += (nes->menu.key_sel == nesmenu_keyconfig_item_b) ? select_msg : key_str;
		*key_code = key2code(nes->menu.start_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_start].item += (nes->menu.key_sel == nesmenu_keyconfig_item_start) ? select_msg : key_str;
		*key_code = key2code(nes->menu.select_key);
		nes->menu.cur_list[nesmenu_keyconfig_item_select].item += (nes->menu.key_sel == nesmenu_keyconfig_item_select) ? select_msg : key_str;
		break;
	case nesmenu_pane_t::JOYCONFIG:
		nes->menu.cur_list_size = nesmenu_joyconfig_items;
		nesmenu_resize_list(&(nes->menu));
		for (index = 0; index < nesmenu_joyconfig_items; index++) {
			nes->menu.cur_list[index].item = nesmenu_joyconfig[index];
			nes->menu.cur_list[index].flag = NESMENU_ITEM_FLAG_NONE;
		}
		if (nes->num_joy > 0) {
			snprintf(key_str, 5, "%d", nes->joy_data[0].button_a + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy0_a].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy0_a) ? select_msg : key_str;
			snprintf(key_str, 5, "%d", nes->joy_data[0].button_b + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy0_b].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy0_b) ? select_msg : key_str;
			snprintf(key_str, 5, "%d", nes->joy_data[0].button_start + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy0_start].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy0_start) ? select_msg : key_str;
			snprintf(key_str, 5, "%d", nes->joy_data[0].button_select + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy0_select].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy0_select) ? select_msg : key_str;
		}
		if (nes->num_joy > 1) {
			snprintf(key_str, 5, "%d", nes->joy_data[1].button_a + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy1_a].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy1_a) ? select_msg : key_str;
			snprintf(key_str, 5, "%d", nes->joy_data[1].button_b + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy1_b].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy1_b) ? select_msg : key_str;
			snprintf(key_str, 5, "%d", nes->joy_data[1].button_start + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy1_start].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy1_start) ? select_msg : key_str;
			snprintf(key_str, 5, "%d", nes->joy_data[1].button_select + 1);
			nes->menu.cur_list[nesmenu_joyconfig_item_joy1_select].item += (nes->menu.key_sel == nesmenu_joyconfig_item_joy1_select) ? select_msg : key_str;
		}
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

	bool joy_dirty = (nes->num_joy != nes->menu.last_num_joy);
	nes->menu.last_num_joy = nes->num_joy;

	if (joy_dirty && nes->menu.pane == nesmenu_pane_t::JOYCONFIG) {
		uint8_t select = nes->menu.select;
		nes->menu.key_sel = 0xFF;
		nesmenu_update_list(nes);
		nes->menu.select = select;
	}

	uint8_t j = (nes->menu.last_joypad_state[1] != nes->apu.joypad[1]) ? 1 : 0;

	if (nes->menu.pane == nesmenu_pane_t::KEYCONFIG && nes->menu.key_sel < 8) {
		if (nes->menu.last_key != k3key::NONE) {
			if (nes->menu.last_key != k3key::ESCAPE) {
				switch (nes->menu.key_sel) {
				case nesmenu_keyconfig_item_up: nes->menu.up_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_down: nes->menu.down_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_left: nes->menu.left_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_right: nes->menu.right_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_a: nes->menu.a_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_b: nes->menu.b_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_start: nes->menu.start_key = nes->menu.last_key; break;
				case nesmenu_keyconfig_item_select: nes->menu.select_key = nes->menu.last_key; break;
				}
			}
			nes->menu.key_sel = 0xFF;
			if (nes->menu.select != nesmenu_keyconfig_item_up && nes->menu.up_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_up;
			else if (nes->menu.select != nesmenu_keyconfig_item_down && nes->menu.down_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_down;
			else if (nes->menu.select != nesmenu_keyconfig_item_left && nes->menu.left_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_left;
			else if (nes->menu.select != nesmenu_keyconfig_item_right && nes->menu.right_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_right;
			else if (nes->menu.select != nesmenu_keyconfig_item_a && nes->menu.a_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_a;
			else if (nes->menu.select != nesmenu_keyconfig_item_b && nes->menu.b_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_b;
			else if (nes->menu.select != nesmenu_keyconfig_item_start && nes->menu.start_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_start;
			else if (nes->menu.select != nesmenu_keyconfig_item_select && nes->menu.select_key == nes->menu.last_key) nes->menu.key_sel = nesmenu_keyconfig_item_select;
			uint8_t select = (nes->menu.key_sel != 0xFF) ? nes->menu.key_sel : nes->menu.select;
			nesmenu_update_list(nes);
			nes->menu.select = select;
			nes->menu.last_key = k3key::NONE;
		}
		nes->menu.last_joypad_state[j] = nes->apu.joypad[j];
	} else if(nes->menu.pane == nesmenu_pane_t::JOYCONFIG && nes->menu.key_sel < 8) {
		if (nes->joy_data[nes->menu.key_sel / 4].last_button < 0xFFFFFFFF) {
			switch (nes->menu.key_sel) {
			case nesmenu_joyconfig_item_joy0_a: nes->joy_data[0].button_a = nes->joy_data[0].last_button; break;
			case nesmenu_joyconfig_item_joy0_b: nes->joy_data[0].button_b = nes->joy_data[0].last_button; break;
			case nesmenu_joyconfig_item_joy0_start: nes->joy_data[0].button_start = nes->joy_data[0].last_button; break;
			case nesmenu_joyconfig_item_joy0_select: nes->joy_data[0].button_select = nes->joy_data[0].last_button; break;
			case nesmenu_joyconfig_item_joy1_a: nes->joy_data[1].button_a = nes->joy_data[1].last_button; break;
			case nesmenu_joyconfig_item_joy1_b: nes->joy_data[1].button_b = nes->joy_data[1].last_button; break;
			case nesmenu_joyconfig_item_joy1_start: nes->joy_data[1].button_start = nes->joy_data[1].last_button; break;
			case nesmenu_joyconfig_item_joy1_select: nes->joy_data[1].button_select = nes->joy_data[1].last_button; break;
			}
			nes->menu.key_sel = 0xFF;
			if (nes->menu.select / 4 == 0) {
				if (nes->menu.select != nesmenu_joyconfig_item_joy0_a && nes->joy_data[0].button_a == nes->joy_data[0].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy0_a;
				else if (nes->menu.select != nesmenu_joyconfig_item_joy0_b && nes->joy_data[0].button_b == nes->joy_data[0].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy0_b;
				else if (nes->menu.select != nesmenu_joyconfig_item_joy0_start && nes->joy_data[0].button_start == nes->joy_data[0].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy0_start;
				else if (nes->menu.select != nesmenu_joyconfig_item_joy0_select && nes->joy_data[0].button_select == nes->joy_data[0].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy0_select;
			}
			if (nes->menu.select / 4 == 1) {
				if (nes->menu.select != nesmenu_joyconfig_item_joy1_a && nes->joy_data[1].button_a == nes->joy_data[1].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy1_a;
				else if (nes->menu.select != nesmenu_joyconfig_item_joy1_b && nes->joy_data[1].button_b == nes->joy_data[1].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy1_b;
				else if (nes->menu.select != nesmenu_joyconfig_item_joy1_start && nes->joy_data[1].button_start == nes->joy_data[1].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy1_start;
				else if (nes->menu.select != nesmenu_joyconfig_item_joy1_select && nes->joy_data[1].button_select == nes->joy_data[1].last_button) nes->menu.key_sel = nesmenu_joyconfig_item_joy1_select;
			}
			uint8_t select = (nes->menu.key_sel != 0xFF) ? nes->menu.key_sel : nes->menu.select;
			nesmenu_update_list(nes);
			nes->menu.select = select;
			nes->joy_data[select / 4].last_button = 0xFFFFFFFF;
		}
		nes->menu.last_joypad_state[j] = nes->apu.joypad[j];
	} else {
		if (nes->menu.last_joypad_state[j] != nes->apu.joypad[j]) {
			if (nes->menu.message_box != "") {
				if (nes->apu.joypad[j]) {
					nes->menu.message_box = "";
				}
			} else {
				if (nes->apu.joypad[j] & NESSYS_STD_CONTROLLER_BUTTON_DOWN_MASK) {
					nes->menu.select++;
					if (nes->menu.select >= cur_menu_items) nes->menu.select = 0;
				}
				if (nes->apu.joypad[j] & NESSYS_STD_CONTROLLER_BUTTON_UP_MASK) {
					if (nes->menu.select == 0) nes->menu.select = cur_menu_items - 1;
					else nes->menu.select--;
				}
				if (nes->apu.joypad[j] & NESSYS_STD_CONTROLLER_BUTTON_RIGHT_MASK) {
					if (nes->menu.select == cur_menu_items - 1) nes->menu.select = 0;
					else {
						nes->menu.select += displayable_menu_items;
						if (nes->menu.select >= cur_menu_items) nes->menu.select = cur_menu_items - 1;
					}
				}
				if (nes->apu.joypad[j] & NESSYS_STD_CONTROLLER_BUTTON_LEFT_MASK) {
					if (nes->menu.select == 0) nes->menu.select = cur_menu_items - 1;
					else if (nes->menu.select < displayable_menu_items) nes->menu.select = 0;
					else nes->menu.select -= displayable_menu_items;
				}
				if (nes->apu.joypad[j] & (NESSYS_STD_CONTROLLER_BUTTON_A_MASK | NESSYS_STD_CONTROLLER_BUTTON_START_MASK)) {
					switch (nes->menu.pane) {
					case nesmenu_pane_t::MAIN:
						switch (nes->menu.select) {
						case nesmenu_main_item_open:
#ifdef WIN32
							nesmenu_win32_open(nes);
							nes->apu.joypad[j] = 0;
							nesmenu_update_list(nes);
#else
							nes->menu.pane = nesmenu_pane_t::OPEN;
							nesmenu_update_list(nes);
#endif
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
						case nesmenu_options_item_key_config:
							nes->menu.pane = nesmenu_pane_t::KEYCONFIG;
							nesmenu_update_list(nes);
							break;
						case nesmenu_options_item_joy_config:
							nes->menu.pane = nesmenu_pane_t::JOYCONFIG;
							nesmenu_update_list(nes);
							break;
						}
						break;
					case nesmenu_pane_t::KEYCONFIG:
						nes->menu.last_key = k3key::NONE;
						nes->menu.key_sel = nes->menu.select;
						nesmenu_update_list(nes);
						nes->menu.select = nes->menu.key_sel;
						break;
					case nesmenu_pane_t::JOYCONFIG:
						if (nes->num_joy > nes->menu.select / 4) {
							nes->joy_data[nes->menu.select / 4].last_button = 0xFFFFFFFF;
							nes->menu.key_sel = nes->menu.select;
							nesmenu_update_list(nes);
							nes->menu.select = nes->menu.key_sel;
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
				if (nes->apu.joypad[j] & NESSYS_STD_CONTROLLER_BUTTON_B_MASK) {
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
					case nesmenu_pane_t::KEYCONFIG:
						nes->menu.pane = nesmenu_pane_t::OPTIONS;
						nesmenu_update_list(nes);
						break;
					case nesmenu_pane_t::JOYCONFIG:
						nes->menu.pane = nesmenu_pane_t::OPTIONS;
						nesmenu_update_list(nes);
						break;
					}
				}

			}
			nes->menu.last_joypad_state[j] = nes->apu.joypad[j];
		}
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
	case nesmenu_pane_t::KEYCONFIG:
		pane_title = "Keyboard Configuration";
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

void nesmenu_clenup_code(char* buffer)
{
	uint32_t i;
	bool clear_to_0 = false;
	for (i = 0; i < 4; i++) {
		if (buffer[i] == '\n' || buffer[i] == '\0' || clear_to_0) {
			buffer[i] = '\0';
			clear_to_0 = true;
		}
	}
	buffer[4] = '\0';
}

void nesmenu_load_options(nessys_t* nes)
{
	if (nes->menu.opt_file != "") {
		FILE* opt_fh = NULL;
		char buffer[256];
		uint32_t* code = (uint32_t*)buffer;
		k3key k;
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
				} else if (!strncmp(buffer, "[UP_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if(k != k3key::NONE) nes->menu.up_key = k;
				} else if (!strncmp(buffer, "[DOWN_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.down_key = k;
				} else if (!strncmp(buffer, "[LEFT_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.left_key = k;
				} else if (!strncmp(buffer, "[RIGHT_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.right_key = k;
				} else if (!strncmp(buffer, "[A_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.a_key = k;
				} else if (!strncmp(buffer, "[B_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.b_key = k;
				} else if (!strncmp(buffer, "[START_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.start_key = k;
				} else if (!strncmp(buffer, "[SELECT_KEY]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nesmenu_clenup_code(buffer);
					k = code2key(*code);
					if (k != k3key::NONE) nes->menu.select_key = k;
				} else if (!strncmp(buffer, "[JOY0_BUTTON_A]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[0].button_a = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY0_BUTTON_B]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[0].button_b = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY0_BUTTON_START]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[0].button_start = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY0_BUTTON_SELECT]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[0].button_select = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY1_BUTTON_A]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[1].button_a = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY1_BUTTON_B]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[1].button_b = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY1_BUTTON_START]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[1].button_start = atoi(buffer) - 1;
				} else if (!strncmp(buffer, "[JOY1_BUTTON_SELECT]\n", 256)) {
					fgets(buffer, 256, opt_fh);
					nes->joy_data[1].button_select = atoi(buffer) - 1;
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
			char key_str[5];
			key_str[4] = '\0';
			uint32_t* key_code = (uint32_t*)key_str;

			fprintf(opt_fh, "[CUR_DIR]\n%s\n\n", nes->menu.cur_dir.c_str());
			fprintf(opt_fh, "[VSYNC]\n%d\n\n", (nes->win->GetVsyncInterval() != 0) ? 1 : 0);
			fprintf(opt_fh, "# Upscale values: 0 - Copy, 1 - xBR\n");
			fprintf(opt_fh, "[UPSCALE_TYPE]\n%d\n\n", nes->menu.upscale_type);
			fprintf(opt_fh, "# Sprite line limit values: 0 - off, 1 - smart limit, 2 - on (true nes emulation)\n");
			fprintf(opt_fh, "[SPRITE_LINE_LIMIT]\n%d\n\n", nes->menu.sprite_line_limit);
			*key_code = key2code(nes->menu.up_key);
			fprintf(opt_fh, "[UP_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.down_key);
			fprintf(opt_fh, "[DOWN_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.left_key);
			fprintf(opt_fh, "[LEFT_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.right_key);
			fprintf(opt_fh, "[RIGHT_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.a_key);
			fprintf(opt_fh, "[A_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.b_key);
			fprintf(opt_fh, "[B_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.start_key);
			fprintf(opt_fh, "[START_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			*key_code = key2code(nes->menu.select_key);
			fprintf(opt_fh, "[SELECT_KEY]\n%c%c%c%c\n\n", key_str[0], key_str[1], key_str[2], key_str[3]);
			fprintf(opt_fh, "[JOY0_BUTTON_A]\n%d\n\n", nes->joy_data[0].button_a + 1);
			fprintf(opt_fh, "[JOY0_BUTTON_B]\n%d\n\n", nes->joy_data[0].button_b + 1);
			fprintf(opt_fh, "[JOY0_BUTTON_START]\n%d\n\n", nes->joy_data[0].button_start + 1);
			fprintf(opt_fh, "[JOY0_BUTTON_SELECT]\n%d\n\n", nes->joy_data[0].button_select + 1);
			fprintf(opt_fh, "[JOY1_BUTTON_A]\n%d\n\n", nes->joy_data[1].button_a + 1);
			fprintf(opt_fh, "[JOY1_BUTTON_B]\n%d\n\n", nes->joy_data[1].button_b + 1);
			fprintf(opt_fh, "[JOY1_BUTTON_START]\n%d\n\n", nes->joy_data[1].button_start + 1);
			fprintf(opt_fh, "[JOY1_BUTTON_SELECT]\n%d\n\n", nes->joy_data[1].button_select + 1);
			fclose(opt_fh);
		}
	}
}

void nesmenu_cleanup(nessys_t* nes)
{
	if (nes != NULL && nes->menu.cur_list != NULL) {
		delete[] nes->menu.cur_list;
		nes->menu.cur_list = NULL;
	}
}

#ifdef WIN32
#include <Windows.h>
#include <ShlObj.h>
void nesmenu_win32_open(nessys_t* nes)
{
	// CoCreate the File Open Dialog object.
	IFileDialog* pfd = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pfd));
	if (SUCCEEDED(hr)) {
		DWORD dwFlags;
		
		// Before setting, always get the options first in order 
		// not to override existing options.
		hr = pfd->GetOptions(&dwFlags);
		if (SUCCEEDED(hr)) {
			// In this case, get shell items only for file system items.
			hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
			if (SUCCEEDED(hr)) {
				size_t strlen = nes->menu.cur_dir.length() + 1;
				wchar_t* w_cur_dir = new wchar_t[strlen];
				size_t converted_chars = 0;
				mbstowcs_s(&converted_chars, w_cur_dir, strlen, nes->menu.cur_dir.c_str(), _TRUNCATE);
				IShellItem* sh_cur_dir;
				SHCreateItemFromParsingName(w_cur_dir, NULL, IID_PPV_ARGS(&sh_cur_dir));
				hr = pfd->SetDefaultFolder(sh_cur_dir);
				sh_cur_dir->Release();
				delete[] w_cur_dir;
				if(SUCCEEDED(hr)) {
					// Show the dialog
					hr = pfd->Show(NULL);
					if (SUCCEEDED(hr)) {
						// Obtain the result once the user clicks 
						// the 'Open' button.
						// The result is an IShellItem object.
						IShellItem* psiResult;
						hr = pfd->GetResult(&psiResult);
						if (SUCCEEDED(hr)) {
							// We are just going to print out the 
							// name of the file for sample sake.
							PWSTR pszFilePath = NULL;
							hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,
								&pszFilePath);
							if (SUCCEEDED(hr)) {
								strlen = wcslen(pszFilePath) + 1;
								char* rom_file = new char[strlen];
								wcstombs_s(&converted_chars, rom_file, strlen, pszFilePath, _TRUNCATE);
								nes->menu.cur_dir = rom_file;
								strlen = nes->menu.cur_dir.rfind('\\');
								if (strlen != std::string::npos) {
									nes->menu.cur_dir = nes->menu.cur_dir.substr(0, strlen);
								}
								bool success = nessys_load_cart_filename(nes, rom_file);
								if (success && (nes->prg_rom_base != NULL)) {
									// successfully loaded cart
									nes->menu.pane = nesmenu_pane_t::NONE;
									nesmenu_update_list(nes);
								} else {
									nes->menu.message_box = "Could not load rom file";
								}
								delete[] rom_file;
								CoTaskMemFree(pszFilePath);
							}
							psiResult->Release();
						}
					}
				}
			}
		}
		pfd->Release();
	}

}
#endif

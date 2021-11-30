// Project:     ArkNES
// File:        main.cpp
// Author:      Kamal Pillai
// Date:        7/13/2021
// Description:	main entry

#include "nessys.h"

#ifdef _WIN32

#ifdef _CONSOLE
int main()
#else
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#endif
#else
int main()
#endif
{
	// create an nes instance
	nessys_t nes_inst;
	nessys_init(&nes_inst);
	k3winObj::WindowLoop();
	nessys_unload_cart(&nes_inst);
	nessys_cleanup(&nes_inst);

	return 0;
}

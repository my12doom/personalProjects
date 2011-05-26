#include "global_funcs.h"
#include "dx_player.h"

char passkey[32];
// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	CoInitialize(NULL);

	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, L"System initialization failed, the program will exit now.", L"Error", MB_OK);
		return -1;
	}

	load_passkey();
	g_bomb_function;
	save_passkey();

	DWORD checksum[5];
	video_checksum(L"Z:\\00013.e3d", checksum);



	dx_player test(screen1, screen2, hinstance);
	while (!test.is_closed())
		Sleep(100);

	return 0;
}

int main()
{
	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);
}

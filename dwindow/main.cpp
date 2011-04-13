
#include "global_funcs.h"
#include "dx_player.h"

// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	CoInitialize(NULL);

	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, L"VMR9 initialization failed, the program will exit now.", L"Error", MB_OK);
		return -1;
	}

	dx_player test(screen1, screen2, hinstance);
	/*
	test.start_loading();
	test.load_file(L"F:\\movie\\101123.µÁÃÎ¿Õ¼ä.Inception.2010.Blu-ray.720p.x264.iPad.SiLUHD\\µÁÃÎ¿Õ¼ä.Inception.2010.Blu-ray.720p.x264.iPad.SiLUHD.01.mp4");
	test.load_file(L"F:\\movie\\101123.µÁÃÎ¿Õ¼ä.Inception.2010.Blu-ray.720p.x264.iPad.SiLUHD\\µÁÃÎ¿Õ¼ä.Inception.2010.Blu-ray.720p.x264.iPad.SiLUHD.02.mp4");
	test.end_loading();
	test.play();
	*/
	while (!test.is_closed())
		Sleep(100);

	return 0;
}

class tester
{
public:
	tester()
	{
		printf("tester");
	}
	~tester()
	{
		printf("~tester");
	}
};

int main()
{
	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);
}

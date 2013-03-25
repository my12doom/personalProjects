#include "dx_player.h"
#include "login.h"
#include "..\lua\my12doom_lua.h"
#include <locale.h>
#include <Dbghelp.h>
#include "..\renderer_prototype\my12doomRenderer_lua.h"

#pragma comment(lib, "DbgHelp")
dx_player *g_player = NULL;
AutoSetting<bool> single_instance(L"SingleInstance", true);

// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo);
extern HRESULT dwindow_dll_go(HINSTANCE inst, HWND owner, Iplayer *p);

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
 	SetUnhandledExceptionFilter(my_handler);
	CoInitialize(NULL);

	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);


	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, C(L"System initialization failed : monitor detection error, the program will exit now."), L"Error", MB_OK);
		return -1;
	}

	check_login();

	save_passkey();

	BasicRsaCheck();

	HWND pre_instance = single_instance ? FindWindowA("DWindowClass", NULL) : NULL;
	if (pre_instance)
	{
		SendMessageW(pre_instance, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
		SetForegroundWindow(pre_instance);
		if (argc>1)
		{
			COPYDATASTRUCT copy = {WM_LOADFILE, wcslen(argv[1])*2+2, argv[1]};
			SendMessageW(pre_instance, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copy);
		}
		return 0;
	}	

	dwindow_lua_init();
	my12doomRenderer_lua_init();
	dx_player *test = new dx_player(hinstance);
	g_player = test;
	BringWindowToTop(test->m_hwnd1);

	if (argc>1)
	{
		test->reset_and_loadfile(argv[1], false);
		SetFocus(test->m_hwnd1);
	}

	setlocale(LC_ALL, "chs");

	while (test->init_done_flag != 0x12345678)
		Sleep(100);
	if (is_theeater_version())
		dwindow_dll_go(hinstance, NULL, test);
	else
	while (!test->is_closed())
		Sleep(100);

	HANDLE killer = CreateThread(NULL, NULL, killer_thread2, new DWORD(3000), NULL, NULL);
	bar_logout();
	delete test;
	WaitForSingleObject(killer, 3000);
	return 0;
}

int main()
{
	WinMain(GetModuleHandle(NULL), 0, "", SW_SHOW);
}

LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	// mini dump
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscpy(wcsrchr(tmp, '\\'), L"\\DWindowDumpFile.dmp");
	HANDLE lhDumpFile = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL ,NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

	CloseHandle(lhDumpFile);

#ifdef ZHUZHU
	restart_this_program();
#endif

	// reset and suicide
	wchar_t description[1024];
	swprintf(description, C(L"Ooops....something bad happened. \n"
							L"Press Retry to debug or Cancel to RESTART progress.\n"
							L"Or continue at your own risk, the program will become UNSTABLE.\n"
							L"\nMini Dump File:\n%s"), tmp);
	int o = MessageBoxW(NULL, description, C(L"Error"), MB_CANCELTRYCONTINUE | MB_ICONERROR);
	if (o == IDCANCEL)
		restart_this_program();
	else if (o == IDCONTINUE)
		TerminateThread(GetCurrentThread(), -1);
	DebugBreak();
	return EXCEPTION_CONTINUE_SEARCH;
}
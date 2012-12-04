#include <time.h>
#include "global_funcs.h"
#include "dx_player.h"
#include "..\AESFile\E3DReader.h"
#include "..\AESFile\rijndael.h"
#include "resource.h"
#include "vobsub_parser.h"
#include "MediaInfo.h"
#include <locale.h>
#include <Dbghelp.h>
#pragma comment(lib, "DbgHelp")

ICommandReciever *command_reciever;

// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
int on_command(HWND hWnd, int uid);
int init_dialog(HWND hWnd);
LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo);

INT_PTR CALLBACK register_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		on_command(hDlg, LOWORD(wParam));
		break;

	case WM_INITDIALOG:
		localize_window(hDlg);
		init_dialog(hDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

extern HRESULT dwindow_dll_go(HINSTANCE inst, HWND owner, Iplayer *p);

DWORD WINAPI pre_read_thread(LPVOID k)
{
	return 0;

	wchar_t *p = (wchar_t*)k;

	wchar_t *tmp = new wchar_t[102400];
	wcscpy(tmp, p);
	wcscat(tmp, L"*.*");

	WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileW(tmp, &find_data);

	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{

			wcscpy(tmp, p);
			wcscat(tmp, find_data.cFileName);
			OutputDebugStringW(tmp);
			OutputDebugStringW(L"\r\n");

			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0
				&& wcscmp(L".",find_data.cFileName ) !=0
				&& wcscmp(L"..", find_data.cFileName) !=0)
			{
				wcscat(tmp, L"\\");
				pre_read_thread(tmp);
			}
			else
			{
				FILE * f = _wfopen(tmp, L"rb");
				if (f)
				{
					while (fread(tmp, 1, 102400, f))
						;
					fclose(f);
				}
			}

		}
		while( FindNextFile(find_handle, &find_data ) );
	}

	return 0;
}

int TCPTest();
DWORD WINAPI TCPThread(LPVOID k)
{
#ifndef VSTAR
	TCPTest();
#endif
	return 0;
}
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	CreateThread(NULL, NULL, pre_read_thread, g_apppath, NULL, NULL);

 	SetUnhandledExceptionFilter(my_handler);
	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	char volumeName[MAX_PATH];
	DWORD sn;
	DWORD max_component_length;
	DWORD filesystem;
	char str_filesystem[MAX_PATH];
	GetVolumeInformationA("C:\\", volumeName, MAX_PATH, &sn, &max_component_length, &filesystem, str_filesystem, MAX_PATH);

	CoInitialize(NULL);

	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, C(L"System initialization failed : monitor detection error, the program will exit now."), L"Error", MB_OK);
		return -1;
	}

retry:
	load_passkey();
	if (FAILED(check_passkey()))
	{
		if (g_bar_server[0] == NULL)
		{
#ifdef nologin
			int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID), NULL, register_proc );
#else
#ifdef VSTAR
			AutoSetting<BOOL> expired(L"VSTAR", FALSE, REG_DWORD);

			if (expired)
			{
			MessageBoxW(NULL, C(L"This version is marked as expired, please vist http://bo3d.net/download and download the latest version."), C(L"New Update ..."), MB_OK);
			ShellExecuteW(NULL, L"open", L"http://bo3d.net/download", NULL, NULL, SW_SHOWNORMAL);
			ExitProcess(-1);
			}
#else
			int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID_PAYED), NULL, register_proc );
#endif
#endif
			if (!is_trial_version())
				return 0;
		}
		else
		{
			if (MessageBoxW(0, C(L"System initialization failed : server not found, the program will exit now."), L"Error", MB_RETRYCANCEL) == IDRETRY)
				goto retry;
		}
	}
#ifndef no_dual_projector
	else if (is_trial_version())
		MessageBoxW(NULL, C(L"You are using a trial copy of DWindow, each clip will play normally for 10 minutes, after that the picture will become grayscale.\nYou can reopen it to play normally for another 10 minutes.\nRegister to remove this limitation."), L"....", MB_ICONINFORMATION);
#endif
	save_passkey();

	BRC();

	AutoSetting<bool> single_instance(L"SingleInstance", true);


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
	

	dx_player *test = new dx_player(hinstance);
	BringWindowToTop(test->m_hwnd1);

	if (argc>1)
	{
		test->reset_and_loadfile(argv[1], false);
		SetFocus(test->m_hwnd1);
	}

	setlocale(LC_ALL, "chs");

	command_reciever = test;
	CreateThread(NULL, NULL, TCPThread, NULL, NULL, NULL);

	if (is_theeater_version())
		dwindow_dll_go(hinstance, NULL, test);
	else
	while (!test->is_closed())
		Sleep(100);

	// reset passkey on need
	if(GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) < 0)
		memset(g_passkey_big, 0, 128);
	save_passkey();


	HANDLE killer = CreateThread(NULL, NULL, killer_thread2, new DWORD(3000), NULL, NULL);
	bar_logout();
	delete test;
	WaitForSingleObject(killer, 3000);
	return 0;
}

int main()
{
	__try 
	{
		char * x = NULL;
		*x = 256;
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{


	}
	WinMain(GetModuleHandle(NULL), 0, "", SW_SHOW);
}

LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	// mini dump
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"DumpFile.dmp");
	HANDLE lhDumpFile = CreateFileW(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL ,NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

	CloseHandle(lhDumpFile);

	// reset and suicide
	wchar_t reset_exe[MAX_PATH];
	wcscpy(reset_exe, g_apppath);
	wcscat(reset_exe, L"reset.exe");
	wchar_t this_exe[MAX_PATH];
	GetModuleFileNameW(NULL, this_exe, MAX_PATH);
	ShellExecute(NULL, NULL, reset_exe, this_exe, NULL, SW_SHOW);
	ExitProcess(-1);
	DebugBreak();

	return EXCEPTION_CONTINUE_SEARCH;
}


int init_dialog(HWND hWnd)
{
	//SetWindowTextW(hWnd, C(L"Enter User ID"));

	return 0;
}

int on_command(HWND hWnd, int uid)
{
	if (uid == IDOK || uid == ID_TRIAL)
	{
		char username[512];
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT1), username, 512);

		char password[512];
		GetWindowTextA(GetDlgItem(hWnd, IDC_EDIT2), password, 512);

		// SHA1 it!
		unsigned char sha1[20];
		SHA1Hash(sha1, (unsigned char*)password, strlen(password));

		// generate message
		dwindow_message_uncrypt message;
		memset(&message, 0, sizeof(message));
		message.zero = 0;
		message.client_rev = my12doom_rev;
		memcpy(message.passkey, username, 32);
		memcpy(message.requested_hash, sha1, 20);
		memcpy(message.password_uncrypted, password, min(19, strlen(password)));
		message.password_uncrypted[min(19, strlen(password))] = NULL;
		message.client_time = _time64(NULL);
		for(int i=0; i<32; i++)
			message.random_AES_key[i] = rand() &0xff;
		unsigned char encrypted_message[128];
		RSA_dwindow_network_public(&message, encrypted_message);

		char url[512];
		strcpy(url, g_server_address);
		strcat(url, uid == IDOK ? g_server_gen_key : g_server_free);
		strcat(url, "?");
		char tmp[3];
		for(int i=0; i<128; i++)
		{
			wsprintfA(tmp, "%02X", encrypted_message[i]);
			strcat(url, tmp);
		}

		char downloaded[800] = "";
		memset(downloaded, 0, 400);
		download_url(url, downloaded, 400);

#ifdef DEBUG
		OutputDebugStringA(url);
		OutputDebugStringA("\n");
		OutputDebugStringA(downloaded);
#endif
		
		if (strlen(downloaded) == 256)
		{
			unsigned char new_key[256];
			for(int i=0; i<128; i++)
				sscanf(downloaded+i*2, "%02X", new_key+i);
			AESCryptor aes;
			aes.set_key(message.random_AES_key, 256);
			for(int i=0; i<128; i+=16)
				aes.decrypt(new_key+i, new_key+i);

			memcpy(&g_passkey_big, new_key, 128);

			save_passkey();
			mytime(true);

			if (!is_trial_version())
				MessageBoxW(hWnd, C(L"This program will exit now, Restart it to use new user id."), C(L"Exiting"), MB_ICONINFORMATION);

			EndDialog(hWnd, IDOK);
		}
		else
		{
			int len = strlen(downloaded);
			USES_CONVERSION;
			wchar_t txt[400];
			wcscpy(txt, C(L"Activation Failed, Server Response:\n."));
			wcscat(txt, A2W(downloaded));
			MessageBoxW(hWnd, txt, C(L"Exiting"), MB_ICONERROR);
		}
	}
	else if (uid == IDCANCEL)
	{
		EndDialog(hWnd, IDCANCEL);
	}

	return 0;
}



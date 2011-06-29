#include <time.h>
#include "global_funcs.h"
#include "dx_player.h"
#include "..\AESFile\E3DReader.h"
#include "..\AESFile\rijndael.h"
#include "resource.h"

// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
int on_command(HWND hWnd, int uid);
int init_dialog(HWND hWnd);

INT_PTR CALLBACK register_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		on_command(hDlg, LOWORD(wParam));
		break;

	case WM_INITDIALOG:
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

int select1=0, select2=0;
INT_PTR CALLBACK select_monitor_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			HWND combo1 = GetDlgItem(hDlg, IDC_COMBO1);
			HWND combo2 = GetDlgItem(hDlg, IDC_COMBO2);
			select1 = SendMessage(combo1, CB_GETCURSEL, 0, 0);
			select2 = SendMessage(combo2, CB_GETCURSEL, 0, 0);

			if (select1 == select2)
				MessageBoxW(hDlg, C(L"You selected the same monitor!"), C(L"Warning"), MB_ICONERROR);
			EndDialog(hDlg, 0);
		}

		break;

	case WM_INITDIALOG:
		{
			USES_CONVERSION;
			HWND combo1 = GetDlgItem(hDlg, IDC_COMBO1);
			HWND combo2 = GetDlgItem(hDlg, IDC_COMBO2);
			for(int i=0; i<g_monitor_count; i++)
			{
				wchar_t tmp[1024];
				MONITORINFOEXW info;
				memset(&info, 0, sizeof(MONITORINFOEXW));
				info.cbSize = sizeof(MONITORINFOEXW);
				GetMonitorInfoW(g_monitors[i], &info);

				wsprintfW(tmp, L"%s @ %s(%dx%d)", info.szDevice, A2W(g_ids[i].Description),
					info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
				SendMessage(combo1, CB_ADDSTRING, 0, (LPARAM) tmp);
				SendMessage(combo2, CB_ADDSTRING, 0, (LPARAM) tmp);
				SendMessage(combo1, CB_SETCURSEL, 0, 0);
				SendMessage(combo2, CB_SETCURSEL, 1, 0);
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, -1);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
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
		MessageBoxW(0, L"System initialization failed, the program will exit now.", L"Error", MB_OK);
		return -1;
	}

	load_passkey();
	if (FAILED(check_passkey()))
	{
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID), NULL, register_proc );
		return 0;
	}
	save_passkey();
#include "bomb_function.h"

	HMONITOR monitor1;
	HMONITOR monitor2;

	if (g_monitor_count == 1)
		monitor1 = monitor2 = g_monitors[0];
	else if (g_monitor_count == 2)
	{
		monitor1 = g_monitors[0];
		monitor2 = g_monitors[1];
	}
	else
	{
		if (DialogBox(hinstance, MAKEINTRESOURCE(IDD_SELECTMONITOR), NULL, select_monitor_proc) < 0)
			ExitProcess(-1);

		monitor1 = g_monitors[select1];
		monitor2 = g_monitors[select2];
	}

	MONITORINFOEX info1, info2;
	memset(&info1, 0, sizeof(MONITORINFOEX));
	memset(&info2, 0, sizeof(MONITORINFOEX));
	info1.cbSize = info2.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(monitor1, &info1);
	GetMonitorInfo(monitor2, &info2);

	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	HWND pre_instance = FindWindowA("DWindowClass", NULL);
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

	dx_player test(info1.rcMonitor, info2.rcMonitor, hinstance);
	BringWindowToTop(test.m_hwnd1);


	if (argc>1)
	{
		test.reset_and_loadfile(argv[1], false);
		while(!test.m_reset_load_done)
			Sleep(50);

		//test.toggle_fullscreen();
		SetFocus(test.m_hwnd1);
	}
	while (!test.is_closed())
		Sleep(100);

	return 0;
}


typedef DWORD ADDR;

int main()
{
	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);


	__try 
	{
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{
		long			StackIndex				= 0;

		ADDR			block[63];
		memset(block,0,sizeof(block));


		USHORT frames = CaptureStackBackTrace(3,59,(void**)block,NULL);

		for (int i = 0; i < frames ; i++)
		{
			ADDR			InstructionPtr = (ADDR)block[i];

			printf("0x%08x\n", block[i]);
			StackIndex++;
		}
	}
}


int init_dialog(HWND hWnd)
{
	SetWindowTextW(hWnd, C(L"Enter User ID"));

	return 0;
}

int on_command(HWND hWnd, int uid)
{
	if (uid == IDOK)
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
		memcpy(message.passkey, username, 32);
		memcpy(message.requested_hash, sha1, 20);
		for(int i=0; i<32; i++)
			message.random_AES_key[i] = rand() &0xff;
		message.zero = 0;
		unsigned char encrypted_message[128];
		RSA_dwindow_network_public(&message, encrypted_message);

		char url[512];
		strcpy(url, g_server_address);
		strcat(url, g_server_gen_key);
		strcat(url, "?");
		char tmp[3];
		for(int i=0; i<128; i++)
		{
			wsprintfA(tmp, "%02X", encrypted_message[i]);
			strcat(url, tmp);
		}

		char downloaded[400] = "";
		memset(downloaded, 0, 400);
		download_url(url, downloaded, 400);

		OutputDebugStringA(url);
		OutputDebugStringA("\n");
		OutputDebugStringA(downloaded);
		
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
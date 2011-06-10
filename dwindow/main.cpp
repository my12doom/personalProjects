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
	if (FAILED(check_passkey()))
	{
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, register_proc );
		return 0;
	}
	save_passkey();
#include "bomb_function.h"


	dx_player test(screen1, screen2, hinstance);

	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc>1)
		test.reset_and_loadfile(argv[1]);
	while (!test.is_closed())
		Sleep(100);

	return 0;
}

int main()
{
	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);
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
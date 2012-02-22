#include <time.h>
#include "global_funcs.h"
#include "dx_player.h"
#include "..\AESFile\E3DReader.h"
#include "..\AESFile\rijndael.h"
#include "resource.h"
#include "vobsub_parser.h"

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
			for(int i=0; i<g_logic_monitor_count; i++)
			{
				wchar_t tmp[1024];
				MONITORINFOEXW info;
				memset(&info, 0, sizeof(MONITORINFOEXW));
				info.cbSize = sizeof(MONITORINFOEXW);
				GetMonitorInfoW(g_logic_monitors[i], &info);

				wsprintfW(tmp, L"%s @ %s(%dx%d)", info.szDevice, A2W(g_logic_ids[i].Description),
					g_logic_monitor_rects[i].right - g_logic_monitor_rects[i].left, g_logic_monitor_rects[i].bottom - g_logic_monitor_rects[i].top);
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
		MessageBoxW(0, C(L"System initialization failed : monitor detection error, the program will exit now."), L"Error", MB_OK);
		return -1;
	}

retry:
	load_passkey();
	if (FAILED(check_passkey()))
	{
		if (g_bar_server[0] == NULL)
		{
			int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID), NULL, register_proc );
			return 0;
		}
		else
		{
			if (MessageBoxW(0, C(L"System initialization failed : server not found, the program will exit now."), L"Error", MB_RETRYCANCEL) == IDRETRY)
				goto retry;
		}
	}
	save_passkey();
#include "bomb_function.h"

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

	dx_player *test = new dx_player(hinstance);
	BringWindowToTop(test->m_hwnd1);


	if (argc>1)
	{
		test->reset_and_loadfile(argv[1], false);
		while(!test->m_reset_load_done)
			Sleep(50);

		//test.toggle_fullscreen();
		SetFocus(test->m_hwnd1);
	}
	while (!test->is_closed())
		Sleep(100);

	CreateThread(NULL, NULL, killer_thread2, new DWORD(3000), NULL, NULL);
	bar_logout();
	delete test;
	return 0;
}


typedef DWORD ADDR;

int test()
{
	VobSubParser p;
	int max_lang_id = p.max_lang_id(L"D:\\Users\\my12doom\\Documents\\DVDFab\\mkv\\RED_BIRD_3D_F2\\RED_BIRD_3D_F2.Title852.idx");

	printf("max langid: %d.\n", max_lang_id);

	p.load_file(L"D:\\Users\\my12doom\\Documents\\DVDFab\\mkv\\RED_BIRD_3D_F2\\RED_BIRD_3D_F2.Title852.idx", 0, 
		L"D:\\Users\\my12doom\\Documents\\DVDFab\\mkv\\RED_BIRD_3D_F2\\RED_BIRD_3D_F2.Title852.sub");

	
	vobsub_subtitle *sub = p.m_subtitles+4;
	p.decode(sub);
	FILE *f = fopen("Z:\\sub.raw", "wb");
	fwrite(sub->rgb, 1, 4*sub->width*sub->height, f);
	fclose(f);

	return 0;
}

static const byte rLPS_table_64x4[64][4]=
{
	{ 128, 176, 208, 240},
	{ 128, 167, 197, 227},
	{ 128, 158, 187, 216},
	{ 123, 150, 178, 205},
	{ 116, 142, 169, 195},
	{ 111, 135, 160, 185},
	{ 105, 128, 152, 175},
	{ 100, 122, 144, 166},
	{  95, 116, 137, 158},
	{  90, 110, 130, 150},
	{  85, 104, 123, 142},
	{  81,  99, 117, 135},
	{  77,  94, 111, 128},
	{  73,  89, 105, 122},
	{  69,  85, 100, 116},
	{  66,  80,  95, 110},
	{  62,  76,  90, 104},
	{  59,  72,  86,  99},
	{  56,  69,  81,  94},
	{  53,  65,  77,  89},
	{  51,  62,  73,  85},
	{  48,  59,  69,  80},
	{  46,  56,  66,  76},
	{  43,  53,  63,  72},
	{  41,  50,  59,  69},
	{  39,  48,  56,  65},
	{  37,  45,  54,  62},
	{  35,  43,  51,  59},
	{  33,  41,  48,  56},
	{  32,  39,  46,  53},
	{  30,  37,  43,  50},
	{  29,  35,  41,  48},
	{  27,  33,  39,  45},
	{  26,  31,  37,  43},
	{  24,  30,  35,  41},
	{  23,  28,  33,  39},
	{  22,  27,  32,  37},
	{  21,  26,  30,  35},
	{  20,  24,  29,  33},
	{  19,  23,  27,  31},
	{  18,  22,  26,  30},
	{  17,  21,  25,  28},
	{  16,  20,  23,  27},
	{  15,  19,  22,  25},
	{  14,  18,  21,  24},
	{  14,  17,  20,  23},
	{  13,  16,  19,  22},
	{  12,  15,  18,  21},
	{  12,  14,  17,  20},
	{  11,  14,  16,  19},
	{  11,  13,  15,  18},
	{  10,  12,  15,  17},
	{  10,  12,  14,  16},
	{   9,  11,  13,  15},
	{   9,  11,  12,  14},
	{   8,  10,  12,  14},
	{   8,   9,  11,  13},
	{   7,   9,  11,  12},
	{   7,   9,  10,  12},
	{   7,   8,  10,  11},
	{   6,   8,   9,  11},
	{   6,   7,   9,  10},
	{   6,   7,   8,   9},
	{   2,   2,   2,   2}
};

int main()
{
	WinMain(GetModuleHandle(NULL), 0, "", SW_SHOW);


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
		memset(&message, 0, sizeof(message));
		message.zero = 0;
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
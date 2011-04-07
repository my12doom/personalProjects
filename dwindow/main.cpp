#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include <math.h>

#include "global_funcs.h"
#include "dx_player.h"
#include "resource.h"

// main window
HRESULT get_monitors_rect(RECT *screen1, RECT *screen2);
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void on_command(HWND hDlg, int ItemID);
void init_dialog(HWND hDlg);
void finish_dialog(HWND hDlg);

// input files
#define type_normal 0
#define type_mkv 1
wchar_t input1[MAX_PATH] = L"";
int type1 = type_normal;
wchar_t input2[MAX_PATH] = L"";
int type2 = type_normal;
wchar_t sound[MAX_PATH] = L"";
wchar_t _subtitle[MAX_PATH] = L"";


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
	test.show_window(1, true);
	while (!test.is_closed())
		Sleep(100);

	return 0;

show_config:
	int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, MainDlgProc );
	if (o != 1)
		return 0;

	dx_player *player = new dx_player(screen1, screen2, hinstance);
	player->show_window(1, true);
	//player->show_window(2, true);
	player->set_window_text(1, L"1");
	player->set_window_text(2, L"2");

	player->start_loading();
	//_subtitle
	if (_subtitle[0])
		player->load_subtitle(_subtitle);
	//sound
	if (sound[0])
		player->load_file(sound);

	// input1
	if (input1[0])
	{
			player->load_file(input1);
	}


	// input2
	if (input2[0])
	{
			player->load_file(input2);
	}

	HRESULT hr = player->end_loading();
	if (FAILED(hr))
	{
		wchar_t *tmp = (wchar_t*)malloc(100000);
		wcscpy(tmp, L"play failed, please check input files and/or config.\n\nerror detail:\n");
		wcscat(tmp, player->m_log);
		MessageBoxW(NULL, tmp, L"Error", MB_OK | MB_ICONERROR);
		free(tmp);
		delete player;
		goto show_config;
	}
	else
	{
		// print load log on console
		printf("load log:\n");
		wprintf(player->m_log);
		printf("\n");
	}

	//BringWindowToTop(player->m_hwnd2);
	BringWindowToTop(player->m_hwnd1);
	player->play();

	while (!player->is_closed())
		Sleep(100);

	delete player;
	goto show_config;
}

int main()
{
	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);
}

INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
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


void on_command(HWND hDlg, int ItemID)
{
	if (ItemID == IDC_PLAY)
	{
		finish_dialog(hDlg);
		EndDialog(hDlg, 1);
	}

	if (ItemID == IDC_CHECK_MKV1)
	{
		int state = Button_GetState(GetDlgItem(hDlg, ItemID));
		if ( state & BST_CHECKED)
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SOUND1), TRUE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SOUND1), FALSE);
	}

	if (ItemID == IDC_CHECK_MKV2)
	{
		int state = Button_GetState(GetDlgItem(hDlg, ItemID));
		if ( state & BST_CHECKED)
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SOUND2), TRUE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SOUND2), FALSE);
	}

	// file boxs
	wchar_t file_selected[MAX_PATH];
	if (ItemID == IDC_BUTTON_INPUT1)
	{
		GetDlgItemTextW(hDlg, IDC_EDIT_INPUT1, file_selected, MAX_PATH);
		open_file_dlg(file_selected, hDlg);
		SetDlgItemTextW(hDlg, IDC_EDIT_INPUT1, file_selected);
	}
	if (ItemID == IDC_BUTTON_INPUT2)
	{
		GetDlgItemTextW(hDlg, IDC_EDIT_INPUT2, file_selected, MAX_PATH);
		open_file_dlg(file_selected, hDlg);
		SetDlgItemTextW(hDlg, IDC_EDIT_INPUT2, file_selected);
	}
	if (ItemID == IDC_BUTTON_SOUND)
	{
		GetDlgItemTextW(hDlg, IDC_EDIT_SOUND, file_selected, MAX_PATH);
		open_file_dlg(file_selected, hDlg, 	L"Audio files\0"
			L"*.wav;*.ac3;*.dts;*.mp3;*.mp2;*.mpa;*.mp4;*.wma;*.flac;*.ape\0"
			L"All Files\0*.*\0\0");
		SetDlgItemTextW(hDlg, IDC_EDIT_SOUND, file_selected);
	}
	if (ItemID == IDC_BUTTON_SUBTITLE)
	{
		GetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, file_selected, MAX_PATH);
		open_file_dlg(file_selected, hDlg, L"Subtitles(*.srt)\0*.srt;*.sup\0All Files\0*.*\0\0");
		SetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, file_selected);
	}
}
void init_dialog(HWND hDlg)
{
	SetDlgItemTextW(hDlg, IDC_EDIT_INPUT1, input1);
	SetDlgItemTextW(hDlg, IDC_EDIT_INPUT2, input2);
	SetDlgItemTextW(hDlg, IDC_EDIT_SOUND, sound);
	SetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, _subtitle);

	HWND combo1 = GetDlgItem(hDlg, IDC_COMBO1);
	HWND combo2 = GetDlgItem(hDlg, IDC_COMBO2);

	SendMessage(combo1, CB_RESETCONTENT, 0, 0);
	SendMessage(combo2, CB_RESETCONTENT, 0, 0);

	USES_CONVERSION;
	SendMessage(combo1, CB_ADDSTRING, 0, (LPARAM)W2T(L"Normal"));
	SendMessage(combo1, CB_ADDSTRING, 0, (LPARAM)W2T(L"MKV"));
	SendMessage(combo2, CB_ADDSTRING, 0, (LPARAM)W2T(L"Normal"));

	SendMessage(combo1, CB_SETCURSEL, type1, 0);
	SendMessage(combo2, CB_SETCURSEL, type2, 0);
}

void finish_dialog(HWND hDlg)
{
	GetDlgItemTextW(hDlg, IDC_EDIT_INPUT1, input1, MAX_PATH);
	GetDlgItemTextW(hDlg, IDC_EDIT_INPUT2, input2, MAX_PATH);
	GetDlgItemTextW(hDlg, IDC_EDIT_SOUND, sound, MAX_PATH);
	GetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, _subtitle, MAX_PATH);

	type1 = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
	type2 = SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
}

// helper functions


// some deleted codes

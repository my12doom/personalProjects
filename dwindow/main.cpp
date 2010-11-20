#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include <math.h>

//#include "private_filter.h"
#include "dx_player.h"
#include "resource.h"

// main window
HRESULT get_monitors_rect(RECT *screen1, RECT *screen2);
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void on_command(HWND hDlg, int ItemID);
void init_dialog(HWND hDlg);
void finish_dialog(HWND hDlg);
bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter = NULL);

// input files
#define type_normal 0
#define type_mkv 1
#define type_PD10 2
wchar_t input1[MAX_PATH] = L"";
int type1 = type_normal;
wchar_t input2[MAX_PATH] = L"";
int type2 = type_normal;
wchar_t sound[MAX_PATH] = L"";
wchar_t subtitle[MAX_PATH] = L"";

// scrennsaver
HMODULE hExe = GetModuleHandle(NULL);
HMODULE hDll = NULL;
void kill_ssaver();
void revive_ssaver();


HRESULT get_monitors_rect(RECT *screen1, RECT *screen2)
{
	if (screen1 == NULL || screen2 == NULL)
		return E_POINTER;

	CComPtr<IGraphBuilder> gb;
	gb.CoCreateInstance(CLSID_FilterGraph);
	if (gb == NULL)
		return E_FAIL;

	CComPtr<IBaseFilter> vmr9;
	vmr9.CoCreateInstance(CLSID_VideoMixingRenderer9);
	if (vmr9 == NULL)
		return E_FAIL;

	gb->AddFilter(vmr9, L"vmr9");

	CComQIPtr<IVMRMonitorConfig9, &IID_IVMRMonitorConfig9> mon9(vmr9);
	if (mon9 == NULL)
		return E_FAIL;

	const int max_mon=4;						// Assume no more than four monitors
	VMR9MonitorInfo mon_infos[max_mon]={0};
	DWORD num_mon=0;

	HRESULT hr = mon9->GetAvailableMonitors(mon_infos, max_mon, &num_mon);
	if (FAILED(hr))
		return hr;

	*screen1 = mon_infos[0].rcMonitor;
	*screen2 = mon_infos[num_mon-1].rcMonitor;

	return S_OK;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	//default_filters();
	CoInitialize(NULL);
	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, L"VMR9 initialization failed, the program will exit now.", L"Error", MB_OK);
		return -1;
	}

show_config:
	int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, MainDlgProc );
	if (o != 1)
		return 0;

	dx_window *player = new dx_window(screen1, screen2, hinstance);
	player->show_window(1, true);
	player->show_window(2, true);
	player->set_window_text(1, L"1");
	player->set_window_text(2, L"2");

	player->start_loading();
	//subtitle
	if (subtitle[0])
		player->load_srt(subtitle);
	//sound
	if (sound[0])
		player->load_file(sound);

	// input1
	if (input1[0])
	{
		if (type1 == type_PD10)
			player->load_PD10_file(input1);
		else if (type1 == type_mkv)
			player->load_mkv_file(input1);
		else
			player->load_file(input1);
	}


	// input2
	if (input2[0])
	{
		if (type2 == type_PD10)
			player->load_PD10_file(input2);
		else if (type2 == type_mkv)
			player->load_mkv_file(input2);
		else
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

	kill_ssaver();
	BringWindowToTop(player->m_hwnd2);
	BringWindowToTop(player->m_hwnd1);
	player->play();

	while (!player->is_closed())
		Sleep(100);

	revive_ssaver();
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

bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter/* = NULL*/)
{
	static wchar_t *default_filter = L"Video files\0"
									 L"*.mp4;*.mkv;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls\0"
									 L"Audio files\0"
									 L"*.wav;*.ac3;*.dts;*.mp3;*.mp2;*.mpa;*.mp4;*.wma;*.flac;*.ape;*.avs\0"
									 L"Subtitles\0*.srt\0"
									 L"All Files\0*.*\0"
									 L"\0";
	if (filter == NULL) filter = default_filter;
	wchar_t strFileName[MAX_PATH] = L"";
	wchar_t strPath[MAX_PATH] = L"";

	wcsncpy(strFileName, pathname, MAX_PATH);
	wcsncpy(strPath, pathname, MAX_PATH);
	for(int i=(int)wcslen(strPath)-2; i>=0; i--)
		if (strPath[i] == L'\\')
		{
			strPath[i+1] = NULL;
			break;
		}

	OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW), hDlg , NULL,
		filter, NULL,
		0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
		L"Open Sound File",
		OFN_FILEMUSTEXIST|OFN_HIDEREADONLY, 0, 0,
		L".mp4", 0, NULL, NULL };

	int o = GetOpenFileNameW( &ofn );
	if (o)
	{
		wcsncpy(pathname, strFileName, MAX_PATH);
		return true;
	}

	return false;
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
		open_file_dlg(file_selected, hDlg, L"Subtitles(*.srt)\0*.srt\0All Files\0*.*\0\0");
		SetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, file_selected);
	}
}
void init_dialog(HWND hDlg)
{
	SetDlgItemTextW(hDlg, IDC_EDIT_INPUT1, input1);
	SetDlgItemTextW(hDlg, IDC_EDIT_INPUT2, input2);
	SetDlgItemTextW(hDlg, IDC_EDIT_SOUND, sound);
	SetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, subtitle);

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
	GetDlgItemTextW(hDlg, IDC_EDIT_SUBTITLE, subtitle, MAX_PATH);

	type1 = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
	type2 = SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
}

// screen saver
void kill_ssaver()
{
	wchar_t dll_path[MAX_PATH];
	GetModuleFileNameW(NULL, dll_path, MAX_PATH);
	for(int i=(int)wcslen(dll_path); i>=0; i--)
		if (dll_path[i] == L'\\')
		{
			dll_path[i+1] = NULL;
			break;
		}
	wcscat(dll_path, L"kill_ssaver.dll");
	//TODO
	HGLOBAL hDllData = LoadResource(hExe, FindResource(hExe, MAKEINTRESOURCE(IDR_DLL), RT_RCDATA));
	if (hDllData == NULL)
		goto try_kill;
	void * dll_data = LockResource(hDllData);
	FILE * f = _wfopen(dll_path, L"wb");
	if (f)
	{
		fwrite(dll_data, 1, 4096, f);
		fflush(f);
		fclose(f);
	}
try_kill:
	if (NULL == hDll)
		hDll = LoadLibraryW(dll_path);
	if (NULL == hDll)
		return;

	typedef int (*p_proc_kill)(void);
	p_proc_kill p_kill = (p_proc_kill)GetProcAddress(hDll, "kill");
	p_kill();
}

void revive_ssaver()
{
	typedef int (*p_proc_revive)(void);
	p_proc_revive p_revive = (p_proc_revive)GetProcAddress(hDll, "revive");
	if (p_revive)
		p_revive();
}
// helper functions


// some deleted codes

#include "fullscreen_modes_select.h"
#include "resource.h"
#include "global_funcs.h"

D3DDISPLAYMODE *g_modes;
int g_modes_count;
int g_selected;
INT_PTR CALLBACK fullscreen_modes_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

HRESULT select_fullscreen_mode(HINSTANCE instance, HWND parent, D3DDISPLAYMODE *modes, int modes_count, int *selected)		// return S_FALSE on cancel, selected undefined in this case
{
	g_modes = modes;
	g_modes_count = modes_count;
	g_selected = selected ? -1 : *selected;

	int o = DialogBoxW(instance, MAKEINTRESOURCEW(IDD_FULLSCREEN_MODES), parent, fullscreen_modes_proc);

	g_selected = g_selected>=0 && g_selected<g_modes_count ? g_selected : -1;

	if (selected)
		*selected = g_selected;

	return o == 0 ? S_OK : S_FALSE;
}

INT_PTR CALLBACK fullscreen_modes_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			g_selected = SendMessageW(GetDlgItem(hDlg, IDC_COMBO1), CB_GETCURSEL, 0, 0);
			EndDialog(hDlg, 0);
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, -1);
		}
		break;

	case WM_INITDIALOG:
		{
			HWND combobox = GetDlgItem(hDlg, IDC_COMBO1);
			wchar_t tmp[1024];
			for(int i=0; i<g_modes_count; i++)
			{
				wsprintfW(tmp, L"%dx%d @ %dHz\n", g_modes[i].Width, g_modes[i].Height,g_modes[i].RefreshRate);
				SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)tmp);
			}
			SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)C(L"Auto Select"));
			SendMessageW(combobox, CB_SETCURSEL, g_selected>=0 && g_selected<g_modes_count ? g_selected : g_modes_count, 0);
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
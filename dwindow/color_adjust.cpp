#include "color_adjust.h"
#include "Resource.h"
#include "global_funcs.h"
#include <CommCtrl.h>

double current_values[color_adjust_max];
IColorAdjustCB *g_cb = NULL;
bool preview;
AutoSetting<bool> sync(L"SyncAdjust", true);
int slider2type(HWND hDlg, HWND slider)
{
	if (slider == GetDlgItem(hDlg, IDC_SAT))
		return saturation;
	if (slider == GetDlgItem(hDlg, IDC_LUM))
		return luminance;
	if (slider == GetDlgItem(hDlg, IDC_HUE))
		return hue;
	if (slider == GetDlgItem(hDlg, IDC_CON))
		return contrast;

	if (slider == GetDlgItem(hDlg, IDC_SAT2))
		return saturation2;
	if (slider == GetDlgItem(hDlg, IDC_LUM2))
		return luminance2;
	if (slider == GetDlgItem(hDlg, IDC_HUE2))
		return hue2;
	if (slider == GetDlgItem(hDlg, IDC_CON2))
		return contrast2;
	return -1;
}

int type2IDC(int type)
{
	int tbl[color_adjust_max] = {IDC_SAT, IDC_LUM, IDC_HUE, IDC_CON, IDC_SAT2, IDC_LUM2, IDC_HUE2, IDC_CON2};
	if (type < 0 || type >= color_adjust_max)
		return -1;
	else
		return tbl[type];
}

int type2type2(int type)
{
	if (type<0 || type >= (color_adjust_max/2))
		return -1;

	return type + color_adjust_max/2;
}


INT_PTR CALLBACK color_adjust_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDOK)
			{
				EndDialog(hDlg, 0);
			}
			else if (id == IDCANCEL)
			{
				EndDialog(hDlg, -1);
			}
			else if (id == IDC_RESET)
			{
				for(int i=0; i<color_adjust_max; i++)
				{
					HWND slider = GetDlgItem(hDlg, type2IDC(i));
					SendMessage(slider, TBM_SETPOS, TRUE, (int)(0.5 * 65535));
					current_values[i] = 0.5;
					g_cb->set_parameter(i, 0.5);
				}
			}
			else if (id == IDC_PREVIEW)
			{
				preview = (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_PREVIEW), BM_GETCHECK, 0, 0));

				for(int i=0; i<color_adjust_max; i++)
				{
					g_cb->set_parameter(i, preview ? current_values[i] : 0.5);
				}
			}
			else if (id == IDC_SYNC)
			{
				sync = (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_SYNC), BM_GETCHECK, 0, 0));

				for(int i=color_adjust_max/2; i<color_adjust_max; i++)
				{
					EnableWindow(GetDlgItem(hDlg, type2IDC(i)), !sync);
				}
			}

		}
		break;
	case WM_INITDIALOG:
		{
			// local
			localize_window(hDlg);

			for(int i=0; i<color_adjust_max; i++)
			{
				HWND slider = GetDlgItem(hDlg, type2IDC(i));
				SendMessage(slider, TBM_SETRANGEMIN, TRUE, 0);
				SendMessage(slider, TBM_SETRANGEMAX, TRUE, 65535);
				SendMessage(slider, TBM_SETPOS, TRUE, (int)(current_values[i] * 65535));
				//SetScrollPos(slider, SB_CTL, (int)(current_values[i] * 65535), TRUE);
				SendMessage(slider, TBM_CLEARTICS, TRUE, 0);
				SendMessage(slider, TBM_SETTIC, 0, 32767);
			}

			preview = true;
			SendMessage(GetDlgItem(hDlg, IDC_PREVIEW), BM_SETCHECK, TRUE, 0);

			SendMessage(GetDlgItem(hDlg, IDC_SYNC), BM_SETCHECK, sync, 0);

			for(int i=color_adjust_max/2; i<color_adjust_max; i++)
				EnableWindow(GetDlgItem(hDlg, type2IDC(i)), !sync);

		}
		break;

	case WM_HSCROLL:
		{
			HWND slider = (HWND)lParam;
			WORD value = (WORD)SendMessage(slider, TBM_GETPOS, 0, 0);;
			int type = slider2type(hDlg, slider);
			if (type != -1 && g_cb && ((LOWORD(wParam) == SB_THUMBTRACK) || (LOWORD(wParam) == SB_PAGEDOWN || (LOWORD(wParam) == SB_PAGEUP))))
			{
				if (!sync)
				{
					current_values[type] = (double)value/65535;
					g_cb->set_parameter(type, preview ? current_values[type] : 0.5);
				}
				else if (type2type2(type) >= 0)
				{
					// set left eye
					current_values[type] = (double)value/65535;
					g_cb->set_parameter(type, preview ? current_values[type] : 0.5);

					// set right eye
					current_values[type2type2(type)] = (double)value/65535;
					g_cb->set_parameter(type2type2(type), preview ? current_values[type2type2(type)] : 0.5);

					// set right scrollbar
					SendMessage(GetDlgItem(hDlg, type2IDC(type2type2(type))), TBM_SETPOS, TRUE, (int)(current_values[type2type2(type)] * 65535));
				}
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

HRESULT show_color_adjust(HINSTANCE instance, HWND parent, IColorAdjustCB *cb)
{
	if (!cb)
		return E_POINTER;

	g_cb = cb;

	double values_before[color_adjust_max];
	for(int i=0; i<color_adjust_max; i++)
	{
		cb->get_parameter(i, current_values+i);
		values_before[i] = current_values[i];
	}

	int o = DialogBoxW(instance, MAKEINTRESOURCEW(IDD_COLOR), parent, color_adjust_proc);
	if (o == -1)
		for(int i=0; i<color_adjust_max; i++)
			cb->set_parameter(i, values_before[i]);			// reverse on cancel
	else
		for(int i=0; i<color_adjust_max; i++)
			cb->set_parameter(i, current_values[i]);

	return o == -1 ? S_FALSE : S_OK;
}
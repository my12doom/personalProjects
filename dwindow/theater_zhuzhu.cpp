#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include "resource1.h"
#include "Iplayer.h"
#include "audio_device.h"
#include "global_funcs.h"

#define SB_RBUTTON 16
namespace zhuzhu
{
Iplayer * player = NULL;
HWND control = NULL;
HWND hProgress;// = GetDlgItem(hDlg, IDC_PROGRESS);
HWND hVolume;// = GetDlgItem(hDlg, IDC_VOLUME);
LONG_PTR g_OldVolumeProc = 0;
LONG_PTR g_OldProgressProc = 0;
bool is_draging = false;
HINSTANCE inst = NULL;

char the_passkey_and_signature[128+128+128] = "my12doom here!";

// overrided slider proc
LRESULT CALLBACK VolumeProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ProgressProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void format_time(int time, wchar_t *out);
void format_time_noms(int time, wchar_t *out);
void format_time2(int current, int total, wchar_t *out);

HWND focus = NULL;
BOOL dhcp_enable;
static INT_PTR CALLBACK threater_countrol_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{

	case WM_INITDIALOG:
		localize_window(hDlg);
		{
			for(int i=0; i<4; i++)
			{
				if (i>= get_audio_device_count())
					ShowWindow(GetDlgItem(hDlg, IDC_DEVICE1+i), SW_HIDE);
				else
				{
					wchar_t tmp[4096];
					get_audio_device_name(i, tmp);
					SetDlgItemTextW(hDlg, IDC_DEVICE1+i, tmp);
				}
			}

			Button_SetCheck (GetDlgItem(hDlg, IDC_DEVICE1+get_default_audio_device()), TRUE);
		}
		break;

	case WM_TIMER:
		break;

	case WM_HSCROLL:
		break;
	case WM_VSCROLL:
		break;

	case WM_COMMAND:

		int id;
		id = LOWORD(wParam);
		switch(id)
		{
		case IDC_IP1:
		case IDC_IP2:
		case IDC_IP3:
		case IDC_IP4:
		case IDC_MASK1:
		case IDC_MASK2:
		case IDC_MASK3:
		case IDC_MASK4:
		case IDC_GATE1:
		case IDC_GATE2:
		case IDC_GATE3:
		case IDC_GATE4:
		case IDC_DNS1:
		case IDC_DNS2:
		case IDC_DNS3:
		case IDC_DNS4:
			if (HIWORD(wParam) == EN_SETFOCUS)
				focus = (HWND)lParam;
			break;

		case IDC_0:
		case IDC_1:
		case IDC_2:
		case IDC_3:
		case IDC_4:
		case IDC_5:
		case IDC_6:
		case IDC_7:
		case IDC_8:
		case IDC_9:
			if (focus)
			{
				wchar_t tmp[256] = {0};
				wchar_t tmp2[20];
				swprintf(tmp2, 20, L"%d", id - IDC_0);
				GetWindowTextW(focus, tmp, 255);
				wcscat(tmp, tmp2);
				SetWindowTextW(focus, tmp);
			}
			break;
		case IDC_B:
			if (focus)
			{
				wchar_t tmp[256] = {0};
				GetWindowTextW(focus, tmp, 255);
				if (wcslen(tmp)>0)
					tmp[wcslen(tmp)-1] = NULL;
				SetWindowTextW(focus, tmp);
			}
			break;

		case IDC_DHCP:
		case IDC_MANUAL:
			{
				dhcp_enable = id == IDC_DHCP;
				focus = NULL;
				int ids[] = {IDC_IP1, IDC_MASK1, IDC_GATE1, IDC_DNS1};
				for(int i=0; i<4; i++)
					for(int j=0; j<4; j++)
				{
					EnableWindow(GetDlgItem(hDlg, ids[i]+j), !dhcp_enable);
				}

				for(int i=0; i<10; i++)
					EnableWindow(GetDlgItem(hDlg, IDC_0+i), !dhcp_enable);
				EnableWindow(GetDlgItem(hDlg, IDC_B), !dhcp_enable);
			}
			break;

		case IDC_AUDIO_MODE:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_AUDIO), hDlg, threater_countrol_proc);
			break;
		case IDC_AUDIO_DEVICE:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_AUDIO_DEVICE), hDlg, threater_countrol_proc);
			break;
		case IDC_VIDEO_DEVICE:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_VIDEO), hDlg, threater_countrol_proc);
			break;
		case IDC_NETWORK:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_NETWORK), hDlg, threater_countrol_proc);
			break;
		case IDC_SYSTEM:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_SYSTEM), hDlg, threater_countrol_proc);
			break;

		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, -2);
			break;
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


HRESULT dwindow_dll_go_zhuzhu(HINSTANCE inst, HWND owner, Iplayer *p)
{
	player = p;
	zhuzhu::inst = inst;

	int o = DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_MAIN), owner, threater_countrol_proc);

	return S_OK;
}

HRESULT dwindow_dll_init_zhuzhu(char *passkey_big, int rev)
{
	return S_OK;
}

LRESULT CALLBACK VolumeProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WPARAM flag = 0;
	bool mouse_event = false;
	static bool dragging = false;
	static int lastflag = 0;
	switch( msg ) 
	{
	case WM_RBUTTONDOWN:
		flag |= SB_RBUTTON;
	case WM_LBUTTONDOWN:
		SetCapture(hDlg);
		SetFocus(hDlg);
		mouse_event = true;
		dragging = true;
		break;
	case WM_RBUTTONUP:
		flag |= SB_RBUTTON;
	case WM_LBUTTONUP:
		ReleaseCapture();
		if (dragging) mouse_event = true;
		dragging = false;
		break;
	case WM_MOUSEMOVE:
		if (dragging) mouse_event = true;
		flag = lastflag;
		break;
	case WM_MBUTTONDOWN:
		SendMessage(hDlg, TBM_SETPOS, (WPARAM)TRUE, 25);
		threater_countrol_proc(control, WM_VSCROLL, SB_RBUTTON, 0);
		break;
	}

	if (mouse_event)
	{
		RECT rc;
		CallWindowProc((WNDPROC)g_OldVolumeProc, hDlg, TBM_GETCHANNELRECT, 0, (LPARAM)&rc);

		POINT point = {LOWORD(lParam), HIWORD(lParam)};


		int nMin = SendMessage(hDlg, TBM_GETRANGEMIN, 0, 0);
		int nMax = SendMessage(hDlg, TBM_GETRANGEMAX, 0, 0);+1;

		if (point.y >= 60000) 
			point.y = rc.left;

		// note: there is a bug in GetChannelRect, it gets the orientation of the rectangle mixed up
		double dPos = (double)(point.y - rc.left)/(rc.right - rc.left);

		int newPos = (int)(nMin + (nMax-nMin)*dPos + 0.5 *(1-dPos) - 0.5 *dPos);

		SendMessage(hDlg, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)newPos);
		SendMessage(control, WM_VSCROLL, flag, (LPARAM)hDlg);

		lastflag = flag;

		return false;
	}

	return 	CallWindowProc((WNDPROC)g_OldVolumeProc,hDlg ,msg, wParam, lParam);   
}

LRESULT CALLBACK ProgressProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WPARAM flag = 0;
	bool mouse_event = false;
	static bool dragging = false;
	switch( msg ) 
	{
	case WM_LBUTTONDOWN:
		flag = SB_THUMBTRACK;
		SetCapture(hDlg);
		SetFocus(hDlg);
		mouse_event = true;
		dragging = true;
		break;
	case WM_LBUTTONUP:
		flag = SB_ENDSCROLL;
		ReleaseCapture();
		if (dragging) mouse_event = true;
		dragging = false;
		break;
	case WM_MOUSEMOVE:
		flag = SB_THUMBTRACK;
		if (dragging) mouse_event = true;
		break;
	}

	if (mouse_event)
	{
		RECT rc;
		SendMessage(hDlg, TBM_GETCHANNELRECT, 0, (LPARAM)&rc);

		POINT point = {LOWORD(lParam), HIWORD(lParam)};


		int nMin = SendMessage(hDlg, TBM_GETRANGEMIN, 0, 0);
		int nMax = SendMessage(hDlg, TBM_GETRANGEMAX, 0, 0);+1;

		if (point.x >= 60000) 
			point.x = rc.left;
		double dPos = (double)(point.x - rc.left)/(rc.right - rc.left);

		int newPos = (int)(nMin + (nMax-nMin)*dPos + 0.5 *(1-dPos) - 0.5 *dPos);

		SendMessage(hDlg, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)newPos);
		SendMessage(control, WM_HSCROLL, flag, (LPARAM)hDlg);

		return false;
	}

	return 	CallWindowProc((WNDPROC)g_OldProgressProc, hDlg ,msg, wParam, lParam);
}

void format_time(int time, wchar_t *out)
{
	int ms = time % 1000;
	int s = (time / 1000) % 60;
	int m = (time / 1000 / 60) % 60;
	int h = (time / 1000 / 3600);

	wsprintfW(out, L"%02d:%02d:%02d.%03d", h, m, s, ms);
}

void format_time_noms(int time, wchar_t *out)
{
	int s = (time / 1000) % 60;
	int m = (time / 1000 / 60) % 60;
	int h = (time / 1000 / 3600);

	wsprintfW(out, L"%02d:%02d:%02d", h, m, s);
}

void format_time2(int current, int total, wchar_t *out)
{
	wchar_t tmp[256];

	format_time(current, tmp);
	wcscpy(out, tmp);
	
	wcscat(out, L" / ");

	format_time(total, tmp);
	wcscat(out, tmp);
}
}
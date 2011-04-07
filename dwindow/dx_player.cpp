#include <math.h>
#include "resource.h"
#include "dx_player.h"
#include "global_funcs.h"
#include "pgs\PGSRenderer.h"
#include "private_filter.h"
#include "srt\srt_renderer.h"
#include "..\libchecksum\libchecksum.h"

#define JIF(x) if (FAILED(hr=(x))){goto CLEANUP;}
#define DS_EVENT (WM_USER + 4)

// helper functions

HRESULT dx_player::CrackPD10(IBaseFilter *filter)
{
	if (!filter)
		return E_POINTER;

	// check if PD10 decoder
	CLSID filter_id;
	filter->GetClassID(&filter_id);
	if (filter_id != CLSID_PD10_DECODER)
		return E_FAIL;

	// query graph builder
	FILTER_INFO fi;
	filter->QueryFilterInfo(&fi);
	if (!fi.pGraph)
		return E_FAIL; // not in a graph
	CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(fi.pGraph);
	fi.pGraph->Release();

	// create source and demuxer and add to graph
	CComPtr<IBaseFilter> h264;
	CComPtr<IBaseFilter> demuxer;
	h264.CoCreateInstance(CLSID_AsyncReader);
	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> h264_control(h264);
	myCreateInstance(CLSID_PD10_DEMUXER, IID_IBaseFilter, (void**)&demuxer);

	if (demuxer == NULL)
		return E_FAIL;	// demuxer not registered

	gb->AddFilter(h264, L"MVC");
	gb->AddFilter(demuxer, L"Demuxer");

	// write active file and load
	unsigned int mvc_data[149] = {0x01000000, 0x29006467, 0x7800d1ac, 0x84e52702, 0xa40f0000, 0x00ee0200, 0x00000010, 0x00806f01, 0x00d1ac29, 0xe5270278, 0x0f000084, 0xee0200a4, 0xaa4a1500, 0xe0f898b2, 0x207d0000, 0x00701700, 0x00000080, 0x63eb6801, 0x0000008b, 0xdd5a6801, 0x0000c0e2, 0x7a680100, 0x00c0e2de, 0x6e010000, 0x00070000, 0x65010000, 0x9f0240b8, 0x1f88f7fe, 0x9c6fcb32, 0x16734a68, 0xc9a57ff0, 0x86ed5c4b, 0xac027e73, 0x0000fca8, 0x03000003, 0x00030000, 0x00000300, 0xb4d40303, 0x696e5f00, 0x70ac954a, 0x00030000, 0x03000300, 0x030000ec, 0x0080ca00, 0x00804600, 0x00e02d00, 0x00401f00, 0x00201900, 0x00401c00, 0x00c01f00, 0x00402600, 0x00404300, 0x00808000, 0x0000c500, 0x00d80103, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00080800, 0x54010000, 0xe0450041, 0xfe9f820c, 0x00802ab5, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0xab010003};
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"ac.mvc");
	FILE *f = _wfopen(tmp, L"wb");
	if(!f)
		return E_FAIL;	// failed writing file
	fwrite(mvc_data,1,596,f);
	fflush(f);
	fclose(f);

	h264_control->Load(tmp, NULL);
	
	// connect source & demuxer
	CComPtr<IPin> h264_o;
	GetUnconnectedPin(h264, PINDIR_OUTPUT, &h264_o);
	CComPtr<IPin> demuxer_i;
	GetUnconnectedPin(demuxer, PINDIR_INPUT, &demuxer_i);
	gb->ConnectDirect(h264_o, demuxer_i, NULL);

	// connect demuxer & decoder
	CComPtr<IPin> demuxer_o;
	GetUnconnectedPin(demuxer, PINDIR_OUTPUT, &demuxer_o);
	CComPtr<IPin> decoder_i;
	GetConnectedPin(filter, PINDIR_INPUT, &decoder_i);
	if (NULL == decoder_i)
		GetUnconnectedPin(filter, PINDIR_INPUT, &decoder_i);
	CComPtr<IPin> decoder_up;
	decoder_i->ConnectedTo(&decoder_up);
	if (decoder_up)
	{
		gb->Disconnect(decoder_i);
		gb->Disconnect(decoder_up);
	}
	gb->ConnectDirect(demuxer_o, decoder_i, NULL);

	// remove source & demuxer, and reconnect decoder(if it is connected before)
	
	gb->RemoveFilter(h264);
	gb->RemoveFilter(demuxer);
	if (decoder_up)gb->ConnectDirect(decoder_up, decoder_i, NULL);

	// delete file
	_wremove(tmp);

	return S_OK;
}


// constructor & destructor
dx_player::dx_player(RECT screen1, RECT screen2, HINSTANCE hExe):
dwindow(screen1, screen2)
{
	// Enable away mode and prevent the sleep idle time-out.
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED);

	// subtitle
	m_lastCBtime = -1;
	m_srenderer = NULL;

	// font
	m_font = NULL;
	m_font_color = RGB(255,255,255);
	m_lFontPointSize = 60;
	wcscpy(m_FontName, L"ËÎÌו");
	wcscpy(m_FontStyle, L"Regular");
	select_font(false);
	m_grenderer.set_font(m_font);
	m_grenderer.set_font_color(m_font_color);

	// vars
	m_file_loaded = false;
	m_renderer1 = NULL;
	m_renderer2 = NULL;
	m_PD10 = false;
	m_select_font_active = false;
	m_log = (wchar_t*)malloc(100000);
	m_log[0] = NULL;
	m_mouse.x = m_mouse.y = -10;
	m_bar_drawer.load_resource(hExe);
	m_revert = false;
	m_mirror1 = 0;
	m_mirror2 = 0;
	m_letterbox_delta = 0.0;
	m_filter_mode = FILTER_MODE_FAIL;
	m_subtitle_center_x = 0.5;
	m_subtitle_bottom_y = 0.95;
	m_subtitle_offset = 0;
	m_loading = false;
	m_hexe = hExe;

	// window size & pos
	int width1 = screen1.right - screen1.left;
	int height1 = screen1.bottom - screen1.top;
	int width2 = screen2.right - screen2.left;
	int height2 = screen2.bottom - screen2.top;

	SetWindowPos(id_to_hwnd(1), NULL, screen1.left, screen1.top, width1, height1, SWP_NOZORDER);

	RECT result;
	GetClientRect(id_to_hwnd(1), &result);

	int dcx = screen1.right - screen1.left - (result.right - result.left);
	int dcy = screen1.bottom - screen1.top - (result.bottom - result.top);

	SetWindowPos(id_to_hwnd(1), NULL, screen1.left + width1/4, screen1.top + height1/4,
					width1/2 + dcx, height1/2 + dcy, SWP_NOZORDER);
	SetWindowPos(id_to_hwnd(2), NULL, screen2.left + width2/4, screen2.top + height2/4,
					width2/2 + dcx, height2/2 + dcy, SWP_NOZORDER);

	// to init video zone
	SendMessage(m_hwnd1, WM_INITDIALOG, 0, 0);
	SendMessage(m_hwnd2, WM_INITDIALOG, 0, 0);


	// set timer for ui drawing
	reset_timer(2, 125);

	// init
	init_direct_show();

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);
}

dx_player::~dx_player()
{
	if (m_log)
	{
		free(m_log);
		m_log = NULL;
	}
	close_and_kill_thread();
	CAutoLock lock(&m_draw_sec);
	exit_direct_show();

	// Clear EXECUTION_STATE flags to disable away mode and allow the system to idle to sleep normally.
	SetThreadExecutionState(ES_CONTINUOUS);
}

HRESULT dx_player::reset()
{
	log_line(L"reset!");
	CAutoLock lock(&m_draw_sec);

	// reinit
	exit_direct_show();
	init_direct_show();
	m_PD10 = false;

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);

	return S_OK;
}

DWORD WINAPI dx_player::select_font_thread(LPVOID lpParame)
{
	dx_player *_this = (dx_player*) lpParame;

	_this->m_select_font_active = true;

	HRESULT hr = E_POINTER;
	{
		CAutoLock lck(&_this->m_subtitle_sec);
		if (_this->m_srenderer)
			hr = _this->m_srenderer->set_font_color(_this->m_font_color);
	}
	if (SUCCEEDED(hr))
	{
		_this->select_font(true);
		CAutoLock lck(&_this->m_subtitle_sec);
		if (_this->m_srenderer)
		{
			_this->m_srenderer->set_font(_this->m_font);
			_this->m_srenderer->set_font_color(_this->m_font_color);
			_this->m_lastCBtime = -1;
		}
	}
	_this->m_select_font_active = false;

	return 0;
}

// play control
HRESULT dx_player::play()
{
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Run();
}
HRESULT dx_player::pause()
{
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	OAFilterState fs;
	m_mc->GetState(INFINITE, &fs);
	if(fs == State_Running)
		return m_mc->Pause();
	else
		return m_mc->Run();
}
HRESULT dx_player::stop()
{
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Stop();
}
HRESULT dx_player::seek(int time)
{
	CAutoLock lock(&m_seek_sec);
	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

	REFERENCE_TIME target = (REFERENCE_TIME)time *10000;

	printf("seeking to %I64d\n", target);
	HRESULT hr = m_ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
	m_ms->GetPositions(&target, NULL);

	printf("seeked to %I64d\n", target);
	return hr;
}
HRESULT dx_player::tell(int *time)
{
	CAutoLock lock(&m_seek_sec);

	if (time == NULL)
		return E_POINTER;

	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

    REFERENCE_TIME current;
	HRESULT hr = m_ms->GetCurrentPosition(&current);
	if(SUCCEEDED(hr))
		*time = (int)(current / 10000);
	return hr;
}
HRESULT dx_player::total(int *time)
{
	CAutoLock lock(&m_seek_sec);
	if (time == NULL)
		return E_POINTER;

	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

	REFERENCE_TIME total;
	HRESULT hr = m_ms->GetDuration(&total);
	if(SUCCEEDED(hr))
		*time = (int)(total / 10000);
	return hr;
}
HRESULT dx_player::set_volume(double volume)
{
	if (m_ba == NULL)
		return VFW_E_WRONG_STATE;

	LONG ddb;
	if (volume>0)
		ddb = (int)(2000 * log10(volume));
	else
		ddb = -10000;
	return m_ba->put_Volume(ddb);
}
HRESULT dx_player::get_volume(double *volume)
{
	if (volume == NULL)
		return E_POINTER;

	if (m_ba == NULL)
		return VFW_E_WRONG_STATE;

	LONG ddb = 0;
	HRESULT hr = m_ba->get_Volume(&ddb);
	if (SUCCEEDED(hr))
		*volume = pow(10.0, (double)ddb/2000);
	return hr;
}

bool dx_player::is_closed()
{
	return !is_visible(1) && !is_visible(2);
}

// window handle part

LRESULT dx_player::on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == DS_EVENT)
	{
		on_dshow_event();
	}
	return dwindow::on_unhandled_msg(id, message, wParam, lParam);
}


LRESULT dx_player::on_display_change()
{
	m_renderer1->DisplayModeChanged();
	m_renderer2->DisplayModeChanged();

	return S_OK;
}

LRESULT dx_player::on_key_down(int id, int key)
{
	switch (key)
	{
	case '1':
		m_mirror1 ++;
		update_video_pos();
		break;

	case '2':
		m_mirror2 ++;
		update_video_pos();
		break;

	case VK_F12:
		// create a new thread to avoid mouse hiding
		if (!m_select_font_active)
			HANDLE show_thread = CreateThread(0,0,select_font_thread, this, NULL, NULL);
		break;

	case VK_SPACE:
		pause();
		break;

	case VK_TAB:
		set_revert(!m_revert);
		break;

	case VK_NUMPAD7:									// image up
		set_letterbox(m_letterbox_delta + 0.05);
		break;

	case VK_NUMPAD1:
		set_letterbox(m_letterbox_delta - 0.05);		// down
		break;

	case VK_NUMPAD5:									// reset position
		set_letterbox(0);
		set_subtitle_pos(0.5, 0.95);
		set_subtitle_offset(0);
		break;

	case VK_NUMPAD4:		//subtitle left
		set_subtitle_pos(m_subtitle_center_x - 0.0025, m_subtitle_bottom_y);
		break;

	case VK_NUMPAD6:		//right
		set_subtitle_pos(m_subtitle_center_x + 0.0025, m_subtitle_bottom_y);
		break;

	case VK_NUMPAD8:		//up
		set_subtitle_pos(m_subtitle_center_x, m_subtitle_bottom_y - 0.0025);
		break;

	case VK_NUMPAD2:		//down
		set_subtitle_pos(m_subtitle_center_x, m_subtitle_bottom_y + 0.0025);
		break;

	case VK_NUMPAD9:
		set_subtitle_offset(m_subtitle_offset + 1);
		break;
		
	case VK_NUMPAD3:
		set_subtitle_offset(m_subtitle_offset - 1);
		break;
	}
	return S_OK;
}

LRESULT dx_player::on_nc_mouse_move(int id, int x, int y)
{
	return on_mouse_move(id, -10, -10);	// just to show ui, should not trigger any other events
}

LRESULT dx_player::on_mouse_move(int id, int x, int y)
{
	POINT mouse;
	GetCursorPos(&mouse);
	if ( (mouse.x - m_mouse.x) * (mouse.x - m_mouse.x) + (mouse.y - m_mouse.y) * (mouse.y - m_mouse.y) >= 100)
	{
		show_mouse(true);
		show_ui(true);
		reset_timer(1, 1000);
	}

	RECT r;
	double v;
	GetClientRect(id_to_hwnd(id), &r);
	m_bar_drawer.total_width = r.right - r.left;
	int type = m_bar_drawer.hit_test(x, y-(r.bottom-r.top)+30, &v);
	if (type == 3 && GetAsyncKeyState(VK_LBUTTON) < 0)
	{
		set_volume(v);
	}
	return S_OK;
}

LRESULT dx_player::on_mouse_down(int id, int button, int x, int y)
{
	show_ui(true);
	show_mouse(true);
	reset_timer(1, 99999999);
	if (button == VK_RBUTTON || !m_file_loaded)
	{
		HMENU menu = LoadMenu(m_hexe, MAKEINTRESOURCE(IDR_MENU1));
		menu = GetSubMenu(menu, 1);


		HMENU sub_open_BD = GetSubMenu(menu, 1);
		// find BD drives
		bool drive_found = false;
		for(int i=L'Z'; i>L'B'; i--)
		{
			wchar_t tmp[MAX_PATH] = L"C:\\";
			wchar_t tmp2[MAX_PATH];
			tmp[0] = i;
			tmp[4] = NULL;
			if (GetDriveTypeW(tmp) == DRIVE_CDROM)
			{
				drive_found = true;
				UINT flag = MF_BYPOSITION | MF_STRING;
				if (!GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
				{
					wcscat(tmp, L" (No Disc)");
					flag |= MF_GRAYED;
				}
				else if (FAILED(find_main_movie(tmp, tmp2)))
				{
					wcscat(tmp, L" (No Movie Disc)");
					flag |= MF_GRAYED;
				}
				else
				{
					GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
					wsprintfW(tmp, L"%s (%s)", tmp, tmp2);
				}
				InsertMenuW(sub_open_BD, 0, flag, i, tmp);
			}
		}
		if (drive_found)
			DeleteMenu(sub_open_BD, ID_OPENBLURAY3D_NONE, MF_BYCOMMAND);
		

		// subtitle menu
		HMENU sub_subtitle = GetSubMenu(menu, 4);
		{
		CAutoLock lck(&m_subtitle_sec);
		if (!m_srenderer || FAILED(m_srenderer->set_font_color(m_font_color)))
		{
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_FONT, MF_GRAYED);
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_COLOR, MF_GRAYED);
		}
		else
		{
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_FONT, MF_ENABLED);
			EnableMenuItem(sub_subtitle, ID_SUBTITLE_COLOR, MF_ENABLED);
		}
		}



		POINT mouse_pos;
		GetCursorPos(&mouse_pos);
		TrackPopupMenu(menu, TPM_TOPALIGN | TPM_LEFTALIGN, mouse_pos.x-5, mouse_pos.y-5, 0, id_to_hwnd(id), NULL);
	}
	else if (button == VK_LBUTTON)
	{
		RECT r;
		GetClientRect(id_to_hwnd(id), &r);
		double v;
		int type;
		
		m_bar_drawer.total_width = r.right - r.left;
		type = m_bar_drawer.hit_test(x, y-(r.bottom-r.top)+30, &v);
		if (type == 1)
		{
			pause();
		}
		else if (type == 2)
		{
			show_window(2, !m_full2);		// show/hide it before set fullscreen, or you may got a strange window
			set_fullscreen(2, !m_full2);		// yeah, this is not a bug
			set_fullscreen(1, m_full2);
			show_window(2, m_full2);		// show/hide it again
		}
		else if (type == 3)
		{
			set_volume(v);
		}
		else if (type == 4)
		{
			int total_time = 0;
			total(&total_time);
			seek((int)(total_time * v));
		}
		else if (type == -1 && !m_full2)
		{
			// move this window
			ReleaseCapture();
			SendMessage(id_to_hwnd(id), WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
	}

	reset_timer(1, 1000);

	return S_OK;
}

LRESULT dx_player::on_timer(int id)
{
	if (id == 1)
	{
		RECT client1, client2;
		POINT mouse1, mouse2;
		GetClientRect(id_to_hwnd(1), &client1);
		GetClientRect(id_to_hwnd(2), &client2);
		GetCursorPos(&mouse1);
		GetCursorPos(&mouse2);
		ScreenToClient(id_to_hwnd(1), &mouse1);
		ScreenToClient(id_to_hwnd(2), &mouse2);

		m_bar_drawer.total_width = client1.right - client1.left;
		int test1 = m_bar_drawer.hit_test(mouse1.x, mouse1.y-(client1.bottom-client1.top)+30, NULL);

		m_bar_drawer.total_width = client2.right - client2.left;
		int test2 = m_bar_drawer.hit_test(mouse2.x, mouse2.y-(client2.bottom-client2.top)+30, NULL);		// warning

		if (test1 < 0 && test2 < 0)
		{
			show_mouse(false);
			show_ui(false);
			GetCursorPos(&m_mouse);
		}
	}

	if (id == 2)
		draw_ui();
	return S_OK;
}

LRESULT dx_player::on_move(int id, int x, int y)
{
	update_video_pos();
	return S_OK;
}

LRESULT dx_player::on_size(int id, int type, int x, int y)
{
	update_video_pos();
	return S_OK;
}

LRESULT dx_player::on_paint(int id, HDC hdc)
{
	if (id == 1)
	{
		CUnifyRenderer *c = m_renderer1;
		if (m_revert) c = m_renderer2;

		if (c)
			c->RepaintVideo(id_to_hwnd(id), hdc);
		else
		{
			RECT client;
			GetClientRect(id_to_hwnd(id), &client);
			FillRect(hdc, &client, (HBRUSH)BLACK_BRUSH+1);
		}
	}

	if (id == 2)
	{
		CUnifyRenderer *c = m_renderer2;
		if (m_revert) c = m_renderer1;
		if (c)
			c->RepaintVideo(id_to_hwnd(id), hdc);
		else
		{
			RECT client;
			GetClientRect(id_to_hwnd(id), &client);
			FillRect(hdc, &client, (HBRUSH)BLACK_BRUSH+1);
		}
	}

	update_video_pos();	// for paint ui and letterbox
	return S_OK;
}

LRESULT dx_player::on_double_click(int id, int button, int x, int y)
{
	show_window(2, !m_full2);		// show/hide it before set fullscreen, or you may got a strange window
	set_fullscreen(2, !m_full2);		// yeah, this is not a bug
	set_fullscreen(1, m_full2);
	show_window(2, m_full2);		// show/hide it again

	update_video_pos();
	return S_OK;
}

LRESULT dx_player::on_getminmaxinfo(int id, MINMAXINFO *info)
{
	info->ptMinTrackSize.x = 300;
	info->ptMinTrackSize.y = 200;

	return S_OK;
}
LRESULT dx_player::on_command(int id, WPARAM wParam, LPARAM lParam)
{
	int uid = LOWORD(wParam);
	show_ui(true);
	show_mouse(true);
	reset_timer(1, 99999999);
	if (uid == ID_OPENFILE)
	{
		wchar_t file[MAX_PATH] = L"";
		if (open_file_dlg(file, id_to_hwnd(1), L"Video files\0"
			L"*.mp4;*.mkv;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d\0"))
		{
			wchar_t * tmp = file;
			on_dropfile(id, 1, &tmp);
		}

	}

	else if (uid == ID_SUBTITLE_FONT)
	{
		if (!m_select_font_active)
			HANDLE show_thread = CreateThread(0,0,select_font_thread, this, NULL, NULL);
	}

	else if (uid == ID_SUBTITLE_COLOR)
	{
		bool ok = select_color(&m_font_color, id_to_hwnd(id));
		CAutoLock lck(&m_subtitle_sec);
		if (m_srenderer && ok)
			m_srenderer->set_font_color(m_font_color);
		m_lastCBtime = -1;
	}

	else if (uid == ID_OPENBDFOLDER)
	{
		wchar_t file[MAX_PATH] = L"";
		if (browse_folder(file, id_to_hwnd(id)))
		{
			wchar_t * tmp = file;
			on_dropfile(id, 1, &tmp);
		}
	}

	else if (uid == ID_PLAY)
	{
		pause();
	}

	else if (uid == ID_FULLSCREEN)
	{
		on_double_click(id, 0, 0, 0);	// should only trigger fullscreen
	}
	else if (uid == ID_EXIT)
	{
		show_window(1, false);
		show_window(2, false);
	}
	else if (uid > 'B' && uid <= 'Z')
	{
		wchar_t tmp[MAX_PATH] = L"C:\\";
		wchar_t *tmp2 = tmp;
		tmp[0] = uid;
		on_dropfile(id, 1, &tmp2);
	}
	reset_timer(1, 1000);
	return S_OK;
}
LRESULT dx_player::on_sys_command(int id, WPARAM wParam, LPARAM lParam)
{
	const int SC_MAXIMIZE2 = 0xF032;
	if (wParam == SC_MAXIMIZE || wParam == SC_MAXIMIZE2)
	{
		show_window(2, !m_full2);		// show/hide it before set fullscreen, or you may got a strange window
		set_fullscreen(1, true);
		set_fullscreen(2, true);
		show_window(2, m_full2);		// show/hide it again
		update_video_pos();
		return S_OK;
	}
	else if (LOWORD(wParam) == SC_SCREENSAVE)
		return 0;

	return S_FALSE;
}

LRESULT dx_player::on_init_dialog(int id, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hinstance = GetModuleHandle(NULL);

	HWND video = CreateWindow(
		"Static",
		"HelloWorld",
		WS_CHILDWINDOW | SS_OWNERDRAW,
		0,
		0,
		100,
		100,
		id_to_hwnd(id),
		(HMENU)NULL,
		hinstance,
		(LPVOID)NULL);
	ShowWindow(video, SW_SHOW);
	SetWindowLongPtr(video, GWL_USERDATA, (LONG_PTR)(dwindow*)this);
	//SetWindowLongPtr(video, GWL_WNDPROC, (LONG_PTR)MainWndProc);

	if (id == 1)
		m_video1 = video;
	else if (id == 2)
		m_video2 = video;
	else 
		return S_OK;

	return S_OK;
}

HWND dx_player::id_to_video(int id)
{
	if (id == 1)
		return m_video1;
	else if (id == 2)
		return m_video2;
	else
		return NULL;
}
int dx_player::video_to_id(HWND video)
{
	if (video == m_video1)
		return 1;
	else if (video == m_video2)
		return 2;
	else
		return 0;
}
int dx_player::hwnd_to_id(HWND hwnd)
{
	if (video_to_id(hwnd))
		return video_to_id(hwnd);
	else
		return __super::hwnd_to_id(hwnd);
}

// directshow part

HRESULT dx_player::init_direct_show()
{
	HRESULT hr = S_OK;

	CoInitialize(NULL);
	JIF(CoCreateInstance(CLSID_FilterGraph , NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&m_gb));
	JIF(m_gb->QueryInterface(IID_IMediaControl,  (void **)&m_mc));
	JIF(m_gb->QueryInterface(IID_IMediaSeeking,  (void **)&m_ms));
	JIF(m_gb->QueryInterface(IID_IBasicAudio,  (void **)&m_ba));

	return S_OK;

CLEANUP:
	exit_direct_show();
	return hr;
}

HRESULT dx_player::exit_direct_show()
{
	if (m_mc)
		m_mc->Stop();
		
	if (m_renderer1)
		{delete m_renderer1; m_renderer1=NULL;}
	if (m_renderer2)
		{delete m_renderer2; m_renderer2=NULL;}
	if (m_stereo)
		m_stereo->SetCallback(NULL);
	if (m_mono1)
		m_mono1->SetCallback(NULL);
	if (m_mono2)
		m_mono2->SetCallback(NULL);

	m_file_loaded = false;
	
	m_stereo = NULL;
	m_mono1 = NULL;
	m_mono2 = NULL;

	m_ba = NULL;
	m_ms = NULL;
	m_mc = NULL;
	m_gb = NULL;

	return S_OK;
}

HRESULT dx_player::SetOffset(REFERENCE_TIME offset)
{
	m_splitter_offset = offset;
	return S_OK;
}

HRESULT dx_player::SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn)
{
	int ms_start = (int)((TimeStart + m_splitter_offset) / 10000);
	int ms_end = (int)((TimeEnd + m_splitter_offset) / 10000);

	// CSubtitleRenderer test
	rendered_subtitle sub;
	HRESULT hr = E_FAIL;
	{
		CAutoLock lck(&m_subtitle_sec);
		if (m_srenderer)
		{
			hr = m_srenderer->get_subtitle(ms_start, &sub, m_lastCBtime);
		}
	}

	{
		m_lastCBtime = ms_start;
		if (S_FALSE == hr)		// same subtitle, ignore
			return S_OK;
		else if (S_OK == hr)	// need to update
		{
			// empty result, clear it
			if( sub.width == 0 || sub.height ==0 || sub.width_pixel==0 || sub.height_pixel == 0 || sub.data == NULL)
			{
				m_renderer1->ClearAlphaBitmap();
				m_renderer2->ClearAlphaBitmap();
				return S_OK;
			}

			// draw it
			else
			{
				// FIXME: assume aspect_screen is always less than aspect_subtitle
				double aspect_screen1 = (double)(m_screen1.right - m_screen1.left)/(m_screen1.bottom - m_screen1.top);
				double aspect_screen2 = (double)(m_screen2.right - m_screen2.left)/(m_screen2.bottom - m_screen2.top);

				double delta1 = (1-aspect_screen1/sub.aspect);
				double delta2 = (1-aspect_screen1/sub.aspect);

				UnifyAlphaBitmap bmp1 = {sub.width_pixel, sub.height_pixel, sub.data, 
					(float)sub.left + (m_subtitle_center_x-0.5),
					(float)sub.top *(1-delta1) + delta1/2 + (m_subtitle_bottom_y-0.95),
					(float)sub.width,
					(float)sub.height * (1-delta1)};

				HRESULT hr = m_renderer1->SetAlphaBitmap(bmp1);
				if (FAILED(hr)) 
				{
					free(sub.data);
					return hr;
				}

				// another eye
				UnifyAlphaBitmap bmp2 = {sub.width_pixel, sub.height_pixel, sub.data, 
					(float)sub.left + (m_subtitle_center_x-0.5),
					(float)sub.top *(1-delta2) + delta2/2 + (m_subtitle_bottom_y-0.95),
					(float)sub.width,
					(float)sub.height * (1-delta2)};
				bmp2.left += (double)m_subtitle_offset*sub.width/sub.width_pixel + sub.delta;
				hr =  m_renderer2->SetAlphaBitmap(bmp2);
				free(sub.data);
				return hr;
			}
		}

	}

	return S_OK;
}


/*
HRESULT dx_player::CheckMediaTypeCB(const CMediaType *inType)
{
	GUID majorType = *inType->Type();
	GUID subType = *inType->Subtype();
	if (majorType != MEDIATYPE_Subtitle)
		return VFW_E_INVALID_MEDIA_TYPE;
	typedef struct SUBTITLEINFO 
	{
		DWORD dwOffset; // size of the structure/pointer to codec init data
		CHAR IsoLang[4]; // three letter lang code + terminating zero
		WCHAR TrackName[256]; // 256 bytes ought to be enough for everyone
	} haali_format;
	haali_format *format = (haali_format*)inType->pbFormat;
	CAutoLock lck(&m_subtitle_sec);
	if (subType == MEDIASUBTYPE_PGS || 
		(subType == GUID_NULL &&inType->FormatLength()>=520 && wcsstr(format->TrackName, L"SUP")))
		m_srenderer = new PGSRenderer();
	else if (subType == MEDIASUBTYPE_UTF8)
		m_srenderer = new CsrtRenderer(m_font, m_font_color);
	else
		return VFW_E_INVALID_MEDIA_TYPE;

	return S_OK;
}

HRESULT dx_player::NewSegmentCB(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		m_srenderer->seek();

	m_subtitle_seg_time = tStart;
	return S_OK;
}

HRESULT dx_player::SampleCB(IMediaSample *pIn)
{
	AM_MEDIA_TYPE *mt = NULL;
	pIn->GetMediaType(&mt);

	REFERENCE_TIME start, end;
	pIn->GetTime(&start, &end);
	BYTE *p = NULL;
	pIn->GetPointer(&p);

	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		m_srenderer->add_data(p, pIn->GetActualDataLength(), (start+m_subtitle_seg_time)/10000, (end+m_subtitle_seg_time)/10000);

	return S_OK;
}
*/

HRESULT dx_player::calculate_movie_rect(RECT *source, RECT *client, RECT *letterboxd, bool ui)
{
	if (source == NULL || client == NULL)
		return E_FAIL;

	RECT letterbox_c;

	memset(&letterbox_c, 0, sizeof(RECT));

	int source_width = source->right - source->left;
	int source_height = source->bottom - source->top;

	if (source_width == 0 || source_height == 0)
	{
		source_width = 1920;
		source_height = 1920*(m_screen1.bottom - m_screen1.top) / (m_screen1.right - m_screen1.left);
	}

    double source_aspect = (double) source_width / source_height;

	int client_width = client->right - client->left;
	int client_height = client->bottom - client->top;
	double client_aspect = (double) client_width / client_height;

	if (client_aspect > source_aspect)
	{
		letterbox_c.left = (int) (client_height * (client_aspect - source_aspect)+0.5);
		letterbox_c.right = letterbox_c.left / 2;
		letterbox_c.left -= letterbox_c.right;

		client->left += letterbox_c.left;
		client->right-= letterbox_c.right;
	}
	else
	{
		letterbox_c.top =  (int) (client_width * (1/client_aspect - 1/source_aspect)+0.5);
		letterbox_c.bottom = letterbox_c.top / 2;
		letterbox_c.top -= letterbox_c.bottom;

		client->top    += letterbox_c.top;
		client->bottom -= letterbox_c.bottom;
	}

	if (ui)
	{
		int delta = max(0, 30-letterbox_c.bottom);
		client->bottom -= delta;
		source->bottom = (int)(source->bottom - (double)delta * source_height / client_height + 0.5);
	}

	if (letterboxd)
		*letterboxd = letterbox_c;
	return S_OK;
}

HRESULT dx_player::paint_letterbox(int id, RECT letterbox)
{
	RECT client;
	HDC hdc = GetDC(id_to_hwnd(id));
	HBRUSH brush = (HBRUSH)BLACK_BRUSH+1;
	
	/*
	//bottom
	GetClientRect(id_to_hwnd(id), &client);
	client.top = client.bottom - letterbox.bottom;
	if (m_show_ui) client.bottom -= 30;	// don't draw here to avoid flash
	if (client.bottom > client.top)
		FillRect(hdc, &client, brush);

	//top
	GetClientRect(id_to_hwnd(id), &client);
	client.bottom = client.top + letterbox.top;
	FillRect(hdc, &client, brush);

	//right
	GetClientRect(id_to_hwnd(id), &client);
	client.left = client.right - letterbox.right;
	if (m_show_ui) client.bottom -= 30;	// don't draw here to avoid flash
	FillRect(hdc, &client, brush);

	//left
	GetClientRect(id_to_hwnd(id), &client);
	client.right = client.left + letterbox.left;
	if (m_show_ui) client.bottom -= 30;	// don't draw here to avoid flash
	FillRect(hdc, &client, brush);
	*/

	ReleaseDC(id_to_hwnd(id), hdc);
	return S_OK;
}

HRESULT dx_player::on_dshow_event()
{
	CComQIPtr<IMediaEvent, &IID_IMediaEvent> event(m_gb);

	long event_code;
	LONG_PTR param1;
	LONG_PTR param2;
	if (FAILED(event->GetEvent(&event_code, &param1, &param2, 0)))
		return S_OK;

	if (event_code == EC_COMPLETE)
	{
		stop();
		seek(0);
	}


	return S_OK;
}

HRESULT dx_player::update_video_pos()
{
	RECT client;
	RECT source = {0,0,0,0};
	RECT letterbox = {0,0,0,0};

	// set window pos
	if (m_renderer1)
	{
		int id = 1;
		if (m_revert) id = 2;

		// move the video zone to proper position
		GetClientRect(id_to_hwnd(id), &client);
		if (m_show_ui)
			client.bottom -= 30;
		MoveWindow(id_to_video(id), 0, 0, client.right - client.left, client.bottom - client.top, TRUE);

		GetClientRect(id_to_hwnd(id), &client);
		if (FAILED(m_renderer1->GetNativeVideoSize(&source.right, &source.bottom, NULL, NULL)) || source.right == source.left || source.bottom == source.top)
		{
			source.right = 1920;
			source.bottom = 1920*(m_screen1.bottom - m_screen1.top) / (m_screen1.right - m_screen1.left);
		};

		m_renderer1->SetVideoPosition(&source, &client);

		// mirror
		UnifyVideoNormalizedRect mirror_rect = {0,0,1,1};
		float tmp = 0.0;
		if (m_mirror1 & 0x1) //mirror horizontal
		{
			tmp = mirror_rect.left;
			mirror_rect.left = mirror_rect.right;
			mirror_rect.right = tmp;
		}
		if (m_mirror1 & 0x2) //mirror horizontal
		{
			tmp = mirror_rect.top;
			mirror_rect.top = mirror_rect.bottom;
			mirror_rect.bottom = tmp;
		}
		m_renderer1->SetOutputRect(0, &mirror_rect);

		// paint letterbox
		paint_letterbox(id, letterbox);
	}

	memset(&source, 0, sizeof(source));
	if (m_renderer2)
	{
		int id = 2;
		if (m_revert) id = 1;

		// move the video zone to proper position
		GetClientRect(id_to_hwnd(id), &client);
		if (m_show_ui)
			client.bottom -= 30;
		MoveWindow(id_to_video(id), 0, 0, client.right - client.left, client.bottom, TRUE);

		GetClientRect(id_to_hwnd(id), &client);
		if (FAILED(m_renderer1->GetNativeVideoSize(&source.right, &source.bottom, NULL, NULL)) || source.right == source.left || source.bottom == source.top)
		{
			source.right = 1920;
			source.bottom = 1920*(m_screen2.bottom - m_screen1.top) / (m_screen2.right - m_screen2.left);
		};
		m_renderer2->SetVideoPosition(&source, &client);

		// mirror
		UnifyVideoNormalizedRect mirror_rect = {0,0,1,1};
		float tmp = 0.0;
		if (m_mirror2 & 0x1) //mirror horizontal
		{
			tmp = mirror_rect.left;
			mirror_rect.left = mirror_rect.right;
			mirror_rect.right = tmp;
		}
		if (m_mirror2 & 0x2) //mirror horizontal
		{
			tmp = mirror_rect.top;
			mirror_rect.top = mirror_rect.bottom;
			mirror_rect.bottom = tmp;
		}
		m_renderer2->SetOutputRect(0, &mirror_rect);

		// paint letterbox
		paint_letterbox(id, letterbox);
	}

	draw_ui();
    
	return S_OK;
}

HRESULT dx_player::set_revert(bool revert)
{
	m_revert = revert;

	if (m_renderer1)
	{
		if (m_revert)
			m_renderer1->SetVideoClippingWindow(id_to_video(2));
		else
			m_renderer1->SetVideoClippingWindow(id_to_video(1));
	}

	if (m_renderer2)
	{
		if (m_revert)
			m_renderer2->SetVideoClippingWindow(id_to_video(1));
		else
			m_renderer2->SetVideoClippingWindow(id_to_video(2));
	}
	update_video_pos();
	return S_OK;
}

HRESULT dx_player::set_letterbox(double delta)
{
	if (m_filter_mode == FILTER_MODE_FAIL)
		return VFW_E_WRONG_STATE;

	if (delta >1)
		delta = 1;
	if (delta <-1)
		delta = -1;

	m_letterbox_delta = delta;

	if (m_filter_mode == FILTER_MODE_MONO)
	{
		int height = 0;
		m_mono1->GetLetterboxHeight(&height);
		m_mono1->SetLetterbox((int)(-height*m_letterbox_delta));

		m_mono2->GetLetterboxHeight(&height);
		m_mono2->SetLetterbox((int)(-height*m_letterbox_delta));
	}

	if (m_filter_mode == FILTER_MODE_STEREO)
	{
		int height = 0;
		m_stereo->GetLetterboxHeight(&height);
		m_stereo->SetLetterbox((int)(-height*m_letterbox_delta));
	}

	return S_OK;
}

HRESULT dx_player::show_ui(bool show)
{
	m_show_ui = show;
	update_video_pos();
	return S_OK;
}

HRESULT dx_player::draw_ui()
{
	CAutoLock lock(&m_draw_sec);

	if (!m_show_ui)
		return S_FALSE;

	int _total = 0, current = 0;
	double volume = 1.0;
	bool paused = true;

	tell(&current);
	total(&_total);
	get_volume(&volume);
	if (m_mc)
	{
		OAFilterState fs;
		HRESULT hr = m_mc->GetState(INFINITE, &fs);
		if (fs == State_Running)
			paused = false;
	}

	for(int id=1; id<=2; id++)
	{
		RECT client_rc;
		GetClientRect(id_to_hwnd(id), &client_rc);
		m_bar_drawer.total_width = client_rc.right - client_rc.left;
		m_bar_drawer.draw_total(paused, current, _total, volume);


		HDC hdc = GetDC(id_to_hwnd(id));
		HDC hdcBmp = CreateCompatibleDC(hdc);
		HBITMAP hbm = CreateCompatibleBitmap(hdc, 4096, 30);
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

		SetBitmapBits(hbm, 4096*30*4, m_bar_drawer.result);
		BitBlt(hdc, client_rc.left, client_rc.bottom-30, m_bar_drawer.total_width, 30, hdcBmp, 0, 0, SRCCOPY);

		DeleteObject(SelectObject(hdcBmp, hbmOld));
		DeleteObject(hbm);
		DeleteDC(hdcBmp);
		ReleaseDC(id_to_hwnd(id), hdc);
	}

	return S_OK;
}

HRESULT dx_player::start_loading()
{
	m_loading = true;
	return S_OK;
}

HRESULT dx_player::on_dropfile(int id, int count, wchar_t **filenames)
{
	if (count>0)
	{
		HRESULT hr = load_subtitle(filenames[0], false);
		if (SUCCEEDED(hr))
			return S_OK;

		reset();
		start_loading();
		load_file(filenames[0]);
		end_loading();
		play();
	}

	return S_OK;
}

HRESULT dx_player::load_file(wchar_t *pathname, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
{
	wchar_t file_to_play[MAX_PATH];
	wcscpy(file_to_play, pathname);

	// Bluray Directory
	log_line(L"start loading %s", file_to_play);
	if(PathIsDirectoryW(file_to_play))
	{
		wchar_t playlist[MAX_PATH];
		HRESULT hr;
		if (FAILED(hr = find_main_movie(file_to_play, playlist)))
		{
			log_line(L"main movie not found for %s, error=0x%08x", file_to_play, hr);
			return hr;
		}
		else
		{
			log_line(L"main movie of %s is %s", file_to_play, playlist);
			wcscpy(file_to_play, playlist);
		}
	}

	// subtitle file
	HRESULT hr = load_subtitle(pathname, false);
	if (SUCCEEDED(hr))
	{
		log_line(L"loaded as subtitle");
		return hr;
	}

	// reset offset
	m_splitter_offset = 0;

	// subtitle renderer
	m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

	// Legacy Remux file
	if (verify_file(file_to_play) == 2)
	{
		log_line(L"loading remux file %s...", file_to_play);
		hr = load_REMUX_file(file_to_play);
		if (SUCCEEDED(hr))
		{
			log_line(L"loaded as remux file");
			m_file_loaded = true;
			return hr;
		}
		else
		{
			log_line(L"loading as remux file failed. error=0x%08x", hr);
			m_file_loaded = false;
			return hr;
		}
	}

	hr = beforeCreateCoreMVC();

	// check private source and whether is MVC content
	CLSID source_clsid;
	hr = GetFileSource(file_to_play, &source_clsid);
	if (SUCCEEDED(hr))
	{
		log_line(L"loading with private filter");
		// private file types
		// create source, load file and join it into graph
		CComPtr<IBaseFilter> source_base;
		hr = myCreateInstance(source_clsid, IID_IBaseFilter, (void**)&source_base);
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source_source(source_base);
		hr = source_source->Load(file_to_play, NULL);
		hr = m_gb->AddFilter(source_base, L"Source");

		// check MVC content, if is, Add a Cracked PD10 Decoder into graph, and set m_PD10=true
		CComQIPtr<IMVC, &IID_IMVC> imvc(source_base);
		if (imvc && imvc->IsMVC() == S_OK)
			{
				/*
				CComPtr<IBaseFilter> pd10_decoder;
				hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10_decoder);
				hr = m_gb->AddFilter(pd10_decoder, L"PD10 Decoder");
				hr = CrackPD10(pd10_decoder);
				imvc->SetPD10(TRUE);
				imvc->SetOffsetSink(this);
				m_PD10 = true;
				*/

				
				CComPtr<IBaseFilter> coremvc;
				hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
				hr = ActiveCoreMVC(coremvc);
				hr = m_gb->AddFilter(coremvc, L"CoreMVC");

				FILTER_INFO fi;
				coremvc->QueryFilterInfo(&fi);
				if (fi.pGraph)
					fi.pGraph->Release();
				else
					log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");

				log_line(L"CoreMVC hr = 0x%08x", hr);
			}

		// then render pins
		CComPtr<IPin> pin;
		CComPtr<IEnumPins> pEnum;
		int audio_num = 0, video_num = 0;
		hr = source_base->EnumPins(&pEnum);
		while (pEnum->Next(1, &pin, NULL) == S_OK)
		{
			PIN_DIRECTION ThisPinDir;
			pin->QueryDirection(&ThisPinDir);
			if (ThisPinDir == PINDIR_OUTPUT)
			{
				CComPtr<IPin> pin_tmp;
				pin->ConnectedTo(&pin_tmp);
				if (pin_tmp)  // Already connected, not the pin we want.
					pin_tmp = NULL;
				else  // Unconnected, this is the pin we may want.
				{
					PIN_INFO info;
					pin->QueryPinInfo(&info);
					if (info.pFilter) info.pFilter->Release();

					if (wcsstr(info.achName, L"Video"))
					{
						if ( (video_track>=0 && (MKV_TRACK_NUMBER(video_num) & video_track ))
							|| video_track == MKV_ALL_TRACK)
							hr = m_gb->Render(pin);
						video_num ++;
					}

					else if (wcsstr(info.achName, L"Audio"))
					{
						if ( (audio_track>=0 && (MKV_TRACK_NUMBER(audio_num) & audio_track ))
							|| audio_track == MKV_ALL_TRACK)
							m_gb->Render(pin);
						audio_num ++;
					}

					else if (wcsstr(info.achName, L"Subtitle"))
					{
						m_gb->Render(pin);
					}

					else;	// other tracks, ignore them
				}
			}
			pin = NULL;
		}

		// if it doesn't work....
		if (video_num == 0)
		{
			log_line(L"private filters failed, trying system filters. (%s)", file_to_play);
			hr = m_gb->RenderFile(file_to_play, NULL);
		}
		else
			hr = S_OK;
	}


	// normal file, just render it.
	else
	{
		log_line(L"private filters failed, trying system filters. (%s)", file_to_play);
		hr = m_gb->RenderFile(file_to_play, NULL);
	}

	// check result
	if (hr == VFW_S_AUDIO_NOT_RENDERED)
		log_line(L"warning: audio not rendered. \"%s\"", file_to_play);

	if (hr == VFW_S_PARTIAL_RENDER)
		log_line(L"warning: Some of the streams in this movie are in an unsupported format. \"%s\"", file_to_play);

	if (hr == VFW_S_VIDEO_NOT_RENDERED)
		log_line(L"warning: video not rendered. \"%s\"", file_to_play);

	if (FAILED(hr))
	{
		log_line(L"failed rendering \"%s\" (error = 0x%08x).", file_to_play, hr);
	}
	else
	{
		log_line(L"load OK.");
		
		m_srenderer = m_grenderer.GetSubtitleRenderer();


		m_file_loaded = true;
		set_window_text(1, file_to_play);
		set_window_text(2, file_to_play);
	}

	return hr;
}

HRESULT dx_player::load_REMUX_file(wchar_t *pathname)
{
	if (pathname == NULL)
		return E_POINTER;

	// create filters and add to graph
	CComPtr<IBaseFilter> decoder;
	myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&decoder);

	CComPtr<IBaseFilter> source_base;
	myCreateInstance(CLSID_SSIFSource, IID_IBaseFilter, (void**)&source_base);
	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source(source_base);

	if (decoder == NULL)
	{
		log_line(L"cyberlink PowerDVD10 module not found, register it first.");
		return E_FAIL;		// cyberlink fail
	}
	if (source_base == NULL || source == NULL)
		return E_FAIL;		// dshow fail

	m_gb->AddFilter(source_base, L"Source");
	m_gb->AddFilter(decoder, L"PD10 decoder");

	// load file
	HRESULT hr = source->Load(pathname, NULL);
	if (FAILED(hr))
		return E_FAIL;		//dshow fail
	
	// render demuxer output pins
	CComPtr<IPin> pin;
	CComPtr<IEnumPins> pEnum;
	hr = source_base->EnumPins(&pEnum);
	if (FAILED(hr))
		return hr;
	while (pEnum->Next(1, &pin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PINDIR_OUTPUT)
		{
			CComPtr<IPin> pin_tmp;
			hr = pin->ConnectedTo(&pin_tmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
				pin_tmp = NULL;
			else  // Unconnected, this is the pin we may want.
			{
				m_gb->Render(pin);
			}
		}
		pin = NULL;
	}
	m_PD10 = true;

	return CrackPD10(decoder);
}

HRESULT dx_player::set_PD10(bool PD10 /*= false*/)
{
	m_PD10 = PD10;

	return S_OK;
}


HRESULT dx_player::end_loading()
{

	int num_renderer_found = 0;

	// enum video renderers
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> filter;
	HRESULT hr = m_gb->EnumFilters(&pEnum);
	if (FAILED(hr))
		return hr;

	while(pEnum->Next(1, &filter, NULL) == S_OK)
	{
		FILTER_INFO filter_info;
		filter->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		if ( wcsstr(filter_info.achName, L"Video Renderer") )
			num_renderer_found ++;

		filter = NULL;
	}
	pEnum = NULL;

	if (num_renderer_found < 1)
	{
		log_line(L"no video stream found.");
		return VFW_E_INVALID_MEDIA_TYPE;
	}

	if (num_renderer_found > 2)
	{
		log_line(L"too many video stream (%d video streams found).", num_renderer_found);
		return VFW_E_INVALID_MEDIA_TYPE;
	}

	CComPtr<IPin> pin1;
	CComPtr<IPin> pin2;

	if (num_renderer_found == 1)
		end_loading_sidebyside(&pin1, &pin2);

	if (num_renderer_found == 2)
		end_loading_double_stream(&pin1, &pin2);

	if (pin1 == NULL || pin2 == NULL)
	{
		log_line(L"pin connection failed.");
		return E_FAIL;
	}

	if ( FAILED(hr = end_loading_step2(pin1, pin2)))
	{
		log_line(L"failed connect dwindow stereo filter with renderer.(%08x)", hr);
		return hr;
	}
	m_loading = false;
	return S_OK;
}

HRESULT dx_player::end_loading_sidebyside(IPin **pin1, IPin **pin2)
{
	m_filter_mode = FILTER_MODE_STEREO;
	*pin1 = NULL;
	*pin2 = NULL;

	// create stereo splitter filter
	if (FAILED(create_myfilter()))
	{
		log_line(L"DWindow Splitter not registered.");
		return E_FAIL;
	}

	m_stereo->SetCallback(this);
	double aspect = (double)(m_screen1.right - m_screen1.left) / (m_screen2.bottom - m_screen2.top);
	if (m_PD10)
		m_stereo->SetMode(DWindowFilter_CUT_MODE_PD10, DWindowFilter_EXTEND_CUSTOM_DECIMAL(aspect));
	else
		m_stereo->SetMode(DWindowFilter_CUT_MODE_AUTO, DWindowFilter_EXTEND_CUSTOM_DECIMAL(aspect));



	CComQIPtr<IBaseFilter, &IID_IBaseFilter> stereo_base(m_stereo);
	m_gb->AddFilter(stereo_base, L"the Splitter");

	// find the only renderer and replace it
	bool hsbs_tested = false;
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> renderer;
	HRESULT hr = m_gb->EnumFilters(&pEnum);
	ULONG fetched = 0;
	while(pEnum->Next(1, &renderer, &fetched) == S_OK)
	{
		FILTER_INFO filter_info;
		renderer->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		if ( wcsstr(filter_info.achName, L"Video Renderer") )
		{
			CComPtr<IPin> video_renderer_input;
			CComPtr<IPin> output;
			GetConnectedPin(renderer, PINDIR_INPUT, &video_renderer_input);
			video_renderer_input->ConnectedTo(&output);
			m_gb->RemoveFilter(renderer);

			// avisynth support
			// remove "AVI Decompressor" and "Color Space Converter"
avisynth:
			PIN_INFO pin_info;
			output->QueryPinInfo(&pin_info);
			FILTER_INFO filter_info;
			pin_info.pFilter->QueryFilterInfo(&filter_info);
			if (wcsstr(filter_info.achName, L"AVI Decompressor") || wcsstr(filter_info.achName, L"Color Space Converter"))
			{
				CComPtr<IPin> avi_de_input;
				GetConnectedPin(pin_info.pFilter, PINDIR_INPUT, &avi_de_input);
				output = NULL;
				avi_de_input->ConnectedTo(&output);
				m_gb->RemoveFilter(pin_info.pFilter);

				filter_info.pGraph->Release();
				pin_info.pFilter->Release();

				wprintf(L"%s removed.\n", filter_info.achName);
				goto avisynth;
			}
			filter_info.pGraph->Release();
			pin_info.pFilter->Release();

			// connect them
			CComPtr<IPin> stereo_pin;
			GetUnconnectedPin(stereo_base, PINDIR_INPUT, &stereo_pin);
test_hsbs:
			m_gb->ConnectDirect(output, stereo_pin, NULL);
			
			CComPtr<IPin> check;
			stereo_pin->ConnectedTo(&check);
			if (check == NULL)
			{
				if (!hsbs_tested)
				{
					log_line(L"trying half SBS.");
					hsbs_tested = true;
					m_stereo->SetMode(DWindowFilter_CUT_MODE_LEFT_RIGHT_HALF, DWindowFilter_EXTEND_CUSTOM_DECIMAL(aspect));

					FILE *f = fopen("lr.txt", "rb");
					if (f)
					{
						m_stereo->SetMode(DWindowFilter_CUT_MODE_TOP_BOTTOM_HALF, DWindowFilter_EXTEND_CUSTOM_DECIMAL(aspect));
						fclose(f);
					}
					goto test_hsbs;
				}
				log_line(L"input format not supported(half-width or half-height is not supported).");
				return VFW_E_INVALID_MEDIA_TYPE;			// not connected
			}

			break;
		}
		renderer = NULL;
	}
	pEnum = NULL;

	// find stereo output pins
	CComPtr<IEnumPins> pEnumPin;
	CComPtr<IPin> pin;
	hr = stereo_base->EnumPins(&pEnumPin);
	if (FAILED(hr))
		return hr;
	while (pEnumPin->Next(1, &pin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PINDIR_OUTPUT)
		{
			PIN_INFO info;
			pin->QueryPinInfo(&info);
			if (info.pFilter) info.pFilter->Release();

			if (wcsstr(info.achName, L"1"))
				*pin1 = pin;
			else if (wcsstr(info.achName, L"2"))
				*pin2 = pin;
			((IPin*)pin)->AddRef();						//add ref...
		}
		pin = NULL;
	}
	return S_OK;
}

HRESULT dx_player::end_loading_double_stream(IPin **pin1, IPin **pin2)
{
	m_filter_mode = FILTER_MODE_MONO;
	*pin1 = NULL;
	*pin2 = NULL;

	if (FAILED(create_myfilter()))
	{
		log_line(L"DWindow Splitter not registered.");
		return E_FAIL;
	}

	m_mono1->SetCallback(this);							// set callback
	
	// set aspect
	double aspect1 = (double)(m_screen1.right - m_screen1.left) / (m_screen2.bottom - m_screen2.top);
	double aspect2 = (double)(m_screen1.right - m_screen1.left) / (m_screen2.bottom - m_screen2.top);
	m_mono1->SetMode(DWindowFilter_CUT_MODE_AUTO, DWindowFilter_EXTEND_CUSTOM_DECIMAL(aspect1));
	m_mono2->SetMode(DWindowFilter_CUT_MODE_AUTO, DWindowFilter_EXTEND_CUSTOM_DECIMAL(aspect2));


	CComQIPtr<IBaseFilter, &IID_IBaseFilter> mono1base(m_mono1);
	CComQIPtr<IBaseFilter, &IID_IBaseFilter> mono2base(m_mono2);

	m_gb->AddFilter(mono1base, L"mono 1");
	m_gb->AddFilter(mono2base, L"mono 2");


	// find renderers and replace it
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> renderer;
restart:
	renderer = NULL;
	pEnum = NULL;
	HRESULT hr = m_gb->EnumFilters(&pEnum);
	ULONG fetched = 0;
	while(pEnum->Next(1, &renderer, &fetched) == S_OK)
	{
		FILTER_INFO filter_info;
		renderer->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		if ( wcsstr(filter_info.achName, L"Video Renderer") )
		{
			CComPtr<IPin> renderer_input;
			CComPtr<IPin> output;
			GetConnectedPin(renderer, PINDIR_INPUT, &renderer_input);
			renderer_input->ConnectedTo(&output);							//TODO: possible fail
			m_gb->RemoveFilter(renderer);

			// avisynth support
			avisynth:
			PIN_INFO pin_info;
			output->QueryPinInfo(&pin_info);
			FILTER_INFO filter_info;
			pin_info.pFilter->QueryFilterInfo(&filter_info);
			if (wcsstr(filter_info.achName, L"AVI Decompressor") || wcsstr(filter_info.achName, L"Color Space Converter"))
			{
				CComPtr<IPin> avi_de_input;
				GetConnectedPin(pin_info.pFilter, PINDIR_INPUT, &avi_de_input);
				output = NULL;
				avi_de_input->ConnectedTo(&output);
				m_gb->RemoveFilter(pin_info.pFilter);

				filter_info.pGraph->Release();
				pin_info.pFilter->Release();

				wprintf(L"%s removed.\n", filter_info.achName);
				goto avisynth;
			}
			filter_info.pGraph->Release();
			pin_info.pFilter->Release();


			CComPtr<IPin> mono1_pin;
			CComPtr<IPin> mono2_pin;
			GetUnconnectedPin(mono1base, PINDIR_INPUT, &mono1_pin);
			GetUnconnectedPin(mono2base, PINDIR_INPUT, &mono2_pin);

			if (mono1_pin != NULL)
			{
				m_gb->ConnectDirect(output, mono1_pin, NULL);
				GetUnconnectedPin(mono1base, PINDIR_OUTPUT, pin1);

				goto restart;
			}
			else if (mono2_pin != NULL)
			{
				m_gb->ConnectDirect(output, mono2_pin, NULL);
				GetUnconnectedPin(mono2base, PINDIR_OUTPUT, pin2);
				break;
			}
		}
		renderer = NULL;
	}
	return S_OK;
}


HRESULT dx_player::create_myfilter()
{
	HRESULT hr = S_OK;
	if (m_filter_mode == FILTER_MODE_FAIL)
		return E_UNEXPECTED;

	if (m_filter_mode == FILTER_MODE_MONO)
	{
		hr = myCreateInstance(CLSID_DWindowMono, IID_IDWindowExtender, (void**)&m_mono1);
		hr = myCreateInstance(CLSID_DWindowMono, IID_IDWindowExtender, (void**)&m_mono2);

		if (m_mono1 == NULL || m_mono2 == NULL)
			hr = E_FAIL;
	}

	if (m_filter_mode == FILTER_MODE_STEREO)
	{
		hr = myCreateInstance(CLSID_DWindowStereo, IID_IDWindowExtender, (void**)&m_stereo);

		if (m_stereo == NULL)
			hr = E_FAIL;
	}

	return hr;
}


HRESULT dx_player::end_loading_step2(IPin *pin1, IPin *pin2)
{
	if (pin1 == NULL || pin2 == NULL)
		return E_FAIL;

	// create, config, and add vmr9 #1 & #2
	//m_renderer1 = new CVMR7Windowless(m_hwnd1);
	//m_renderer2 = new CVMR7Windowless(m_hwnd2);

	//m_renderer1 = new CVMR9Windowless(m_hwnd1);
	//m_renderer2 = new CVMR9Windowless(m_hwnd2);

	
	m_renderer1 = new CEVRVista(m_hwnd1);
	if (m_renderer1->base_filter == NULL)
	{
		// no EVR, possible XP
		delete m_renderer1;
		m_renderer1 = new CVMR9Windowless(m_hwnd1);
		m_renderer2 = new CVMR9Windowless(m_hwnd2);
	}
	else
		m_renderer2 = new CEVRVista(m_hwnd2);
	

	m_gb->AddFilter(m_renderer1->base_filter, L"Renderer #1");
	m_gb->AddFilter(m_renderer2->base_filter, L"Renderer #2");

	// config output window
	m_renderer1->SetAspectRatioMode(UnifyARMode_LetterBox);
	m_renderer2->SetAspectRatioMode(UnifyARMode_LetterBox);
	set_revert(m_revert);

	// connect pins
	CComPtr<IPin> vmr1pin;
	CComPtr<IPin> vmr2pin;

	GetUnconnectedPin(m_renderer1->base_filter, PINDIR_INPUT, &vmr1pin);
	GetUnconnectedPin(m_renderer2->base_filter, PINDIR_INPUT, &vmr2pin);

	if (vmr1pin == NULL || vmr2pin == NULL)
		return E_FAIL;

	HRESULT hr = m_gb->ConnectDirect(pin1, vmr1pin, NULL);
	if (FAILED(hr))
		return hr;

	hr = m_gb->ConnectDirect(pin2, vmr2pin, NULL);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

// font/d3d/subtitle part
HRESULT dx_player::load_subtitle(wchar_t *pathname, bool reset)			//FIXME : always reset 
{
	if (pathname == NULL)
		return E_POINTER;

	CAutoLock lck(&m_subtitle_sec);
	if (reset && m_srenderer)
		m_srenderer->reset();

	if (m_srenderer)
	{
		delete m_srenderer;
		m_srenderer = NULL;
	}

	wchar_t *p_3 = pathname + wcslen(pathname) -3;
	if ( (p_3[0] == L's' || p_3[0] == L'S') &&
		(p_3[1] == L'r' || p_3[1] == L'R') &&
		(p_3[2] == L't' || p_3[2] == L'T'))
	{
		m_srenderer = new CsrtRenderer(m_font, m_font_color);
	}
	else if ( (p_3[0] == L's' || p_3[0] == L'S') &&
		(p_3[1] == L'u' || p_3[1] == L'U') &&
		(p_3[2] == L'p' || p_3[2] == L'P'))
	{
		m_srenderer = new PGSRenderer();
	}
	else
	{
		return E_NOTIMPL;
	}

	m_srenderer->load_file(pathname);
	m_lastCBtime = -1;

	return S_OK;
}

HRESULT dx_player::draw_subtitle()
{
	// force refresh on next frame
	m_lastCBtime = -1;

	if (m_mc)
	{
		OAFilterState fs;
		HRESULT hr = m_mc->GetState(50, &fs);
		if (SUCCEEDED(hr) && fs != State_Running)
			m_mc->StopWhenReady();
	}
	return S_OK;
}

HRESULT dx_player::set_subtitle_offset(int offset)
{
	m_subtitle_offset = offset;
	draw_subtitle();
	return S_OK;
}

HRESULT dx_player::set_subtitle_pos(double center_x, double bottom_y)
{
	m_subtitle_center_x = center_x;
	m_subtitle_bottom_y = bottom_y;
	return set_subtitle_offset(m_subtitle_offset);	// this is for m_mc->StopWhenReady();
}

HRESULT dx_player::log_line(wchar_t *format, ...)
{
	wcscat(m_log, L"\r\n");

	wchar_t tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	wprintf(L"log: %s\n", tmp);
	wcscat(m_log, tmp);
	return S_OK;
}

HRESULT dx_player::select_font(bool show_dlg)
{
	if (!show_dlg && m_font)
		return S_OK;

	CHOOSEFONTW cf={0};
	memset(&cf, 0, sizeof(cf));
	LOGFONTW lf={0}; 
	HDC hdc;
	LONG lHeight;

	// Convert requested font point size to logical units
	hdc = GetDC( NULL );
	lHeight = -MulDiv( m_lFontPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
	ReleaseDC( NULL, hdc );

	// Initialize members of the LOGFONT structure. 
	lstrcpynW(lf.lfFaceName, m_FontName, 32);
	lf.lfHeight = lHeight;      // Logical units
	lf.lfQuality = ANTIALIASED_QUALITY;

	// Initialize members of the CHOOSEFONT structure. 
	cf.lStructSize = sizeof(CHOOSEFONT); 
	cf.hwndOwner   = NULL; 
	cf.hDC         = (HDC)NULL; 
	cf.lpLogFont   = &lf; 
	cf.iPointSize  = m_lFontPointSize * 10; 
	cf.rgbColors   = m_font_color; 
	cf.lCustData   = 0L; 
	cf.lpfnHook    = (LPCFHOOKPROC)NULL; 
	cf.hInstance   = (HINSTANCE) NULL; 
	cf.lpszStyle   = m_FontStyle; 
	cf.nFontType   = SCREEN_FONTTYPE; 
	cf.nSizeMin    = 0; 
	cf.lpTemplateName = NULL; 
	cf.nSizeMax = 720; 
	cf.Flags = CF_SCREENFONTS | CF_SCALABLEONLY | CF_INITTOLOGFONTSTRUCT | 
		CF_EFFECTS     | CF_USESTYLE     | CF_LIMITSIZE; 

	if (show_dlg)
		ChooseFontW(&cf);

	lstrcpynW(m_FontName, lf.lfFaceName, sizeof(m_FontName)/sizeof(TCHAR));
	m_lFontPointSize = cf.iPointSize / 10;  // Specified in 1/10 point units
	//m_font_color = cf.rgbColors;
	m_font = CreateFontIndirectW(cf.lpLogFont); 

	return S_OK;
}
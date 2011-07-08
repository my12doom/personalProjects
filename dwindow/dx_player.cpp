#include <math.h>
#include "resource.h"
#include "dx_player.h"
#include "global_funcs.h"
#include "private_filter.h"
#include "..\libchecksum\libchecksum.h"
#include "..\AESFile\E3DReader.h"
#include "latency_dialog.h"

#define JIF(x) if (FAILED(hr=(x))){goto CLEANUP;}
#define DS_EVENT (WM_USER + 4)

#include "bomb_network.h"

// helper functions

DWORD color_GDI2ARGB(DWORD in)
{
	BYTE tmp[5];
	*((DWORD*)tmp) = in;
	tmp[3] = 0xff;

	tmp[4] = tmp[0];
	tmp[0] = tmp[2];
	tmp[2] = tmp[4];

	return *((DWORD*)tmp);
}

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
m_renderer1(NULL),
dwindow(screen1, screen2),
m_lFontPointSize(L"FontSize", 40),
m_FontName(L"Font", L"Arial"),
m_FontStyle(L"FontStyle", L"Regular"),
m_font_color(L"FontColor", 0x00ffffff),
m_input_layout(L"InputLayout", input_layout_auto),
m_output_mode(L"OutputMode", anaglyph),
m_mask_mode(L"MaskMode", row_interlace),
m_display_subtitle(L"DisplaySubtitle", true),
m_anaglygh_left_color(L"AnaglyghLeftColor", RGB(255,0,0)),
m_anaglygh_right_color(L"AnaglyghRightColor", RGB(0,255,255)),
m_volume(L"Volume", 1.0),
m_aspect(L"Aspect", -1),
m_subtitle_latency(L"SubtitleLatency", 0),
m_subtitle_ratio(L"SubtitleRatio", 1.0),
m_bitstreaming(L"BitStreaming", false)
{
	// Enable away mode and prevent the sleep idle time-out.
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED);

	// font init
	select_font(false);
	m_grenderer.set_font(m_font);
	m_grenderer.set_font_color(m_font_color);

	// subtitle
	m_lastCBtime = -1;
	m_srenderer = NULL;

	// vars
	m_reset_and_load = false;
	m_file_loaded = false;
	m_select_font_active = false;
	m_log = (wchar_t*)malloc(100000);
	m_log[0] = NULL;
	m_mouse.x = m_mouse.y = -10;
	m_bar_drawer.load_resource(hExe);
	m_revert = false;
	m_mirror1 = 0;
	m_mirror2 = 0;
	m_letterbox_delta = 0.0;
	m_parallax = 0;
	m_subtitle_center_x = 0.5;
	m_subtitle_bottom_y = 0.95;
	m_hexe = hExe;

	// window size & pos
	int width1 = screen1.right - screen1.left;
	int height1 = screen1.bottom - screen1.top;
	int width2 = screen2.right - screen2.left;
	int height2 = screen2.bottom - screen2.top;
	m_user_offset = 0;
	m_internel_offset = 10; // offset set to 10*0.1% of width

	SetWindowPos(id_to_hwnd(1), NULL, screen1.left, screen1.top, width1, height1, SWP_NOZORDER);

	RECT result;
	GetClientRect(id_to_hwnd(1), &result);

	int dcx = screen1.right - screen1.left - (result.right - result.left);
	int dcy = screen1.bottom - screen1.top - (result.bottom - result.top);

	SetWindowPos(id_to_hwnd(1), NULL, screen1.left + width1/4, screen1.top + height1/4,
					width1/2 + dcx, height1/2 + dcy, SWP_NOZORDER);
	SetWindowPos(id_to_hwnd(2), NULL, screen2.left + width2/4, screen2.top + height2/4,
					width2/2 + dcx, height2/2 + dcy, SWP_NOZORDER);


	// show it!
	show_window(1, true);

	// to init video zone
	SendMessage(m_hwnd1, WM_INITDIALOG, 0, 0);
	SendMessage(m_hwnd2, WM_INITDIALOG, 0, 0);

	// set timer for ui drawing
	reset_timer(2, 125);

	// init dshow
	init_direct_show();

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);

	// network bomb thread
	CreateThread(0,0,bomb_network_thread, id_to_hwnd(1), NULL, NULL);
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
	delete m_renderer1;

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
	CAutoLock lck(&m_subtitle_sec);
	m_srenderer = NULL;
	m_external_subtitles.RemoveAll();

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);

	// caption
	set_window_text(1, L"");
	set_window_text(2, L"");
	
	// repaint
	InvalidateRect(m_hwnd1, NULL, FALSE);

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

	if(m_renderer1) m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0);				// refresh subtitle on next frame
	printf("seeking to %I64d\n", target);
	HRESULT hr = m_ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
	m_ms->GetPositions(&target, NULL);

	printf("seeked to %I64d\n", target);
	m_lastCBtime = (REFERENCE_TIME)time * 10000;

	OAFilterState state = State_Running;
	m_mc->GetState(500, &state);
	if (state != State_Running)
	{
		m_mc->Run();
		m_mc->StopWhenReady();
	}

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
	m_volume = volume;
	if (m_ba)
	{
		LONG ddb;
		if (volume>0)
			ddb = (int)(2000 * log10(volume));
		else
			ddb = -10000;
		m_ba->put_Volume(ddb);
	}

	if (m_renderer1)
		m_renderer1->m_volume = m_volume;
	return S_OK;
}
HRESULT dx_player::get_volume(double *volume)
{
	if (volume == NULL)
		return E_POINTER;

	*volume = m_volume;
	return S_OK;
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

	else if (message == WM_COPYDATA)
	{
		COPYDATASTRUCT *copy = (COPYDATASTRUCT*) lParam;
		wchar_t next_to_load[MAX_PATH];
		memcpy(next_to_load, copy->lpData, min(MAX_PATH*2, copy->cbData));
		next_to_load[MAX_PATH-1] = NULL;

		if (copy->dwData == WM_LOADFILE)
			reset_and_loadfile_internal(next_to_load);
	}


	else if (message == WM_NV_NOTIFY)
	{
		if (m_renderer1)
			m_renderer1->NV3D_notify(wParam);
	}
	return dwindow::on_unhandled_msg(id, message, wParam, lParam);
}


LRESULT dx_player::on_display_change()
{
	return S_OK;
}

LRESULT dx_player::on_key_down(int id, int key)
{
	switch (key)
	{
	case '1':
		m_mirror1 ++;
		break;

	case '2':
		m_mirror2 ++;
		break;

	case VK_SPACE:
		pause();
		break;

	case VK_LEFT:
		{
			int t;
			tell(&t);
			seek(max(0, t-5000));
		}
		break;

	case VK_RIGHT:
		{
			int t, mt;
			total(&mt);
			tell(&t);
			seek(min(t+5000, mt));
		}
		break;

	case VK_ESCAPE:
		if (m_full1 || (m_renderer1 && m_renderer1->get_fullscreen()))
			toggle_fullscreen();
		break;

	case VK_TAB:
		set_revert(!m_revert);
		break;

	case VK_NUMPAD7:									// image up
		set_letterbox(m_letterbox_delta - 0.005);
		break;

	case VK_NUMPAD1:
		set_letterbox(m_letterbox_delta + 0.005);		// down
		break;

	case VK_NUMPAD5:									// reset position
		set_letterbox(0);
		set_subtitle_pos(0.5, 0.95);
		set_subtitle_offset(0);
		m_parallax = 0;
		if (m_renderer1) m_renderer1->set_parallax(m_parallax);
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
		set_subtitle_offset(m_user_offset + 1);
		break;
		
	case VK_NUMPAD3:
		set_subtitle_offset(m_user_offset - 1);
		break;

	case VK_MULTIPLY:
		m_parallax += 0.001;
		if (m_renderer1)m_renderer1->set_parallax(m_parallax);
		break;

	case VK_DIVIDE:
		m_parallax -= 0.001;
		if (m_renderer1)m_renderer1->set_parallax(m_parallax);
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
		reset_timer(1, 2000);
	}

	RECT r;
	double v;
	GetClientRect(id_to_hwnd(id), &r);
	m_bar_drawer.total_width = r.right - r.left;
	int height = r.bottom - r.top;
	int hit_x = x;
	int hit_y = y;

	if (m_output_mode == out_tb)
	{
		if (hit_y < height/2)
			hit_y += height/2;
	}

	if (m_output_mode == out_htb)
	{
		if (hit_y < height/2)
			hit_y *= 2;
	}

	if (m_output_mode == out_sbs)
	{
		m_bar_drawer.total_width /= 2;
		hit_x %= m_bar_drawer.total_width;
	}

	if (m_output_mode == out_hsbs)
	{
		hit_x *= 2;
		hit_x %= m_bar_drawer.total_width;
	}

	int type = -1;
	if (m_renderer1) type = m_renderer1->hittest(hit_x, hit_y, &v);
	if (type == hit_volume && GetAsyncKeyState(VK_LBUTTON) < 0)
	{
		set_volume(v);
	}
	return S_OK;
}


LRESULT dx_player::on_mouse_up(int id, int button, int x, int y)
{

	return S_OK;
}

LRESULT dx_player::on_mouse_down(int id, int button, int x, int y)
{
	if (!m_gb)
		return __super::on_mouse_down(id, button, x, y);
	show_ui(true);
	show_mouse(true);
	reset_timer(1, 99999999);
	if ( (button == VK_RBUTTON || (!m_file_loaded && m_renderer1 && m_renderer1->hittest(x, y, NULL) == -2) && 
		(!m_renderer1 || !m_renderer1->get_fullscreen())))
	{

		HMENU menu = LoadMenu(m_hexe, MAKEINTRESOURCE(IDR_MENU1));
		menu = GetSubMenu(menu, 0);
		localize_menu(menu);

		// disable output mode when fullscreen
		if (m_full1 || (m_renderer1 ? m_renderer1->get_fullscreen() : false))
		{
			HMENU video = GetSubMenu(menu, 4);
			ModifyMenuW(video, 1, MF_BYPOSITION | MF_GRAYED, ID_PLAY, C(L"Output Mode"));
		}


		// play / pause
		bool paused = true;
		if (m_mc)
		{
			static OAFilterState fs = State_Running;
			HRESULT hr = m_mc->GetState(100, &fs);
			if (fs == State_Running)
				paused = false;
		}
		int flag = GetMenuState(menu, ID_PLAY, MF_BYCOMMAND);
		ModifyMenuW(menu, ID_PLAY, MF_BYCOMMAND| flag, ID_PLAY, paused ? C(L"Play\t(Space)") : C(L"Pause\t(Space)"));

		// find BD drives
		HMENU sub_open_BD = GetSubMenu(menu, 1);
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
					wcscat(tmp, C(L" (No Disc)"));
					flag |= MF_GRAYED;
				}
				else if (FAILED(find_main_movie(tmp, tmp2)))
				{
					wcscat(tmp, C(L" (No Movie Disc)"));
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

		// input mode
		CheckMenuItem(menu, ID_INPUTLAYOUT_AUTO,		m_input_layout == input_layout_auto ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_INPUTLAYOUT_SIDEBYSIDE,	m_input_layout == side_by_side ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_INPUTLAYOUT_TOPBOTTOM,	m_input_layout == top_bottom ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_INPUTLAYOUT_MONOSCOPIC,	m_input_layout == mono2d ? MF_CHECKED:MF_UNCHECKED);

		// output mode
		CheckMenuItem(menu, ID_OUTPUTMODE_NVIDIA3DVISION,		m_output_mode == NV3D ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_MONOSCOPIC2D,			m_output_mode == mono ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_ROWINTERLACE,			m_output_mode == masking && m_mask_mode == row_interlace ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_LINEINTERLACE,		m_output_mode == masking && m_mask_mode == line_interlace? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_CHECKBOARDINTERLACE,	m_output_mode == masking && m_mask_mode == checkboard_interlace ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_DUALPROJECTOR,		m_output_mode == dual_window ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_DUALPROJECTOR_SBS,	m_output_mode == out_sbs ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_DUALPROJECTOR_TB,		m_output_mode == out_tb ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_GERNERAL120HZGLASSES,	m_output_mode == pageflipping ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_3DTV_SBS,				m_output_mode == out_hsbs ? MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(menu, ID_OUTPUTMODE_3DTV_TB,				m_output_mode == out_htb ? MF_CHECKED:MF_UNCHECKED);

		if (m_output_mode == anaglyph)
		{
			if (m_anaglygh_left_color == RGB(255, 0, 0) && m_anaglygh_right_color == RGB(0, 255, 255))
				CheckMenuItem(menu, ID_ANAGLYPH_REDCYAN, MF_CHECKED);
			else
				CheckMenuItem(menu, ID_ANAGLYPH_CUSTOMCOLOR, MF_CHECKED);

		}

		// Aspect Ration
		if (m_aspect == -1) CheckMenuItem(menu, ID_ASPECTRATIO_DEFAULT, MF_CHECKED);
		if (m_aspect == (double)16/9) CheckMenuItem(menu, ID_ASPECTRATIO_169, MF_CHECKED);
		if (m_aspect == (double)4/3) CheckMenuItem(menu, ID_ASPECTRATIO_43, MF_CHECKED);

		// subtitle menu
		CheckMenuItem(menu, ID_SUBTITLE_DISPLAYSUBTITLE, MF_BYCOMMAND | (m_display_subtitle ? MF_CHECKED : MF_UNCHECKED));
		HMENU sub_subtitle = GetSubMenu(menu, 6);
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


		// audio tracks
		CheckMenuItem(menu, ID_AUDIO_BITSTREAM, MF_BYCOMMAND | (m_bitstreaming?MF_CHECKED : MF_UNCHECKED));
		HMENU sub_audio = GetSubMenu(menu, 5);
		list_audio_track(sub_audio);

		// subtitle tracks
		list_subtitle_track(sub_subtitle);

		// language
		if (g_active_language == ENGLISH)
			CheckMenuItem(menu, ID_LANGUAGE_ENGLISH, MF_CHECKED | MF_BYCOMMAND);
		else if (g_active_language == CHINESE)
			CheckMenuItem(menu, ID_LANGUAGE_CHINESE, MF_CHECKED | MF_BYCOMMAND);

		// swap
		CheckMenuItem(menu, ID_SWAPEYES, m_revert ? MF_CHECKED:MF_UNCHECKED);

		// CUDA
		CheckMenuItem(menu, ID_CUDA, g_CUDA ? MF_CHECKED:MF_UNCHECKED);


		// show it
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
		int height = r.bottom - r.top;
		int hit_x = x;
		int hit_y = y;

		if (m_output_mode == out_tb)
		{
			if (hit_y < height/2)
				hit_y += height/2;
		}

		if (m_output_mode == out_htb)
		{
			if (hit_y < height/2)
				hit_y *= 2;
		}

		if (m_output_mode == out_sbs)
		{
			m_bar_drawer.total_width /= 2;
			hit_x %= m_bar_drawer.total_width;
		}

		if (m_output_mode == out_hsbs)
		{
			hit_x *= 2;
			hit_x %= m_bar_drawer.total_width;
		}

		type = -1;
		if (m_renderer1) type = m_renderer1->hittest(hit_x, hit_y, &v);

		if (type == hit_play)
		{
			pause();
		}
		else if (type == hit_full)
		{
			toggle_fullscreen();
		}
		else if (type == hit_volume)
		{
			set_volume(v);
		}
		else if (type == hit_progress)
		{
			int total_time = 0;
			total(&total_time);
			seek((int)(total_time * v));
		}
		else if (type < 0 && !m_full1)
		{
			// move this window
			ReleaseCapture();
			SendMessage(id_to_hwnd(id), WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
	}

	reset_timer(1, 2000);

	return S_OK;
}

LRESULT dx_player::on_timer(int id)
{
	if (id == 1)
	{
		RECT client1, client2;
		POINT mouse1, mouse2;
		int test1 = -1, test2 = -1;
		GetClientRect(id_to_hwnd(1), &client1);
		GetClientRect(id_to_hwnd(2), &client2);
		GetCursorPos(&mouse1);
		GetCursorPos(&mouse2);
		ScreenToClient(id_to_hwnd(1), &mouse1);
		ScreenToClient(id_to_hwnd(2), &mouse2);

		m_bar_drawer.total_width = client1.right - client1.left;
		if (m_bar_drawer.total_width == 0)
			test1 = -1;
		else
		{
			int height = client1.bottom - client1.top;
			int hit_x = mouse1.x;
			int hit_y = mouse1.y;

			if (m_output_mode == out_tb)
			{
				if (hit_y < height/2)
					hit_y += height/2;
			}

			if (m_output_mode == out_htb)
			{
				if (hit_y < height/2)
					hit_y *= 2;
			}

			if (m_output_mode == out_sbs)
			{
				m_bar_drawer.total_width /= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if (m_output_mode == out_hsbs)
			{
				hit_x *= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if (m_renderer1) test1 = m_renderer1->hittest(hit_x, hit_y, NULL);
		}

		m_bar_drawer.total_width = client2.right - client2.left;
		if (m_bar_drawer.total_width == 0)
			test2 = -1;
		else
		{
			int height = client2.bottom - client2.top;
			int hit_x = mouse2.x;
			int hit_y = mouse2.y;

			if (m_output_mode == out_tb)
			{
				if (hit_y < height/2)
					hit_y += height/2;
			}

			if (m_output_mode == out_htb)
			{
				if (hit_y < height/2)
					hit_y *= 2;
			}

			if (m_output_mode == out_sbs)
			{
				m_bar_drawer.total_width /= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if (m_output_mode == out_hsbs)
			{
				hit_x *= 2;
				hit_x %= m_bar_drawer.total_width;
			}

			if(m_renderer1) test2 = m_renderer1->hittest(hit_x, hit_y, NULL);
		}
		if (test1 < 0 && test2 < 0)
		{
			show_mouse(false);
			show_ui(false);
		}
	}

	if (id == 2)
		draw_ui();
	return S_OK;
}

LRESULT dx_player::on_move(int id, int x, int y)
{
	if (m_renderer1)
		m_renderer1->recaculate_mask();
	return S_OK;
}

LRESULT dx_player::on_size(int id, int type, int x, int y)
{
	if (m_renderer1)
		m_renderer1->pump();
	return S_OK;
}

LRESULT dx_player::on_paint(int id, HDC hdc)
{
	// paint Icon
	/*
	int cxIcon = GetSystemMetrics(SM_CXICON);
	int cyIcon = GetSystemMetrics(SM_CYICON);
	RECT rect;
	GetClientRect(id_to_hwnd(id), &rect);
	int x = (rect.right - rect.left - cxIcon + 1) / 2;
	int y = (rect.bottom - rect.top - cyIcon + 1) / 2;
	HICON hIcon = LoadIcon(m_hexe, MAKEINTRESOURCE(IDI_ICON1));
	DrawIcon(hdc, x, y, hIcon);
	*/

	if (m_renderer1)
		m_renderer1->repaint_video();
	else
	{
		RECT client;
		GetClientRect(id_to_hwnd(id), &client);
		FillRect(hdc, &client, (HBRUSH)BLACK_BRUSH+1);
	}

	draw_ui();
	return S_OK;
}

LRESULT dx_player::on_double_click(int id, int button, int x, int y)
{
	if (button == VK_LBUTTON)
		toggle_fullscreen();

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
			reset_and_loadfile_internal(file);
		}
	}

	// Bitstreaming
	else if (uid == ID_AUDIO_BITSTREAM)
	{
		m_bitstreaming = !m_bitstreaming;
		CComPtr<IEnumFilters> ep;
		CComPtr<IBaseFilter> filter;
		m_gb->EnumFilters(&ep);

		while (ep->Next(1, &filter, NULL) == S_OK)
		{
			set_lav_audio_bitstreaming(filter, m_bitstreaming);
			filter = NULL;
		}
		if (m_file_loaded) MessageBoxW(id_to_hwnd(1), C(L"Bitstreaming setting may not apply until next file play or audio swtiching."), L"...", MB_OK);
	}

	// CUDA
	else if (uid == ID_CUDA)
	{
		g_CUDA = !g_CUDA;
		if (m_file_loaded) MessageBoxW(id_to_hwnd(1), C(L"CUDA setting will apply on next file play."), L"...", MB_OK);
	}

	// input layouts
	else if (uid == ID_INPUTLAYOUT_AUTO)
	{
		m_input_layout = input_layout_auto;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	else if (uid == ID_INPUTLAYOUT_SIDEBYSIDE)
	{
		m_input_layout = side_by_side;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	else if (uid == ID_INPUTLAYOUT_TOPBOTTOM)
	{
		m_input_layout = top_bottom;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	else if (uid == ID_INPUTLAYOUT_MONOSCOPIC)
	{
		m_input_layout = mono2d;
		if (m_renderer1)
			m_renderer1->set_input_layout(m_input_layout);			
	}

	// output mode
	else if (uid == ID_OUTPUTMODE_NVIDIA3DVISION)
	{
		m_output_mode = NV3D;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_MONOSCOPIC2D)
	{
		m_output_mode = mono;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_ROWINTERLACE)
	{
		m_output_mode = masking;
		m_mask_mode = row_interlace;
		if (m_renderer1)
		{
			m_renderer1->set_mask_mode(m_mask_mode);
			m_renderer1->set_output_mode(m_output_mode);
		}
	}
	else if (uid == ID_OUTPUTMODE_LINEINTERLACE)
	{
		m_output_mode = masking;
		m_mask_mode = line_interlace;
		if (m_renderer1)
		{
			m_renderer1->set_mask_mode(m_mask_mode);
			m_renderer1->set_output_mode(m_output_mode);
		}
	}
	else if (uid == ID_OUTPUTMODE_CHECKBOARDINTERLACE)
	{
		m_output_mode = masking;
		m_mask_mode = checkboard_interlace;
		if (m_renderer1)
		{
			m_renderer1->set_mask_mode(m_mask_mode);
			m_renderer1->set_output_mode(m_output_mode);
		}
	}
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR)
	{
		m_output_mode = dual_window;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR_SBS)
	{
		m_output_mode = out_sbs;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_DUALPROJECTOR_TB)
	{
		m_output_mode = out_tb;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_3DTV_SBS)
	{
		m_output_mode = out_hsbs;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_3DTV_TB)
	{
		m_output_mode = out_htb;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}
	else if (uid == ID_OUTPUTMODE_GERNERAL120HZGLASSES)
	{
		m_output_mode = pageflipping;
		if (m_renderer1)
			m_renderer1->set_output_mode(m_output_mode);			
	}

	// aspect ratio

	else if (uid == ID_ASPECTRATIO_DEFAULT)
	{
		m_aspect = -1;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	else if (uid == ID_ASPECTRATIO_169)
	{
		m_aspect = (double)16/9;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	else if (uid == ID_ASPECTRATIO_43)
	{
		m_aspect = (double)4/3;
		if (m_renderer1)
			m_renderer1->set_aspect(m_aspect);
	}

	// swap eyes
	else if (uid == ID_SWAPEYES)
	{
		set_revert(!m_revert);
	}

	// anaglygh
	else if (uid == ID_ANAGLYPH_REDCYAN)
	{
		m_anaglygh_left_color = RGB(255,0,0);
		m_anaglygh_right_color = RGB(0,255,255);
		m_output_mode = anaglyph;
		if (m_renderer1)
		{
			m_renderer1->set_output_mode(m_output_mode);
			m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
			m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
		}

	}
	else if (uid == ID_ANAGLYPH_CUSTOMCOLOR)
	{
		m_output_mode = anaglyph;
		if (m_renderer1)
		{
			m_renderer1->set_output_mode(m_output_mode);
			m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
			m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
		}
	}


	else if (uid == ID_OUTPUTMODE_ANAGLYPHCOLOR)
	{
		DWORD tmp = m_anaglygh_left_color;
		if (select_color(&tmp, id_to_hwnd(id)))
		{
			m_anaglygh_left_color = tmp;

			m_output_mode = anaglyph;

			if (m_renderer1)
			{
				m_renderer1->set_output_mode(m_output_mode);
				m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
			}
		}
	}
	else if (uid == ID_OUTPUTMODE_ANAGLYPHCOLORRIGHTEYE)
	{
		DWORD tmp = m_anaglygh_right_color;
		if (select_color(&tmp, id_to_hwnd(id)))
		{
			m_anaglygh_right_color = tmp;

			m_output_mode = anaglyph;

			if (m_renderer1)
			{
				m_renderer1->set_output_mode(m_output_mode);
				m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
			}
		}
	}

	else if (uid == ID_LOADAUDIOTRACK)
	{
		wchar_t file[MAX_PATH] = L"";
		if (open_file_dlg(file, id_to_hwnd(1), L"Audio Tracks\0*.mp3;*.dts;*.ac3;*.aac;*.m4a;*.mka\0All Files\0*.*\0\0"))
		{
			load_audiotrack(file);
		}
	}

	else if (uid == ID_SUBTITLE_LOADFILE)
	{
		wchar_t file[MAX_PATH] = L"";
		if (open_file_dlg(file, id_to_hwnd(1), L"Subtitles\0*.srt;*.sup\0All Files\0*.*\0\0"))
		{
			load_subtitle(file, false);
		}
	}

	else if (uid == ID_CLOSE)
	{
		reset();
	}

	else if (uid == ID_SUBTITLE_LATENCY)
	{
		int t_latency = m_subtitle_latency;
		double t_ratio = m_subtitle_ratio;
		HRESULT hr = latency_modify_dialog(m_hexe, id_to_hwnd(id), &t_latency, &t_ratio);
		m_subtitle_latency = t_latency;
		m_subtitle_ratio = t_ratio;

		draw_subtitle();
	}

	else if (uid == ID_SUBTITLE_FONT)
	{
		if (!m_select_font_active)
			HANDLE show_thread = CreateThread(0,0,select_font_thread, this, NULL, NULL);
	}

	else if (uid == ID_SUBTITLE_COLOR)
	{
		DWORD tmp = m_font_color;
		bool ok;
		if (ok = select_color(&tmp, id_to_hwnd(id)))
			m_font_color = tmp;
		CAutoLock lck(&m_subtitle_sec);
		if (m_srenderer && ok)
			m_srenderer->set_font_color(m_font_color);
		m_lastCBtime = -1;
	}

	else if (uid == ID_SUBTITLE_DISPLAYSUBTITLE)
	{
		m_display_subtitle = !m_display_subtitle;
		if (m_display_subtitle)
			set_subtitle_pos(m_subtitle_center_x, m_subtitle_bottom_y);
		else
			if (m_renderer1) m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0);
	}

	else if (uid == ID_OPENBDFOLDER)
	{
		wchar_t file[MAX_PATH] = L"";
		if (browse_folder(file, id_to_hwnd(id)))
		{
			reset_and_loadfile_internal(file);
		}
	}

	else if (uid == ID_PLAY) 
	{
		pause();
	}

	else if (uid == ID_FULLSCREEN)
	{
		toggle_fullscreen();
	}
	else if (uid == ID_EXIT)
	{
		show_window(1, false);
		show_window(2, false);
	}
	else if (uid == ID_LANGUAGE_CHINESE)
	{
		set_localization_language(CHINESE);
	}
	else if (uid == ID_LANGUAGE_ENGLISH)
	{
		set_localization_language(ENGLISH);
	}

	// open drive
	else if (uid > 'B' && uid <= 'Z')
	{
		wchar_t tmp[MAX_PATH] = L"C:\\";
		tmp[0] = uid;
		reset_and_loadfile_internal(tmp);
	}

	// audio track
	else if (uid >= 'A0' && uid <= 'B0')
	{
		int trackid = uid - 'A0';
		enable_audio_track(trackid);
	}

	else if (uid >= 'S0' && uid <= 'T0')
	{
		int trackid = uid - 'S0';
		enable_subtitle_track(trackid);
	}

	if (m_output_mode == dual_window)
	{
		show_window(2, m_full1);
		set_fullscreen(2, m_full1);
	}
	else
	{
		show_window(2, false);
	}
	reset_timer(1, 2000);
	return S_OK;
}
LRESULT dx_player::on_sys_command(int id, WPARAM wParam, LPARAM lParam)
{
	const int SC_MAXIMIZE2 = 0xF032;
	if (wParam == SC_MAXIMIZE || wParam == SC_MAXIMIZE2)
	{
		toggle_fullscreen();
		return S_OK;
	}
	else if (LOWORD(wParam) == SC_SCREENSAVE)
		return 0;

	return S_FALSE;
}

LRESULT dx_player::on_init_dialog(int id, WPARAM wParam, LPARAM lParam)
{
	HICON h_icon = LoadIcon(m_hexe, MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(id_to_hwnd(id), WM_SETICON, TRUE, (LPARAM)h_icon);
	SendMessage(id_to_hwnd(id), WM_SETICON, FALSE, (LPARAM)h_icon);
	
	m_accel = LoadAccelerators(m_hexe, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	/*
	LONG style1 = GetWindowLongPtr(m_hwnd1, GWL_STYLE);
	LONG exstyle1 = GetWindowLongPtr(m_hwnd1, GWL_EXSTYLE);

	LONG f = style1 & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
	LONG exf =  exstyle1 & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

	SetWindowLongPtr(m_hwnd1, GWL_STYLE, f);
	SetWindowLongPtr(m_hwnd1, GWL_EXSTYLE, exf);
	*/


	SetFocus(id_to_hwnd(id));
	if (id == 1)m_renderer1 = new my12doomRenderer(id_to_hwnd(1), id_to_hwnd(2));
	init_done_flag = 0x12345678;
	return S_OK;
}

// directshow part

HRESULT dx_player::init_direct_show()
{
	HRESULT hr = S_OK;
	hr = exit_direct_show();

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

	if(m_gb)
	{
		m_gb->RemoveFilter(m_renderer1->m_dshow_renderer1);
		m_gb->RemoveFilter(m_renderer1->m_dshow_renderer2);
	}

	// reconfig renderer
	m_renderer1->reset();
	m_renderer1->set_output_mode(m_output_mode);
	m_renderer1->set_input_layout(m_input_layout);
	m_renderer1->set_mask_mode(m_mask_mode);
	m_renderer1->set_callback(this);
	m_renderer1->set_mask_color(1, color_GDI2ARGB(m_anaglygh_left_color));
	m_renderer1->set_mask_color(2, color_GDI2ARGB(m_anaglygh_right_color));
	m_renderer1->set_bmp_offset((double)m_internel_offset/1000 + (double)m_user_offset/1920);
	m_renderer1->set_aspect(m_aspect);

	m_file_loaded = false;
	
	m_offset_metadata = NULL;
	m_ba = NULL;
	m_ms = NULL;
	m_mc = NULL;
	m_gb = NULL;


	return S_OK;
}

HRESULT dx_player::SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn)
{
	if (!m_display_subtitle || !m_renderer1)
		return S_OK;

	// latency and ratio
	TimeStart -= m_subtitle_latency * 10000;
	TimeStart /= m_subtitle_ratio;

	m_internel_offset = 10;
	if (m_offset_metadata)
	{
		REFERENCE_TIME frame_time = m_renderer1->m_frame_length;
		HRESULT hr = m_offset_metadata->GetOffset(TimeStart+frame_time*2, frame_time, &m_internel_offset);	//preroll 2 frames
		//log_line(L"offset = %d(%s)", m_internel_offset, hr == S_OK ? L"S_OK" : L"S_FALSE");
	}
	m_renderer1->set_bmp_offset((double)m_internel_offset/1000 + (double)m_user_offset/1920);

	int ms_start = (int)(TimeStart / 10000);
	int ms_end = (int)(TimeEnd / 10000);

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

	// clear
	if (m_lastCBtime == -1)
	{
		//m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0); // FIXME: maybe we don't need to clear here?
	}

	{
		if (S_FALSE == hr)		// same subtitle, ignore
			return S_OK;
		else if (S_OK == hr)	// need to update
		{
			// empty result, clear it
			if( sub.width == 0 || sub.height ==0 || sub.width_pixel==0 || sub.height_pixel == 0 || sub.data == NULL)
			{
				m_renderer1->set_bmp(NULL, 0, 0, 0, 0, 0, 0);
			}

			// draw it
			else
			{
				// FIXME: assume aspect_screen is always less than aspect_subtitle
				double aspect_screen1 = (double)(m_screen1.right - m_screen1.left)/(m_screen1.bottom - m_screen1.top);
				double aspect_screen2 = (double)(m_screen2.right - m_screen2.left)/(m_screen2.bottom - m_screen2.top);

				double delta1 = (1-aspect_screen1/sub.aspect);
				double delta2 = (1-aspect_screen1/sub.aspect);

				hr = m_renderer1->set_bmp(sub.data, sub.width_pixel, sub.height_pixel, sub.width,
					sub.height * (1-delta1),
					sub.left + (m_subtitle_center_x-0.5),
					sub.top *(1-delta1) + delta1/2 + (m_subtitle_bottom_y-0.95));
				if (FAILED(hr)) 
				{
					free(sub.data);
					m_lastCBtime = -1;				// failed, refresh on next frame
					return hr;
				}

				//TODO2
				/*hr = m_renderer1->set_bmp(sub.data, sub.width_pixel, sub.height_pixel, sub.width, 
					sub.height * (1-delta2),
					sub.left + (m_subtitle_center_x-0.5) + (double)m_subtitle_offset*sub.width/sub.width_pixel + sub.delta,
					sub.top *(1-delta2) + delta2/2 + (m_subtitle_bottom_y-0.95));
				*/
				free(sub.data);
				if (FAILED(hr))
				{
					m_lastCBtime = -1;				// failed, refresh on next frame
					return hr;
				}
			}
		}
		m_lastCBtime = ms_start;		// only do this if SetAlphaBitmap didn't fail, yes, it CAN FAIL!
	}

	return S_OK;
}

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

HRESULT dx_player::set_revert(bool revert)
{
	m_revert = revert;

	if (m_renderer1)
	{
		m_renderer1->set_swap_eyes(m_revert);
	}

	return S_OK;
}

HRESULT dx_player::set_letterbox(double delta)
{
	m_letterbox_delta = delta;

	if (m_renderer1)
		m_renderer1->set_movie_pos(2, m_letterbox_delta);

	return S_OK;
}

HRESULT dx_player::show_ui(bool show)
{
	m_show_ui = show;
	return S_OK;
}

HRESULT dx_player::draw_ui()
{
	CAutoLock lock(&m_draw_sec);

	if (!m_show_ui)
	{
		if (m_renderer1)
			m_renderer1->set_ui_visible(false);
		return S_FALSE;
	}

	int _total = 0, current = 0;
	double volume = 1.0;
	bool paused = true;

	tell(&current);
	total(&_total);
	get_volume(&volume);
	if (m_mc)
	{
		static OAFilterState fs = State_Running;
		HRESULT hr = m_mc->GetState(100, &fs);
		if (fs == State_Running)
			paused = false;
	}

	RECT client_rc;
	GetClientRect(id_to_hwnd(1), &client_rc);
	m_bar_drawer.total_width = client_rc.right - client_rc.left;
	if (m_output_mode == out_sbs) m_bar_drawer.total_width /= 2;
	m_bar_drawer.draw_total(paused, current, _total, volume);

	//TODO: it seems that it is not a good idea to draw UI here..

	if (m_renderer1)
	{
		m_renderer1->set_ui(m_bar_drawer.result, 4096*4);
		m_renderer1->set_ui_visible(true);
	}
	else
	{
		/*
		for(int id=1; id<=2; id++)
		{
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
		*/
	}

	return S_OK;
}

HRESULT dx_player::start_loading()
{

	unsigned char passkey_big_decrypted[128];
	RSA_dwindow_public(&g_passkey_big, passkey_big_decrypted);

	m_renderer1->m_AES.set_key((unsigned char*)passkey_big_decrypted+64, 256);
	memset(passkey_big_decrypted, 0, 128);
	m_gb->AddFilter(m_renderer1->m_dshow_renderer1, L"Renderer #1");
	m_gb->AddFilter(m_renderer1->m_dshow_renderer2, L"Renderer #2");

	return S_OK;
}

HRESULT dx_player::reset_and_loadfile(const wchar_t *pathname, bool stop)
{
	wcscpy(m_file_to_load, pathname);
	m_reset_and_load = true;
	m_stop_after_load = stop;
	m_reset_load_done = false;
	return S_OK;
}

HRESULT dx_player::reset_and_loadfile_internal(const wchar_t *pathname)
{
	reset();
	start_loading();
	HRESULT hr;
	//hr = load_file(L"Z:\\00001.m2ts");
	hr = load_file(pathname);
	//hr = load_file(L"D:\\Users\\my12doom\\Desktop\\test\\00005n.m2ts");
	//hr = load_file(L"D:\\Users\\my12doom\\Desktop\\test\\00006.m2ts");
	//hr = load_file(L"K:\\BDMV\\STREAM\\00001.m2ts");
	//hr = load_file(L"K:\\BDMV\\STREAM\\00002.m2ts");
	//hr = load_file(L"D:\\Users\\my12doom\\Documents\\00002.m2ts");
	if (FAILED(hr))
		goto fail;
	hr = end_loading();
	if (FAILED(hr))
		goto fail;
	play();

	// search and load subtitles
	wchar_t file_to_search[MAX_PATH];
	wchar_t file_folder[MAX_PATH];
	GetWindowTextW(m_hwnd1, file_to_search, MAX_PATH);
	GetWindowTextW(m_hwnd1, file_folder, MAX_PATH);
	for(int i=wcslen(file_to_search)-1; i>0; i--)
		if (file_to_search[i] == L'.')
		{
			file_to_search[i] = NULL;
			break;
		}
	for(int i=wcslen(file_folder)-1; i>0; i--)
		if (file_folder[i] == L'\\')
		{
			file_folder[i+1] = NULL;
			break;
		}

	wchar_t search_pattern[MAX_PATH];
	for(int i=0; i<2; i++)
	{
		wcscpy(search_pattern, file_to_search);
		wcscat(search_pattern, i == 0 ? L"*.srt" : L"*.sup");

		WIN32_FIND_DATAW find_data;
		HANDLE find_handle = FindFirstFileW(search_pattern, &find_data);

		if (find_handle != INVALID_HANDLE_VALUE)
		{
			wchar_t subtitle_to_load[MAX_PATH];
			wcscpy(subtitle_to_load, file_folder);
			wcscat(subtitle_to_load, find_data.cFileName);
			load_subtitle(subtitle_to_load, false);

			while( FindNextFile(find_handle, &find_data ) )
			{
				wcscpy(subtitle_to_load, file_folder);
				wcscat(subtitle_to_load, find_data.cFileName);
				load_subtitle(subtitle_to_load, false);
			}
		}
	}
	return hr;
fail:
	reset();
	set_window_text(1, C(L"Open Failed"));
	return hr;
}

HRESULT dx_player::on_dropfile(int id, int count, wchar_t **filenames)
{
	if (count>0)
	{
		HRESULT hr = load_subtitle(filenames[0], false);
		if (SUCCEEDED(hr) && m_file_loaded)
			return S_OK;
		reset_and_loadfile_internal(filenames[0]);
	}

	return S_OK;
}

LRESULT dx_player::on_idle_time()
{
	if (init_done_flag != 0x12345678)
		return S_FALSE;

	if (m_renderer1)
		m_renderer1->pump();

	if (m_reset_and_load)
	{
		reset_and_loadfile_internal(m_file_to_load);
		m_reset_and_load = false;

		if (m_stop_after_load)
			pause();

		m_reset_load_done = true;
	}
	return S_FALSE;
}
HRESULT dx_player::load_file(const wchar_t *pathname, bool non_mainfile /* = false */, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
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

	// subtitle renderer
	m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

	/*
	// Legacy Remux file
	if (verify_file(file_to_play) == 2)
	{
		log_line(L"remux file no longer supported! %s...", file_to_play);
		return E_FAIL;
	}
	*/



	// check private source and whether is MVC content
	CLSID source_clsid;
	hr = GetFileSource(file_to_play, &source_clsid);

	// E3D keys
	if (source_clsid == CLSID_E3DSource)
	{
		log_line(L"loading local E3D key for %s", file_to_play);
		HANDLE h_file = CreateFileW (file_to_play, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		file_reader reader;
		reader.SetFile(h_file);
		CloseHandle(h_file);

		if (!reader.m_is_encrypted)
		{
			// normal file, do nothing
		}
		else
		{
			unsigned char key[32];
			load_e3d_key(reader.m_hash, key);
			reader.set_key((unsigned char*)key);

			if (!reader.m_key_ok)
			{
				log_line(L"local E3D key failed.");
				log_line(L"downloading E3D key for %s", file_to_play);
				if (SUCCEEDED(download_e3d_key(file_to_play)))
				{
					log_line(L"download E3D key OK, saving to local store.");
					e3d_get_process_key(key);
					save_e3d_key(reader.m_hash, key);
				}
			}
			else
			{
				log_line(L"local E3D key OK.");
			}
			e3d_set_process_key(key);
		}
	}
	if (SUCCEEDED(hr))
	{
		log_line(L"loading with private filter");
		// private file types
		// create source, load file and join it into graph
		CComPtr<IBaseFilter> source_base;
		hr = myCreateInstance(source_clsid, IID_IBaseFilter, (void**)&source_base);
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source_source(source_base);
		if (source_source)
		{
			source_source->Load(file_to_play, NULL);
			m_gb->AddFilter(source_base, L"Source");

			CComQIPtr<IOffsetMetadata, &IID_IOffsetMetadata> offset(source_base);
			if (offset)
				m_offset_metadata = offset;
		}
		else
		{
			CComPtr<IBaseFilter> async_reader;
			async_reader.CoCreateInstance(CLSID_AsyncReader);
			CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> reader_source(async_reader);
			reader_source->Load(file_to_play, NULL);
			m_gb->AddFilter(source_base, L"Splitter");
			m_gb->AddFilter(async_reader, L"Reader");

			CComPtr<IPin> reader_o;
			CComPtr<IPin> source_i;
			GetUnconnectedPin(async_reader, PINDIR_OUTPUT, &reader_o);
			GetUnconnectedPin(source_base, PINDIR_INPUT, &source_i);

			m_gb->ConnectDirect(reader_o, source_i, NULL);
		}

		log_line(L"renderer ing pins");
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
					log_line(L"testing pin %s", info.achName);

					if (S_OK == DeterminPin(pin, NULL, MEDIATYPE_Video))
					{

						if ( (video_track>=0 && (LOADFILE_TRACK_NUMBER(video_num) & video_track ))
							|| video_track == LOADFILE_ALL_TRACK)
						{
							CLSID CLSID_mp4v = FOURCCMap('v4pm');
							if(S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('v4pm')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('V4PM')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('XVID')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('xvid')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('divx')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('DIVX')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('05XD')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('05xd')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('3VID')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('4VID')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('5VID')) )
							{
								log_line(L"adding xvid decoder");
								make_xvid_support_mp4v();
								CComPtr<IBaseFilter> xvid;
								hr = myCreateInstance(CLSID_XvidDecoder, IID_IBaseFilter, (void**)&xvid);
								hr = m_gb->AddFilter(xvid, L"Xvid Deocder");
							}

							if(S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('1cva')) || 
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('1CVA')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('CVMA')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462h')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462H')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462x')) ||
							   S_OK == DeterminPin(pin, NULL, CLSID_NULL, FOURCCMap('462X')))
							{
								log_line(L"adding coremvc decoder");
								coremvc_hooker mvc_hooker;
								CComPtr<IBaseFilter> coremvc;
								hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
								hr = ActiveCoreMVC(coremvc);
								hr = m_gb->AddFilter(coremvc, L"CoreMVC");

								FILTER_INFO fi;
								fi.pGraph = NULL;
								if (coremvc) coremvc->QueryFilterInfo(&fi);
								if (fi.pGraph)
									fi.pGraph->Release();
								else
									log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");

								log_line(L"CoreMVC hr = 0x%08x", hr);
							}

							if (S_OK == DeterminPin(pin, NULL, CLSID_NULL, MEDIASUBTYPE_MPEG2_VIDEO) ||
								S_OK == DeterminPin(pin, NULL, CLSID_NULL, MEDIASUBTYPE_MPEG1Payload)  )
							{
								log_line(L"adding pd10 decoder");
								CComPtr<IBaseFilter> pd10;
								hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
								hr = m_gb->AddFilter(pd10, L"PD10 Decoder");
							}

							log_line(L"renderering video pin #%d", video_num);
							//debug_list_filters();
							hr = m_gb->Render(pin);
							log_line(L"done renderering video pin #%d", video_num);
						}
						video_num ++;
					}

					else if (S_OK == DeterminPin(pin, NULL, MEDIATYPE_Audio))
					{
						if ( (audio_track>=0 && (LOADFILE_TRACK_NUMBER(audio_num) & audio_track ))
							|| audio_track == LOADFILE_ALL_TRACK)
						{
							CComPtr<IBaseFilter> lav_audio;
							hr = myCreateInstance(CLSID_LAVAudio, IID_IBaseFilter, (void**)&lav_audio);
							set_lav_audio_bitstreaming(lav_audio, m_bitstreaming);
							hr = m_gb->AddFilter(lav_audio, L"LAV Audio Decoder");
							log_line(L"renderering audio pin #%d", audio_num);
							m_gb->Render(pin);
						}
						audio_num ++;
					}

					else if (S_OK == DeterminPin(pin, NULL, MEDIATYPE_Subtitle))
					{
						m_gb->Render(pin);
					}

					else;	// other tracks, ignore them
				}
			}
			pin = NULL;
		}

		// if it doesn't work....
		if (video_num == 0 && audio_num == 0)
		{
			log_line(L"private filters failed, trying system filters. (%s)", file_to_play);

			CComPtr<IBaseFilter> pd10;
			hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
			hr = m_gb->AddFilter(pd10, L"PD10 Deocder");

			CComPtr<IBaseFilter> xvid;
			hr = myCreateInstance(CLSID_XvidDecoder, IID_IBaseFilter, (void**)&xvid);
			hr = m_gb->AddFilter(xvid, L"Xvid Deocder");

			{
				CComPtr<IBaseFilter> coremvc;
				coremvc_hooker mvc_hooker;

				hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
				hr = ActiveCoreMVC(coremvc);
				hr = m_gb->AddFilter(coremvc, L"CoreMVC");
				FILTER_INFO fi;
				fi.pGraph = NULL;
				if (coremvc) coremvc->QueryFilterInfo(&fi);
				if (fi.pGraph)
					fi.pGraph->Release();
				else
					log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");
			}

			CComPtr<IBaseFilter> lav_audio;
			hr = myCreateInstance(CLSID_LAVAudio, IID_IBaseFilter, (void**)&lav_audio);
			set_lav_audio_bitstreaming(lav_audio, m_bitstreaming);
			hr = m_gb->AddFilter(lav_audio, L"LAV Audio Decoder");

			hr = m_gb->RenderFile(file_to_play, NULL);
		}
		else
		{
			log_line(L"private filter OK");
			hr = S_OK;
		}
	}


	// normal file, just render it.
	else
	{
		log_line(L"private filters failed, trying system filters. (%s)", file_to_play);

		CComPtr<IBaseFilter> pd10;
		hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10);
		hr = m_gb->AddFilter(pd10, L"PD10 Deocder");

		CComPtr<IBaseFilter> xvid;
		hr = myCreateInstance(CLSID_XvidDecoder, IID_IBaseFilter, (void**)&xvid);
		hr = m_gb->AddFilter(xvid, L"Xvid Deocder");

		{
			CComPtr<IBaseFilter> coremvc;
			coremvc_hooker mvc_hooker;

			hr = myCreateInstance(CLSID_CoreAVC, IID_IBaseFilter, (void**)&coremvc);
			hr = ActiveCoreMVC(coremvc);
			hr = m_gb->AddFilter(coremvc, L"CoreMVC");
			FILTER_INFO fi;
			fi.pGraph = NULL;
			if (coremvc) coremvc->QueryFilterInfo(&fi);
			if (fi.pGraph)
				fi.pGraph->Release();
			else
				log_line(L"couldn't add CoreMVC to graph(need rename to StereoPlayer.exe.");
		}

		CComPtr<IBaseFilter> lav_audio;
		hr = myCreateInstance(CLSID_LAVAudio, IID_IBaseFilter, (void**)&lav_audio);
		set_lav_audio_bitstreaming(lav_audio, m_bitstreaming);
		hr = m_gb->AddFilter(lav_audio, L"LAV Audio Decoder");

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
		

		if (!non_mainfile)
		{
			m_srenderer = m_grenderer.GetSubtitleRenderer();
			m_file_loaded = true;
			set_window_text(1, file_to_play);
			set_window_text(2, file_to_play);
		}
	}

	return hr;
}

HRESULT dx_player::end_loading()
{
	set_volume(m_volume);

	int num_renderer_found = 0;

	debug_list_filters();
	RemoveUselessFilters(m_gb);

	CComPtr<IPin> renderer1_input;
	GetUnconnectedPin(m_renderer1->m_dshow_renderer1, PINDIR_INPUT, &renderer1_input);
	if (renderer1_input)
	{
		log_line(L"no video stream found.");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT dx_player::debug_list_filters()
{
	// debug: list filters
	log_line(L"Listing filters.");
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> filter;
	m_gb->EnumFilters(&pEnum);
	while(pEnum->Next(1, &filter, NULL) == S_OK)
	{

		FILTER_INFO filter_info;
		filter->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		wchar_t tmp[10240];
		wchar_t tmp2[1024];
		wcscpy(tmp, filter_info.achName);
		
		CComPtr<IEnumPins> ep;
		CComPtr<IPin> pin;
		filter->EnumPins(&ep);
		while (ep->Next(1, &pin, NULL) == S_OK)
		{
			PIN_DIRECTION dir;
			PIN_INFO pi;
			pin->QueryDirection(&dir);
			pin->QueryPinInfo(&pi);
			if (pi.pFilter) pi.pFilter->Release();

			CComPtr<IPin> connected;
			PIN_INFO pi2;
			FILTER_INFO fi;
			pin->ConnectedTo(&connected);
			pi2.pFilter = NULL;
			if (connected) connected->QueryPinInfo(&pi2);
			if (pi2.pFilter)
			{
				pi2.pFilter->QueryFilterInfo(&fi);
				if (fi.pGraph) fi.pGraph->Release();
				pi2.pFilter->Release();
			}
			
			wsprintfW(tmp2, L", %s %s", pi.achName, connected?L"Connected to ":L"Unconnected");
			if (connected) wcscat(tmp2, fi.achName);

			wcscat(tmp, tmp2);
			pin = NULL;
		}


		log_line(L"%s", tmp);

		filter = NULL;
	}
	log_line(L"");

	return S_OK;
}

// font/d3d/subtitle part

HRESULT dx_player::load_audiotrack(const wchar_t *pathname)
{
	//disable all audio first
	enable_audio_track(-1);

	// Save Filter State and Stop
	int time = 0;
	tell(&time);
	time += 10;
	OAFilterState state_before, state;
	m_mc->GetState(INFINITE, &state_before);
	if (state_before != State_Stopped)
	{
		m_mc->Stop();
		m_mc->GetState(INFINITE, &state);
	}

	// load and restore
	HRESULT hr = load_file(pathname, true, LOADFILE_FIRST_TRACK, LOADFILE_NO_TRACK);
	if (state_before == State_Running)
		m_mc->Run();
	m_mc->GetState(INFINITE, &state);
	hr = seek(time);
	return hr;
}
HRESULT dx_player::load_subtitle(const wchar_t *pathname, bool reset)			//FIXME : always reset 
{
	if (pathname == NULL)
		return E_POINTER;

	// find duplication
	for(POSITION i = m_external_subtitles.GetHeadPosition(); i; m_external_subtitles.GetNext(i))
	{
		CAutoPtr<subtitle_file_handler> &tmp = m_external_subtitles.GetAt(i);
		if (wcscmp(pathname, tmp->m_pathname) == 0)
			return S_FALSE;
	}

	CAutoPtr<subtitle_file_handler> tmp;
	tmp.Attach(new subtitle_file_handler(pathname));
	if(tmp->m_renderer == NULL)
		return E_FAIL;

	m_external_subtitles.AddTail(tmp);
	return enable_subtitle_track((int)m_external_subtitles.GetCount()-1);
}

HRESULT dx_player::draw_subtitle()
{	
	REFERENCE_TIME t = (REFERENCE_TIME)m_lastCBtime * 10000;
	m_lastCBtime = -1;

	return SampleCB(t+1, t+2, NULL);
}

HRESULT dx_player::set_subtitle_offset(int offset)
{
	m_user_offset = offset;
	m_renderer1->set_bmp_offset((double)m_internel_offset/1000 + (double)m_user_offset/1920);
	draw_subtitle();
	return S_OK;
}

HRESULT dx_player::set_subtitle_pos(double center_x, double bottom_y)
{
	m_subtitle_center_x = center_x;
	m_subtitle_bottom_y = bottom_y;

	return draw_subtitle();
}

HRESULT dx_player::log_line(wchar_t *format, ...)
{
#ifdef DEBUG
	wcscat(m_log, L"\r\n");

	wchar_t tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	wprintf(L"log: %s\n", tmp);
	wcscat(m_log, tmp);
#endif
	return S_OK;
}

HRESULT dx_player::select_font(bool show_dlg)
{
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
	lf.lfCharSet = GB2312_CHARSET;
	lf.lfOutPrecision =  OUT_STROKE_PRECIS;
	lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH;

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

HRESULT dx_player::toggle_fullscreen()
{
	if (m_output_mode == pageflipping || m_output_mode == NV3D)
	{
		if (m_renderer1)
			m_renderer1->set_fullscreen(!m_renderer1->get_fullscreen());
	}

	else if (m_output_mode == dual_window)
	{
		show_window(2, !m_full2);		// show/hide it before set fullscreen, or you may got a strange window
		set_fullscreen(2, !m_full2);	// yeah, this is not a bug
		set_fullscreen(1, m_full2);
		show_window(2, m_full2);		// show/hide it again
	}

	else
	{
		show_window(2, false);
		set_fullscreen(1, !m_full1);
	}
	return S_OK;
}

HRESULT dx_player::enable_audio_track(int track)
{
	CComPtr<IEnumFilters> ef;

	// Save Filter State
	int time;
	tell(&time);
	OAFilterState state_before, state;
	m_mc->GetState(INFINITE, &state_before);
	if (state_before != State_Stopped)
	{
		m_mc->Stop();
		m_mc->GetState(INFINITE, &state);
	}
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int audio_track_found = 0;

	CComPtr<IPin> pin_to_render;

	// delete any downstream of audio pins
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				pin = NULL;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}

			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				filter->EnumPins(&ep);
				bool efreset = false;
				pin = NULL;
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK)
					{
						if (connected)
						{
							RemoveDownstream(connected);
							ef->Reset();
							efreset = true;
						}
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin

				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(DWORD i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);
					if (DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK || wcsstr(name , L"Audio") || wcsstr(name, L"A:"))
					{
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"Audio", &pin);
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"A:", &pin);

						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);

						if (connected)
						{
							RemoveDownstream(connected);
							ef->Reset();
						}
					}
					CoTaskMemFree (name);
				}

			}

		}
		filter = NULL;
	}

	ef = NULL;
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				pin = NULL;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}
			//break;

			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				pin = NULL;
				filter->EnumPins(&ep);
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK)
					{
						if (audio_track_found == track)
						{
							pin_to_render = pin;
						}

						audio_track_found++;
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin

				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(DWORD i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);
					if (DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK || wcsstr(name , L"Audio") || wcsstr(name, L"A:"))
					{
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"Audio", &pin);
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"A:", &pin);

						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);

						if (audio_track_found == track)
						{
							stream_select->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
							pin_to_render = pin;
						}

						audio_track_found++;
					}
					CoTaskMemFree (name);
				}

			}

		}
		filter = NULL;
	}




	HRESULT hr = S_OK;
	if (pin_to_render)
	{
		CComPtr<IBaseFilter> LAV;
		HRESULT hr = myCreateInstance(CLSID_LAVAudio, IID_IBaseFilter, (void**)&LAV);
		set_lav_audio_bitstreaming(LAV, m_bitstreaming);
		m_gb->AddFilter(LAV, L"LAV Audio Deocder");
		m_gb->Render(pin_to_render);
	}
	else
		hr = VFW_E_NOT_FOUND;


	// restore filter state
	seek(time);
	if (state_before == State_Running) 
		m_mc->Run();
	else if (state_before == State_Paused)
		m_mc->Pause();

	return hr;
}
HRESULT dx_player::enable_subtitle_track(int track)
{
	CComPtr<IEnumFilters> ef;
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int subtitle_track_found = 0;

	// Save Filter State
	bool filter_stopped = false;
	OAFilterState state_before = 3, state;
	HRESULT hr = m_mc->GetState(1000, &state_before);
	if (state_before == 3)
		return E_FAIL;
	CComPtr<IPin> grenderer_input_pin;
	GetConnectedPin(m_grenderer.m_filter, PINDIR_INPUT, &grenderer_input_pin);

	POSITION t = m_external_subtitles.GetHeadPosition();
	for(DWORD i=0; i<m_external_subtitles.GetCount(); i++, m_external_subtitles.GetNext(t))
	{
		CAutoPtr<subtitle_file_handler> &tmp = m_external_subtitles.GetAt(t);
		if (i == track)
		{
			tmp->actived = true;
			{
				CAutoLock lck(&m_subtitle_sec);
				m_srenderer = tmp->m_renderer;
			}

			// disable dshow subtitle if needed
			if (state_before != State_Stopped && !filter_stopped && grenderer_input_pin)
			{
				m_mc->Stop();
				m_mc->GetState(INFINITE, &state);

				// remove (and re-add later if needed) the subtitle renderer
				m_gb->RemoveFilter(m_grenderer.m_filter);
			}
		}
		else
		{
			tmp->actived = false;
		}
		subtitle_track_found++;
	}


	if (track >= m_external_subtitles.GetCount())
	{

		// find splitter, which supports IFileSourceFilter, if it is AsyncReader, use its downstream
		m_gb->EnumFilters(&ef);
		while (ef->Next(1, &filter, NULL) == S_OK)
		{
			CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
			if (fs != NULL)
			{
				CLSID clsid;
				filter->GetClassID(&clsid);
				if (clsid == CLSID_AsyncReader)
				{
					// assume connected
					GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					PIN_INFO pi;
					connected->QueryPinInfo(&pi);
					pin = NULL;
					filter = NULL;
					filter.Attach(pi.pFilter);
				}
				CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
				if (stream_select == NULL)
				{
					// splitter that doesn't support IAMStreamSelect should have multiple Pins
					ep = NULL;
					filter->EnumPins(&ep);
					while (ep->Next(1, &pin, NULL) == S_OK)
					{
						PIN_INFO pi;
						pin->QueryPinInfo(&pi);
						if (pi.pFilter) pi.pFilter->Release();

						CComPtr<IPin> connected;
						pin->ConnectedTo(&connected);

						if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK)
						{
							if (subtitle_track_found == track)
							{
								// stop filters if needed
								if (state_before != State_Stopped && !filter_stopped)
								{
									m_mc->Stop();
									m_mc->GetState(INFINITE, &state);

								}
								// remove (and re-add later if needed) the subtitle renderer
								m_gb->RemoveFilter(m_grenderer.m_filter);
								m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

								// render new subtitle pin
								m_gb->Render(pin);
							}

							subtitle_track_found++;
						}

						pin = NULL;
					}
				}
				else
				{
					// splitter that supports IAMStreamSelect should have only one subtitle Pin
					DWORD stream_count = 0;
					stream_select->Count(&stream_count);
					for(int i=0; i<stream_count; i++)
					{
						WCHAR *name = NULL;
						DWORD enabled = 0;
						stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

						pin = NULL;
						GetPinByName(filter, PINDIR_OUTPUT, name, &pin);

						if (DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK || wcsstr(name , L"Subtitle") || wcsstr(name, L"S:"))
						{
							if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"Subtitle", &pin);
							if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"S:", &pin);
							if (subtitle_track_found == track)
							{
								if (state_before != State_Stopped && !filter_stopped)
								{
									m_mc->Stop();
									m_mc->GetState(INFINITE, &state);

								}
								// remove (and re-add later if needed) the subtitle renderer
								m_gb->RemoveFilter(m_grenderer.m_filter);
								m_gb->AddFilter(m_grenderer.m_filter, L"Subtitle Renderer");

								// render new subtitle pin
								stream_select->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
								m_gb->Render(pin);
							}

							subtitle_track_found++;
						}
						CoTaskMemFree (name);
					}
				}
			}
			filter = NULL;
		}

	}

	// restore filter state
	if (state_before == State_Running) 
		m_mc->Run();
	else if (state_before == State_Paused)
		m_mc->Pause();

	CAutoLock lck(&m_subtitle_sec);
	if (track >= m_external_subtitles.GetCount()) m_srenderer = m_grenderer.GetSubtitleRenderer();
	if (m_srenderer)
	{
		m_srenderer->set_font(m_font);
		m_srenderer->set_font_color(m_font_color);
	}
	m_display_subtitle = true;
	draw_subtitle();
	return S_OK;
}
HRESULT dx_player::list_audio_track(HMENU submenu)
{
	CComPtr<IEnumFilters> ef;
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int audio_track_found = 0;

	// find splitter, which supports IFileSourceFilter, if it is AsyncReader, use its downstream
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}
			//break;
			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				pin = NULL;
				filter->EnumPins(&ep);
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK)
					{
						int flag = MF_STRING | MF_BYPOSITION;
						if (connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, audio_track_found, flag, 'A0'+audio_track_found, pi.achName);
						audio_track_found++;
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin
				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(int i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);

					if (DeterminPin(pin, NULL, MEDIATYPE_Audio) == S_OK || wcsstr(name , L"Audio") || wcsstr(name, L"A:"))
					{
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"Audio", &pin);
						if (pin == NULL) GetPinByName(filter, PINDIR_OUTPUT, L"A:", &pin);

						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);
						int flag = MF_STRING | MF_BYPOSITION;
						if (enabled && connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, audio_track_found, flag, 'A0'+audio_track_found, name);
						audio_track_found++;
					}
					CoTaskMemFree (name);
				}

			}
		}
		filter = NULL;
	}

	if (audio_track_found != 0)
	{
		DeleteMenu(submenu, ID_AUDIO_NONE, MF_BYCOMMAND);
	}

	return S_OK;
}

HRESULT dx_player::list_subtitle_track(HMENU submenu)
{
	CComPtr<IEnumFilters> ef;
	CComPtr<IEnumPins> ep;
	CComPtr<IBaseFilter> filter;
	CComPtr<IPin> pin;
	int subtitle_track_found = 0;

	// external subtitle files
	for(POSITION i = m_external_subtitles.GetHeadPosition(); i; m_external_subtitles.GetNext(i))
	{
		int flag = MF_STRING | MF_BYPOSITION;
		CAutoPtr<subtitle_file_handler> &tmp = m_external_subtitles.GetAt(i);
		if (tmp->actived) flag |= MF_CHECKED;
		InsertMenuW(submenu, subtitle_track_found, flag, 'S0'+subtitle_track_found, tmp->m_pathname);
		subtitle_track_found++;
	}

	// find splitter, which supports IFileSourceFilter, if it is AsyncReader, use its downstream
	m_gb->EnumFilters(&ef);
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter>  fs(filter);
		if (fs != NULL)
		{
			CLSID clsid;
			filter->GetClassID(&clsid);
			if (clsid == CLSID_AsyncReader)
			{
				// assume connected
				pin = NULL;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin);
				CComPtr<IPin> connected;
				pin->ConnectedTo(&connected);

				PIN_INFO pi;
				connected->QueryPinInfo(&pi);
				pin = NULL;
				filter = NULL;
				filter.Attach(pi.pFilter);
			}

			CComQIPtr<IAMStreamSelect, &IID_IAMStreamSelect> stream_select(filter);
			if (stream_select == NULL)
			{
				// splitter that doesn't support IAMStreamSelect should have multiple Audio Pins
				ep = NULL;
				filter->EnumPins(&ep);
				pin = NULL;
				while (ep->Next(1, &pin, NULL) == S_OK)
				{
					PIN_INFO pi;
					pin->QueryPinInfo(&pi);
					if (pi.pFilter) pi.pFilter->Release();

					CComPtr<IPin> connected;
					pin->ConnectedTo(&connected);

					if (pi.dir == PINDIR_OUTPUT && DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK)
					{
						int flag = MF_STRING | MF_BYPOSITION;
						if (connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, subtitle_track_found, flag, 'S0'+subtitle_track_found, pi.achName);
						subtitle_track_found++;
					}

					pin = NULL;
				}
			}
			else
			{
				// splitter that supports IAMStreamSelect should have only one Audio Pin
				DWORD stream_count = 0;
				stream_select->Count(&stream_count);
				for(int i=0; i<stream_count; i++)
				{
					WCHAR *name = NULL;
					DWORD enabled = 0;
					stream_select->Info(i, NULL, &enabled, NULL, NULL, &name, NULL, NULL);

					pin = NULL;
					GetPinByName(filter, PINDIR_OUTPUT, name, &pin);

					if (DeterminPin(pin, NULL, MEDIATYPE_Subtitle) == S_OK || wcsstr(name , L"Subtitle") || wcsstr(name, L"S:"))
					{
						if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"Subtitle", &pin);
						if (NULL == pin) GetPinByName(filter, PINDIR_OUTPUT, L"S:", &pin);
						CComPtr<IPin> connected;
						if (pin) pin->ConnectedTo(&connected);

						int flag = MF_STRING | MF_BYPOSITION;
						if (enabled && connected) flag |= MF_CHECKED;
						InsertMenuW(submenu, subtitle_track_found, flag, 'S0'+subtitle_track_found, name);
						subtitle_track_found++;
					}
					CoTaskMemFree (name);
				}
			}
		}
		filter = NULL;
	}

	if (subtitle_track_found != 0)
	{
		InsertMenuW(submenu, subtitle_track_found, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	}

	return S_OK;
}


subtitle_file_handler::subtitle_file_handler(const wchar_t *pathname)
{
	wcscpy(m_pathname, pathname);
	actived = false;
	m_renderer = NULL;

	FILE * f = _wfopen(pathname, L"rb");
	if (!f)
		return;
	fclose(f);

	const wchar_t *p_3 = pathname + wcslen(pathname) -3;
	if ( (p_3[0] == L's' || p_3[0] == L'S') &&
		(p_3[1] == L'r' || p_3[1] == L'R') &&
		(p_3[2] == L't' || p_3[2] == L'T'))
	{
		m_renderer = new CsrtRenderer(NULL, 0xffffff);
	}
	else if ( (p_3[0] == L's' || p_3[0] == L'S') &&
		(p_3[1] == L'u' || p_3[1] == L'U') &&
		(p_3[2] == L'p' || p_3[2] == L'P'))
	{
		m_renderer = new PGSRenderer();
	}
	else
	{
		return;
	}

	m_renderer->load_file(m_pathname);
}

subtitle_file_handler::~subtitle_file_handler()
{
	if (m_renderer)
		delete m_renderer;
	m_renderer = NULL;
}
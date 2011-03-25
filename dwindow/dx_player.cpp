#include <math.h>
#include "dx_player.h"
#include "pgs\PGSRenderer.h"
#include "private_filter.h"
#include "srt\srt_renderer.h"
#include "..\libchecksum\libchecksum.h"
#include "..\SsifSource\src\filters\parser\MpegSplitter\Imvc.h"
static const GUID CLSID_my12doomSource = { 0x8FD7B1DE, 0x3B84, 0x4817, { 0xA9, 0x6F, 0x4C, 0x94, 0x72, 0x8B, 0x1A, 0xAE } };

//#include "private_filter.h"

#define JIF(x) if (FAILED(hr=(x))){goto CLEANUP;}
#define DS_EVENT (WM_USER + 4)

// helper functions
HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

HRESULT GetConnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr)) // Connected, this is the pin we want. 
			{
				pTmp->Release();
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
			else  // Unconnected, not the pin we want.
			{
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

HRESULT dx_window::CrackPD10(IBaseFilter *filter)
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
dx_window::dx_window(RECT screen1, RECT screen2, HINSTANCE hExe):dwindow(screen1, screen2)
{
	// Enable away mode and prevent the sleep idle time-out.
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_AWAYMODE_REQUIRED);

	//PGS
	m_lastCBtime = -1;
	m_srenderer = NULL;

	// vars
	m_subtile_bitmap = NULL;
	m_renderer1 = NULL;
	m_renderer2 = NULL;
	m_PD10 = false;
	m_demuxer_config_active = false;
	m_select_font_active = false;
	m_log = (wchar_t*)malloc(100000);
	m_log[0] = NULL;
	m_mouse.x = m_mouse.y = -10;
	m_bar.load_resource(hExe);
	m_revert = false;
	m_mirror1 = 0;
	m_mirror2 = 0;
	m_letterbox_delta = 0.0;
	m_filter_mode = FILTER_MODE_FAIL;
	m_subtitle_center_x = 0.5;
	m_subtitle_bottom_y = 0.95;
	m_subtitle_offset = 0;
	m_last_subtitle[0] = NULL;
	m_loading = false;
	m_font_color = RGB(255,255,255);
	lFontPointSize = 60;
	_tcscpy(szFontName, TEXT("ËÎÌו"));
	_tcscpy(szFontStyle, TEXT("Regular"));
	select_font(false);

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
	set_timer(2, 125);

	// inits
	init_D3D9();
	init_direct_show();

	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(m_gb);
	event_ex->SetNotifyWindow((OAHWND)id_to_hwnd(1), DS_EVENT, 0);
}

dx_window::~dx_window()
{
	if (m_log)
	{
		free(m_log);
		m_log = NULL;
	}
	close_and_kill_thread();
	if (m_stereo)
		m_stereo->SetCallback(NULL);
	if (m_mono1)
		m_mono1->SetCallback(NULL);
	if (m_mono2)
		m_mono2->SetCallback(NULL);

	CAutoLock lock(&m_draw_sec);
	exit_direct_show();
	exit_D3D9();

	// Clear EXECUTION_STATE flags to disable away mode and allow the system to idle to sleep normally.
	SetThreadExecutionState(ES_CONTINUOUS);
}
DWORD WINAPI dx_window::select_font_thread(LPVOID lpParame)
{
	dx_window *_this = (dx_window*) lpParame;

	_this->m_select_font_active = true;

	HRESULT hr = _this->m_srenderer->select_font(false);
	if (SUCCEEDED(hr))
		hr = _this->m_srenderer->select_font(true);

	_this->m_select_font_active = false;

	return 0;
}

DWORD WINAPI dx_window::property_page_thread(LPVOID lpParame)
{
	/*
	dx_window *_this = (dx_window*) lpParame;
	if (_this->m_demuxer)
	{
		_this->m_demuxer_config_active = true;

		CComQIPtr<ISpecifyPropertyPages, &IID_ISpecifyPropertyPages> prop(_this->m_demuxer);
		if (prop)
		{
			CAUUID caGUID;
			prop->GetPages(&caGUID);

			IBaseFilter* tmp = _this->m_demuxer;

			OleCreatePropertyFrame(GetActiveWindow(), 0, 0, NULL, 1, (IUnknown **)&tmp, caGUID.cElems, caGUID.pElems, 0, 0, NULL);
			CoTaskMemFree(caGUID.pElems);
		}

		_this->m_demuxer_config_active = false;
	}
	*/

	return 0;
}

HRESULT dx_window::show_PD10_demuxer_config()
{
	if (m_demuxer_config_active)
		return E_FAIL;

	// create a new thread to avoid mouse hiding
	HANDLE show_thread = CreateThread(0,0,property_page_thread, this, NULL, NULL);
	return S_OK;
}

// play control
HRESULT dx_window::play()
{
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Run();
}
HRESULT dx_window::pause()
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
HRESULT dx_window::stop()
{
	CAutoLock lock(&m_seek_sec);
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Stop();
}
HRESULT dx_window::seek(int time)
{
	CAutoLock lock(&m_seek_sec);
	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

	REFERENCE_TIME target = (REFERENCE_TIME)time *10000;

	printf("seeking to %I64d\n", target);
	return m_ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
}
HRESULT dx_window::tell(int *time)
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
HRESULT dx_window::total(int *time)
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
HRESULT dx_window::set_volume(double volume)
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
HRESULT dx_window::get_volume(double *volume)
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

bool dx_window::is_closed()
{
	return !is_visible(1) && !is_visible(2);
}

// window handle part

LRESULT dx_window::on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == DS_EVENT)
	{
		on_dshow_event();
	}
	return dwindow::on_unhandled_msg(id, message, wParam, lParam);
}


LRESULT dx_window::on_display_change()
{
	m_renderer1->DisplayModeChanged();
	m_renderer2->DisplayModeChanged();

	return S_OK;
}

LRESULT dx_window::on_key_down(int id, int key)
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

	case VK_F11:
		show_PD10_demuxer_config();
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

LRESULT dx_window::on_nc_mouse_move(int id, int x, int y)
{
	return on_mouse_move(id, -10, -10);	// just to show ui, should not trigger any other events
}

LRESULT dx_window::on_mouse_move(int id, int x, int y)
{
	POINT mouse;
	GetCursorPos(&mouse);
	if ( (mouse.x - m_mouse.x) * (mouse.x - m_mouse.x) + (mouse.y - m_mouse.y) * (mouse.y - m_mouse.y) >= 100)
	{
		show_mouse(true);
		show_ui(true);
		set_timer(1, 1000);
	}

	RECT r;
	double v;
	GetClientRect(id_to_hwnd(id), &r);
	m_bar.total_width = r.right - r.left;
	int type = m_bar.hit_test(x, y-(r.bottom-r.top)+30, &v);
	if (type == 3 && GetAsyncKeyState(VK_LBUTTON) < 0)
	{
		set_volume(v);
	}
	return S_OK;
}

LRESULT dx_window::on_mouse_down(int id, int button, int x, int y)
{
	on_mouse_move(id, -10, -10);	// to show ui, should not trigger any other events
	if (button == VK_LBUTTON)
	{
		RECT r;
		GetClientRect(id_to_hwnd(id), &r);
		double v;
		int type;

		m_bar.total_width = r.right - r.left;
		type = m_bar.hit_test(x, y-(r.bottom-r.top)+30, &v);
		if (type == 1)
		{
			pause();
		}
		else if (type == 2)
		{
			set_fullscreen(2, !m_full2);		// yeah, this is not a bug
			set_fullscreen(1, m_full2);
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
		else if (type == -1)
		{
			// move this window
			ReleaseCapture();
			SendMessage(id_to_hwnd(id), WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
	}

	return S_OK;
}

LRESULT dx_window::on_timer(int id)
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

		m_bar.total_width = client1.right - client1.left;
		int test1 = m_bar.hit_test(mouse1.x, mouse1.y-(client1.bottom-client1.top)+30, NULL);

		m_bar.total_width = client2.right - client2.left;
		int test2 = m_bar.hit_test(mouse2.x, mouse2.y-(client2.bottom-client2.top)+30, NULL);		// warning

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

LRESULT dx_window::on_move(int id, int x, int y)
{
	update_video_pos();
	return S_OK;
}

LRESULT dx_window::on_size(int id, int type, int x, int y)
{
	update_video_pos();
	return S_OK;
}

LRESULT dx_window::on_paint(int id, HDC hdc)
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

LRESULT dx_window::on_double_click(int id, int button, int x, int y)
{
	set_fullscreen(2, !m_full2);		// yeah, this is not a bug
	set_fullscreen(1, m_full2);

	update_video_pos();
	return S_OK;
}

LRESULT dx_window::on_getminmaxinfo(int id, MINMAXINFO *info)
{
	info->ptMinTrackSize.x = 300;
	info->ptMinTrackSize.y = 200;

	return S_OK;
}

LRESULT dx_window::on_sys_command(int id, WPARAM wParam, LPARAM lParam)
{
	const int SC_MAXIMIZE2 = 0xF032;
	if (wParam == SC_MAXIMIZE || wParam == SC_MAXIMIZE2)
	{
		set_fullscreen(1, true);
		set_fullscreen(2, true);
		update_video_pos();
		return S_OK;
	}
	else if (LOWORD(wParam) == SC_SCREENSAVE)
		return 0;

	return S_FALSE;
}

LRESULT dx_window::on_init_dialog(int id, WPARAM wParam, LPARAM lParam)
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

HWND dx_window::id_to_video(int id)
{
	if (id == 1)
		return m_video1;
	else if (id == 2)
		return m_video2;
	else
		return NULL;
}
int dx_window::video_to_id(HWND video)
{
	if (video == m_video1)
		return 1;
	else if (video == m_video2)
		return 2;
	else
		return 0;
}
int dx_window::hwnd_to_id(HWND hwnd)
{
	if (video_to_id(hwnd))
		return video_to_id(hwnd);
	else
		return __super::hwnd_to_id(hwnd);
}

// directshow part

HRESULT dx_window::init_direct_show()
{
	HRESULT hr = S_OK;

	JIF(CoCreateInstance(CLSID_FilterGraph , NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&m_gb));
	JIF(m_gb->QueryInterface(IID_IMediaControl,  (void **)&m_mc));
	JIF(m_gb->QueryInterface(IID_IMediaSeeking,  (void **)&m_ms));
	JIF(m_gb->QueryInterface(IID_IBasicAudio,  (void **)&m_ba));

	return S_OK;

CLEANUP:
	exit_direct_show();
	return hr;
}

HRESULT dx_window::exit_direct_show()
{
	if (m_mc)
		m_mc->Stop();
		
	if (m_renderer1)
		{delete m_renderer1; m_renderer1=NULL;}
	if (m_renderer2)
		{delete m_renderer2; m_renderer2=NULL;}
	m_stereo = NULL;
	m_mono1 = NULL;
	m_mono2 = NULL;
		 
	m_ba = NULL;
	m_ms = NULL;
	m_mc = NULL;
	m_gb = NULL;

	return S_OK;
}

HRESULT dx_window::SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn)
{
	int ms_start = (int)(TimeStart / 10000);
	int ms_end = (int)(TimeEnd / 10000);

	// PGS test
	/*
	subtitle sub;
	m_pgs.find_subtitle(ms_start, ms_end, &sub);
	if (sub.start > 0 && sub.end >0 && sub.rgb)
	{
		m_text_surface_width = sub.width;
		m_text_surface_height = sub.height;

		m_text_surface = NULL;
		m_D3Ddevice->CreateOffscreenPlainSurface(
			m_text_surface_width, m_text_surface_height,
			D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM,
			&m_text_surface, NULL);

		D3DLOCKED_RECT locked_rect;
		m_text_surface->LockRect(&locked_rect, NULL, NULL);
		memcpy(locked_rect.pBits, sub.rgb, m_text_surface_width * m_text_surface_height * 4);
		m_text_surface->UnlockRect();

		//draw_subtitle();

		UnifyAlphaBitmap bmp = {sub.width, sub.height, sub.rgb, 
								(float)sub.left/1920.0,
								(float)sub.top/1080,
								(float)sub.width/1920,
								(float)sub.height/1080};

		HRESULT hr = m_renderer1->SetAlphaBitmap(bmp);
		hr = m_renderer2->SetAlphaBitmap(bmp);

		free(sub.rgb);
		return S_OK;
	}
	else
	{
		m_renderer1->ClearAlphaBitmap();
		m_renderer2->ClearAlphaBitmap();
	}
	*/

	// normal

	/*
	wchar_t subtitle[5000] = L"";
	m_srt.get_subtitle(ms_start, ms_end, subtitle);

	if (wcscmp(subtitle, m_last_subtitle) != 0)
	{
		draw_text(subtitle);
		draw_subtitle();
		wcscpy(m_last_subtitle, subtitle);
	}
	*/

	// CSubtitleRenderer test
	rendered_subtitle sub;
	if (m_srenderer)
	{
		HRESULT hr = m_srenderer->get_subtitle(ms_start, &sub, m_lastCBtime);
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

HRESULT dx_window::calculate_movie_rect(RECT *source, RECT *client, RECT *letterboxd, bool ui)
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

HRESULT dx_window::paint_letterbox(int id, RECT letterbox)
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

HRESULT dx_window::on_dshow_event()
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

HRESULT dx_window::update_video_pos()
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

HRESULT dx_window::set_revert(bool revert)
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

HRESULT dx_window::set_letterbox(double delta)
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

HRESULT dx_window::show_ui(bool show)
{
	m_show_ui = show;
	update_video_pos();
	return S_OK;
}

HRESULT dx_window::draw_ui()
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
		m_bar.total_width = client_rc.right - client_rc.left;
		m_bar.draw_total(paused, current, _total, volume);


		HDC hdc = GetDC(id_to_hwnd(id));
		HDC hdcBmp = CreateCompatibleDC(hdc);
		HBITMAP hbm = CreateCompatibleBitmap(hdc, 4096, 30);
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

		SetBitmapBits(hbm, 4096*30*4, m_bar.result);
		BitBlt(hdc, client_rc.left, client_rc.bottom-30, m_bar.total_width, 30, hdcBmp, 0, 0, SRCCOPY);

		DeleteObject(SelectObject(hdcBmp, hbmOld));
		DeleteObject(hbm);
		DeleteDC(hdcBmp);
		ReleaseDC(id_to_hwnd(id), hdc);
	}

	return S_OK;
}

HRESULT dx_window::start_loading()
{
	m_loading = true;
	return S_OK;
}

HRESULT dx_window::load_file(wchar_t *pathname, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
{
	wchar_t file_to_play[MAX_PATH];
	wcscpy(file_to_play, pathname);

	// Bluray Directory
	if(PathIsDirectoryW(file_to_play))
	{
		HMODULE hax = LoadLibraryW(L"codec\\SsifSource.ax");
		if (!hax)
		{
			log_line(L"module SsifSource.ax not found.");
			return E_FAIL;
		}

		HRESULT (*pfind) (wchar_t *, wchar_t *) = (HRESULT (__cdecl *)(wchar_t *, wchar_t *))GetProcAddress(hax, "find_main_movie");
		if (pfind == NULL)
		{
			log_line(L"module SsifSource.ax corrupted.");
			return E_FAIL;
		}

		wchar_t playlist[MAX_PATH];
		HRESULT hr;
		if (FAILED(hr = pfind(file_to_play, playlist)))
			return hr;
		else
			wcscpy(file_to_play, playlist);
	}

	// Legacy Remux file
	if (verify_file(file_to_play) == 2)
		return load_REMUX_file(file_to_play);

	// check private source and whether is MVC content
	CLSID source_clsid;
	HRESULT hr = GetFileSource(file_to_play, &source_clsid);
	if (SUCCEEDED(hr))
	{
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
				CComPtr<IBaseFilter> pd10_decoder;
				hr = myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&pd10_decoder);
				hr = m_gb->AddFilter(pd10_decoder, L"PD10 Decoder");
				hr = CrackPD10(pd10_decoder);
				imvc->SetPD10(TRUE);
				m_PD10 = true;
			}

		// then render pins
		CComPtr<IPin> pin;
		CComPtr<IEnumPins> pEnum;
		int audio_num = 0, video_num = 0;
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
							m_gb->Render(pin);
						video_num ++;
					}

					else if (wcsstr(info.achName, L"Audio"))
					{
						if ( (audio_track>=0 && (MKV_TRACK_NUMBER(audio_num) & audio_track ))
							|| audio_track == MKV_ALL_TRACK)
							m_gb->Render(pin);
						audio_num ++;
					}

					else;	// other tracks, ignore them
				}
			}
			pin = NULL;
		}

		// if it doesn't work....
		if (video_num == 0)
			hr = m_gb->RenderFile(file_to_play, NULL);
	}


	// normal file, just render it.
	else
		hr = m_gb->RenderFile(file_to_play, NULL);

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

	return hr;
}

HRESULT dx_window::load_BD3D_file(wchar_t *pathname)
{
	// add a decoder
	CComPtr<IBaseFilter> decoder;
	myCreateInstance(CLSID_PD10_DECODER, IID_IBaseFilter, (void**)&decoder);

	m_gb->AddFilter(decoder, L"PD10 decoder");

	// render file
	HRESULT hr = m_gb->RenderFile(pathname, NULL);

	if (hr == VFW_S_AUDIO_NOT_RENDERED)
		log_line(L"warning: audio not rendered. \"%s\"", pathname);

	if (hr == VFW_S_PARTIAL_RENDER)
		log_line(L"warning: Some of the streams in this movie are in an unsupported format. \"%s\"", pathname);

	if (hr == VFW_S_VIDEO_NOT_RENDERED)
		log_line(L"warning: video not rendered. \"%s\"", pathname);

	if (FAILED(hr))
	{
		log_line(L"failed rendering iso \"%s\" (error = 0x%08x).", pathname, hr);
		return hr;
	}

	// find and test if mvc
	bool ismvc = false;
	CComPtr<IEnumFilters> ef;
	m_gb->EnumFilters(&ef);

	CComPtr<IBaseFilter> filter;
	while (S_OK == ef->Next(1, &filter, NULL))
	{
		CComQIPtr<IMVC, &IID_IMVC> imvc(filter);
		if (imvc)
			if (imvc->IsMVC() == S_OK)
			{
				imvc->SetPD10(TRUE);
				ismvc = true;
			}
		filter = NULL;
	}

	if (!ismvc)
	{
		log_line(L"failed activing MVC.");
		return E_FAIL;
	}

	hr = CrackPD10(decoder);
	if (FAILED(hr))
	{
		log_line(L"failed activing MVC (error = 0x%08x).", hr);
	}

	m_PD10 = true;
	return hr;
}

HRESULT dx_window::load_REMUX_file(wchar_t *pathname)
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

	m_gb->AddFilter(source_base, L"PD10 Source");
	//m_gb->AddFilter(m_demuxer, L"PD10 demuxer");
	m_gb->AddFilter(decoder, L"PD10 decoder");

	// load file
	HRESULT hr = source->Load(pathname, NULL);
	if (FAILED(hr))
		return E_FAIL;		//dshow fail
	
	/*
	// connect source and demuxer
	CComPtr<IPin> source_out;
	CComPtr<IPin> demuxer_in;
	CComPtr<IPin> demuxer_out;

	GetUnconnectedPin(source_base, PINDIR_OUTPUT, &source_out);
	GetUnconnectedPin(m_demuxer, PINDIR_INPUT, &demuxer_in);
	hr = m_gb->ConnectDirect(source_out, demuxer_in, NULL);

	GetUnconnectedPin(m_demuxer, PINDIR_OUTPUT, &demuxer_out);
	if (demuxer_out == NULL || FAILED(hr))
	{
		log_line(L"demuxer doesn't accept this file");
		return E_FAIL;
	}
	*/

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

HRESULT dx_window::load_mkv_file(wchar_t *pathname, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
{
	if (pathname == NULL)
		return E_POINTER;

	// create & load & add to filter
	CComPtr<IBaseFilter> haali;
	haali.CoCreateInstance(CLSID_HaaliSimple);
	//load_object(CLSID_HaaliSimple, IID_IBaseFilter, (void**)&haali);

	if (haali == NULL)
		return E_FAIL;

	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> haali_source(haali);
	if (haali_source == NULL)
	{
		log_line(L"haali media splitter not found, install it first.");
		return E_FAIL;
	}

	HRESULT hr = haali_source->Load(pathname, NULL);
	if (FAILED(hr))
		return hr;

	m_gb->AddFilter(haali, L"haali simple source");

	// render pins
	CComPtr<IPin> pin;
	CComPtr<IEnumPins> pEnum;
	int audio_num = 0, video_num = 0;
	hr = haali->EnumPins(&pEnum);
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
				PIN_INFO info;
				pin->QueryPinInfo(&info);
				if (info.pFilter) info.pFilter->Release();

				if (wcsstr(info.achName, L"Video"))
				{
					if ( (video_track>=0 && (MKV_TRACK_NUMBER(video_num) & video_track ))
						|| video_track == MKV_ALL_TRACK)
						m_gb->Render(pin);
					video_num ++;
				}

				else if (wcsstr(info.achName, L"Audio"))
				{
					if ( (audio_track>=0 && (MKV_TRACK_NUMBER(audio_num) & audio_track ))
						|| audio_track == MKV_ALL_TRACK)
						m_gb->Render(pin);
					audio_num ++;
				}

				else;	// other tracks, ignore them
			}
		}
		pin = NULL;
	}
	return S_OK;
}


HRESULT dx_window::load_3dv_file(wchar_t *pathname, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
{
	if (pathname == NULL)
		return E_POINTER;

	// create & load & add to filter
	CComPtr<IBaseFilter> source;
	myCreateInstance(CLSID_3dvSource, IID_IBaseFilter, (void**)&source);

	if (source == NULL)
		return E_FAIL;

	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> haali_source(source);
	if (haali_source == NULL)
	{
		log_line(L"haali media splitter not found, install it first.");
		return E_FAIL;
	}

	HRESULT hr = haali_source->Load(pathname, NULL);
	if (FAILED(hr))
		return hr;

	m_gb->AddFilter(source, L"haali simple source");

	// render pins
	CComPtr<IPin> pin;
	CComPtr<IEnumPins> pEnum;
	int audio_num = 0, video_num = 0;
	hr = source->EnumPins(&pEnum);
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
			else	// Unconnected, this is the pin we may want.
			{		//just render every out pin
				m_gb->Render(pin);
			}
		}
		pin = NULL;
	}
	return S_OK;
}


HRESULT dx_window::load_E3D_file(wchar_t *pathname, int audio_track /* = MKV_FIRST_TRACK */, int video_track /* = MKV_ALL_TRACK */)
{
	if (pathname == NULL)
		return E_POINTER;

	// create & load & add to filter
	CComPtr<IBaseFilter> haali;
	haali.CoCreateInstance(CLSID_E3DSource);
	//load_object(CLSID_HaaliSimple, IID_IBaseFilter, (void**)&haali);

	if (haali == NULL)
		return E_FAIL;

	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> haali_source(haali);
	if (haali_source == NULL)
	{
		log_line(L"E3D Source filter not found, install it first.");
		return E_FAIL;
	}

	HRESULT hr = haali_source->Load(pathname, NULL);
	if (FAILED(hr))
		return hr;

	m_gb->AddFilter(haali, L"E3D source");

	// render pins
	CComPtr<IPin> pin;
	CComPtr<IEnumPins> pEnum;
	int audio_num = 0, video_num = 0;
	hr = haali->EnumPins(&pEnum);
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
				PIN_INFO info;
				pin->QueryPinInfo(&info);
				if (info.pFilter) info.pFilter->Release();

				if (wcsstr(info.achName, L"Video"))
				{
					if ( (video_track>=0 && (MKV_TRACK_NUMBER(video_num) & video_track ))
						|| video_track == MKV_ALL_TRACK)
						m_gb->Render(pin);
					video_num ++;
				}

				else if (wcsstr(info.achName, L"Audio"))
				{
					if ( (audio_track>=0 && (MKV_TRACK_NUMBER(audio_num) & audio_track ))
						|| audio_track == MKV_ALL_TRACK)
						m_gb->Render(pin);
					audio_num ++;
				}

				else;	// other tracks, ignore them
			}
		}
		pin = NULL;
	}
	return S_OK;
}
HRESULT dx_window::set_PD10(bool PD10 /*= false*/)
{
	m_PD10 = PD10;

	return S_OK;
}


HRESULT dx_window::end_loading()
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

HRESULT dx_window::end_loading_sidebyside(IPin **pin1, IPin **pin2)
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

HRESULT dx_window::end_loading_double_stream(IPin **pin1, IPin **pin2)
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


HRESULT dx_window::create_myfilter()
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


HRESULT dx_window::end_loading_step2(IPin *pin1, IPin *pin2)
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
HRESULT dx_window::load_subtitle(wchar_t *pathname, bool reset)			//FIXME : always reset 
{
	if (pathname == NULL)
		return E_POINTER;

	if (!m_loading)
		return VFW_E_WRONG_STATE;

	if (reset && m_srenderer)
		m_srenderer->reset();

	if (m_srenderer)
		delete m_srenderer;

	wchar_t *p_3 = pathname + wcslen(pathname) -3;
	if ( (p_3[0] == L's' || p_3[0] == L'S') &&
		(p_3[1] == L'r' || p_3[1] == L'R') &&
		(p_3[2] == L't' || p_3[2] == L'T'))
	{
		m_srenderer = new CsrtRenderer();
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

	return S_OK;
}

HRESULT dx_window::init_D3D9()
{
	exit_D3D9();

	m_D3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));

	D3DPRESENT_PARAMETERS d3dpp;    
	ZeroMemory( &d3dpp, sizeof(d3dpp) );   
	d3dpp.Windowed         = TRUE;   
	d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;   
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;   
	d3dpp.hDeviceWindow    = m_hwnd1;   

	m_D3D->CreateDevice(    
		D3DADAPTER_DEFAULT, // always the primary display adapter   
		D3DDEVTYPE_HAL,   
		NULL,   
		D3DCREATE_HARDWARE_VERTEXPROCESSING,   
		&d3dpp,   
		&m_D3Ddevice);

	return S_OK;
}

HRESULT dx_window::exit_D3D9()
{
	m_text_surface = NULL;
	m_D3Ddevice = NULL;
	m_D3D = NULL;

	return S_OK;
}

HRESULT dx_window::draw_text(wchar_t *text)
{
	if (text[0] == NULL)
	{
		m_text_surface_width = m_text_surface_height = 0;
		return S_OK;
	}

	HDC hdc = GetDC(NULL);
	HDC hdcBmp = CreateCompatibleDC(hdc);

	HFONT hOldFont = (HFONT) SelectObject(hdcBmp, m_font);

	RECT rect = {0,0,0,0};
	DrawTextW(hdcBmp, text, (int)wcslen(text), &rect, DT_CENTER | DT_CALCRECT);
	m_text_surface_height = rect.bottom - rect.top;
	m_text_surface_width  = rect.right - rect.left;

	HBITMAP hbm = CreateCompatibleBitmap(hdc, m_text_surface_width, m_text_surface_height);

	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

	RECT rcText;
	SetRect(&rcText, 0, 0, m_text_surface_width, m_text_surface_height);
	SetBkColor(hdcBmp, RGB(0, 0, 0));					// Pure black background
	SetTextColor(hdcBmp, RGB(255, 255, 255));			// white text for alpha

	DrawTextW(hdcBmp, text, (int)wcslen(text), &rect, DT_CENTER);

	m_text_surface = NULL;
	m_D3Ddevice->CreateOffscreenPlainSurface(
		m_text_surface_width, m_text_surface_height,
		D3DFMT_A8R8G8B8,
		D3DPOOL_SYSTEMMEM,
		&m_text_surface, NULL);

	D3DLOCKED_RECT locked_rect;
	m_text_surface->LockRect(&locked_rect, NULL, NULL);
	GetBitmapBits(hbm, m_text_surface_height * m_text_surface_width * 4, locked_rect.pBits);
	if(m_subtile_bitmap) free(m_subtile_bitmap);
	m_subtile_bitmap = (BYTE *) malloc(m_text_surface_width * m_text_surface_height*4);
	GetBitmapBits(hbm, m_text_surface_height * m_text_surface_width * 4, m_subtile_bitmap);


	BYTE *data = (BYTE*)m_subtile_bitmap;
	DWORD *data_dw = (DWORD*)m_subtile_bitmap;
	unsigned char color_r = (BYTE)(m_font_color       & 0xff);
	unsigned char color_g = (BYTE)((m_font_color>>8)  & 0xff);
	unsigned char color_b = (BYTE)((m_font_color>>16) & 0xff);
	DWORD color = (color_r<<16) | (color_g <<8) | (color_b);// reverse big endian

	for(int i=0; i<m_text_surface_width * m_text_surface_height; i++)
	{
		unsigned char alpha = data[i*4+2];
		data_dw[i] = color;
		data[i*4+3] = alpha;
	}

	m_text_surface->UnlockRect();

	DeleteObject(SelectObject(hdcBmp, hbmOld));
	SelectObject(hdc, hOldFont);
	DeleteObject(hbm);
	DeleteDC(hdcBmp);
	ReleaseDC(NULL, hdc);

	return S_OK;
}

HRESULT dx_window::draw_subtitle()
{
	// force refresh on next frame
	m_lastCBtime = -1;
	/*
	if (m_text_surface_width == 0 || m_text_surface_height == 0)
	{
		m_renderer1->ClearAlphaBitmap();
		m_renderer2->ClearAlphaBitmap();
		return S_OK;
	}

	float p_x = (float)m_text_surface_width / 1920;
	float p_y = (float)m_text_surface_height / 1440;

	UnifyAlphaBitmap bmp;
	bmp.data = m_subtile_bitmap;
	bmp.width = m_text_surface_width;
	bmp.height = m_text_surface_height;
	bmp.left = (float)(m_subtitle_center_x - p_x/2);
	bmp.top = (float)(m_subtitle_bottom_y - p_y);
	bmp.fwidth = (float)p_x;
	bmp.fheight = (float)p_y;

	m_renderer1->SetAlphaBitmap(bmp);
	bmp.left += (float) m_subtitle_offset / 1920;
	m_renderer2->SetAlphaBitmap(bmp);
	*/

	if (m_mc)
	{
		OAFilterState fs;
		HRESULT hr = m_mc->GetState(50, &fs);
		if (SUCCEEDED(hr) && fs != State_Running)
			m_mc->StopWhenReady();
	}
	return S_OK;
}

HRESULT dx_window::set_subtitle_offset(int offset)
{
	m_subtitle_offset = offset;
	draw_subtitle();
	return S_OK;
}

HRESULT dx_window::set_subtitle_pos(double center_x, double bottom_y)
{
	m_subtitle_center_x = center_x;
	m_subtitle_bottom_y = bottom_y;
	return set_subtitle_offset(m_subtitle_offset);	// this is for m_mc->StopWhenReady();
}

HRESULT dx_window::select_font(bool show_dlg)
{

	CHOOSEFONT cf={0};
	memset(&cf, 0, sizeof(cf));
	LOGFONT lf={0}; 
	HDC hdc;
	LONG lHeight;

	// Convert requested font point size to logical units
	hdc = GetDC( NULL );
	lHeight = -MulDiv( lFontPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
	ReleaseDC( NULL, hdc );

	// Initialize members of the LOGFONT structure. 
	lstrcpyn(lf.lfFaceName, szFontName, 32);
	lf.lfHeight = lHeight;      // Logical units
	lf.lfQuality = ANTIALIASED_QUALITY;

	// Initialize members of the CHOOSEFONT structure. 
	cf.lStructSize = sizeof(CHOOSEFONT); 
	cf.hwndOwner   = NULL; 
	cf.hDC         = (HDC)NULL; 
	cf.lpLogFont   = &lf; 
	cf.iPointSize  = lFontPointSize * 10; 
	cf.rgbColors   = m_font_color; 
	cf.lCustData   = 0L; 
	cf.lpfnHook    = (LPCFHOOKPROC)NULL; 
	cf.hInstance   = (HINSTANCE) NULL; 
	cf.lpszStyle   = szFontStyle; 
	cf.nFontType   = SCREEN_FONTTYPE; 
	cf.nSizeMin    = 0; 
	cf.lpTemplateName = NULL; 
	cf.nSizeMax = 720; 
	cf.Flags = CF_SCREENFONTS | CF_SCALABLEONLY | CF_INITTOLOGFONTSTRUCT | 
		CF_EFFECTS     | CF_USESTYLE     | CF_LIMITSIZE; 

	if (show_dlg)
		ChooseFont(&cf);

	lstrcpyn(szFontName, lf.lfFaceName, sizeof(szFontName)/sizeof(TCHAR));
	lFontPointSize = cf.iPointSize / 10;  // Specified in 1/10 point units
	m_font_color = cf.rgbColors;
	m_font = CreateFontIndirect(cf.lpLogFont); 

	return S_OK;
}

HRESULT dx_window::log_line(wchar_t *format, ...)
{
	wcscat(m_log, L"\r\n");

	wchar_t tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	wcscat(m_log, tmp);
	return S_OK;
}

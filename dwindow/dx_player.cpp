#include <math.h>
#include "dx_player.h"
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
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
			else  // Unconnected, not the pin we want.
			{
				if(pTmp) pTmp->Release();
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

// constructor & destructor
dx_window::dx_window(RECT screen1, RECT screen2, HINSTANCE hExe):dwindow(screen1, screen2)
{
	// vars
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
}
DWORD WINAPI dx_window::select_font_thread(LPVOID lpParame)
{
	dx_window *_this = (dx_window*) lpParame;

	_this->m_select_font_active = true;
	_this->select_font();
	_this->m_select_font_active = false;

	return 0;
}

DWORD WINAPI dx_window::property_page_thread(LPVOID lpParame)
{
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
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Run();
}
HRESULT dx_window::pause()
{
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
	if (m_mc == NULL)
		return VFW_E_WRONG_STATE;

	return m_mc->Stop();
}
HRESULT dx_window::seek(int time)
{
	if (m_ms == NULL)
		return VFW_E_WRONG_STATE;

	REFERENCE_TIME target = (REFERENCE_TIME)time *10000;
	return m_ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
}
HRESULT dx_window::tell(int *time)
{
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
	if (m_vmr1c)
		m_vmr1c->DisplayModeChanged();

	if (m_vmr2c)
		m_vmr2c->DisplayModeChanged();

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
	int type = m_bar.test(x, y-(r.bottom-r.top)+30, &v);
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
		type = m_bar.test(x, y-(r.bottom-r.top)+30, &v);
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
		int test1 = m_bar.test(mouse1.x, mouse1.y-(client1.bottom-client1.top)+30, NULL);

		m_bar.total_width = client2.right - client2.left;
		int test2 = m_bar.test(mouse2.x, mouse2.y-(client2.bottom-client2.top)+30, NULL);		// warning

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
		IVMRWindowlessControl9 *c = m_vmr1c;
		if (m_revert) c = m_vmr2c;

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
		IVMRWindowlessControl9 *c = m_vmr2c;
		if (m_revert) c = m_vmr1c;
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

	return S_FALSE;
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
		
	m_vmr1c = NULL;
	m_vmr2c = NULL;
	m_vmr1bmp = NULL;
	m_vmr2bmp = NULL;
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

	wchar_t subtitle[5000] = L"";
	m_srt.get_subtitle(ms_start, ms_end, subtitle);

	if (wcscmp(subtitle, m_last_subtitle) != 0)
	{
		draw_text(subtitle);
		draw_subtitle();
		wcscpy(m_last_subtitle, subtitle);
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
	RECT letterbox;

	// set window pos
	if (m_vmr1c)
	{
		int id = 1;
		if (m_revert) id = 2;

		GetClientRect(id_to_hwnd(id), &client);
		m_vmr1c->GetNativeVideoSize(&source.right, &source.bottom, NULL, NULL);
		calculate_movie_rect(&source, &client, &letterbox, m_show_ui);

		m_vmr1c->SetVideoPosition(&source, &client);

		// mirror
		VMR9NormalizedRect mirror_rect = {0,0,1,1};
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
		CComQIPtr<IVMRMixerControl9, &IID_IVMRMixerControl9> mixer1(m_vmr1c);
		mixer1->SetOutputRect(0, &mirror_rect);

		// paint letterbox
		paint_letterbox(id, letterbox);
	}

	if (m_vmr2c)
	{
		int id = 2;
		if (m_revert) id = 1;

		GetClientRect(id_to_hwnd(id), &client);
		m_vmr2c->GetNativeVideoSize(&source.right, &source.bottom, NULL, NULL);
		calculate_movie_rect(&source, &client, &letterbox, m_show_ui);

		m_vmr2c->SetVideoPosition(&source, &client);

		// mirror
		VMR9NormalizedRect mirror_rect = {0,0,1,1};
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
		CComQIPtr<IVMRMixerControl9, &IID_IVMRMixerControl9> mixer2(m_vmr2c);
		mixer2->SetOutputRect(0, &mirror_rect);

		// paint letterbox
		paint_letterbox(id, letterbox);
	}

	draw_ui();
    
	return S_OK;
}

HRESULT dx_window::set_revert(bool revert)
{
	m_revert = revert;

	if (m_vmr1c)
	{
		if (m_revert)
			m_vmr1c->SetVideoClippingWindow(id_to_hwnd(2));
		else
			m_vmr1c->SetVideoClippingWindow(id_to_hwnd(1));
	}

	if (m_vmr2c)
	{
		if (m_revert)
			m_vmr2c->SetVideoClippingWindow(id_to_hwnd(1));
		else
			m_vmr2c->SetVideoClippingWindow(id_to_hwnd(2));
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

HRESULT dx_window::load_file(wchar_t *pathname)
{
	HRESULT hr = m_gb->RenderFile(pathname, NULL);

	if (hr == VFW_S_AUDIO_NOT_RENDERED)
		log_line(L"warning: audio not rendered. \"%s\"", pathname);

	if (hr == VFW_S_PARTIAL_RENDER)
		log_line(L"warning: Some of the streams in this movie are in an unsupported format. \"%s\"", pathname);

	if (hr == VFW_S_VIDEO_NOT_RENDERED)
		log_line(L"warning: video not rendered. \"%s\"", pathname);

	if (FAILED(hr))
	{
		log_line(L"failed rendering \"%s\" (error = 0x%08x).", pathname, hr);
	}

	return hr;
}

HRESULT dx_window::load_PD10_file(wchar_t *pathname)
{
	if (pathname == NULL)
		return E_POINTER;

	// create filters and add to graph
	CComPtr<IBaseFilter> decoder;
	m_demuxer.CoCreateInstance(CLSID_PD10_DEMUXER);
	decoder.CoCreateInstance(CLSID_PD10_DECODER);

	CComPtr<IBaseFilter> source_base;
	source_base.CoCreateInstance(CLSID_AsyncReader);
	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source(source_base);

	if (m_demuxer == NULL || decoder == NULL)
	{
		log_line(L"cyberlink PowerDVD10 module not found, register it first.");
		return E_FAIL;		// cyberlink fail
	}
	if (source_base == NULL || source == NULL)
		return E_FAIL;		// dshow fail

	m_gb->AddFilter(source_base, L"PD10 Source");
	m_gb->AddFilter(m_demuxer, L"PD10 demuxer");
	m_gb->AddFilter(decoder, L"PD10 decoder");

	// load file
	HRESULT hr = source->Load(pathname, NULL);
	if (FAILED(hr))
		return E_FAIL;		//dshow fail
	
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

	// render demuxer output pins
	CComPtr<IPin> pin;
	CComPtr<IEnumPins> pEnum;
	hr = m_demuxer->EnumPins(&pEnum);
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
	return S_OK;
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

	end_loading_step2(pin1, pin2);
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
			m_gb->ConnectDirect(output, stereo_pin, NULL);
			
			CComPtr<IPin> check;
			stereo_pin->ConnectedTo(&check);
			if (check == NULL)
			{
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
		//hr = load_object(CLSID_DWindowMono, IID_IDWindowExtender, (void**)&m_mono1);
		m_mono1.CoCreateInstance(CLSID_DWindowMono);
		//hr = load_object(CLSID_DWindowMono, IID_IDWindowExtender, (void**)&m_mono2);
		m_mono2.CoCreateInstance(CLSID_DWindowMono);

		if (m_mono1 == NULL || m_mono2 == NULL)
			hr = E_FAIL;
	}

	if (m_filter_mode == FILTER_MODE_STEREO)
	{
		//hr = load_object(CLSID_DWindowStereo, IID_IDWindowExtender, (void**)&m_stereo);
		m_stereo.CoCreateInstance(CLSID_DWindowStereo);

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
	CComPtr<IBaseFilter> vmr1base;
	CComPtr<IBaseFilter> vmr2base;
	vmr1base.CoCreateInstance(CLSID_VideoMixingRenderer9);
	vmr2base.CoCreateInstance(CLSID_VideoMixingRenderer9);

	CComQIPtr<IVMRFilterConfig9, &IID_IVMRFilterConfig9> config1(vmr1base);
	config1->SetRenderingMode(VMR9Mode_Windowless);
	CComQIPtr<IVMRFilterConfig9, &IID_IVMRFilterConfig9> config2(vmr2base);
	config2->SetRenderingMode(VMR9Mode_Windowless);

	vmr1base->QueryInterface(IID_IVMRWindowlessControl9, (void**)&m_vmr1c);
	vmr2base->QueryInterface(IID_IVMRWindowlessControl9, (void**)&m_vmr2c);
	vmr1base->QueryInterface(IID_IVMRMixerBitmap9, (void**)&m_vmr1bmp);
	vmr2base->QueryInterface(IID_IVMRMixerBitmap9, (void**)&m_vmr2bmp);

	m_gb->AddFilter(vmr1base, L"VMR9 #1");
	m_gb->AddFilter(vmr2base, L"VMR9 #2");

	// config output window
	m_vmr1c->SetAspectRatioMode(VMR9ARMode_LetterBox);
	m_vmr2c->SetAspectRatioMode(VMR9ARMode_LetterBox);
	set_revert(m_revert);

	// connect pins
	CComPtr<IPin> vmr1pin;
	CComPtr<IPin> vmr2pin;

	GetUnconnectedPin(vmr1base, PINDIR_INPUT, &vmr1pin);
	GetUnconnectedPin(vmr2base, PINDIR_INPUT, &vmr2pin);

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
HRESULT dx_window::load_srt(wchar_t *pathname, bool reset)
{
	if (pathname == NULL)
		return E_POINTER;

	if (!m_loading)
		return VFW_E_WRONG_STATE;

	if (reset)
		m_srt.init(20480, 2048*1024);

	m_srt.load(pathname);

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

	BYTE *data = (BYTE*)locked_rect.pBits;
	DWORD *data_dw = (DWORD*)locked_rect.pBits;
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
	// when width=0 or height=0, the VMR don't show anything
	// so we don't care about what happened
	VMR9AlphaBitmap bmp_info;

	float p_x = (float)m_text_surface_width / 1920;
	float p_y = (float)m_text_surface_height / 1440;

	RECT rect = {0,0,m_text_surface_width,m_text_surface_height};
	VMR9NormalizedRect rect_dst = {(float)(m_subtitle_center_x - p_x/2),
								   (float)(m_subtitle_bottom_y - p_y),
								   (float)(m_subtitle_center_x + p_x/2),
								   (float)m_subtitle_bottom_y};

	bmp_info.dwFlags = VMR9AlphaBitmap_EntireDDS | VMR9AlphaBitmap_FilterMode;
	bmp_info.hdc = NULL;
	bmp_info.pDDS = m_text_surface;
	bmp_info.rSrc = rect;
	bmp_info.rDest= rect_dst;
	bmp_info.fAlpha = 1;
	bmp_info.clrSrcKey = 0xffffffff;
	bmp_info.dwFilterMode = MixerPref9_AnisotropicFiltering;

	m_vmr1bmp->SetAlphaBitmap(&bmp_info);

	bmp_info.rDest.left += (float)m_subtitle_offset / 1920;
	bmp_info.rDest.right += (float)m_subtitle_offset / 1920;
	m_vmr2bmp->SetAlphaBitmap(&bmp_info);


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

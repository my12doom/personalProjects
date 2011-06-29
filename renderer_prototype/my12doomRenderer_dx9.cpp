// Direct3D9 part of my12doom renderer

#include "my12doomRenderer.h"
#include "PixelShaders/YV12.h"
#include "PixelShaders/NV12.h"
#include "PixelShaders/YUY2.h"
#include "PixelShaders/anaglyph.h"
#include "PixelShaders/stereo_test.h"
#include "3dvideo.h"
#include <dvdmedia.h>
#include <math.h>
#include "..\dwindow\nvapi.h"

#define FAIL_RET(x) hr=x; if(FAILED(hr)){return hr;}
#define FAIL_SLEEP_RET(x) hr=x; if(FAILED(hr)){Sleep(1); return hr;}

bool DWINDOW_SURFACE_MANAGED = LOBYTE(GetVersion()) <= 5 ? true : false;

typedef struct tagTHREADNAME_INFO {
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try {
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_CONTINUE_EXECUTION) {
	}
}

HRESULT mylog(wchar_t *format, ...)
{
#ifdef DEBUG
	wchar_t tmp[10240];
	wchar_t tmp2[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	wsprintfW(tmp2, L"(tid=%d)%s", GetCurrentThreadId(), tmp);
	OutputDebugStringW(tmp);
#endif
	return S_OK;
}


HRESULT mylog(const char *format, ...)
{
#ifdef DEBUG
	char tmp[10240];
	char tmp2[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfA(tmp, format, valist);
	va_end(valist);

	wsprintfA(tmp2, "(tid=%d)%s", GetCurrentThreadId(), tmp);
	OutputDebugStringA(tmp);
#endif
	return S_OK;
}

my12doomRenderer::my12doomRenderer(HWND hwnd, HWND hwnd2/* = NULL*/):
m_left_queue(_T("left queue")),
m_right_queue(_T("right queue"))
{
	// D3D && NV3D
	m_D3D = Direct3DCreate9( D3D_SDK_VERSION );
	m_nv3d_enabled = false;
	m_nv3d_actived = false;
	NvAPI_Status res = NvAPI_Initialize();
	if (NVAPI_OK == res)
	{
		NvU8 enabled3d;
		res = NvAPI_Stereo_IsEnabled(&enabled3d);
		if (res == NVAPI_OK)
		{
			printf("NV3D enabled.\n");
			m_nv3d_enabled = (bool)enabled3d;
		}
	}

	// UI
	m_uidrawer = new ui_drawer_dwindow;

	// AES
	unsigned char ran[32];
	m_AES.set_key(ran, 256);

	// window
	m_hWnd = hwnd;
	m_hWnd2 = hwnd2;

	// thread
	m_device_threadid = GetCurrentThreadId();
	m_device_state = need_create;
	m_render_thread = INVALID_HANDLE_VALUE;
	m_render_thread_exit = false;


	// ui
	m_showui = false;
	m_ui_visible_last_change_time = m_last_ui_draw = timeGetTime();

	init_variables();

	//CreateThread(0,0,test_thread, this, NULL, NULL);
}

void my12doomRenderer::init_variables()
{
	// parallax
	m_parallax = 0;

	// just for surface creation
	m_lVidWidth = m_lVidHeight = 64;
	// assume already removed from graph
	m_recreating_dshow_renderer = true;
	m_dshow_renderer1 = NULL;
	m_dshow_renderer2 = NULL;

	// dshow creation
	HRESULT hr;
	m_dsr0 = new my12doomRendererDShow(NULL, &hr, this, 0);
	m_dsr1 = new my12doomRendererDShow(NULL, &hr, this, 1);

	m_dsr0->QueryInterface(IID_IBaseFilter, (void**)&m_dshow_renderer1);
	m_dsr1->QueryInterface(IID_IBaseFilter, (void**)&m_dshow_renderer2);
	m_recreating_dshow_renderer = false;

	// callback
	m_cb = NULL;

	// aspect and offset
	m_bmp_offset_x = 0.0;
	m_bmp_offset_y = 0.0;
	m_source_aspect = 1.0;
	m_forced_aspect = -1;

	// input / output
	m_input_layout = input_layout_auto;
	m_swapeyes = false;
	m_output_mode = mono;
	m_mask_mode = row_interlace;
	m_color1 = D3DCOLOR_XRGB(255, 0, 0);
	m_color2 = D3DCOLOR_XRGB(0, 255, 255);

	// clear rgb backup
	m_surface_rgb_backup_full = NULL;

	// ui & bitmap
	m_volume = 0;
	m_bmp_offset = 0;
	m_bmp_width = 0;
	m_bmp_height = 0;
	m_total_time = 0;
}

my12doomRenderer::~my12doomRenderer()
{
	terminate_render_thread();
	invalidate_objects();
	m_Device = NULL;
	m_D3D = NULL;
}

HRESULT my12doomRenderer::CheckMediaType(const CMediaType *pmt, int id)
{
	int width, height, aspect_x, aspect_y;
	if (*pmt->FormatType() == FORMAT_VideoInfo)
	{
		VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)pmt->Format();
		width = vihIn->bmiHeader.biWidth;
		height = vihIn->bmiHeader.biHeight;
		aspect_x = width;
		aspect_y = abs(height);
		m_frame_length = vihIn->AvgTimePerFrame;
	}

	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->Format();
		if (vihIn->dwInterlaceFlags & AMINTERLACE_IsInterlaced)
			return E_FAIL;
		width = vihIn->bmiHeader.biWidth;
		height = vihIn->bmiHeader.biHeight;
		aspect_x = vihIn->dwPictAspectRatioX;
		aspect_y = vihIn->dwPictAspectRatioY;
		m_frame_length = vihIn->AvgTimePerFrame;

		//AMINTERLACE_IsInterlaced
		// 0x21
	}
	else
		return E_FAIL;

	if (height > 0)
		m_revert_RGB32 = true;
	else
		m_revert_RGB32 = false;

	height = abs(height);

	if (aspect_x == 1920 && aspect_y == 1088)
	{
		aspect_x = 16;
		aspect_y = 9;
	}

	if (id == 0)
	{
		m_lVidWidth = width;
		m_lVidHeight = height;
		m_source_aspect = (double)aspect_x / aspect_y;

		return S_OK;
	}

	if (!m_dsr0->is_connected())
		return E_FAIL;

	if (width == m_lVidWidth && height == m_lVidHeight)			// format is not important, but resolution is
	{
		m_source_aspect = (double)aspect_x / aspect_y;
		return S_OK;
	}
	else
		return E_FAIL;
}
HRESULT	my12doomRenderer::BreakConnect(int id)
{
	return S_OK;
}
HRESULT my12doomRenderer::CompleteConnect(IPin *pRecievePin, int id)
{
	m_normal = m_sbs = m_tb = 0;
	m_layout_detected = mono2d;
	m_no_more_detect = false;

	if (m_source_aspect > 2.425)
	{
		m_layout_detected = side_by_side;
		m_no_more_detect = true;
	}
	else if (m_source_aspect < 1.2125)
	{
		m_layout_detected = top_bottom;
		m_no_more_detect = true;
	}

	return S_OK;
}

HRESULT my12doomRenderer::DoRender(int id, IMediaSample *media_sample)
{
	int l1 = timeGetTime();
	bool should_render = true;
	REFERENCE_TIME start, end;
	media_sample->GetTime(&start, &end);
	if (m_cb && id == 0)
		m_cb->SampleCB(start + m_dsr0->m_thisstream, end + m_dsr0->m_thisstream, media_sample);

	int l2 = timeGetTime();
	if (!m_dsr1->is_connected())
	{
		// single stream
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;
		do
		{
			if (sample) 
			{
				printf("drop left\n");
				delete sample;
				sample = NULL;
			}
			if (m_left_queue.GetCount())
				sample = m_left_queue.RemoveHead();
		}while (sample && sample->m_start < start);

		if(sample)
		{
			CAutoLock lck(&m_packet_lock);
			m_sample2render_1.Free();
			m_sample2render_1.Attach(sample);
			should_render = true;
		}
	}
	else
	{
find_match:
		bool matched = false;
		REFERENCE_TIME matched_time = -1;
		gpu_sample *sample_left = NULL;
		gpu_sample *sample_right = NULL;

		// media sample matching
		{
			// find match
			CAutoLock lck(&m_queue_lock);
			for(POSITION pos_left = m_left_queue.GetHeadPosition(); pos_left; pos_left = m_left_queue.Next(pos_left))
			{
				gpu_sample *left_sample = m_left_queue.Get(pos_left);
				for(POSITION pos_right = m_right_queue.GetHeadPosition(); pos_right; pos_right = m_right_queue.Next(pos_right))
				{
					gpu_sample *right_sample = m_right_queue.Get(pos_right);
					if (abs((int)(left_sample->m_start - right_sample->m_start)) < 100)
					{
						matched_time = left_sample->m_start;
						matched = true;
						break;
					}
				}
			}


			if(matched)
			{
				// release any unmatched and retrive matched
				CAutoLock lck(&m_queue_lock);
				while(true)
				{
					sample_left = m_left_queue.RemoveHead();
					if(sample_left->m_start != matched_time)
					{
						printf("drop left\n");
						delete sample_left;
					}
					else
						break;
				}

				while(true)
				{
					sample_right = m_right_queue.RemoveHead();
					if(sample_right->m_start != matched_time)
					{
						printf("drop right\n");
						delete sample_right;
					}
					else
						break;
				}
			}
		}

		if (sample_left && sample_right)
		{
			if (matched_time < start)
			{
				printf("drop a matched pair\n");
				delete sample_left;
				delete sample_right;
				sample_left = NULL;
				sample_right = NULL;
				goto find_match;
			}
			else
			{
				CAutoLock lck(&m_packet_lock);
				m_sample2render_1.Free();
				m_sample2render_1.Attach(sample_left);
				m_sample2render_2.Free();
				m_sample2render_2.Attach(sample_right);
				should_render = true;
			}
		}
	}
	int l3 = timeGetTime();
	if (id == 0 && m_output_mode != pageflipping && should_render)
	{
		render(true);
	}
	int l4 = timeGetTime();
	if (timeGetTime()-l1 > 25)
		mylog("DoRender() cost %dms(%d+%d+%d).\n", l4-l1, l2-l1, l3-l2, l4-l3);
	return S_OK;
}

HRESULT my12doomRenderer::DataPreroll(int id, IMediaSample *media_sample)
{
	SetThreadName(GetCurrentThreadId(), id==0?"Data thread 0":"Data thread 1");

	int l1 = timeGetTime();
	int bit_per_pixel = 12;
	if (m_dsr0->m_format == MEDIASUBTYPE_YUY2)
		bit_per_pixel = 16;
	else if (m_dsr0->m_format == MEDIASUBTYPE_RGB32)
		bit_per_pixel = 32;

	bool should_render = false;
	gpu_sample * loaded_sample = new gpu_sample(media_sample, m_Device, m_lVidWidth, m_lVidHeight, m_dsr0->m_format, m_revert_RGB32, m_output_mode == pageflipping);
	int l2 = timeGetTime();
	if (!m_dsr1->is_connected())
	{
		// single stream
		CAutoLock lck(&m_queue_lock);
		m_left_queue.AddTail(loaded_sample);
	}

	else
	{
		// double stream
		// insert
		CAutoLock lck(&m_queue_lock);
		if (id == 0)
			m_left_queue.AddTail(loaded_sample);
		else
			m_right_queue.AddTail(loaded_sample);


	}

	if (l2-l1 > 30)
		mylog("DataPreroll() time it :%d, %d ms\n", l2-l1, timeGetTime()-l2);

	return S_OK;
}

HRESULT my12doomRenderer::delete_render_targets()
{
	m_swap1 = NULL;
	m_swap2 = NULL;
	m_mask_temp_left = NULL;
	m_mask_temp_right = NULL;
	m_sbs_surface = NULL;
	m_tex_mask = NULL;
	return S_OK;
}

HRESULT my12doomRenderer::create_render_targets()
{
	CAutoLock lck2(&m_frame_lock);


	HRESULT hr = S_OK;
	if (m_active_pp.Windowed)
	{
		RECT rect;
		GetClientRect(m_hWnd, &rect);
		m_active_pp.BackBufferWidth = rect.right - rect.left;
		m_active_pp.BackBufferHeight = rect.bottom - rect.top;
		if (m_active_pp.BackBufferWidth > 0 && m_active_pp.BackBufferHeight > 0)
			FAIL_RET(m_Device->CreateAdditionalSwapChain(&m_active_pp, &m_swap1))

		GetClientRect(m_hWnd2, &rect);
		m_active_pp2 = m_active_pp;
		m_active_pp2.BackBufferWidth = rect.right - rect.left;
		m_active_pp2.BackBufferHeight = rect.bottom - rect.top;
		if (m_active_pp2.BackBufferWidth > 0 && m_active_pp2.BackBufferHeight > 0)
			FAIL_RET(m_Device->CreateAdditionalSwapChain(&m_active_pp2, &m_swap2));
	}
	else
	{
		FAIL_RET(m_Device->GetSwapChain(0, &m_swap1));
	}

	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_mask_temp_left, NULL));
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_mask_temp_right, NULL));
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_tex_mask, NULL));
	FAIL_RET( m_Device->CreateRenderTarget(m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1, m_active_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &m_sbs_surface, NULL));
	FAIL_RET(generate_mask());

	// add 3d vision tag at last line on need
	if (m_output_mode == NV3D && m_sbs_surface)
	{
		D3DLOCKED_RECT lr;
		RECT lock_tar={0, m_active_pp.BackBufferHeight, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1};
		FAIL_RET(m_sbs_surface->LockRect(&lr,&lock_tar,0));
		LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
		pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		pSIH->dwBPP = 32;
		pSIH->dwFlags = SIH_SIDE_BY_SIDE;
		pSIH->dwWidth = m_active_pp.BackBufferWidth;
		pSIH->dwHeight = m_active_pp.BackBufferHeight;
		FAIL_RET(m_sbs_surface->UnlockRect());
	}

	return hr;
}

HRESULT my12doomRenderer::handle_device_state()							//handle device create/recreate/lost/reset
{
	if (!m_recreating_dshow_renderer)
	{
		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		if (m_tex_rgb_left) m_tex_rgb_left->GetLevelDesc(0, &desc);
		bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();
		input_layout_types input = get_active_input_layout();
		if (dual_stream && (m_lVidWidth != desc.Width || m_lVidHeight != desc.Height))
			set_device_state(need_reset_object);

		if (!dual_stream)
		{
			if (input == side_by_side && (m_lVidWidth != desc.Width*2 || m_lVidHeight != desc.Height))
				set_device_state(need_reset_object);
			if (input == top_bottom && (m_lVidWidth != desc.Width || m_lVidHeight != desc.Height*2))
				set_device_state(need_reset_object);
			if (input == mono2d && (m_lVidWidth != desc.Width || m_lVidHeight != desc.Height))
				set_device_state(need_reset_object);
		}
	}

	if (GetCurrentThreadId() != m_device_threadid || m_recreating_dshow_renderer)
	{
		if (m_device_state == fine)
			return S_OK;
		else
			return E_FAIL;
	}

	HRESULT hr;
	if (m_device_state != fine)
	{
		char * states[10]=
		{
			"fine",							// device is fine
			"need_resize_back_buffer",		// just resize back buffer and recaculate vertex
			"need_reset_object",				// objects size changed, should recreate objects
			"need_reset",						// reset requested by program, usually to change back buffer size, but program can continue rendering without reset
			"device_lost",					// device lost, can't continue
			"need_create",					// device not created, or need to recreate, can't continue
			"create_failed",					// 
			"device_state_max",				// not used
		};
		printf("handling device state(%s)\n", states[m_device_state]);
	}

	if (m_device_state == fine)
		return S_OK;

	else if (m_device_state == create_failed)
		return E_FAIL;

	else if (m_device_state == need_resize_back_buffer)
	{
		/*
		terminate_render_thread();
		CAutoLock lck(&m_frame_lock);
		FAIL_SLEEP_RET(invalidate_objects());
		FAIL_SLEEP_RET(restore_objects());
		m_device_state = fine;
		*/

		CAutoLock lck(&m_frame_lock);
		FAIL_RET(delete_render_targets());
		FAIL_RET(create_render_targets());
		FAIL_RET(calculate_vertex());
		m_device_state = fine;		
		/*
		terminate_render_thread();
		m_swap1 = NULL;
		m_swap2 = NULL;
		m_tex_mask = NULL;
		create_swap_chains();
		FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_tex_mask, NULL));
		calculate_vertex();
		m_device_state = fine;		
		create_render_thread();
		*/
	}
	else if (m_device_state == need_reset_object)
	{
		terminate_render_thread();
		CAutoLock lck(&m_frame_lock);
		FAIL_SLEEP_RET(invalidate_objects());
		FAIL_SLEEP_RET(restore_objects());
		m_device_state = fine;
	}

	else if (m_device_state == need_reset)
	{
		terminate_render_thread();
		CAutoLock lck(&m_frame_lock);
		mylog("reseting device.\n");
		int l = timeGetTime();
		FAIL_SLEEP_RET(invalidate_objects());
		mylog("invalidate objects: %dms.\n", timeGetTime() - l);
		HRESULT hr = m_Device->Reset( &m_new_pp );
		hr = m_Device->Reset( &m_new_pp );
		if( FAILED(hr ) )
		{
			m_device_state = device_lost;
			return hr;
		}
		m_active_pp = m_new_pp;
		mylog("Device->Reset: %dms.\n", timeGetTime() - l);
		FAIL_SLEEP_RET(restore_objects());
		mylog("restore objects: %dms.\n", timeGetTime() - l);
		m_device_state = fine;

		mylog("m_active_pp : %dx%d@%dHz, format %d\n", m_active_pp.BackBufferWidth, 
			m_active_pp.BackBufferHeight, m_active_pp.FullScreen_RefreshRateInHz, m_active_pp.BackBufferFormat);
	}

	else if (m_device_state == device_lost)
	{
		Sleep(100);
		hr = m_Device->TestCooperativeLevel();
		if( hr  == D3DERR_DEVICENOTRESET )
		{
			terminate_render_thread();
			CAutoLock lck(&m_frame_lock);
			FAIL_SLEEP_RET(invalidate_objects());
			HRESULT hr = m_Device->Reset( &m_new_pp );

			if( FAILED(hr ) )
			{
				m_device_state = device_lost;
				return hr;
			}
			m_active_pp = m_new_pp;
			FAIL_SLEEP_RET(restore_objects());
			
			m_device_state = fine;
			return hr;
		}

		else if (hr == S_OK)
			m_device_state = fine;

		else
			return E_FAIL;
	}

	else if (m_device_state == need_create)
	{

		ZeroMemory( &m_active_pp, sizeof(m_active_pp) );
		m_active_pp.Windowed               = TRUE;
		m_active_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
		m_active_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
		m_active_pp.BackBufferCount = 1;
		m_active_pp.Flags = D3DPRESENTFLAG_VIDEO;

		m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
		m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
		GetWindowRect(m_hWnd, &m_window_pos);

		set_fullscreen(false);

		UINT AdapterToUse=D3DADAPTER_DEFAULT;
		D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;

		D3DADAPTER_IDENTIFIER9  id1, id2; 
		for (UINT Adapter=0;Adapter<m_D3D->GetAdapterCount();Adapter++)
		{
			HMONITOR windowmonitor1 = MonitorFromWindow(m_hWnd, NULL);
			HMONITOR windowmonitor2 = MonitorFromWindow(m_hWnd2, NULL);

			if (windowmonitor1 == m_D3D->GetAdapterMonitor(Adapter))
				m_D3D->GetAdapterIdentifier(Adapter,0,&id1); 
			if (windowmonitor2 == m_D3D->GetAdapterMonitor(Adapter))
				m_D3D->GetAdapterIdentifier(Adapter,0,&id2); 
		}

		for (UINT Adapter=0;Adapter<m_D3D->GetAdapterCount();Adapter++)
		{ 
			D3DADAPTER_IDENTIFIER9  Identifier; 
			HRESULT       Res; 

			Res = m_D3D->GetAdapterIdentifier(Adapter,0,&Identifier); 
			HMONITOR windowmonitor = MonitorFromWindow(m_hWnd, NULL);
			if (windowmonitor != m_D3D->GetAdapterMonitor(Adapter))
				continue;


			AdapterToUse = Adapter;
#ifdef PERFHUD
			if (strstr(Identifier.Description,"PerfHUD") != 0) 
			{ 
				AdapterToUse=Adapter; 
				DeviceType=D3DDEVTYPE_REF; 

				break; 
			}
#endif
		}
		HRESULT hr = m_D3D->CreateDevice( AdapterToUse, DeviceType,
			m_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&m_active_pp, &m_Device );

		if (FAILED(hr))
			return hr;
		m_new_pp = m_active_pp;
		m_device_state = need_reset_object;

		// NV3D acitivation check
		if (m_nv3d_enabled)
		{
			StereoHandle h3d;
			NvAPI_Status res;
			res = NvAPI_Stereo_CreateHandleFromIUnknown(m_Device, &h3d);
			res = NvAPI_Stereo_SetNotificationMessage(h3d, (NvU64)m_hWnd, WM_NV_NOTIFY);
			res = NvAPI_Stereo_Activate(h3d);
			NvU8 actived = 0;
			res = NvAPI_Stereo_IsActivated(h3d, &actived);
			if (actived)
			{
				printf("init: NV3D actived\n");
				m_nv3d_actived = true;
			}
			else
			{
				printf("init: NV3D deactived\n");
				m_nv3d_actived = false;
			}

			if (res == NVAPI_OK)
				m_nv3d_actived = true;

			res = NvAPI_Stereo_DestroyHandle(h3d);
		}

		FAIL_SLEEP_RET(restore_objects());
		m_device_state = fine;
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_device_state(device_state new_state)
{
	m_device_state = max(m_device_state, new_state);

	if (m_device_state >= device_lost)
		Sleep(50);
	return S_OK;
}
HRESULT my12doomRenderer::reset()
{
	terminate_render_thread();
	CAutoLock lck(&m_frame_lock);
	invalidate_objects();
	set_device_state(need_reset_object);
	init_variables();

	return S_OK;
}
HRESULT my12doomRenderer::create_render_thread()
{
	if (INVALID_HANDLE_VALUE != m_render_thread)
		terminate_render_thread();
	m_render_thread_exit = false;
	m_render_thread = CreateThread(0,0,render_thread, this, NULL, NULL);
	return S_OK;
}
HRESULT my12doomRenderer::terminate_render_thread()
{
	if (INVALID_HANDLE_VALUE == m_render_thread)
		return S_FALSE;
	m_render_thread_exit = true;
	WaitForSingleObject(m_render_thread, INFINITE);
	m_render_thread = INVALID_HANDLE_VALUE;

	return S_OK;
}
HRESULT my12doomRenderer::invalidate_objects()
{
	m_uidrawer->invalidate();
	// pixel shader
	m_ps_yv12 = NULL;
	m_ps_nv12 = NULL;
	m_ps_yuy2 = NULL;
	m_ps_test = NULL;
	m_ps_anaglyph = NULL;

	// textures
	m_tex_rgb_left = NULL;
	m_tex_rgb_right = NULL;
	m_tex_rgb_full = NULL;

	// pending samples
	{
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;
		while (sample = m_left_queue.RemoveHead())
			delete sample;
		while (sample = m_right_queue.RemoveHead())
			delete sample;
	}
	{
		CAutoLock lck(&m_packet_lock);
		m_sample2render_1.Free();
		m_sample2render_2.Free();
	}

	// surfaces
	m_test_rt64 = NULL;

	// vertex buffers
	g_VertexBuffer = NULL;

	// swap chains
	delete_render_targets();

	return S_OK;
}
HRESULT my12doomRenderer::restore_objects()
{
	int l = timeGetTime();
	m_pass1_width = m_lVidWidth;
	m_pass1_height = m_lVidHeight;
	if (!(m_dsr0->is_connected() && m_dsr1->is_connected()))
	{
		input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
		if (input == side_by_side)
			m_pass1_width /= 2;
		else if (input == top_bottom)
			m_pass1_height /= 2;
	}

	HRESULT hr;
	FAIL_RET(create_render_targets());

	DWORD use_mipmap = D3DUSAGE_AUTOGENMIPMAP;

	FAIL_RET( m_Device->CreateTexture(m_lVidWidth, m_lVidHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_tex_rgb_full, NULL));
	FAIL_RET( m_Device->CreateTexture(m_pass1_width, m_pass1_height, 1, D3DUSAGE_RENDERTARGET | use_mipmap, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_tex_rgb_left, NULL));
	FAIL_RET( m_Device->CreateTexture(m_pass1_width, m_pass1_height, 1, D3DUSAGE_RENDERTARGET | use_mipmap, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_tex_rgb_right, NULL));
	if(m_tex_bmp == NULL) FAIL_RET( m_Device->CreateTexture(2048, 1024, 0, use_mipmap, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,	&m_tex_bmp, NULL));
	FAIL_RET( m_Device->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_test_rt64, NULL));
	if (m_mem == NULL) FAIL_RET( m_Device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_mem, NULL));
	
	// clear m_tex_rgb_left & right
	CComPtr<IDirect3DSurface9> rgb_surface;
	FAIL_RET(m_tex_rgb_left->GetSurfaceLevel(0, &rgb_surface));
	clear(rgb_surface);
	rgb_surface = NULL;
	FAIL_RET(m_tex_rgb_right->GetSurfaceLevel(0, &rgb_surface));
	clear(rgb_surface);

	// vertex
	FAIL_RET(m_Device->CreateVertexBuffer( sizeof(m_vertices), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, FVF_Flags, D3DPOOL_DEFAULT, &g_VertexBuffer, NULL ));
	FAIL_RET(calculate_vertex());

	// pixel shader
	unsigned char *yv12 = (unsigned char *)malloc(sizeof(g_code_YV12toRGB));
	unsigned char *nv12 = (unsigned char *)malloc(sizeof(g_code_NV12toRGB));
	unsigned char *yuy2 = (unsigned char *)malloc(sizeof(g_code_YUY2toRGB));
	unsigned char *tester = (unsigned char *)malloc(sizeof(g_code_main));
	unsigned char *anaglyph = (unsigned char *)malloc(sizeof(g_code_anaglyph));

	memcpy(yv12, g_code_YV12toRGB, sizeof(g_code_YV12toRGB));
	memcpy(nv12, g_code_NV12toRGB, sizeof(g_code_NV12toRGB));
	memcpy(yuy2, g_code_YUY2toRGB, sizeof(g_code_YUY2toRGB));
	memcpy(tester, g_code_main, sizeof(g_code_main));
	memcpy(anaglyph, g_code_anaglyph, sizeof(g_code_anaglyph));

	int size = sizeof(g_code_main);
	for(int i=0; i<16384; i+=16)
	{
		if (i<sizeof(g_code_YV12toRGB)/16*16)
			m_AES.decrypt(yv12+i, yv12+i);
		if (i<sizeof(g_code_NV12toRGB)/16*16)
			m_AES.decrypt(nv12+i, nv12+i);
		if (i<sizeof(g_code_YUY2toRGB)/16*16)
			m_AES.decrypt(yuy2+i, yuy2+i);
		if (i<sizeof(g_code_main)/16*16)
			m_AES.decrypt(tester+i, tester+i);
		if (i<sizeof(g_code_anaglyph)/16*16)
			m_AES.decrypt(anaglyph+i, anaglyph+i);
	}

	int r = memcmp(yv12, g_code_YV12toRGB, sizeof(g_code_YV12toRGB));

	m_Device->CreatePixelShader((DWORD*)yv12, &m_ps_yv12);
	m_Device->CreatePixelShader((DWORD*)nv12, &m_ps_nv12);
	m_Device->CreatePixelShader((DWORD*)yuy2, &m_ps_yuy2);
	m_Device->CreatePixelShader((DWORD*)tester, &m_ps_test);
	m_Device->CreatePixelShader((DWORD*)anaglyph, &m_ps_anaglyph);

	free(yv12);
	free(nv12);
	free(yuy2);
	free(tester);
	free(anaglyph);

	// load and start thread
	restore_rgb();
	create_render_thread();
	return S_OK;
}

HRESULT my12doomRenderer::backup_rgb()
{
	if (m_dsr0->m_State == State_Running)		// don't backup or restore when running, that cause great jitter
		return S_FALSE;

	if (m_backuped)								// no repeated backup
		return S_OK;

	HRESULT hr = S_OK;



	mylog("backup_rgb() start\n");

	CAutoLock lck(&m_frame_lock);
	if (!m_tex_rgb_left || ! m_tex_rgb_right)
		return E_FAIL;

	D3DSURFACE_DESC desc;
	FAIL_RET(m_tex_rgb_left->GetLevelDesc(0, &desc));
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	if (m_swapeyes)								// swap back
	{
		FAIL_RET(m_tex_rgb_left->GetSurfaceLevel(0, &right_surface));
		FAIL_RET(m_tex_rgb_right->GetSurfaceLevel(0, &left_surface));
	}
	else
	{
		FAIL_RET(m_tex_rgb_left->GetSurfaceLevel(0, &left_surface));
		FAIL_RET(m_tex_rgb_right->GetSurfaceLevel(0, &right_surface));
	}

	input_layout_types input = get_active_input_layout();
	bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();
	CComPtr<IDirect3DSurface9> full_surface;
	m_Device->CreateRenderTarget(dual_stream ? m_lVidWidth*2 : m_lVidWidth, m_lVidHeight, desc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &full_surface, NULL);
	clear(full_surface);

	if (dual_stream)
	{
		RECT dst = {0, 0, m_lVidWidth, m_lVidHeight};
		hr = m_Device->StretchRect(left_surface, NULL, full_surface, &dst, D3DTEXF_NONE);
		dst.left += m_lVidWidth;
		dst.right += m_lVidWidth;
		hr = m_Device->StretchRect(right_surface, NULL, full_surface, &dst, D3DTEXF_NONE);
	}
	else if (input == side_by_side)
	{
		RECT dst = {0, 0, m_lVidWidth/2, m_lVidHeight};
		hr = m_Device->StretchRect(left_surface, NULL, full_surface, &dst, D3DTEXF_NONE);
		dst.left += m_lVidWidth/2;
		dst.right += m_lVidWidth/2;
		hr = m_Device->StretchRect(right_surface, NULL, full_surface, &dst, D3DTEXF_NONE);
	}
	else if (input == top_bottom)
	{
		RECT dst = {0, 0, m_lVidWidth, m_lVidHeight/2};
		hr = m_Device->StretchRect(left_surface, NULL, full_surface, &dst, D3DTEXF_NONE);
		dst.top += m_lVidHeight/2;
		dst.bottom += m_lVidHeight/2;
		hr = m_Device->StretchRect(right_surface, NULL, full_surface, &dst, D3DTEXF_NONE);
	}
	else if (input == mono2d)
	{
		hr = m_Device->StretchRect(left_surface, NULL, full_surface, NULL, D3DTEXF_NONE);
	}

	m_surface_rgb_backup_full = NULL;
	FAIL_RET(m_Device->CreateOffscreenPlainSurface(dual_stream ? m_lVidWidth*2 : m_lVidWidth, m_lVidHeight, desc.Format, D3DPOOL_SYSTEMMEM, &m_surface_rgb_backup_full, NULL));
	FAIL_RET(m_Device->GetRenderTargetData(full_surface, m_surface_rgb_backup_full));

	mylog("backup_rgb() ok\n");
	m_backuped = true;
	return S_OK;
}

#define JIF(x) {if(FAILED(hr=x))goto clearup;}

HRESULT my12doomRenderer::restore_rgb()
{
	if (m_dsr0->m_State == State_Running)		// don't backup or restore when running, that cause great jitter
		return S_FALSE;

	mylog("restore_rgb() start\n");
	HRESULT hr = S_OK;
	CComPtr<IDirect3DSurface9> live_surface_left;
	CComPtr<IDirect3DSurface9> live_surface_right;
	CAutoLock lck(&m_frame_lock);
	if (!m_tex_rgb_left || ! m_tex_rgb_right || !m_surface_rgb_backup_full)
		goto clearup;

	D3DSURFACE_DESC desc_backup, desc_live;
	m_surface_rgb_backup_full->GetDesc(&desc_backup);
	if(FAILED(m_tex_rgb_left->GetLevelDesc(0, &desc_live)))
		goto clearup;
	if (desc_live.Format != desc_backup.Format)
		goto clearup;

	if (m_swapeyes)
	{
		if(FAILED(m_tex_rgb_left->GetSurfaceLevel(0, &live_surface_right)))
			goto clearup;
		if(FAILED(m_tex_rgb_right->GetSurfaceLevel(0, &live_surface_left)))
			goto clearup;
	}
	else
	{
		if(FAILED(m_tex_rgb_left->GetSurfaceLevel(0, &live_surface_left)))
			goto clearup;
		if(FAILED(m_tex_rgb_right->GetSurfaceLevel(0, &live_surface_right)))
			goto clearup;
	}

	input_layout_types input = get_active_input_layout();
	bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();

	if (dual_stream)
	{
		RECT src = {0, 0, m_lVidWidth, m_lVidHeight};
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, &src, live_surface_left, NULL));
		src.left += m_lVidWidth;
		src.right += m_lVidWidth;
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, &src, live_surface_right, NULL));
	}
	else if (input == side_by_side)
	{
		RECT src = {0, 0, m_lVidWidth/2, m_lVidHeight};
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, &src, live_surface_left, NULL));
		src.left += m_lVidWidth/2;
		src.right += m_lVidWidth/2;
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, &src, live_surface_right, NULL));
	}
	else if (input == top_bottom)
	{
		RECT src = {0, 0, m_lVidWidth, m_lVidHeight/2};
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, &src, live_surface_left, NULL));
		src.top += m_lVidHeight/2;
		src.bottom += m_lVidHeight/2;
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, &src, live_surface_right, NULL));
	}
	else if (input == mono2d)
	{
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, NULL, live_surface_left, NULL));
		JIF(hr = m_Device->UpdateSurface(m_surface_rgb_backup_full, NULL, live_surface_right, NULL));
	}

	mylog("restore_rgb : OK.\n");
	return hr;
clearup:
	mylog("restore_rgb : fail.\n");
	return S_OK;
}

HRESULT my12doomRenderer::render_nolock(bool forced)
{
	// device state check and restore
	if (FAILED(handle_device_state()))
		return E_FAIL;

	// image loading and idle check
	if (load_image() != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!m_Device)
		return E_FAIL;

	static int last_render_time = timeGetTime();

	HRESULT hr;

	CAutoLock lck(&m_frame_lock);
	if (!m_tex_rgb_left)
		return VFW_E_NOT_CONNECTED;

	// pass 2: drawing to back buffer!
	hr = m_Device->BeginScene();
	hr = m_Device->SetPixelShader(NULL);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = m_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = m_tex_rgb_right->GetSurfaceLevel(0, &right_surface);

	// set render target to back buffer
	CComPtr<IDirect3DSurface9> back_buffer;
	if (m_swap1) hr = m_swap1->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);

	clear(back_buffer);

	// reset texture blending stages
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	m_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
	m_Device->SetTextureStageState( 0, D3DTSS_RESULTARG, D3DTA_CURRENT);
	m_Device->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	m_Device->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 0 );
	m_Device->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	m_Device->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, 0 );

	// use mipmap only for full resolution videos or very small backbuffer
	if ((m_source_aspect > 2.425 || m_source_aspect < 1.2125 ) || 
		(m_active_pp.BackBufferWidth * 4 < m_lVidWidth && m_active_pp.BackBufferHeight < m_lVidHeight * 4) )
	{
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
		hr = m_Device->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	}
	else
	{
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = m_Device->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	}

	hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

	hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

	if (m_output_mode == NV3D && m_nv3d_enabled && m_nv3d_actived && !m_active_pp.Windowed)
	{
		clear(back_buffer);
		draw_movie(back_buffer, true);
		draw_bmp(back_buffer, true);

		// copy left to nv3d surface
		RECT dst = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
		hr = m_Device->StretchRect(back_buffer, NULL, m_sbs_surface, &dst, D3DTEXF_NONE);

		clear(back_buffer);
		draw_movie(back_buffer, false);
		draw_bmp(back_buffer, false);

		// copy right to nv3d surface
		dst.left += m_active_pp.BackBufferWidth;
		dst.right += m_active_pp.BackBufferWidth;
		hr = m_Device->StretchRect(back_buffer, NULL, m_sbs_surface, &dst, D3DTEXF_NONE);

		// StretchRect to backbuffer!, this is how 3D vision works
		RECT tar = {0,0, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight};
		hr = m_Device->StretchRect(m_sbs_surface, &tar, back_buffer, NULL, D3DTEXF_NONE);		//source is as previous, tag line not overwrited

		draw_ui(back_buffer);
	}

	else if (m_output_mode == mono || (m_output_mode == NV3D && !(m_nv3d_enabled && m_nv3d_actived && !m_active_pp.Windowed)))
	{
		clear(back_buffer);
		draw_movie(back_buffer, true);
		draw_bmp(back_buffer, true);
		draw_ui(back_buffer);
	}

	else if (m_output_mode == anaglyph)
	{
		CComPtr<IDirect3DSurface9> temp_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, true);
		draw_bmp(temp_surface, true);
		temp_surface = NULL;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, false);
		draw_bmp(temp_surface, false);

		// pass3: analyph
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_Device->SetRenderTarget(0, back_buffer);
		m_Device->SetTexture( 0, m_mask_temp_left );
		m_Device->SetTexture( 1, m_mask_temp_right );
		m_Device->SetPixelShader(m_ps_anaglyph);

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );

		// UI
		m_Device->SetPixelShader(NULL);
		draw_ui(back_buffer);
	}

	else if (m_output_mode == masking)
	{
		CComPtr<IDirect3DSurface9> temp_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, true);
		draw_bmp(temp_surface, true);
		temp_surface = NULL;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, false);
		draw_bmp(temp_surface, false);


		// pass 3: render to backbuffer with masking
		// black = left
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_Device->SetRenderTarget(0, back_buffer);
		m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );	// current = texture(mask)

		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
		m_Device->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SUBTRACT ); // subtract... to show only black 
		m_Device->SetTextureStageState( 1, D3DTSS_RESULTARG, D3DTA_TEMP);	  // temp = texture(left) - current(mask)

		m_Device->SetTextureStageState( 2, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTextureStageState( 2, D3DTSS_COLORARG2, D3DTA_CURRENT );
		m_Device->SetTextureStageState( 2, D3DTSS_COLORARG0, D3DTA_TEMP );		 // arg3 = arg0...
		m_Device->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD ); // Modulate then add... to show only white then add with temp(black)
		m_Device->SetTextureStageState( 2, D3DTSS_RESULTARG, D3DTA_CURRENT);     // current = texture(right) * current(mask) + temp

		m_Device->SetTexture( 0, m_tex_mask );
		m_Device->SetTexture( 1, m_mask_temp_left );
		m_Device->SetTexture( 2, m_mask_temp_right );

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );

		// UI
		draw_ui(back_buffer);
	}

	else if (m_output_mode == pageflipping)
	{
		static bool left = true;
		left = !left;

		clear(back_buffer);
		draw_movie(back_buffer, left);
		draw_bmp(back_buffer, left);
		draw_ui(back_buffer);
	}

	else if (m_output_mode == dual_window)
	{
		clear(back_buffer);
		draw_movie(back_buffer, true);
		draw_bmp(back_buffer, true);
		draw_ui(back_buffer);

		// set render target to swap chain2
		if (m_swap2)
		{
			CComPtr<IDirect3DSurface9> back_buffer2;
			m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);
			hr = m_Device->SetRenderTarget(0, back_buffer2);

			// back ground
			#ifdef DEBUG
			m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
			#else
			m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
			#endif

			clear(back_buffer2);
			draw_movie(back_buffer2, false);
			draw_bmp(back_buffer2, false);
			draw_ui(back_buffer2);
		}

	}

	else if (m_output_mode == out_sbs || m_output_mode == out_tb || m_output_mode == out_hsbs || m_output_mode == out_htb)
	{

		CComPtr<IDirect3DSurface9> temp_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		CComPtr<IDirect3DSurface9> temp_surface2;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &temp_surface2);
		clear(temp_surface);
		clear(temp_surface2);
		draw_movie(temp_surface, true);
		draw_movie(temp_surface2, false);
		draw_bmp(temp_surface, true);
		draw_bmp(temp_surface2, false);
		draw_ui(temp_surface);
		draw_ui(temp_surface2);


		// pass 3: copy to backbuffer
		if (m_output_mode == out_sbs)
		{
			RECT src = {0, 0, m_active_pp.BackBufferWidth/2, m_active_pp.BackBufferHeight};
			RECT dst = src;

			m_Device->StretchRect(temp_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

			dst.left += m_active_pp.BackBufferWidth/2;
			dst.right += m_active_pp.BackBufferWidth/2;
			m_Device->StretchRect(temp_surface2, &src, back_buffer, &dst, D3DTEXF_NONE);
		}

		else if (m_output_mode == out_tb)
		{
			RECT src = {0, 0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight/2};
			RECT dst = src;

			m_Device->StretchRect(temp_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

			dst.top += m_active_pp.BackBufferHeight/2;
			dst.bottom += m_active_pp.BackBufferHeight/2;
			m_Device->StretchRect(temp_surface2, &src, back_buffer, &dst, D3DTEXF_NONE);

		}

		else if (m_output_mode == out_hsbs)
		{
			RECT dst = {0, 0, m_active_pp.BackBufferWidth/2, m_active_pp.BackBufferHeight};

			m_Device->StretchRect(temp_surface, NULL, back_buffer, &dst, D3DTEXF_LINEAR);

			dst.left += m_active_pp.BackBufferWidth/2;
			dst.right += m_active_pp.BackBufferWidth/2;
			m_Device->StretchRect(temp_surface2, NULL, back_buffer, &dst, D3DTEXF_LINEAR);
		}

		else if (m_output_mode == out_htb)
		{
			RECT dst = {0, 0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight/2};

			m_Device->StretchRect(temp_surface, NULL, back_buffer, &dst, D3DTEXF_LINEAR);

			dst.top += m_active_pp.BackBufferHeight/2;
			dst.bottom += m_active_pp.BackBufferHeight/2;
			m_Device->StretchRect(temp_surface2, NULL, back_buffer, &dst, D3DTEXF_LINEAR);

		}
	}

	m_Device->EndScene();

presant:
	static int n = timeGetTime();
	//printf("presant delta: %dms\n", timeGetTime()-n);
	n = timeGetTime();

	if (m_output_mode == dual_window)
	{
		if(m_swap1) hr = m_swap1->Present(NULL, NULL, m_hWnd, NULL, D3DPRESENT_DONOTWAIT);
		if (FAILED(hr) && hr != DDERR_WASSTILLDRAWING)
			set_device_state(device_lost);

		if(m_swap2) if (m_swap2->Present(NULL, NULL, m_hWnd2, NULL, NULL) == D3DERR_DEVICELOST)
			set_device_state(device_lost);
	}

	else
	{
		if(m_swap1) hr = m_swap1->Present(NULL, NULL, m_hWnd, NULL, NULL);
		if (FAILED(hr))
			set_device_state(device_lost);

		//static int n = timeGetTime();
		//if (timeGetTime()-n > 15)printf("delta = %d.\n", timeGetTime()-n);
		//n = timeGetTime();
	}

	return S_OK;
}

HRESULT my12doomRenderer::clear(IDirect3DSurface9 *surface, DWORD color)
{
	if (!surface)
		return E_POINTER;
	m_Device->SetRenderTarget(0, surface);
	m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L );
	return S_OK;
}

HRESULT my12doomRenderer::draw_movie(IDirect3DSurface9 *surface, bool left_eye)
{
	if (!surface)
		return E_POINTER;

	if (!m_dsr0->is_connected() && m_uidrawer)
		return m_uidrawer->draw_nonmovie_bg(surface, left_eye);

	D3DSURFACE_DESC desc;
	surface->GetDesc(&desc);
	bool main = false;
	if (desc.Width == m_active_pp.BackBufferWidth  && desc.Height == m_active_pp.BackBufferHeight)
		main = true;

	HRESULT hr = E_FAIL;

	m_Device->SetRenderTarget(0, surface);
	m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_Device->SetTexture( 0, left_eye ? m_tex_rgb_left : m_tex_rgb_right );
	hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, left_eye ? vertex_pass2_main : vertex_pass2_main_r, 2 );
	return S_OK;
}
HRESULT my12doomRenderer::draw_bmp(IDirect3DSurface9 *surface, bool left_eye)
{
	if (!surface)
		return E_POINTER;

	HRESULT hr = E_FAIL;

	m_Device->SetRenderTarget(0, surface);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	hr = m_Device->SetTexture( 0, m_tex_bmp );
	hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, left_eye ? vertex_bmp : vertex_bmp2, 2 );
	return S_OK;
}
HRESULT my12doomRenderer::draw_ui(IDirect3DSurface9 *surface)
{
	float delta_alpha = 1-(float)(timeGetTime()-m_ui_visible_last_change_time)/fade_in_out_time;
	delta_alpha = max(0, delta_alpha);
	delta_alpha = min(1, delta_alpha);
	float alpha = m_showui ? 1.0f : 0.0f;
	alpha -= m_showui ? delta_alpha : -delta_alpha;

	if (alpha <=0)
		return S_FALSE;

	if (!surface)
		return E_POINTER;

	if (m_total_time == 0)
	{
		CComPtr<IFilterGraph> fg;
		FILTER_INFO fi;
		m_dsr0->QueryFilterInfo(&fi);
		fg.Attach(fi.pGraph);
		CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> ms(fg);

		if (ms)
			ms->GetDuration(&m_total_time);			
	}
	return m_uidrawer->draw_ui(surface, m_dsr0->m_thisstream + m_dsr0->m_time, m_total_time, m_volume, m_dsr0->m_State == State_Running, alpha);
}

HRESULT my12doomRenderer::render(bool forced)
{
	if(GetCurrentThread() != m_render_thread && m_output_mode == pageflipping)
		return S_FALSE;

	CAutoLock lck(&m_frame_lock);
	if (m_Device)
		return render_nolock(forced);
	else
		return E_FAIL;
	return S_OK;
}
DWORD WINAPI my12doomRenderer::test_thread(LPVOID param)
{
	my12doomRenderer *_this = (my12doomRenderer*)param;
	while(_this->m_Device == NULL)
		Sleep(1);

	Sleep(5000);

	while (!_this->m_render_thread_exit)
	{
		D3DLOCKED_RECT locked;
		HRESULT hr;

		int l1 = timeGetTime();
		//hr = _this->m_Device->CreateOffscreenPlainSurface(1920, 1080, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &_this->m_just_a_test_surface, NULL);
		int l2 = timeGetTime();
		//hr = _this->m_just_a_test_surface->LockRect(&locked, NULL, NULL);
		int l3 = timeGetTime();
		//memset(locked.pBits, 0, locked.Pitch * 1080);
		int l4 = timeGetTime();
		//hr = _this->m_just_a_test_surface->UnlockRect();
		int l5 = timeGetTime();
		//_this->m_just_a_test_surface = NULL;
		int l6 = timeGetTime();

		if (l6-l1 > 2)
			mylog("thread: %d,%d,%d,%d,%d\n", l2-l1, l3-l2, l4-l3, l5-l4, l6-l5);

		Sleep(1);
	}

	return 0;
}
DWORD WINAPI my12doomRenderer::render_thread(LPVOID param)
{
	my12doomRenderer *_this = (my12doomRenderer*)param;

	int l = timeGetTime();
	while (timeGetTime() - l < 0 && !_this->m_render_thread_exit)
		Sleep(1);

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while(!_this->m_render_thread_exit)
	{
		if (_this->m_output_mode != pageflipping)
		{
			if (_this->m_dsr0->m_State == State_Running)
			{
				Sleep(1);
				continue;
			}
			else
			{
				_this->render_nolock(true);
				l = timeGetTime();
				while (timeGetTime() - l < 33 && !_this->m_render_thread_exit)
					Sleep(1);
			}
		}
		else
		{
			_this->render_nolock(true);
			Sleep(1);
		}

		Sleep(0);		// always let other thread to do their job
	}	
	
	return S_OK;
}

// dx9 helper functions
HRESULT my12doomRenderer::load_image(int id /*= -1*/, bool forced /* = false */)
{
	m_packet_lock.Lock();
	if (!m_sample2render_1 && !m_sample2render_2)
	{
		m_packet_lock.Unlock();
		return S_FALSE;
	}

	int l = timeGetTime();
	if(m_sample2render_1)
		m_sample2render_1->load();
	if(m_sample2render_2)
		m_sample2render_2->load();

	HRESULT hr = load_image_convert(m_sample2render_1, m_sample2render_2);

	static int n = timeGetTime();
	printf("load and convert time:%dms, presant delta: %dms(%.02f circle)\n", timeGetTime()-l, timeGetTime()-n, (double)(timeGetTime()-n) / (1000/60));
	n = timeGetTime();

	return hr;
}

input_layout_types my12doomRenderer::get_active_input_layout()
{
	if (m_input_layout != input_layout_auto)
		return m_input_layout;
	else
		return m_layout_detected;
}

double my12doomRenderer::get_active_aspect()
{
	if (m_forced_aspect > 0)
		return m_forced_aspect;

	bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();
	if (dual_stream || get_active_input_layout() == mono2d)
		return m_source_aspect;
	else if (get_active_input_layout() == side_by_side)
	{
		if (m_source_aspect > 2.425)
			return m_source_aspect / 2;
		else
			return m_source_aspect;
	}
	else if (get_active_input_layout() == top_bottom)
	{
		if (m_source_aspect < 1.2125)
			return m_source_aspect * 2;
		else
			return m_source_aspect;
	}

	return m_source_aspect;
}

HRESULT my12doomRenderer::load_image_convert(gpu_sample * sample1, gpu_sample *sample2)
{
	if (!sample1)
		return E_POINTER;

	bool dual_stream = sample1 && sample2;
	CLSID format = sample1->m_format;
	bool topdown = sample1->m_topdown;

	CComPtr<IDirect3DTexture9> sample1_tex_RGB32 = !sample1 ? NULL : sample1->m_tex_RGB32;						// RGB32 planes, in A8R8G8B8, full width
	CComPtr<IDirect3DTexture9> sample1_tex_YUY2 = !sample1 ? NULL : sample1->m_tex_YUY2;						// YUY2 planes, in A8R8G8B8, half width
	CComPtr<IDirect3DTexture9> sample1_tex_Y = !sample1 ? NULL : sample1->m_tex_Y;							// Y plane of YV12/NV12, in L8
	CComPtr<IDirect3DTexture9> sample1_tex_YV12_UV = !sample1 ? NULL : sample1->m_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CComPtr<IDirect3DTexture9> sample1_tex_NV12_UV = !sample1 ? NULL : sample1->m_tex_NV12_UV;					// UV plane of NV12, in A8L8

	CComPtr<IDirect3DTexture9> sample2_tex_RGB32 = !sample2 ? NULL : sample2->m_tex_RGB32;						// RGB32 planes, in A8R8G8B8, full width
	CComPtr<IDirect3DTexture9> sample2_tex_YUY2 = !sample2 ? NULL : sample2->m_tex_YUY2;						// YUY2 planes, in A8R8G8B8, half width
	CComPtr<IDirect3DTexture9> sample2_tex_Y = !sample2 ? NULL : sample2->m_tex_Y;							// Y plane of YV12/NV12, in L8
	CComPtr<IDirect3DTexture9> sample2_tex_YV12_UV = !sample2 ? NULL : sample2->m_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CComPtr<IDirect3DTexture9> sample2_tex_NV12_UV = !sample2 ? NULL : sample2->m_tex_NV12_UV;					// UV plane of NV12, in A8L8
	m_sample2render_1.Free();
	m_sample2render_2.Free();

	m_packet_lock.Unlock();	//release queue lock!

	CAutoLock lck(&m_frame_lock);

	// pass 1: render full resolution to two seperate texture
	HRESULT hr = m_Device->BeginScene();
	m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	hr = m_Device->SetPixelShader(NULL);
	if (format == MEDIASUBTYPE_YV12) hr = m_Device->SetPixelShader(m_ps_yv12);
	if (format == MEDIASUBTYPE_NV12) hr = m_Device->SetPixelShader(m_ps_nv12);
	if (format == MEDIASUBTYPE_YUY2) hr = m_Device->SetPixelShader(m_ps_yuy2);
	float rect_data[4] = {m_lVidWidth, m_lVidHeight, m_lVidWidth/2, m_lVidHeight};
	hr = m_Device->SetPixelShaderConstantF(0, rect_data, 1);
	hr = m_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = m_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = m_tex_rgb_right->GetSurfaceLevel(0, &right_surface);


	// drawing
	hr = m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	hr = m_Device->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_NONE );

	hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

	hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );

	input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
	if (dual_stream)
	{
		// left sample
		if (format == MEDIASUBTYPE_RGB32)
			hr = m_Device->SetTexture(0, sample1_tex_RGB32);
		else
		{
			hr = m_Device->SetTexture( 0, format == MEDIASUBTYPE_YUY2 ? sample1_tex_YUY2 : sample1_tex_Y );
			hr = m_Device->SetTexture( 1, format == MEDIASUBTYPE_NV12 ? sample1_tex_NV12_UV : sample1_tex_YV12_UV);
		}
		hr = m_Device->SetRenderTarget(0, m_swapeyes ? right_surface : left_surface);
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );


		// right sample
		if (format == MEDIASUBTYPE_RGB32)
			hr = m_Device->SetTexture(0, sample2_tex_RGB32);
		else
		{
			hr = m_Device->SetTexture( 0, format == MEDIASUBTYPE_YUY2 ? sample2_tex_YUY2 : sample2_tex_Y );
			hr = m_Device->SetTexture( 1, format == MEDIASUBTYPE_NV12 ? sample2_tex_NV12_UV : sample2_tex_YV12_UV);
		}
		hr = m_Device->SetRenderTarget(0, m_swapeyes ? left_surface : right_surface);
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );
	}
	else
	{
		if (format == MEDIASUBTYPE_RGB32)
			hr = m_Device->SetTexture(0, sample1_tex_RGB32);
		else
		{

			hr = m_Device->SetTexture( 0, format == MEDIASUBTYPE_YUY2 ? sample1_tex_YUY2 : sample1_tex_Y );
			hr = m_Device->SetTexture( 1, format == MEDIASUBTYPE_NV12 ? sample1_tex_NV12_UV : sample1_tex_YV12_UV);
		}

		if (!m_swapeyes)
			hr = m_Device->SetRenderTarget(0, left_surface);
		else
			hr = m_Device->SetRenderTarget(0, right_surface);

		if (input == side_by_side)
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_left, 2 );
		else if (input == top_bottom)
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_top, 2 );
		else if (input == mono2d)
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );

		if (!m_swapeyes)
			hr = m_Device->SetRenderTarget(0, right_surface);
		else
			hr = m_Device->SetRenderTarget(0, left_surface);

		if (input == side_by_side)
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_right, 2 );
		else if (input == top_bottom)
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_bottom, 2 );
		else if (input == mono2d)
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );
	}

	m_backuped = false;

	// repack it up...
	// no need to handle swap eyes
	if (!dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect)
	{
		CComPtr<IDirect3DSurface9> target_surface;
		m_tex_rgb_full->GetSurfaceLevel(0, &target_surface);
		{
			if (input == side_by_side)
			{
				RECT tar = {0, 0, m_lVidWidth/2, m_lVidHeight};
				hr = m_Device->StretchRect(left_surface, NULL, target_surface, &tar, D3DTEXF_NONE);
				tar.left += m_lVidWidth/2;
				tar.right += m_lVidWidth/2;
				hr = m_Device->StretchRect(right_surface, NULL, target_surface, &tar, D3DTEXF_NONE);
			}
			else if (input == top_bottom)
			{
				RECT tar = {0, 0, m_lVidWidth, m_lVidHeight/2};
				hr = m_Device->StretchRect(left_surface, NULL, target_surface, &tar, D3DTEXF_NONE);
				tar.top += m_lVidHeight/2;
				tar.bottom += m_lVidHeight/2;
				hr = m_Device->StretchRect(right_surface, NULL, target_surface, &tar, D3DTEXF_NONE);
			}
			else if (input == mono2d)
			{
				hr = m_Device->StretchRect(left_surface, NULL, target_surface, NULL, D3DTEXF_NONE);
			}
		}

		hr = m_Device->SetRenderTarget(0, m_test_rt64);
		hr = m_Device->Clear(0, NULL, D3DCLEAR_TARGET,  D3DCOLOR_XRGB(0,0,0), 1.0f, 0L);
		hr = m_Device->SetTexture(0, m_tex_rgb_full);

		hr = m_Device->SetPixelShader(m_ps_test);
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_test, 2 );


		hr = m_Device->EndScene();

		hr = m_Device->GetRenderTargetData(m_test_rt64, m_mem);
		//hr = m_Device->StretchRect(m_test_rt64, NULL, left_surface, NULL, D3DTEXF_LINEAR);		//if you want to see debug image, decomment this line

		D3DLOCKED_RECT locked;
		m_mem->LockRect(&locked, NULL, NULL);

		BYTE* src = (BYTE*)locked.pBits;
		double average1 = 0;
		double average2 = 0;
		double delta1 = 0;
		double delta2 = 0;
		for(int y=0; y<64; y++)
			for(int x=0; x<64; x++)
			{
				double &average = x<32 ?average1:average2;
				average += src[2];
				src += 4;
			}

		average1 /= 32*64;
		average2 /= 32*64;

		src = (BYTE*)locked.pBits;
		for(int y=0; y<64; y++)
			for(int x=0; x<64; x++)
			{
				double &average = x<32 ?average1:average2;
				double &tdelta = x<32 ? delta1 : delta2;

				int delta = abs(src[2] - average);
				tdelta += delta * delta;
				src += 4;
			}
		delta1 = sqrt((double)delta1)/(32*64-1);
		delta2 = sqrt((double)delta2)/(32*64-1);
                      
		double times = 0;
		double var1 = average1 * delta1;
		double var2 = average2 * delta2;
		if ( (var1 > 0.001 && var2 > 0.001) || (var1>var2*10000) || (var2>var1*10000))
			times = var1 > var2 ? var1 / var2 : var2 / var1;

		printf("%f - %f, %f - %f, %f - %f, %f\r\n", average1, average2, delta1, delta2, var1, var2, times);

		if (times > 31.62/2)		// 10^1.5
		{
			mylog("stereo(%s).\r\n", var1 > var2 ? "tb" : "sbs");
			(var1 > var2 ? m_tb : m_sbs) ++ ;
		}
		else if ( 1.0 < times && times < 4.68 /*&& 1.0 < times2 && times2 < 1.8*/)
		{
			m_normal ++;
			mylog("normal.\r\n");
		}
		else
		{
			mylog("unkown.\r\n");
		}
		m_mem->UnlockRect();

		input_layout_types next_layout = m_layout_detected;
		if (m_normal - m_sbs > 5)
			next_layout = mono2d;
		if (m_sbs - m_normal > 5)
			next_layout = side_by_side;
		if (m_tb - max(m_sbs,m_normal) > 5)
			next_layout = top_bottom;

		if (m_normal - m_sbs > 500 || m_sbs - m_normal > 500 || m_tb - max(m_sbs,m_normal) > 500)
			m_no_more_detect = true;

		if (next_layout != m_layout_detected)
		{
			m_layout_detected = next_layout;
			set_device_state(need_reset_object);
		}

	}

	return S_OK;
}

HRESULT my12doomRenderer::calculate_vertex()
{
	double active_aspect = get_active_aspect();

	CAutoLock lck(&m_frame_lock);
	if (m_swap1)
	{
		CComPtr<IDirect3DSurface9> surface;
		m_swap1->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &surface);
		D3DSURFACE_DESC desc;
		if (SUCCEEDED(surface->GetDesc(&desc)))
			m_uidrawer->init(desc.Width, desc.Height, m_Device);
	}

	// w,color
	for(int i=0; i<sizeof(m_vertices) / sizeof(MyVertex); i++)
	{
		//m_vertices[i].diffuse = m_color1;
		//m_vertices[i].specular = m_color2;
		m_vertices[i].w = 1.0f;
		m_vertices[i].z = 1.0f;
	}
	// texture coordinate
	// pass1 whole
	m_vertices[0].tu = 0.0f; m_vertices[0].tv = 0.0f;
	m_vertices[1].tu = 1.0f; m_vertices[1].tv = 0.0f;
	m_vertices[2].tu = 0.0f; m_vertices[2].tv = 1.0f;
	m_vertices[3].tu = 1.0f; m_vertices[3].tv = 1.0f;

	// pass1 left
	m_vertices[4].tu = 0.0f; m_vertices[4].tv = 0.0f;
	m_vertices[5].tu = 0.5f; m_vertices[5].tv = 0.0f;
	m_vertices[6].tu = 0.0f; m_vertices[6].tv = 1.0f;
	m_vertices[7].tu = 0.5f; m_vertices[7].tv = 1.0f;

	// pass1 right
	m_vertices[8].tu = 0.5f; m_vertices[8].tv = 0.0f;
	m_vertices[9].tu = 1.0f; m_vertices[9].tv = 0.0f;
	m_vertices[10].tu = 0.5f; m_vertices[10].tv = 1.0f;
	m_vertices[11].tu = 1.0f; m_vertices[11].tv = 1.0f;

	// pass1 top
	m_vertices[12].tu = 0.0f; m_vertices[12].tv = 0.0f;
	m_vertices[13].tu = 1.0f; m_vertices[13].tv = 0.0f;
	m_vertices[14].tu = 0.0f; m_vertices[14].tv = 0.5f;
	m_vertices[15].tu = 1.0f; m_vertices[15].tv = 0.5f;

	// pass1 bottom
	m_vertices[16].tu = 0.0f; m_vertices[16].tv = 0.5f;
	m_vertices[17].tu = 1.0f; m_vertices[17].tv = 0.5f;
	m_vertices[18].tu = 0.0f; m_vertices[18].tv = 1.0f;
	m_vertices[19].tu = 1.0f; m_vertices[19].tv = 1.0f;

	// pass2 whole texture, fixed aspect output for main back buffer
	m_vertices[20].tu = 0.0f; m_vertices[20].tv = 0.0f;
	m_vertices[21].tu = 1.0f; m_vertices[21].tv = 0.0f;
	m_vertices[22].tu = 0.0f; m_vertices[22].tv = 1.0f;
	m_vertices[23].tu = 1.0f; m_vertices[23].tv = 1.0f;

	// pass2 whole texture, fixed aspect output for second back buffer
	m_vertices[24].tu = 0.0f; m_vertices[24].tv = 0.0f;
	m_vertices[25].tu = 1.0f; m_vertices[25].tv = 0.0f;
	m_vertices[26].tu = 0.0f; m_vertices[26].tv = 1.0f;
	m_vertices[27].tu = 1.0f; m_vertices[27].tv = 1.0f;

	// pass3 whole texture, whole back buffer output
	m_vertices[28].tu = 0.0f; m_vertices[28].tv = 0.0f;
	m_vertices[29].tu = 1.0f; m_vertices[29].tv = 0.0f;
	m_vertices[30].tu = 0.0f; m_vertices[30].tv = 1.0f;
	m_vertices[31].tu = 1.0f; m_vertices[31].tv = 1.0f;

	// pass1 coordinate
	for(int i=0; i<vertex_pass1_types_count; i++)
	{
		MyVertex * tmp = m_vertices + vertex_pass1_whole + vertex_point_per_type * i ;
		tmp[0].x = -0.5f; tmp[0].y = -0.5f;
		tmp[1].x = m_pass1_width-0.5f; tmp[1].y = -0.5f;
		tmp[2].x = -0.5f; tmp[2].y = m_pass1_height-0.5f;
		tmp[3].x =  m_pass1_width-0.5f; tmp[3].y =m_pass1_height-0.5f;
	}

	// pass2-3 coordinate
	// main window coordinate
	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

	if(m_Device && m_uidrawer ) m_uidrawer->init(tar.right, tar.bottom, m_Device);

	// ui zone
	MyVertex *ui = m_vertices + vertex_ui;
	ui[0].x = -0.5f; ui[0].y = tar.bottom-30-0.5f;
	ui[1].x = tar.right-0.5f; ui[1].y = ui[0].y;
	ui[2].x = ui[0].x; ui[2].y = ui[0].y + 30;
	ui[3].x = ui[1].x; ui[3].y = ui[1].y + 30;
	ui[0].tu = 0; ui[0].tv = (1024-30)/1024.0f;
	ui[1].tu = (tar.right-1)/2048.0f; ui[1].tv = ui[0].tv;
	ui[2].tu = 0; ui[2].tv = ui[0].tv + (30-1)/1024.0f;
	ui[3].tu = ui[1].tu; ui[3].tv = ui[1].tv + (30-1)/1024.0f;

	int delta_w = (int)(tar.right - tar.bottom * active_aspect + 0.5);
	int delta_h = (int)(tar.bottom - tar.right  / active_aspect + 0.5);
	if (delta_w > 0)
	{
		tar.left += delta_w/2;
		tar.right -= delta_w/2;
	}
	else if (delta_h > 0)
	{
		tar.top += delta_h/2;
		tar.bottom -= delta_h/2;
	}


	int tar_width = tar.right-tar.left;
	int tar_height = tar.bottom - tar.top;
	tar.left += (LONG)(tar_width * m_bmp_offset_x);
	tar.right += (LONG)(tar_width * m_bmp_offset_x);
	tar.top += (LONG)(tar_height * m_bmp_offset_y);
	tar.bottom += (LONG)(tar_height * m_bmp_offset_y);

	MyVertex *tmp = m_vertices + vertex_pass2_main;
	tmp[0].x = tar.left-0.5f; tmp[0].y = tar.top-0.5f;
	tmp[1].x = tar.right-0.5f; tmp[1].y = tar.top-0.5f;
	tmp[2].x = tar.left-0.5f; tmp[2].y = tar.bottom-0.5f;
	tmp[3].x = tar.right-0.5f; tmp[3].y = tar.bottom-0.5f;

	MyVertex *pass2_main_r = m_vertices + vertex_pass2_main_r;
	memcpy(pass2_main_r, tmp, sizeof(MyVertex) * 4);

	if (m_parallax > 0)
	{
		// cut right edge of right eye and left edge of left eye
		tmp[0].tu += m_parallax;
		tmp[2].tu += m_parallax;
		pass2_main_r[1].tu -= m_parallax;
		pass2_main_r[3].tu -= m_parallax;

	}
	else if (m_parallax < 0)
	{
		// cut left edge of right eye and right edge of left eye
		pass2_main_r[0].tu += abs(m_parallax);
		pass2_main_r[2].tu += abs(m_parallax);
		tmp[1].tu -= abs(m_parallax);
		tmp[3].tu -= abs(m_parallax);
	}

	MyVertex *bmp = m_vertices + vertex_bmp;
	tar_width = tar.right-tar.left;
	tar_height = tar.bottom - tar.top;
	bmp[0] = tmp[0];
	bmp[1] = tmp[1];
	bmp[2] = tmp[2];
	bmp[3] = tmp[3];
	bmp[0].x += m_bmp_fleft * tar_width; bmp[0].y += m_bmp_ftop * tar_height;
	bmp[1] = bmp[0]; bmp[1].x += m_bmp_fwidth * tar_width;
	bmp[3] = bmp[1]; bmp[3].y += m_bmp_fheight* tar_height;
	bmp[2] = bmp[3]; bmp[2].x -= m_bmp_fwidth * tar_width;
	bmp[0].tu = 0; bmp[0].tv = 0;
	bmp[1].tu = (m_bmp_width-1)/2048.0f; bmp[1].tv = 0;
	bmp[2].tu = 0; bmp[2].tv = (m_bmp_height-1)/1024.0f;
	bmp[3].tu = (m_bmp_width-1)/2048.0f; bmp[3].tv = (m_bmp_height-1)/1024.0f;

	MyVertex *bmp2 = m_vertices + vertex_bmp2;
	for(int i=0; i<4; i++)
	{
		bmp2[i] = bmp[i];
		bmp2[i].x -= tar_width * (m_bmp_offset);
	}

	tmp = m_vertices + vertex_pass3;
	tmp[0].x = -0.5f; tmp[0].y = -0.5f;
	tmp[1].x = m_active_pp.BackBufferWidth-0.5f; tmp[1].y = -0.5f;
	tmp[2].x = -0.5f; tmp[2].y = m_active_pp.BackBufferHeight-0.5f;
	tmp[3].x = m_active_pp.BackBufferWidth-0.5f; tmp[3].y = m_active_pp.BackBufferHeight-0.5f;

	// second window coordinate
	tar.left = tar.top = 0;
	tar.right = m_active_pp2.BackBufferWidth; tar.bottom = m_active_pp2.BackBufferHeight;
	delta_w = (int)(tar.right - tar.bottom * active_aspect + 0.5);
	delta_h = (int)(tar.bottom - tar.right  / active_aspect + 0.5);
	if (delta_w > 0)
	{
		tar.left += delta_w/2;
		tar.right -= delta_w/2;
	}
	else if (delta_h > 0)
	{
		tar.top += delta_h/2;
		tar.bottom -= delta_h/2;
	}

	tar_width = tar.right-tar.left;
	tar_height = tar.bottom - tar.top;
	tar.left += (LONG)(tar_width * m_bmp_offset_x);
	tar.right += (LONG)(tar_width * m_bmp_offset_x);
	tar.top += (LONG)(tar_height * m_bmp_offset_y);
	tar.bottom += (LONG)(tar_height * m_bmp_offset_y);

	tmp = m_vertices + vertex_pass2_second;
	tmp[0].x = tar.left-0.5f; tmp[0].y = tar.top-0.5f;
	tmp[1].x = tar.right-0.5f; tmp[1].y = tar.top-0.5f;
	tmp[2].x = tar.left-0.5f; tmp[2].y = tar.bottom-0.5f;
	tmp[3].x = tar.right-0.5f; tmp[3].y = tar.bottom-0.5f;

	MyVertex *test = m_vertices + vertex_test;
	test[0].x = -0.5f; test[0].y = -0.5f;
	test[1].x = 64-0.5f; test[1].y = -0.5f;
	test[2].x = -0.5f; test[2].y = test[0].y + 64;
	test[3].x = 64-0.5f; test[3].y = test[1].y + 64;
	test[0].tu = 0; test[0].tv = 0;
	test[1].tu = 1.0f; test[1].tv = 0;
	test[2].tu = 0; test[2].tv = 1.0f;
	test[3].tu = 1.0f; test[3].tv = 1.0f;

	if (!g_VertexBuffer)
		return S_FALSE;

	void *pVertices = NULL;
	g_VertexBuffer->Lock( 0, sizeof(m_vertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, m_vertices, sizeof(m_vertices) );
	g_VertexBuffer->Unlock();
	
	return S_OK;
}
HRESULT my12doomRenderer::generate_mask()
{
	if (m_output_mode != masking)
		return S_FALSE;

	if (!m_tex_mask)
		return VFW_E_NOT_CONNECTED;

	CAutoLock lck(&m_frame_lock);
	D3DLOCKED_RECT locked;
	HRESULT hr = m_tex_mask->LockRect(0, &locked, NULL, D3DLOCK_DISCARD);
	if (FAILED(hr)) return hr;

	RECT rect;
	GetWindowRect(m_hWnd, &rect);
	BYTE *dst = (BYTE*) locked.pBits;

	if (m_mask_mode == row_interlace)
	{
		// init row mask texture
		BYTE one_line[2048];
		for(DWORD i=0; i<2048; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, m_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == line_interlace)
	{
		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memset(dst, (i+rect.top)%2 == 0 ? 255 : 0 ,m_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == checkboard_interlace)
	{
		// init row mask texture
		BYTE one_line[2048];
		for(DWORD i=0; i<2048; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (i%2), m_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	hr = m_tex_mask->UnlockRect(0);

	return hr;
}
HRESULT my12doomRenderer::set_fullscreen(bool full)
{
	D3DDISPLAYMODE d3ddm;
	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	m_new_pp.BackBufferFormat       = d3ddm.Format;

	mylog("mode:%dx%d@%dHz, format:%d.\n", d3ddm.Width, d3ddm.Height, d3ddm.RefreshRate, d3ddm.Format);

	if(full && m_active_pp.Windowed)
	{
		GetWindowRect(m_hWnd, &m_window_pos);
		m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
		m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);

		LONG f = m_style & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
		LONG exf =  m_exstyle & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

		SetWindowLongPtr(m_hWnd, GWL_STYLE, f);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exf);
		if ((DWORD)(LOBYTE(LOWORD(GetVersion()))) < 6)
			SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

		m_new_pp.Windowed = FALSE;
		m_new_pp.BackBufferWidth = d3ddm.Width;
		m_new_pp.BackBufferHeight = d3ddm.Height;
		m_new_pp.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;

		set_device_state(need_reset);
		if (m_device_state < need_create)
			handle_device_state();
	}
	else if (!full && !m_active_pp.Windowed)
	{
		m_new_pp.Windowed = TRUE;
		m_new_pp.BackBufferWidth = 0;
		m_new_pp.BackBufferHeight = 0;
		m_new_pp.hDeviceWindow = m_hWnd;
		m_new_pp.FullScreen_RefreshRateInHz = 0;
		SetWindowLongPtr(m_hWnd, GWL_STYLE, m_style);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_exstyle);
		SetWindowPos(m_hWnd, m_exstyle & WS_EX_TOPMOST ? HWND_TOPMOST : HWND_NOTOPMOST,
			m_window_pos.left, m_window_pos.top, m_window_pos.right - m_window_pos.left, m_window_pos.bottom - m_window_pos.top, SWP_FRAMECHANGED);

		set_device_state(need_reset);
		if (m_device_state < need_create)
			handle_device_state();
	}

	return S_OK;
}

HRESULT my12doomRenderer::recaculate_mask()
{
	return generate_mask();
	repaint_video();
}

HRESULT my12doomRenderer::pump()
{
	BOOL success = FALSE;
	RECT rect;
	if (m_hWnd)
	{
		success = GetClientRect(m_hWnd, &rect);
		if (success && rect.right > 0 && rect.bottom > 0)
		if (m_active_pp.BackBufferWidth != rect.right-rect.left || m_active_pp.BackBufferHeight != rect.bottom - rect.top)
		{
			set_device_state(need_resize_back_buffer);
		}
	}

	if (m_hWnd2)
	{
		success = GetClientRect(m_hWnd2, &rect);
		if (success && rect.right > 0 && rect.bottom > 0)
		if (m_active_pp2.BackBufferWidth != rect.right-rect.left || m_active_pp2.BackBufferHeight != rect.bottom - rect.top)
			set_device_state(need_resize_back_buffer);
	}

	backup_rgb();
	return handle_device_state();
}


HRESULT my12doomRenderer::NV3D_notify(WPARAM wparam)
{
	BOOL actived = LOWORD(wparam);
	WORD separation = HIWORD(wparam);
	
	if (actived)
	{
		m_nv3d_actived = true;
		printf("actived!\n");
	}
	else
	{
		m_nv3d_actived = false;
		printf("deactived!\n");
	}


	return S_OK;
}

HRESULT my12doomRenderer::set_input_layout(int layout)
{
	m_input_layout = (input_layout_types)layout;
	set_device_state(need_reset_object);
	handle_device_state();
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_output_mode(int mode)
{
	m_output_mode = (output_mode_types)(mode % output_mode_types_max);

	set_device_state(need_resize_back_buffer);
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_mode(int mode)
{
	m_mask_mode = (mask_mode_types)(mode % mask_mode_types_max);
	calculate_vertex();
	generate_mask();
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_color(int id, DWORD color)
{
	if (id == 1)
		m_color1 = color;
	else if (id == 2)
		m_color2 = color;
	else
		return E_INVALIDARG;

	calculate_vertex();
	repaint_video();
	return S_OK;
}

DWORD my12doomRenderer::get_mask_color(int id)
{
	if (id == 1)
		return m_color1;
	else if (id == 2)
		return m_color2;
	else
		return 0;
}

HRESULT my12doomRenderer::set_swap_eyes(bool swap)
{
	m_swapeyes = swap;

	if (m_output_mode == pageflipping)
		terminate_render_thread();
	restore_rgb();
	repaint_video();
	if (m_output_mode == pageflipping)
		create_render_thread();
	return S_OK;
}

input_layout_types my12doomRenderer::get_input_layout()
{
	return m_input_layout;
}

output_mode_types my12doomRenderer::get_output_mode()
{
	return m_output_mode;
}
mask_mode_types my12doomRenderer::get_mask_mode()
{
	return m_mask_mode;
}

bool my12doomRenderer::get_swap_eyes()
{
	return m_swapeyes;
}

bool my12doomRenderer::get_fullscreen()
{
	return !m_active_pp.Windowed;
}
HRESULT my12doomRenderer::set_movie_pos(int dimention, double offset)		// dimention1 = x, dimention2 = y
{
	if (dimention == 1)
		m_bmp_offset_x = offset;
	else if (dimention == 2)
		m_bmp_offset_y = offset;
	else
		return E_INVALIDARG;

	calculate_vertex();
	repaint_video();
	return S_OK;
}
HRESULT my12doomRenderer::set_aspect(double aspect)
{
	m_forced_aspect = aspect;
	calculate_vertex();
	if (m_output_mode == pageflipping)
		terminate_render_thread();
	repaint_video();
	if (m_output_mode == pageflipping)
		create_render_thread();
	return S_OK;
}
double my12doomRenderer::get_offset(int dimention)
{
	if (dimention == 1)
		return m_bmp_offset_x;
	else if (dimention == 2)
		return m_bmp_offset_y;
	else
		return 0.0;
}
double my12doomRenderer::get_aspect()
{
	return get_active_aspect();
}

HRESULT my12doomRenderer::set_window(HWND wnd, HWND wnd2)
{
	return E_NOTIMPL;

	reset();
	m_hWnd = wnd;
	m_hWnd2 = wnd2;
	handle_device_state();
	return S_OK;
}

HRESULT my12doomRenderer::repaint_video()
{
	if (m_dsr0->m_State != State_Running && m_output_mode != pageflipping)
		render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_bmp(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop)
{
	if (m_tex_bmp == NULL)
		return VFW_E_WRONG_STATE;

	if (m_device_state >= device_lost)
		return S_FALSE;			// TODO : of source it's not SUCCESS

	CAutoLock lck2(&m_frame_lock);

	bool changed = true;
	if (data == NULL)
	{
		if (m_bmp_fleft < -9990.0 && m_bmp_ftop < -9990.0)
			changed = false;
		m_bmp_fleft = m_bmp_ftop = -9999.0;
		m_bmp_fwidth = m_bmp_fheight = 1.0;
		m_bmp_width = 2048;
		m_bmp_height = 1024;
	}
	else
	{
		RECT lock_rect = {0, 0, 2048, min(1024, height+16)};
		D3DLOCKED_RECT locked;
		HRESULT hr = m_tex_bmp->LockRect(0, &locked, &lock_rect, NULL);
		if (FAILED(hr))
			return hr;		// device might be lost

		m_bmp_fleft = fleft;
		m_bmp_ftop = ftop;
		m_bmp_fwidth = fwidth;
		m_bmp_fheight = fheight;
		m_bmp_width = width;
		m_bmp_height = height;


		BYTE *src = (BYTE*)data;
		BYTE *dst = (BYTE*) locked.pBits;
		for(int y=0; y<min(994,height); y++)
		{
			memset(dst, 0, locked.Pitch);
			memcpy(dst, src, width*4);
			dst += locked.Pitch;
			src += width*4;
		}
		for(int y=min(994,height); y<min(994, height+16); y++)
		{
			memset(dst, 0, locked.Pitch);
			dst += locked.Pitch;
		}

		m_tex_bmp->UnlockRect(0);
	}

	if (changed)
	{
		calculate_vertex();
		repaint_video();
	}
	return S_OK;
}

HRESULT my12doomRenderer::set_ui(void* data, int pitch)
{
	return VFW_E_WRONG_STATE;

	// set ui speed limit: 15fps
	if (abs((int)(m_last_ui_draw - timeGetTime())) < 66)
		return E_FAIL;

	m_last_ui_draw = timeGetTime();

	{
		CAutoLock lck2(&m_frame_lock);

		bool should_recalculate_vertex = true;
		if (data == NULL)
		{
			m_showui = false;
		}
		else
		{
			int width = m_active_pp.BackBufferWidth;
			int height = 30;

			RECT lock_rect = {0, 1024-30, width, 1024-30+height};
			D3DLOCKED_RECT locked;
			HRESULT hr = m_tex_bmp->LockRect(0, &locked, &lock_rect, NULL);
			if (FAILED(hr))
				return hr;		// device might be lost

			BYTE *src = (BYTE*)data;
			BYTE *dst = (BYTE*) locked.pBits;
			for(int y=0; y<height; y++)
			{
				memcpy(dst, src, width*4);
				dst += locked.Pitch;
				src += pitch;
			}
			m_tex_bmp->UnlockRect(0);
			m_showui = true;
		}
	}

	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_ui_visible(bool visible)
{
	if (m_showui != visible)
	{
		m_showui = visible;
		m_ui_visible_last_change_time = timeGetTime();
		repaint_video();
	}

	m_showui = visible;
	return S_OK;
}

HRESULT my12doomRenderer::set_bmp_offset(double offset)
{
	if (m_bmp_offset != offset)
	{
		m_bmp_offset = offset;
		calculate_vertex();
		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_parallax(double parallax)
{
	if (m_parallax != parallax)
	{
		m_parallax = parallax;
		calculate_vertex();
		repaint_video();
	}

	return S_OK;
}

gpu_sample::~gpu_sample()
{
	if(m_data)free(m_data);
	m_data = NULL;

}

gpu_sample::gpu_sample(IMediaSample *memory_sample, IDirect3DDevice9 *device, int width, int height, CLSID format, bool topdown_RGB32, bool to_memory)
{
	m_width = width;
	m_height = height;
	m_ready = false;
	m_format = format;
	m_topdown = topdown_RGB32;
	m_data = NULL;
	m_device = device;
	HRESULT hr;
	if (!device || !memory_sample)
		goto clearup;

	memory_sample->GetTime(&m_start, &m_end);

	m_ready = true;
	if (to_memory)
	{
		m_data = (BYTE*)malloc(memory_sample->GetActualDataLength());
		BYTE* src=NULL;
		memory_sample->GetPointer(&src);
		memcpy(m_data, src, memory_sample->GetActualDataLength());

		return;
	}

	int l = timeGetTime();
	// texture creation
	if (DWINDOW_SURFACE_MANAGED)
	{
		if (m_format == MEDIASUBTYPE_YUY2)
		{
			JIF( device->CreateTexture(m_width/2, m_height, 1, NULL, D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,	&m_tex_YUY2, NULL));
		}

		else if (m_format == MEDIASUBTYPE_RGB32)
		{
			JIF( device->CreateTexture(m_width, m_height, 1, NULL, D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,	&m_tex_RGB32, NULL));
		}

		else if (m_format == MEDIASUBTYPE_NV12)
		{
			JIF( device->CreateTexture(m_width, m_height, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&m_tex_Y, NULL));
			JIF( device->CreateTexture(m_width/2, m_height/2, 1, NULL, D3DFMT_A8L8,D3DPOOL_MANAGED,	&m_tex_NV12_UV, NULL));
		}

		else if (m_format == MEDIASUBTYPE_YV12)
		{
			JIF( device->CreateTexture(m_width, m_height, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&m_tex_Y, NULL));
			JIF( device->CreateTexture(m_width/2, m_height, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&m_tex_YV12_UV, NULL));
		}
	}
	else
	{
		if (m_format == MEDIASUBTYPE_YUY2)
		{
			JIF( device->CreateTexture(m_width/2, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_YUY2, NULL));
		}

		else if (m_format == MEDIASUBTYPE_RGB32)
		{
			JIF( device->CreateTexture(m_width, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_RGB32, NULL));
		}

		else if (m_format == MEDIASUBTYPE_NV12)
		{
			JIF( device->CreateTexture(m_width, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_Y, NULL));
			JIF( device->CreateTexture(m_width/2, m_height/2, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8L8,D3DPOOL_DEFAULT,	&m_tex_NV12_UV, NULL));
		}

		else if (m_format == MEDIASUBTYPE_YV12)
		{
			JIF( device->CreateTexture(m_width, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_Y, NULL));
			JIF( device->CreateTexture(m_width/2, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_YV12_UV, NULL));
		}
	}


	int l2 = timeGetTime();

	// data loading
	D3DLOCKED_RECT d3dlr;
	BYTE * src;
	BYTE * dst;
	memory_sample->GetPointer(&src);
	DWORD lock_flag = DWINDOW_SURFACE_MANAGED ? NULL : D3DLOCK_DISCARD;
	if (m_format == MEDIASUBTYPE_RGB32)
	{
		JIF(hr = m_tex_RGB32->LockRect(0, &d3dlr, 0, lock_flag))
		for(int i=0; i<m_height; i++)
		{
			memory_sample->GetPointer(&src);
			src += m_width*4*(topdown_RGB32?m_height-i-1:i);
			dst = (BYTE*)d3dlr.pBits + d3dlr.Pitch*i;

			memcpy(dst, src, m_width*4);
		}
		m_tex_RGB32->UnlockRect(0);
	}

	else if (m_format == MEDIASUBTYPE_YUY2)
	{
		// loading YUY2 image as one ARGB half width texture
		JIF(hr = m_tex_YUY2->LockRect(0, &d3dlr, 0, lock_flag))
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width*2);
			src += m_width*2;
			dst += d3dlr.Pitch;
		}
		m_tex_YUY2->UnlockRect(0);
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{
		// loading NV12 image as one L8 texture and one A8L8 texture

		//load Y
		JIF(hr = m_tex_Y->LockRect(0, &d3dlr, 0, lock_flag))
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width);
			src += m_width;
			dst += d3dlr.Pitch;
		}
		m_tex_Y->UnlockRect(0);

		// load UV
		JIF(hr = m_tex_NV12_UV->LockRect(0, &d3dlr, 0, lock_flag))
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height/2; i++)
		{
			memcpy(dst, src, m_width);
			src += m_width;
			dst += d3dlr.Pitch;
		}
		m_tex_NV12_UV->UnlockRect(0);
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		// loading YV12 image as two L8 texture
		// load Y
		JIF(hr = m_tex_Y->LockRect(0, &d3dlr, 0, lock_flag))
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width);
			src += m_width;
			dst += d3dlr.Pitch;
		}
		m_tex_Y->UnlockRect(0);

		// load UV
		JIF(hr = m_tex_YV12_UV->LockRect(0, &d3dlr, 0, lock_flag))
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width/2);
			src += m_width/2;
			dst += d3dlr.Pitch;
		}
		m_tex_YV12_UV->UnlockRect(0);
	}
	//if (timeGetTime() - l > 5)
		mylog("load():createTexture time:%d ms, load data to GPU cost %d ms.\n", l2- l, timeGetTime()-l2);

	return;

clearup:
	m_tex_RGB32 = NULL;
	m_tex_YUY2 = NULL;
	m_tex_Y = NULL;
	m_tex_YV12_UV = NULL;
	m_tex_NV12_UV = NULL;
}

HRESULT gpu_sample::load()
{
	if(!m_data)
		return S_FALSE;


	int l = timeGetTime();
	// texture creation
	HRESULT hr;
	if (DWINDOW_SURFACE_MANAGED)
	{
		if (m_format == MEDIASUBTYPE_YUY2)
		{
			JIF( m_device->CreateTexture(m_width/2, m_height, 1, NULL, D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,	&m_tex_YUY2, NULL));
		}

		else if (m_format == MEDIASUBTYPE_RGB32)
		{
			JIF( m_device->CreateTexture(m_width, m_height, 1, NULL, D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,	&m_tex_RGB32, NULL));
		}

		else if (m_format == MEDIASUBTYPE_NV12)
		{
			JIF( m_device->CreateTexture(m_width, m_height, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&m_tex_Y, NULL));
			JIF( m_device->CreateTexture(m_width/2, m_height/2, 1, NULL, D3DFMT_A8L8,D3DPOOL_MANAGED,	&m_tex_NV12_UV, NULL));
		}

		else if (m_format == MEDIASUBTYPE_YV12)
		{
			JIF( m_device->CreateTexture(m_width, m_height, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&m_tex_Y, NULL));
			JIF( m_device->CreateTexture(m_width/2, m_height, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&m_tex_YV12_UV, NULL));
		}
	}
	else
	{
		if (m_format == MEDIASUBTYPE_YUY2)
		{
			JIF( m_device->CreateTexture(m_width/2, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_YUY2, NULL));
		}

		else if (m_format == MEDIASUBTYPE_RGB32)
		{
			JIF( m_device->CreateTexture(m_width, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_RGB32, NULL));
		}

		else if (m_format == MEDIASUBTYPE_NV12)
		{
			JIF( m_device->CreateTexture(m_width, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_Y, NULL));
			JIF( m_device->CreateTexture(m_width/2, m_height/2, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8L8,D3DPOOL_DEFAULT,	&m_tex_NV12_UV, NULL));
		}

		else if (m_format == MEDIASUBTYPE_YV12)
		{
			JIF( m_device->CreateTexture(m_width, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_Y, NULL));
			JIF( m_device->CreateTexture(m_width/2, m_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_YV12_UV, NULL));
		}
	}

	// data loading
	int l2 = timeGetTime();
	D3DLOCKED_RECT d3dlr;
	BYTE * src = m_data;
	BYTE * dst;
	DWORD lock_flag = DWINDOW_SURFACE_MANAGED ? NULL : D3DLOCK_DISCARD;
	if (m_format == MEDIASUBTYPE_RGB32)
	{
		JIF(hr = m_tex_RGB32->LockRect(0, &d3dlr, 0, lock_flag))
			for(int i=0; i<m_height; i++)
			{
				
				src = m_data + m_width*4*(m_topdown?m_height-i-1:i);
				dst = (BYTE*)d3dlr.pBits + d3dlr.Pitch*i;

				memcpy(dst, src, m_width*4);
			}
			m_tex_RGB32->UnlockRect(0);
	}

	else if (m_format == MEDIASUBTYPE_YUY2)
	{
		// loading YUY2 image as one ARGB half width texture
		JIF(hr = m_tex_YUY2->LockRect(0, &d3dlr, 0, lock_flag))
			dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width*2);
			src += m_width*2;
			dst += d3dlr.Pitch;
		}
		m_tex_YUY2->UnlockRect(0);
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{
		// loading NV12 image as one L8 texture and one A8L8 texture

		//load Y
		JIF(hr = m_tex_Y->LockRect(0, &d3dlr, 0, lock_flag))
			dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width);
			src += m_width;
			dst += d3dlr.Pitch;
		}
		m_tex_Y->UnlockRect(0);

		// load UV
		JIF(hr = m_tex_NV12_UV->LockRect(0, &d3dlr, 0, lock_flag))
			dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height/2; i++)
		{
			memcpy(dst, src, m_width);
			src += m_width;
			dst += d3dlr.Pitch;
		}
		m_tex_NV12_UV->UnlockRect(0);
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		// loading YV12 image as two L8 texture
		// load Y
		JIF(hr = m_tex_Y->LockRect(0, &d3dlr, 0, lock_flag))
			dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width);
			src += m_width;
			dst += d3dlr.Pitch;
		}
		m_tex_Y->UnlockRect(0);

		// load UV
		JIF(hr = m_tex_YV12_UV->LockRect(0, &d3dlr, 0, lock_flag))
			dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width/2);
			src += m_width/2;
			dst += d3dlr.Pitch;
		}
		m_tex_YV12_UV->UnlockRect(0);
	}

	free(m_data);
	m_data = NULL;

	if (timeGetTime() - l > 10)
		mylog("load():createTexture time:%d ms, load data to GPU cost %d ms.\n", l2- l, timeGetTime()-l2);

	return hr;
clearup:
	m_ready = false;
	return hr;
}
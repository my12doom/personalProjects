// Direct3D9 part of my12doom renderer

#include "my12doomRenderer.h"
#include "PixelShaders/YV12.h"
#include "PixelShaders/NV12.h"
#include "PixelShaders/YUY2.h"
#include "PixelShaders/anaglyph.h"
#include "PixelShaders/masking.h"
#include "PixelShaders/stereo_test_sbs.h"
#include "PixelShaders/stereo_test_sbs2.h"
#include "PixelShaders/stereo_test_tb.h"
#include "PixelShaders/stereo_test_tb2.h"
#include "PixelShaders/vs_subtitle.h"
#include "PixelShaders/iz3d_back.h"
#include "PixelShaders/iz3d_front.h"
#include "PixelShaders/color_adjust.h"
#include "PixelShaders/blur.h"
#include "PixelShaders/blur2.h"
#include "PixelShaders/lanczosAll.h"
#include "3dvideo.h"
#include <dvdmedia.h>
#include <math.h>
#include <assert.h>
#include "..\dwindow\global_funcs.h"

enum helper_sample_format
{
	helper_sample_format_rgb32,
	helper_sample_format_y,
	helper_sample_format_yv12,
	helper_sample_format_nv12,
	helper_sample_format_yuy2,
};

#pragma comment (lib, "igfx_s3dcontrol.lib")
#pragma comment (lib, "comsupp.lib")
#pragma comment (lib, "dxva2.lib")
#pragma comment (lib, "nvapi.lib")

int lockrect_surface = 0;
int lockrect_texture = 0;
__int64 lockrect_texture_cycle = 0;
const int BIG_TEXTURE_SIZE = 2048;
AutoSetting<BOOL> GPUIdle(L"GPUIdle", true, REG_DWORD);
AutoSetting<int> MovieResizing(L"MovieResampling", bilinear_mipmap_minus_one, REG_DWORD);
AutoSetting<int> SubtitleResizing(L"SubtitleResampling", bilinear_mipmap_minus_one, REG_DWORD);


IDirect3DTexture9* helper_get_texture(gpu_sample *sample, helper_sample_format format);
HRESULT myMatrixOrthoRH(D3DMATRIX *matrix, float w, float h, float zn, float zf);
HRESULT myMatrixIdentity(D3DMATRIX *matrix);

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
	OutputDebugStringA(tmp2);
#endif
	return S_OK;
}

my12doomRenderer::my12doomRenderer(HWND hwnd, HWND hwnd2/* = NULL*/):
m_left_queue(_T("left queue")),
m_right_queue(_T("right queue"))
{

	typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)(UINT, IDirect3D9Ex**);

	// D3D && NV3D && intel 3D
	m_nv3d_enabled = false;
	m_nv3d_actived = false;
	m_nv3d_display = NULL;
	m_nv3d_display = 0;
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

		//NvAPI_Stereo_SetDriverMode(NVAPI_STEREO_DRIVER_MODE_DIRECT);

		res = NvAPI_EnumNvidiaDisplayHandle(0, &m_nv3d_display);

		m_nv_version.version = NV_DISPLAY_DRIVER_VERSION_VER;
		res = NvAPI_GetDisplayDriverVersion(m_nv3d_display, &m_nv_version);

		if (res == NVAPI_OK && m_nv_version.drvVersion >= 27051)		// only 270.51+ supports windowed 3d vision
			m_nv3d_windowed = true;
		else
			m_nv3d_windowed = false;

		// disable new 3d vision interface for now
		m_nv_version.drvVersion = min(m_nv_version.drvVersion, 28500);
	}
	m_nv3d_handle = NULL;
	m_output_mode = mono;
	m_intel_s3d = NULL;
	m_HD3DStereoModesCount = m_HD3Dlineoffset = 0;
	m_render_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pool = NULL;
	m_last_rendered_sample1 = m_last_rendered_sample2 = m_sample2render_1 = m_sample2render_2 = NULL;
	m_enter_counter = 0;
	HMODULE h = LoadLibrary( L"d3d9.dll" );
	if ( h )
	{
		typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)( UINT, 
			IDirect3D9Ex**);

		LPDIRECT3DCREATE9EX func = (LPDIRECT3DCREATE9EX)GetProcAddress( h, 
			"Direct3DCreate9Ex" );

		if ( func )
			func( D3D_SDK_VERSION, &m_D3DEx );

		FreeLibrary( h );
	}

	if (m_D3DEx)
		m_D3DEx->QueryInterface(IID_IDirect3D9, (void**)&m_D3D);
	else
		m_D3D = Direct3DCreate9( D3D_SDK_VERSION );


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
	// Vertical Sync
	m_vertical_sync = true;

	// Display Orientation
	m_display_orientation = horizontal;

	// PC level test
	m_PC_level = 0;
	m_last_reset_time = timeGetTime();

	// 2D - 3D Convertion
	m_convert3d = false;

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
	m_movie_offset_x = 0.0;
	m_movie_offset_y = 0.0;
	m_source_aspect = 1.0;
	m_forced_aspect = -1;
	m_aspect_mode = aspect_letterbox;

	// input / output
	m_input_layout = input_layout_auto;
	m_swapeyes = false;
	m_mask_mode = row_interlace;
	m_mask_parameter = 0;
	m_color1 = D3DCOLOR_XRGB(255, 0, 0);
	m_color2 = D3DCOLOR_XRGB(0, 255, 255);
	m_normal = m_sbs = m_tb = 0;
	m_forced_deinterlace = false;

	// color adjust
	m_saturation1 =
	m_luminance1 =
	m_hue1 =
	m_contrast1 = 0.5;

	// ui & bitmap
	m_has_subtitle = false;
	m_volume = 0;
	m_bmp_parallax = 0;
	m_bmp_width = 0;
	m_bmp_height = 0;
	m_total_time = 0;
}

my12doomRenderer::~my12doomRenderer()
{
	terminate_render_thread();
	invalidate_gpu_objects();
	if (m_pool) delete m_pool;
	m_Device = NULL;
	m_D3D = NULL;
}

HRESULT my12doomRenderer::SetMediaType(const CMediaType *pmt, int id)
{
	if (id == 1)
		return S_OK;


	if (*pmt->FormatType() == FORMAT_VideoInfo)
	{
		printf("SetMediaType() VideoInfo1,  %s deinterlace\n", m_deinterlace ? "DO" : "NO");
		m_deinterlace = false;
	}
	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->Format();
		if (vihIn->dwInterlaceFlags & AMINTERLACE_IsInterlaced)
		{
			m_deinterlace = true;
		}
		printf("SetMediaType() VideoInfo2,  %s deinterlace\n", m_deinterlace ? "DO" : "NO");
	}

	printf("SetMediaType() %s deinterlace\n", m_deinterlace ? "DO" : "NO");

	return S_OK;
}

HRESULT my12doomRenderer::CheckMediaType(const CMediaType *pmt, int id)
{
	if (m_sbs + m_tb + m_normal > 5 || (m_dsr0->is_connected() && m_dsr1->is_connected()))
		return E_FAIL;

	HRESULT hr = S_OK;
	bool deinterlace = false;
	int width, height, aspect_x, aspect_y;
	if (*pmt->FormatType() == FORMAT_VideoInfo)
	{
		printf("CheckMediaType(%d) VideoInfo1\n", id);
		VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)pmt->Format();
		width = vihIn->bmiHeader.biWidth;
		height = vihIn->bmiHeader.biHeight;
		aspect_x = width;
		aspect_y = abs(height);
		m_frame_length = vihIn->AvgTimePerFrame;
		m_first_PTS = 0;
		deinterlace = false;


		//if (m_dsr0->m_formattype == FORMAT_VideoInfo2)
		//	hr = E_FAIL;

	}

	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->Format();
		printf("CheckMediaType(%d) VideoInfo2, flag: %d(%02x), %s deinterlace\n", id, vihIn->dwInterlaceFlags, vihIn->dwInterlaceFlags, deinterlace ? "DO" : "NO");
		if (vihIn->dwInterlaceFlags & AMINTERLACE_IsInterlaced)
		{
			deinterlace = true;
			if ((vihIn->dwInterlaceFlags & AMINTERLACE_FieldPatBothRegular))
			{
				//printf("AMINTERLACE_FieldPatBothRegular not supported, E_FAIL\n");
				//hr = E_FAIL;
			}
			//return E_FAIL;
		}
		else
		{
			deinterlace = false;
		}
		width = vihIn->bmiHeader.biWidth;
		height = vihIn->bmiHeader.biHeight;
		aspect_x = vihIn->dwPictAspectRatioX;
		aspect_y = vihIn->dwPictAspectRatioY;
		m_frame_length = vihIn->AvgTimePerFrame;
		m_first_PTS = 0;

		//AMINTERLACE_IsInterlaced
		// 0x21

	}
	else
	{
		printf("Not Video.\n");
		hr = E_FAIL;
	}


	if (height > 0)
		m_revert_RGB32 = true;
	else
		m_revert_RGB32 = false;

	height = abs(height);

	if (m_remux_mode && height == 1088)
	{
		height = 1080;
		aspect_x = 16;
		aspect_y = 9;
	}

	if (*pmt->Subtype() != MEDIASUBTYPE_YUY2 && m_remux_mode)
		hr = E_FAIL;

	if (aspect_x == 1920 && aspect_y == 1088)
	{
		aspect_x = 16;
		aspect_y = 9;
	}

	m_last_frame_time = 0;
	m_dsr0->set_queue_size(m_remux_mode ? my12doom_queue_size : my12doom_queue_size/2-1);
	m_dsr0->set_queue_size(my12doom_queue_size/2-1);

	if (id == 0 && SUCCEEDED(hr))
	{
		m_lVidWidth = width;
		m_lVidHeight = height;
		m_source_aspect = (double)aspect_x / aspect_y;

		hr = S_OK;
	}
	else if (SUCCEEDED(hr))
	{
		if (!m_dsr0->is_connected())
		{
			printf("don't connect 2nd renderer before 1st, E_FAIL.\n");
			hr = E_FAIL;
		}

		if (width == m_lVidWidth && height == m_lVidHeight  && SUCCEEDED(hr))			// format is not important, but resolution and formattype is
		{
			m_source_aspect = (double)aspect_x / aspect_y;
			hr = S_OK;
		}
		else if (SUCCEEDED(hr))
		{
			printf("resolution mismatch, E_FAIL.\n");
			hr = E_FAIL;
		}
	}

	if (hr == S_OK)
		m_deinterlace = deinterlace;

	printf("return %s.\n", hr == S_OK ? "S_OK" : "E_FAIL");

	return hr;
}
HRESULT	my12doomRenderer::BreakConnect(int id)
{
	return S_OK;
}
HRESULT my12doomRenderer::CompleteConnect(IPin *pRecievePin, int id)
{
	AM_MEDIA_TYPE mt;
	pRecievePin->ConnectionMediaType(&mt);
	if (mt.formattype == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)mt.pbFormat;

		vihIn = vihIn;

	}

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
	m_last_frame_time = timeGetTime();

	int l1 = timeGetTime();
	bool should_render = true;
	REFERENCE_TIME start=0, end=0;
	media_sample->GetTime(&start, &end);
	if (m_cb && id == 0)
		m_cb->SampleCB(start + m_dsr0->m_thisstream, end + m_dsr0->m_thisstream, media_sample);
	if (id == 0)
	{
		m_first_PTS = m_first_PTS != 0 ? m_first_PTS : start;
		m_frame_length = m_frame_length != 0 ? m_frame_length :(start - m_first_PTS) ;
	}

	int l2 = timeGetTime();
	if (!m_dsr1->is_connected() &&!m_remux_mode)
	{
		// single stream
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;
		do
		{
			if (sample) 
			{
				printf("drop left\n");
				m_dsr0->drop_packets(1);
				delete sample;
				sample = NULL;
			}
			if (m_left_queue.GetCount())
				sample = m_left_queue.RemoveHead();
		}while (sample && sample->m_start < start);

		if(sample)
		{
			CAutoLock lck(&m_packet_lock);
			safe_delete(m_sample2render_1);
			m_sample2render_1 = sample;
			should_render = true;
		}
	}
	else
	{
		// pd10
		if (m_remux_mode)
		{
			int fn;
			if (m_lVidHeight == 1280)
			{
				fn = (int)((double)(start+m_dsr0->m_thisstream)/10000*120/1001 + 0.5);
				start = (REFERENCE_TIME)(fn - (fn&1) +2) *1001*10000/120;
			}
			else
			{
				fn = (int)((double)(start+m_dsr0->m_thisstream)/10000*48/1001 + 0.5);
				start = (REFERENCE_TIME)(fn - (fn&1) +2) *1001*10000/48;
			}
		}

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
					if (abs((int)(left_sample->m_start - right_sample->m_start)) < m_frame_length/2)
					{
						matched_time = left_sample->m_start;
						matched = true;
						goto match_end;
					}
				}
			}

match_end:
			if (!matched || matched_time > start)
				return S_FALSE;

			if(matched)
			{
				// release any unmatched and retrive matched
				CAutoLock lck(&m_queue_lock);
				while(true)
				{
					sample_left = m_left_queue.RemoveHead();
					if(abs((int)(sample_left->m_start - matched_time)) > m_frame_length/2)
					{
						printf("drop left\n");
						m_dsr0->drop_packets(1);
						delete sample_left;
					}
					else
						break;
				}

				while(true)
				{
					sample_right = m_right_queue.RemoveHead();
					if(abs((int)(sample_right->m_start - matched_time)) > m_frame_length/2)
					{
						printf("drop right\n");
						(m_remux_mode?m_dsr0:m_dsr1)->drop_packets(1);
						delete sample_right;
					}
					else
						break;
				}
			}
		}

		if (sample_left && sample_right)
		{
			if (matched_time < start && false)
			{
				printf("drop a matched pair\n");
				m_dsr0->drop_packets(1);
				(m_remux_mode?m_dsr0:m_dsr1)->drop_packets(1);
				delete sample_left;
				delete sample_right;
				sample_left = NULL;
				sample_right = NULL;
				goto find_match;
			}
			else
			{
				CAutoLock lck(&m_packet_lock);
				safe_delete (m_sample2render_1);
				m_sample2render_1 = sample_left;
				safe_delete(m_sample2render_2);
				m_sample2render_2 = sample_right;
				should_render = true;
			}
		}
	}
	int l3 = timeGetTime();
	if (m_output_mode != pageflipping && should_render)
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

	REFERENCE_TIME start, end;
	media_sample->GetTime(&start, &end);
	if (m_cb && id == 0)
		m_cb->PrerollCB(start + m_dsr0->m_thisstream, end + m_dsr0->m_thisstream, media_sample);


	int l1 = timeGetTime();
	int bit_per_pixel = 12;
	if (m_dsr0->m_format == MEDIASUBTYPE_YUY2)
		bit_per_pixel = 16;
	else if (m_dsr0->m_format == MEDIASUBTYPE_RGB32 || m_dsr0->m_format == MEDIASUBTYPE_ARGB32)
		bit_per_pixel = 32;

	bool should_render = false;

	//mylog("%s : %dms\n", id==0?"left":"right", (int)(start/10000));

	bool dual_stream = m_remux_mode || (m_dsr0->is_connected() && m_dsr1->is_connected() );
	input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
	bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;
	gpu_sample * loaded_sample = NULL;
	int max_retry = 5;
	{
retry:
		//CAutoLock lck(&m_pool_lock);
		loaded_sample = new gpu_sample(media_sample, m_pool, m_lVidWidth, m_lVidHeight, m_dsr0->m_format, m_revert_RGB32, need_detect, m_remux_mode, D3DPOOL_SYSTEMMEM, m_PC_level);

//		if ( (m_dsr0->m_format == MEDIASUBTYPE_RGB32 && (!loaded_sample->m_tex_RGB32 || loaded_sample->m_tex_RGB32->creator != m_Device)) ||
//			 (m_dsr0->m_format == MEDIASUBTYPE_YUY2 && (!loaded_sample->m_tex_YUY2 || loaded_sample->m_tex_YUY2->creator != m_Device)) ||
//			 ((!loaded_sample->m_tex_Y || loaded_sample->m_tex_Y->creator != m_Device) && (m_dsr0->m_format == MEDIASUBTYPE_YV12 || m_dsr1->m_format == MEDIASUBTYPE_NV12))  )
		if (!loaded_sample->m_ready)
		{
			delete loaded_sample;
			if (max_retry--)
			{
				mylog("video upload failed, retry left: %d", max_retry);
				goto retry;
			}
			else
				return S_FALSE;
		}
	}

	//loaded_sample->prepare_rendering();
	if (false)
	{
		CAutoLock frame_lock(&m_frame_lock);
		int l = timeGetTime();
		loaded_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, NULL, m_last_reset_time);
		int l2 = timeGetTime();
		if (need_detect) loaded_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
		mylog("queue size:%d, convert_to_RGB32(): %dms, stereo_test:%dms\n", m_left_queue.GetCount(), l2-l, timeGetTime()-l2);
	}

	/// GPU RAM protector:
	if (m_left_queue.GetCount() > my12doom_queue_size*2 || m_right_queue.GetCount() > my12doom_queue_size*2)
	{
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;
		while (sample = m_left_queue.RemoveHead())
			delete sample;
		while (sample = m_right_queue.RemoveHead())
			delete sample;
	}


	AM_MEDIA_TYPE *pmt = NULL;
	media_sample->GetMediaType(&pmt);
	if (pmt)
	{
		if (pmt->formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->pbFormat;
			printf("sample video header 2.\n");

		}

		else if (pmt->formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)pmt->pbFormat;
			printf("sample video header.\n");

		}

	}

	int l2 = timeGetTime();
	if (!m_dsr1->is_connected() && !m_remux_mode)
	{
		// single stream
		CAutoLock lck(&m_queue_lock);
		m_left_queue.AddTail(loaded_sample);
	}

	else if (m_remux_mode)
	{
		// PD10 remux
		// time and frame number
		REFERENCE_TIME TimeStart, TimeEnd;
		media_sample->GetTime(&TimeStart, &TimeEnd);
		int fn;
		if (m_lVidHeight == 1280)
		{
			fn = (int)((double)(TimeStart+m_dsr0->m_thisstream)/10000*120/1001 + 0.5);
			loaded_sample->m_start = (REFERENCE_TIME)(fn - (fn&1)) *1001*10000/120;
		}
		else
		{
			fn = (int)((double)(TimeStart+m_dsr0->m_thisstream)/10000*48/1001 + 0.5);
			loaded_sample->m_start = (REFERENCE_TIME)(fn - (fn&1)) *1001*10000/48;
		}

		float frn = (float)(TimeStart+m_dsr0->m_thisstream)/10000*48/1001 + 0.5;
		loaded_sample->m_end = loaded_sample->m_start + 1;
		loaded_sample->m_fn = fn;

		CAutoLock lck(&m_queue_lock);
		if ( 1- (fn & 0x1))
		{
			printf("left(%f)", frn);
			m_left_queue.AddTail(loaded_sample);
		}
		else
		{
			printf("right(%f)", frn);
			m_right_queue.AddTail(loaded_sample);
		}
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

HRESULT my12doomRenderer::fix_nv3d_bug()
{
	HRESULT hr = S_OK;
	CComPtr<IDirect3DSurface9> m_nv3d_bugfix;
	FAIL_RET( m_Device->CreateRenderTarget(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_nv3d_bugfix, NULL));


	// NV3D acitivation
	NvAPI_Status res;
	if (m_nv3d_handle == NULL)
		res = NvAPI_Stereo_CreateHandleFromIUnknown(m_Device, &m_nv3d_handle);
	res = NvAPI_Stereo_SetNotificationMessage(m_nv3d_handle, (NvU64)m_hWnd, WM_NV_NOTIFY);
	if (m_output_mode == NV3D)
	{
		//mylog("activating NV3D\n");
		res = NvAPI_Stereo_Activate(m_nv3d_handle);
	}

	else
	{
		//mylog("deactivating NV3D\n");
		res = NvAPI_Stereo_Deactivate(m_nv3d_handle);
	}
	NvU8 actived = 0;
	res = NvAPI_Stereo_IsActivated(m_nv3d_handle, &actived);
	if (actived)
	{
		//mylog("init: NV3D actived\n");
		m_nv3d_actived = true;
	}
	else
	{
		//mylog("init: NV3D deactived\n");
		m_nv3d_actived = false;
	}

	if (res == NVAPI_OK && m_nv3d_windowed)
		m_nv3d_actived = m_output_mode == NV3D ? true : false;

	//res = NvAPI_Stereo_DestroyHandle(m_nv3d_handle);
	return hr;
}
HRESULT my12doomRenderer::delete_render_targets()
{
	intel_delete_rendertargets();
	m_swap1 = NULL;
	m_swap2 = NULL;
	m_mask_temp_left = NULL;
	m_mask_temp_right = NULL;
	m_nv3d_surface = NULL;
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

		D3DPRESENT_PARAMETERS pp = m_active_pp;
// 		pp.SwapEffect = D3DSWAPEFFECT_OVERLAY;
// 		pp.MultiSampleType = D3DMULTISAMPLE_NONE;
// 		pp.BackBufferFormat = D3DFMT_X8R8G8B8;
// 		m_new_pp.BackBufferFormat = D3DFMT_X8R8G8B8;
		if (m_active_pp.BackBufferWidth > 0 && m_active_pp.BackBufferHeight > 0)
			FAIL_RET(m_Device->CreateAdditionalSwapChain(&pp, &m_swap1))

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

	if (m_swap1)
	{
		RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
		if (m_output_mode == out_sbs)
			tar.right /= 2;
		else if (m_output_mode == out_tb)
			tar.bottom /= 2;

		if(m_Device && m_uidrawer ) m_uidrawer->init(tar.right, tar.bottom, m_Device);
	}


	FAIL_RET(intel_create_rendertargets());
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_mask_temp_left, NULL));
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &m_mask_temp_right, NULL));
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_tex_mask, NULL));
	FAIL_RET( m_Device->CreateRenderTarget(m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_nv3d_surface, NULL));
	FAIL_RET(generate_mask());

	// add 3d vision tag at last line on need
	if (m_output_mode == NV3D && m_nv3d_surface)
	{
		mylog("Adding NV3D Tag.\n");
		D3DLOCKED_RECT lr;
		RECT lock_tar={0, m_active_pp.BackBufferHeight, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1};
		FAIL_RET(m_nv3d_surface->LockRect(&lr,&lock_tar,0));
		LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
		pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		pSIH->dwBPP = 32;
		pSIH->dwFlags = SIH_SIDE_BY_SIDE;
		pSIH->dwWidth = m_active_pp.BackBufferWidth*2;
		pSIH->dwHeight = m_active_pp.BackBufferHeight;
		FAIL_RET(m_nv3d_surface->UnlockRect());

		mylog("NV3D Tag Added.\n");
	}

	fix_nv3d_bug();
	return hr;
}

HRESULT my12doomRenderer::handle_device_state()							//handle device create/recreate/lost/reset
{
	if (m_device_state < need_reset)
	{
		HRESULT hr = m_Device->TestCooperativeLevel();
		if (FAILED(hr))
			set_device_state(device_lost);
	}

	if (GetCurrentThreadId() != m_device_threadid || m_recreating_dshow_renderer)
	{
		if (m_device_state == fine)
			return S_OK;
		else
			return E_FAIL;
	}

	anti_reenter anti(&m_anti_reenter, &m_enter_counter);
	if (anti.error)
		return S_OK;

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
		mylog("handling device state(%s)\n", states[m_device_state]);
	}

	if (m_device_state == fine)
		return S_OK;

	if (m_device_state == create_failed)
		return E_FAIL;

	else if (m_device_state == need_resize_back_buffer)
	{
		CAutoLock lck(&m_frame_lock);
		if (FAILED(hr=(delete_render_targets())))
		{
			m_device_state = device_lost;
			return hr;
		}
		if (FAILED(hr=(create_render_targets())))
		{
			m_device_state = device_lost;
			return hr;
		}
		m_device_state = fine;

		// clear DEFAULT pool
		{
			CAutoLock lck(&m_pool_lock);
			if (m_pool)
				m_pool->DestroyPool(D3DPOOL_DEFAULT);
		}

		render_nolock(true);
	}
	else if (m_device_state == need_reset_object)
	{
		terminate_render_thread();
		CAutoLock lck(&m_frame_lock);
		if (FAILED(hr=(invalidate_gpu_objects())))
		{
			m_device_state = device_lost;
			return hr;
		}
		if (FAILED(hr=(restore_gpu_objects())))
		{
			m_device_state = device_lost;
			return hr;
		}
		m_device_state = fine;
	}

	else if (m_device_state == need_reset)
	{
		terminate_render_thread();
		CAutoLock lck(&m_frame_lock);
		mylog("reseting device.\n");
		int l = timeGetTime();
		FAIL_SLEEP_RET(invalidate_gpu_objects());
		mylog("invalidate objects: %dms.\n", timeGetTime() - l);
		HRESULT hr = m_Device->Reset( &m_new_pp );
		hr = m_Device->Reset( &m_new_pp );
		if( FAILED(hr ) )
		{
			m_device_state = device_lost;
			return hr;
		}
		m_d3d_manager->ResetDevice(m_Device, m_resetToken);
		m_active_pp = m_new_pp;
		mylog("Device->Reset: %dms.\n", timeGetTime() - l);
		if (FAILED(hr=(restore_gpu_objects())))
		{
			m_device_state = device_lost;
			return hr;
		}
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
			FAIL_SLEEP_RET(invalidate_gpu_objects());
			HRESULT hr = m_Device->Reset( &m_new_pp );

			if( FAILED(hr ) )
			{
				m_device_state = device_lost;
				return hr;
			}
			m_active_pp = m_new_pp;
			m_d3d_manager->ResetDevice(m_Device, m_resetToken);
			FAIL_SLEEP_RET(restore_gpu_objects());
			
			m_device_state = fine;
			return hr;
		}

		else if (hr == D3DERR_DEVICELOST)
		{
			terminate_render_thread();
			CAutoLock lck(&m_frame_lock);
			FAIL_SLEEP_RET(invalidate_gpu_objects());
		}

		else if (hr == S_OK)
			m_device_state = fine;

		else
			return E_FAIL;
	}

	else if (m_device_state == need_create)
	{
		if (!m_Device)
		{
			ZeroMemory( &m_active_pp, sizeof(m_active_pp) );
			m_active_pp.Windowed               = TRUE;
			m_active_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
			m_active_pp.PresentationInterval   = m_vertical_sync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
			m_active_pp.BackBufferCount = 1;
			m_active_pp.Flags = D3DPRESENTFLAG_VIDEO;
			m_active_pp.BackBufferFormat = D3DFMT_A8R8G8B8;
			m_active_pp.MultiSampleType = D3DMULTISAMPLE_NONE;

			m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
			m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
			GetWindowRect(m_hWnd, &m_window_pos);

			set_fullscreen(false);

			/*
			set_fullscreen(true);
			m_active_pp.BackBufferWidth = 1680;
			m_active_pp.BackBufferHeight = 1050;
			m_active_pp.Windowed = FALSE;
			*/
		}
		else
		{
			terminate_render_thread();
			CAutoLock lck(&m_frame_lock);
			invalidate_gpu_objects();
			invalidate_cpu_objects();
			if(m_uidrawer)
			{
				delete m_uidrawer;
				m_uidrawer = NULL;
			}
			m_uidrawer = new ui_drawer_dwindow;
			m_Device = NULL;
			m_active_pp = m_new_pp;
		}

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
		HRESULT hr;
		CAutoLock lck(&m_frame_lock);

		FAIL_RET(intel_get_caps());

		if (m_D3DEx)
		{
			hr = m_D3DEx->CreateDeviceEx( AdapterToUse, DeviceType,
				m_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
				&m_active_pp, NULL, &m_DeviceEx );

			m_DeviceEx->QueryInterface(IID_IDirect3DDevice9, (void**)&m_Device);
		}
		else
		{
			hr = m_D3D->CreateDevice( AdapterToUse, DeviceType,
				m_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
				&m_active_pp, &m_Device );
		}

		if (FAILED(hr))
			return hr;

		FAIL_RET(intel_d3d_init());

		m_new_pp = m_active_pp;
		m_device_state = need_reset_object;

		FAIL_RET(HD3D_one_time_init());


		set_output_mode(m_output_mode);		// just to active nv3d

		{
			CAutoLock lck(&m_pool_lock);
			if (m_pool) delete m_pool;
			m_pool = new CTextureAllocator(m_Device);
		}

		FAIL_SLEEP_RET(restore_gpu_objects());
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
	invalidate_gpu_objects();
	set_device_state(need_reset_object);
	init_variables();

	// what invalidate_gpu_objects() don't do
	gpu_sample *sample = NULL;
	while (sample = m_left_queue.RemoveHead())
		delete sample;
	while (sample = m_right_queue.RemoveHead())
		delete sample;
	m_dsr0->drop_packets();
	m_dsr1->drop_packets();
	{
		CAutoLock lck(&m_packet_lock);
		safe_delete(m_sample2render_1);
		safe_delete(m_sample2render_2);
	}

	{
		CAutoLock rendered_lock(&m_rendered_packet_lock);
		safe_delete(m_last_rendered_sample1);
		safe_delete(m_last_rendered_sample2);
	}


	return S_OK;
}
HRESULT my12doomRenderer::create_render_thread()
{
	CAutoLock lck(&m_thread_lock);
	if (INVALID_HANDLE_VALUE != m_render_thread)
		terminate_render_thread();
	m_render_thread_exit = false;
	m_render_thread = CreateThread(0,0,render_thread, this, NULL, NULL);
	return S_OK;
}
HRESULT my12doomRenderer::terminate_render_thread()
{
	CAutoLock lck(&m_thread_lock);
	if (INVALID_HANDLE_VALUE == m_render_thread)
		return S_FALSE;
	m_render_thread_exit = true;
	WaitForSingleObject(m_render_thread, INFINITE);
	m_render_thread = INVALID_HANDLE_VALUE;

	return S_OK;
}

HRESULT my12doomRenderer::invalidate_cpu_objects()
{
	m_uidrawer->invalidate_cpu();
	if (m_pool) m_pool->DestroyPool(D3DPOOL_MANAGED);
	if (m_pool) m_pool->DestroyPool(D3DPOOL_SYSTEMMEM);

	return S_OK;
}

HRESULT my12doomRenderer::invalidate_gpu_objects()
{
	HD3D_invalidate_objects();
	m_uidrawer->invalidate_gpu();
	m_red_blue.invalid();
	m_ps_masking.invalid();
	m_lanczosX.invalid();
	m_lanczosX_NV12.invalid();
	m_lanczosX_YV12.invalid();
	m_lanczosY.invalid();
	m_lanczos.invalid();
	m_lanczos_NV12.invalid();
	m_lanczos_YV12.invalid();

	// query
	//m_d3d_query = NULL;

	// pixel shader
	m_ps_yv12 = NULL;
	m_ps_nv12 = NULL;
	m_ps_yuy2 = NULL;
	m_ps_test = NULL;
	m_ps_test_sbs = NULL;
	m_ps_test_sbs2 = NULL;
	m_ps_test_tb = NULL;
	m_ps_test_tb2 = NULL;
	m_ps_anaglyph = NULL;
	m_ps_iz3d_back = NULL;
	m_ps_iz3d_front = NULL;
	m_ps_color_adjust = NULL;
	m_ps_bmp_lanczos = NULL;
	m_ps_bmp_blur = NULL;

	// textures
	m_tex_bmp = NULL;

	// pending samples
	{
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;

		for(POSITION pos = m_left_queue.GetHeadPosition();
			pos; pos = m_left_queue.Next(pos))
		{
			sample = m_left_queue.Get(pos);
			safe_decommit(sample);
		}

		for(POSITION pos = m_right_queue.GetHeadPosition();
			pos; pos = m_right_queue.Next(pos))
		{
			sample = m_right_queue.Get(pos);
			safe_decommit(sample);
		}

	}
	{
		CAutoLock lck(&m_packet_lock);
		safe_decommit(m_sample2render_1);
		safe_decommit(m_sample2render_2);
	}

	{
		CAutoLock rendered_lock(&m_rendered_packet_lock);
		safe_decommit(m_last_rendered_sample1);
		safe_decommit(m_last_rendered_sample2);
	}

	{
		CAutoLock lck(&m_pool_lock);
		if (m_pool) m_pool->DestroyPool(D3DPOOL_DEFAULT);
	}

	// surfaces
	m_deinterlace_surface = NULL;
	m_stereo_test_gpu = NULL;
	m_PC_level_test = NULL;

	// swap chains
	delete_render_targets();

	return S_OK;
}

// test_PC_leve()'s helper function
HRESULT mark_level_result(IDirect3DSurface9 *result, DWORD *out, DWORD flag)
{
	HRESULT hr;
	D3DLOCKED_RECT lock_rect;
	FAIL_RET(result->LockRect(&lock_rect, NULL, NULL));
	BYTE *p = (BYTE*)lock_rect.pBits;

	for(int i=0; i<stereo_test_texture_size; i++)
	{
		for (int j=0; j<stereo_test_texture_size; j++)
			if (p[j*4] || p[j*4+1] || p[j*4+2])
			{
				*out &= ~flag;
				result->UnlockRect();
				return S_FALSE;
			}

		p += lock_rect.Pitch;
	}

	*out |= flag;
	result->UnlockRect();
	return S_OK;
}

HRESULT my12doomRenderer::test_PC_level()
{
	return S_OK;
	HRESULT hr;

	CAutoLock lck(&m_frame_lock);

	// YV12
	m_PC_level_test = NULL;
	FAIL_RET(m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &m_PC_level_test, NULL));
	D3DLOCKED_RECT lock_rect;
	m_PC_level_test->LockRect(&lock_rect, NULL, NULL);
	if (lock_rect.pBits)
	{
		memset(lock_rect.pBits, 16, stereo_test_texture_size * lock_rect.Pitch);
		memset((BYTE*)lock_rect.pBits +  stereo_test_texture_size * lock_rect.Pitch, 128, stereo_test_texture_size * lock_rect.Pitch / 2);
	}
	hr = m_PC_level_test->UnlockRect();
	hr = clear(m_stereo_test_gpu, D3DCOLOR_XRGB(255,255,255));
	hr = m_Device->StretchRect(m_PC_level_test, NULL, m_stereo_test_gpu, NULL, D3DTEXF_NONE);
	hr = m_Device->GetRenderTargetData(m_stereo_test_gpu, m_stereo_test_cpu);
	mark_level_result(m_stereo_test_cpu, &m_PC_level, PCLEVELTEST_YV12);


	// NV12
	m_PC_level_test = NULL;
	FAIL_RET(m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, (D3DFORMAT)MAKEFOURCC('N','V','1','2'), D3DPOOL_DEFAULT, &m_PC_level_test, NULL));
	m_PC_level_test->LockRect(&lock_rect, NULL, NULL);
	if (lock_rect.pBits)
	{
		memset(lock_rect.pBits, 16, stereo_test_texture_size * lock_rect.Pitch);
		memset((BYTE*)lock_rect.pBits +  stereo_test_texture_size * lock_rect.Pitch, 128, stereo_test_texture_size * lock_rect.Pitch / 2);
	}
	hr = m_PC_level_test->UnlockRect();
	hr = clear(m_stereo_test_gpu, D3DCOLOR_XRGB(255,255,255));
	hr = m_Device->StretchRect(m_PC_level_test, NULL, m_stereo_test_gpu, NULL, D3DTEXF_NONE);
	hr = m_Device->GetRenderTargetData(m_stereo_test_gpu, m_stereo_test_cpu);
	mark_level_result(m_stereo_test_cpu, &m_PC_level, PCLEVELTEST_NV12);


	// YUY2
	unsigned char one_line[stereo_test_texture_size*2];
	for(int i=0; i<stereo_test_texture_size; i++)
	{
		one_line[i*2] = 16;
		one_line[i*2+1] = 128;
	}
	m_PC_level_test = NULL;
	FAIL_RET(m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, (D3DFORMAT)MAKEFOURCC('Y','U','Y','2'), D3DPOOL_DEFAULT, &m_PC_level_test, NULL));
	m_PC_level_test->LockRect(&lock_rect, NULL, NULL);
	if (lock_rect.pBits)
	{
		for(int i=0; i<stereo_test_texture_size; i++)
			memcpy((BYTE*)lock_rect.pBits + lock_rect.Pitch * i, one_line, sizeof(one_line));
	}
	hr = m_PC_level_test->UnlockRect();
	hr = clear(m_stereo_test_gpu, D3DCOLOR_XRGB(255,255,255));
	hr = m_Device->StretchRect(m_PC_level_test, NULL, m_stereo_test_gpu, NULL, D3DTEXF_NONE);
	hr = m_Device->GetRenderTargetData(m_stereo_test_gpu, m_stereo_test_cpu);
	mark_level_result(m_stereo_test_cpu, &m_PC_level, PCLEVELTEST_YUY2);


	m_PC_level |= PCLEVELTEST_TESTED;
	m_PC_level_test = NULL;
	m_PC_level = PCLEVELTEST_TESTED;

	return S_OK;
}

HRESULT my12doomRenderer::restore_gpu_objects()
{
	HRESULT hr;

	m_red_blue.set_source(m_Device, g_code_anaglyph, sizeof(g_code_anaglyph), true, (DWORD*)m_key);
	m_ps_masking.set_source(m_Device, g_code_masking, sizeof(g_code_masking), true, (DWORD*)m_key);
	m_lanczosX.set_source(m_Device, g_code_lanczosX, sizeof(g_code_lanczosX), true, (DWORD*)m_key);
	m_lanczosX_NV12.set_source(m_Device, g_code_lanczosX_NV12, sizeof(g_code_lanczosX_NV12), true, (DWORD*)m_key);
	m_lanczosX_YV12.set_source(m_Device, g_code_lanczosX_YV12, sizeof(g_code_lanczosX_YV12), true, (DWORD*)m_key);
	m_lanczosY.set_source(m_Device, g_code_lanczosY, sizeof(g_code_lanczosY), true, (DWORD*)m_key);
	m_lanczos.set_source(m_Device, g_code_lanczos, sizeof(g_code_lanczos), true, (DWORD*)m_key);
	m_lanczos_NV12.set_source(m_Device, g_code_lanczos_NV12, sizeof(g_code_lanczos_NV12), true, (DWORD*)m_key);
	m_lanczos_YV12.set_source(m_Device, g_code_lanczos_YV12, sizeof(g_code_lanczos_YV12), true, (DWORD*)m_key);

	int l = timeGetTime();
	m_pass1_width = m_lVidWidth;
	m_pass1_height = m_lVidHeight;
	if (!(m_dsr0->is_connected() && m_dsr1->is_connected()) && !m_remux_mode)
	{
		input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
		if (input == side_by_side)
			m_pass1_width /= 2;
		else if (input == top_bottom)
			m_pass1_height /= 2;
	}

	FAIL_RET( m_Device->CreateRenderTarget(stereo_test_texture_size, stereo_test_texture_size, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_stereo_test_gpu, NULL));
	FAIL_RET(HD3D_restore_objects());
	fix_nv3d_bug();
	if (m_stereo_test_cpu == NULL) FAIL_RET( m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_stereo_test_cpu, NULL));

	DWORD use_mipmap = D3DUSAGE_AUTOGENMIPMAP;

	// query
	//m_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &m_d3d_query);

	// textures
	FAIL_RET(m_Device->CreateRenderTarget(m_pass1_width, m_pass1_height/2, m_active_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &m_deinterlace_surface, NULL));
	FAIL_RET( m_Device->CreateTexture(BIG_TEXTURE_SIZE, BIG_TEXTURE_SIZE, 0, D3DUSAGE_RENDERTARGET | use_mipmap, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,	&m_tex_bmp, NULL));
	if(m_tex_bmp_mem == NULL)
	{
		// only first time, so we don't need lock CritSec
		FAIL_RET( m_Device->CreateOffscreenPlainSurface(BIG_TEXTURE_SIZE, BIG_TEXTURE_SIZE, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,	&m_tex_bmp_mem, NULL));
		m_tex_bmp_mem->LockRect(&m_bmp_locked_rect, NULL, NULL);
		m_bmp_changed = false;
	}

	FAIL_RET(create_render_targets());

	// pixel shader
	unsigned char *yv12 = (unsigned char *)malloc(sizeof(g_code_YV12toRGB));
	unsigned char *nv12 = (unsigned char *)malloc(sizeof(g_code_NV12toRGB));
	unsigned char *yuy2 = (unsigned char *)malloc(sizeof(g_code_YUY2toRGB));
	//unsigned char *tester = (unsigned char *)malloc(sizeof(g_code_main));
	unsigned char *anaglyph = (unsigned char *)malloc(sizeof(g_code_anaglyph));

	memcpy(yv12, g_code_YV12toRGB, sizeof(g_code_YV12toRGB));
	memcpy(nv12, g_code_NV12toRGB, sizeof(g_code_NV12toRGB));
	memcpy(yuy2, g_code_YUY2toRGB, sizeof(g_code_YUY2toRGB));
	//memcpy(tester, g_code_main, sizeof(g_code_main));
	memcpy(anaglyph, g_code_anaglyph, sizeof(g_code_anaglyph));

	//int size = sizeof(g_code_main);
	for(int i=0; i<16384; i+=16)
	{
		if (i<sizeof(g_code_YV12toRGB)/16*16)
			m_AES.decrypt(yv12+i, yv12+i);
		if (i<sizeof(g_code_NV12toRGB)/16*16)
			m_AES.decrypt(nv12+i, nv12+i);
		if (i<sizeof(g_code_YUY2toRGB)/16*16)
			m_AES.decrypt(yuy2+i, yuy2+i);
		//if (i<sizeof(g_code_main)/16*16)
		//	m_AES.decrypt(tester+i, tester+i);
		if (i<sizeof(g_code_anaglyph)/16*16)
			m_AES.decrypt(anaglyph+i, anaglyph+i);
	}

	int r = memcmp(yv12, g_code_YV12toRGB, sizeof(g_code_YV12toRGB));

	// shaders
	m_Device->CreatePixelShader((DWORD*)yv12, &m_ps_yv12);
	m_Device->CreatePixelShader((DWORD*)nv12, &m_ps_nv12);
	m_Device->CreatePixelShader((DWORD*)yuy2, &m_ps_yuy2);
	//m_Device->CreatePixelShader((DWORD*)tester, &m_ps_test);
	m_Device->CreatePixelShader((DWORD*)g_code_sbs, &m_ps_test_sbs);
	m_Device->CreatePixelShader((DWORD*)g_code_sbs2, &m_ps_test_sbs2);
	m_Device->CreatePixelShader((DWORD*)g_code_tb, &m_ps_test_tb);
	m_Device->CreatePixelShader((DWORD*)g_code_tb2, &m_ps_test_tb2);
	m_Device->CreatePixelShader((DWORD*)anaglyph, &m_ps_anaglyph);
	m_Device->CreatePixelShader((DWORD*)g_code_iz3d_back, &m_ps_iz3d_back);
	m_Device->CreatePixelShader((DWORD*)g_code_iz3d_front, &m_ps_iz3d_front);
	m_Device->CreatePixelShader((DWORD*)g_code_color_adjust, &m_ps_color_adjust);
	m_Device->CreatePixelShader((DWORD*)g_code_lanczos, &m_ps_bmp_lanczos);
	//m_Device->CreatePixelShader((DWORD*)g_code_bmp_blur, &m_ps_bmp_blur);
	if (m_ps_bmp_blur == NULL)
		m_Device->CreatePixelShader((DWORD*)g_code_bmp_blur2, &m_ps_bmp_blur);

	// for pixel shader 2.0 cards
	if (m_ps_test_tb2 != NULL && m_ps_test_tb == NULL)
		m_ps_test_tb = m_ps_test_tb2;
	if (m_ps_test_sbs2 != NULL && m_ps_test_sbs == NULL)
		m_ps_test_sbs = m_ps_test_sbs2;


	free(yv12);
	free(nv12);
	free(yuy2);
	//free(tester);
	free(anaglyph);

	// thread
	create_render_thread();

	// restore image if possible
	reload_image();

	return S_OK;
}


HRESULT my12doomRenderer::reload_image()
{
	CAutoLock lck(&m_packet_lock);
	CAutoLock rendered_lock(&m_rendered_packet_lock);

	if (!m_sample2render_1)
	{
		safe_delete(m_sample2render_1);
		safe_delete(m_sample2render_2);
		m_sample2render_1 = m_last_rendered_sample1;
		m_sample2render_2 = m_last_rendered_sample2;

		safe_decommit(m_sample2render_1);
		safe_decommit(m_sample2render_2);

		m_last_rendered_sample1 = NULL;
		m_last_rendered_sample2 = NULL;
	}
	return S_OK;
}

HRESULT my12doomRenderer::render_nolock(bool forced)
{
	HRESULT hr;
	// device state check and restore
	if (FAILED(handle_device_state()))
		return E_FAIL;

	//if (m_d3d_query) m_d3d_query->Issue(D3DISSUE_BEGIN);

	if (!(m_PC_level&PCLEVELTEST_TESTED))
		test_PC_level();

	// image loading and idle check
	m_Device->BeginScene();
	hr = load_image();
	m_Device->EndScene();

	if (FAILED(hr))
		return hr;

	if (hr != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!m_Device)
		return E_FAIL;

	static int last_render_time = timeGetTime();
	static int last_nv3d_fix_time = timeGetTime();

	CAutoLock lck(&m_frame_lock);

	// pass 2: drawing to back buffer!

	if (timeGetTime() - last_nv3d_fix_time > 1000)
	{
		fix_nv3d_bug();
		last_nv3d_fix_time = timeGetTime();
	}


	hr = m_Device->BeginScene();
	hr = m_Device->SetTexture(0, NULL);
	hr = m_Device->SetTexture(1, NULL);
	hr = m_Device->SetTexture(2, NULL);

	// prepare all samples in queue for rendering
	if(false)
	{
		bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();
		input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
		bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;

		CAutoLock lck(&m_queue_lock);
		for(POSITION pos_left = m_left_queue.GetHeadPosition(); pos_left; pos_left = m_left_queue.Next(pos_left))
		{
			gpu_sample *left_sample = m_left_queue.Get(pos_left);
// 			left_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
			if (need_detect) left_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
		}
		for(POSITION pos_right = m_right_queue.GetHeadPosition(); pos_right; pos_right = m_right_queue.Next(pos_right))
		{
			gpu_sample *right_sample = m_right_queue.Get(pos_right);
// 			right_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
			if (need_detect) right_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
		}
	}

#ifndef no_dual_projector
	if (timeGetTime() - m_last_reset_time > TRAIL_TIME_2 && is_trial_version())
		m_ps_yv12 = m_ps_nv12 = m_ps_yuy2 = NULL;
#endif

	int l = timeGetTime();

	// load subtitle
	CAutoPtr<CAutoLock> bmp_lock;
	bmp_lock.Attach(new CAutoLock(&m_bmp_lock));
	if (m_bmp_changed )
	{
		int l = timeGetTime();
		if (!m_tex_bmp || !m_tex_bmp_mem)
			return VFW_E_WRONG_STATE;

		m_tex_bmp_mem->UnlockRect();

		CComPtr<IDirect3DSurface9> dst;
		RECT src = {0,0,min(m_bmp_width+100, BIG_TEXTURE_SIZE), min(m_bmp_height+100, BIG_TEXTURE_SIZE)};

		FAIL_RET(m_tex_bmp->GetSurfaceLevel(0, &dst));

		m_Device->UpdateSurface(m_tex_bmp_mem, &src, dst, NULL);

		mylog("UpdateSurface = %d ms.\n", timeGetTime() - l);
	}
	else
	{
		bmp_lock.Free();
	}


	hr = m_Device->SetPixelShader(NULL);

	// set render target to back buffer
	CComPtr<IDirect3DSurface9> back_buffer;
	if (m_swap1) hr = m_swap1->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);

	clear(back_buffer, D3DCOLOR_ARGB(255, 0, 0, 0));

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

	hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

	hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

	// vertex
	MyVertex whole_backbuffer_vertex[4];
	whole_backbuffer_vertex[0].x = -0.5f; whole_backbuffer_vertex[0].y = -0.5f;
	whole_backbuffer_vertex[1].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[1].y = -0.5f;
	whole_backbuffer_vertex[2].x = -0.5f; whole_backbuffer_vertex[2].y = m_active_pp.BackBufferHeight-0.5f;
	whole_backbuffer_vertex[3].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[3].y = m_active_pp.BackBufferHeight-0.5f;
	for(int i=0; i<4; i++)
		whole_backbuffer_vertex[i].z = whole_backbuffer_vertex[i].w = 1;
	whole_backbuffer_vertex[0].tu = 0;
	whole_backbuffer_vertex[0].tv = 0;
	whole_backbuffer_vertex[1].tu = 1;
	whole_backbuffer_vertex[1].tv = 0;
	whole_backbuffer_vertex[2].tu = 0;
	whole_backbuffer_vertex[2].tv = 1;
	whole_backbuffer_vertex[3].tu = 1;
	whole_backbuffer_vertex[3].tv = 1;

	static NvU32 l_counter = 0;
	if (m_output_mode == intel3d)
	{
		CComPtr<IDirect3DSurface9> left_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &left_surface);
		clear(left_surface);
		draw_movie(left_surface, true);
		draw_bmp(left_surface, true);
		adjust_temp_color(left_surface, true);
		draw_ui(left_surface);

		CComPtr<IDirect3DSurface9> right_surface;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &right_surface);
		clear(right_surface);
		draw_movie(right_surface, false);
		draw_bmp(right_surface, false);
		adjust_temp_color(right_surface, false);
		draw_ui(right_surface);

		intel_render_frame(left_surface, right_surface);
	}
	else if (m_output_mode == NV3D
#ifdef explicit_nv3d
		&& m_nv3d_enabled && m_nv3d_actived /*&& !m_active_pp.Windowed*/
#endif
		)
	{
		//m_nv_version.drvVersion = 0;
		if (m_nv_version.drvVersion>28500)
		{
			NvAPI_Stereo_SetActiveEye(m_nv3d_handle, NVAPI_STEREO_EYE_LEFT);
		}

		CComPtr<IDirect3DSurface9> left_surface;
		CComPtr<IDirect3DSurface9> right_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &left_surface);
		hr = m_mask_temp_right->GetSurfaceLevel(0, &right_surface);

		clear(left_surface);
		draw_movie(left_surface, true);
		draw_bmp(left_surface, true);
		adjust_temp_color(left_surface, true);
		draw_ui(left_surface);

		// copy left to nv3d surface
		RECT dst = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
		if (m_nv_version.drvVersion>28500)
		{
			hr = m_Device->StretchRect(left_surface, NULL, back_buffer, NULL, D3DTEXF_NONE);

		}
		else
		{
			hr = m_Device->StretchRect(left_surface, NULL, m_nv3d_surface, &dst, D3DTEXF_NONE);
		}

		clear(right_surface);
		draw_movie(right_surface, false);
		draw_bmp(right_surface, false);
		adjust_temp_color(right_surface, false);
		draw_ui(right_surface);

		// copy right to nv3d surface
		if (m_nv_version.drvVersion>28500)
		{
			//NvAPI_Stereo_SetActiveEye(m_nv3d_handle, NVAPI_STEREO_EYE_RIGHT);
			hr = m_Device->StretchRect(right_surface, NULL, back_buffer, NULL, D3DTEXF_NONE);
			NvAPI_Stereo_SetActiveEye(m_nv3d_handle, NVAPI_STEREO_EYE_LEFT);

		}
		else
		{
			dst.left += m_active_pp.BackBufferWidth;
			dst.right += m_active_pp.BackBufferWidth;
			hr = m_Device->StretchRect(right_surface, NULL, m_nv3d_surface, &dst, D3DTEXF_NONE);

			// StretchRect to backbuffer!, this is how 3D vision works
			RECT tar = {0,0, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight};
			hr = m_Device->StretchRect(m_nv3d_surface, &tar, back_buffer, NULL, D3DTEXF_NONE);		//source is as previous, tag line not overwrited
		}
	}

	else if (m_output_mode == mono 
#ifdef explicit_nv3d
		|| (m_output_mode == NV3D && !(m_nv3d_enabled && m_nv3d_actived))
#endif
		)
	{
		clear(back_buffer);
		draw_movie(back_buffer, true);
		draw_bmp(back_buffer, true);
		adjust_temp_color(back_buffer, true);
		draw_ui(back_buffer);
	}
	else if (m_output_mode == hd3d)
	{
		CComPtr<IDirect3DSurface9> left_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &left_surface);
		clear(left_surface);
		draw_movie(left_surface, true);
		draw_bmp(left_surface, true);
		adjust_temp_color(left_surface, true);
		draw_ui(left_surface);

		CComPtr<IDirect3DSurface9> right_surface;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &right_surface);
		clear(right_surface);
		draw_movie(right_surface, false);
		draw_bmp(right_surface, false);
		adjust_temp_color(right_surface, false);
		draw_ui(right_surface);

		HD3DDrawStereo(left_surface, right_surface, back_buffer);
	}
	else if (m_output_mode == anaglyph)
	{
		CComPtr<IDirect3DSurface9> temp_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, true);
		draw_bmp(temp_surface, true);
		adjust_temp_color(temp_surface, true);

		temp_surface = NULL;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, false);
		draw_bmp(temp_surface, false);
		adjust_temp_color(temp_surface, false);

		// pass3: analyph
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_Device->SetRenderTarget(0, back_buffer);
		m_Device->SetTexture( 0, m_mask_temp_left );
		m_Device->SetTexture( 1, m_mask_temp_right );
		if(get_active_input_layout() != mono2d 
			|| m_convert3d
			|| (m_dsr0->is_connected() && m_dsr1->is_connected())
			|| (!m_dsr0->is_connected() && !m_dsr1->is_connected())
			|| m_remux_mode)
			m_Device->SetPixelShader(m_red_blue);

		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );

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
		adjust_temp_color(temp_surface, true);

		temp_surface = NULL;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, false);
		draw_bmp(temp_surface, false);
		adjust_temp_color(temp_surface, false);

		// pass3: mask!
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_Device->SetRenderTarget(0, back_buffer);
		m_Device->SetPixelShader(m_ps_masking);
		m_Device->SetTexture( 0, m_tex_mask );
		m_Device->SetTexture( 1, m_mask_temp_left );
		m_Device->SetTexture( 2, m_mask_temp_right );

		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );

		// UI
		m_Device->SetPixelShader(NULL);
		draw_ui(back_buffer);

	}

	else if (m_output_mode == pageflipping)
	{
		double delta = (double)timeGetTime()-m_pageflipping_start;
		int frame_passed = 1;
		if (delta/(1500/m_d3ddm.RefreshRate) > 1)
			frame_passed = max(1, floor(delta/(1000/m_d3ddm.RefreshRate)));
		m_pageflip_frames += frame_passed;
		m_pageflip_frames %= 2;

		if (frame_passed>1 || frame_passed <= 0)
		{
			if (m_nv3d_display && frame_passed > 2)
			{
				DWORD counter;
				NvAPI_GetVBlankCounter(m_nv3d_display, &counter);
				m_pageflip_frames = counter - m_nv_pageflip_counter;
			}
			mylog("delta=%d.\n", (int)delta);
		}

		LARGE_INTEGER l1, l2, l3, l4, l5;
		QueryPerformanceCounter(&l1);
		clear(back_buffer);
		QueryPerformanceCounter(&l2);
		draw_movie(back_buffer, m_pageflip_frames);
		QueryPerformanceCounter(&l3);
		draw_bmp(back_buffer, m_pageflip_frames);
		adjust_temp_color(back_buffer, m_pageflip_frames);
		QueryPerformanceCounter(&l4);
		draw_ui(back_buffer);
		QueryPerformanceCounter(&l5);


		//mylog("clear, draw_movie, draw_bmp, draw_ui = %d, %d, %d, %d.\n", (int)(l2.QuadPart-l1.QuadPart), 
		//	(int)(l3.QuadPart-l2.QuadPart), (int)(l4.QuadPart-l3.QuadPart), (int)(l5.QuadPart-l4.QuadPart) );
	}

#ifndef no_dual_projector	
	else if (m_output_mode == iz3d)
	{
		CComPtr<IDirect3DSurface9> temp_surface;
		hr = m_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, true);
		draw_bmp(temp_surface, true);
		adjust_temp_color(temp_surface, true);

		temp_surface = NULL;
		hr = m_mask_temp_right->GetSurfaceLevel(0, &temp_surface);
		clear(temp_surface);
		draw_movie(temp_surface, false);
		draw_bmp(temp_surface, false);
		adjust_temp_color(temp_surface, false);

		// pass3: IZ3D
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		m_Device->SetRenderTarget(0, back_buffer);
		m_Device->SetTexture( 0, m_mask_temp_left );
		m_Device->SetTexture( 1, m_mask_temp_right );
		m_Device->SetPixelShader(m_ps_iz3d_back);

		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );

		// set render target to swap chain2
		if (m_swap2)
		{
			CComPtr<IDirect3DSurface9> back_buffer2;
			m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);

			clear(back_buffer2);

			hr = m_Device->SetRenderTarget(0, back_buffer2);
			m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			m_Device->SetPixelShader(m_ps_iz3d_front);
			hr = m_Device->SetFVF( FVF_Flags );
			hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );
		}

		// UI
		m_Device->SetPixelShader(NULL);
		draw_ui(back_buffer);
	}
	else if (m_output_mode == dual_window)
	{
		clear(back_buffer);
		draw_movie(back_buffer, true);
		draw_bmp(back_buffer, true);
		adjust_temp_color(back_buffer, true);
		draw_ui(back_buffer);

		// set render target to swap chain2
		if (m_swap2)
		{
			CComPtr<IDirect3DSurface9> back_buffer2;
			m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);
			hr = m_Device->SetRenderTarget(0, back_buffer2);

			clear(back_buffer2);
			draw_movie(back_buffer2, false);
			draw_bmp(back_buffer2, false);
			adjust_temp_color(back_buffer2, false);
			draw_ui(back_buffer2);
		}

	}

#endif
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
		adjust_temp_color(temp_surface, true);
		adjust_temp_color(temp_surface2, false);
		draw_ui(temp_surface);
		draw_ui(temp_surface2);


		// pass 3: copy to backbuffer
		if(false)
		{

		}
#ifndef no_dual_projector
		else if (m_output_mode == out_sbs)
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
#endif

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
	if (timeGetTime() - l > 5)
		printf("All Draw Calls = %dms\n", timeGetTime() - l);
presant:
	static int n = timeGetTime();
	if (timeGetTime() - n > 43)
		printf("(%d):presant delta: %dms\n", timeGetTime(), timeGetTime()-n);
	n = timeGetTime();


	// presenting...
	if (m_output_mode == intel3d && m_overlay_swap_chain)
	{
		RECT rect= {0, 0, min(m_intel_active_3d_mode.ulResWidth,m_active_pp.BackBufferWidth), 
			min(m_active_pp.BackBufferHeight, m_active_pp.BackBufferHeight)};
		hr = m_overlay_swap_chain->Present(&rect, &rect, NULL, NULL, D3DPRESENT_UPDATECOLORKEY);

		//mylog("%08x\n", hr);
	}



	else if (m_output_mode == dual_window || m_output_mode == iz3d)
	{
		if(m_swap1) hr = m_swap1->Present(NULL, NULL, m_hWnd, NULL, D3DPRESENT_DONOTWAIT);
		if (FAILED(hr) && hr != DDERR_WASSTILLDRAWING)
			set_device_state(device_lost);

		if(m_swap2) if (m_swap2->Present(NULL, NULL, m_hWnd2, NULL, NULL) == D3DERR_DEVICELOST)
			set_device_state(device_lost);
	}

	else
	{
		if (m_output_mode == pageflipping)
		{
			if (m_pageflipping_start == -1 && m_nv3d_display)
				NvAPI_GetVBlankCounter(m_nv3d_display, &m_nv_pageflip_counter);
			m_pageflipping_start = timeGetTime();
		}

		int l2 = timeGetTime();
		hr = DDERR_WASSTILLDRAWING;
		while (hr == DDERR_WASSTILLDRAWING)
			hr = m_swap1->Present(NULL, NULL, m_hWnd, NULL, NULL);
		if (timeGetTime()-l2 > 9) printf("Presant() cost %dms.\n", timeGetTime() - l2);
		if (FAILED(hr))
			set_device_state(device_lost);

		static int n = timeGetTime();
		//if (timeGetTime()-n > 0)printf("delta = %d.\n", timeGetTime()-n);
		n = timeGetTime();
	}

	// debug LockRect times
	//if (lockrect_surface + lockrect_texture)
	//	printf("LockRect: surface, texture, total, cycle = %d, %d, %d, %d.\n", lockrect_surface, lockrect_texture, lockrect_surface+lockrect_texture, (int)lockrect_texture_cycle);
	lockrect_texture_cycle = lockrect_surface = lockrect_texture = 0;

	if (bmp_lock != NULL)
	{
		int l = timeGetTime();
		m_tex_bmp_mem->LockRect(&m_bmp_locked_rect, NULL, NULL);
		mylog("LockRect for subtitle cost %dms \n", timeGetTime()-l);

		lockrect_surface ++;

		m_bmp_changed = false;
	}



	return S_OK;
}

HRESULT my12doomRenderer::clear(IDirect3DSurface9 *surface, DWORD color)
{
	if (!surface)
		return E_POINTER;

	m_Device->SetRenderTarget(0, surface);
	return m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L );
}

HRESULT my12doomRenderer::draw_movie(IDirect3DSurface9 *surface, bool left_eye)
{
	left_eye = left_eye ^ m_swapeyes;

	HRESULT hr;
	if (!surface)
		return E_POINTER;

	if (!m_dsr0->is_connected() && m_uidrawer)
	{
		m_last_reset_time = timeGetTime();
		return m_uidrawer->draw_nonmovie_bg(surface, left_eye);
	}

	CComPtr<IDirect3DSurface9> src;

	bool dual_stream = (m_dsr0->is_connected() && m_dsr1->is_connected()) || m_remux_mode;
	CAutoLock rendered_lock(&m_rendered_packet_lock);
	gpu_sample *sample = dual_stream ? (left_eye ? m_last_rendered_sample1 : m_last_rendered_sample2) : m_last_rendered_sample1;

	if (!sample)
		return S_FALSE;

	FAIL_RET(sample->commit());

	if (sample->m_tex_gpu_RGB32)
		sample->m_tex_gpu_RGB32->get_first_level(&src);

	if (!src)
		return S_FALSE;


	// movie picture position
	RECT target = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	calculate_movie_position(&target);

	// source rect calculation
	RECT src_rect = {0,0,m_lVidWidth, m_lVidHeight};
	if (!dual_stream)
	{
		input_layout_types layout = get_active_input_layout();

		switch(layout)
		{
		case side_by_side:
			if (left_eye)
				src_rect.right/=2;
			else
				src_rect.left = m_lVidWidth/2;
			break;
		case top_bottom:
			if (left_eye)
				src_rect.bottom/=2;
			else
				src_rect.top = m_lVidHeight/2;
			break;
		case mono2d:
			break;
		default:
			assert(0);
		}
	}

	// parallax adjustments
	if (m_parallax > 0)
	{
		// cut right edge of right eye and left edge of left eye
		if (left_eye)
			src_rect.left += abs(m_parallax) * m_lVidWidth;
		else
			src_rect.right -= abs(m_parallax) * m_lVidWidth;

	}
	else if (m_parallax < 0)
	{
		// cut left edge of right eye and right edge of left eye
		if (left_eye)
			src_rect.right -= abs(m_parallax) * m_lVidWidth;
		else
			src_rect.left += abs(m_parallax) * m_lVidWidth;
	}


	// render
	m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	return resize_surface(NULL, sample, surface, &src_rect, &target, (resampling_method)(int)MovieResizing);
}

HRESULT my12doomRenderer::draw_bmp(IDirect3DSurface9 *surface, bool left_eye)
{
	if (m_display_orientation != horizontal)
		return E_NOTIMPL;

	if (!surface)
		return E_POINTER;

	if (!m_has_subtitle)
		return S_FALSE;

	// assume draw_movie() handles pointer and not connected issues, so no more check
	HRESULT hr = E_FAIL;

	// movie picture position
	RECTF src_rect = {0,0,m_bmp_width, m_bmp_height};
	RECTF dst_rect = {0};
	calculate_subtitle_position(&dst_rect, left_eye);

	CComPtr<IDirect3DSurface9> src;
	m_tex_bmp->GetSurfaceLevel(0, &src);

	m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	return resize_surface(src, NULL, surface, &src_rect, &dst_rect, (resampling_method)(int)SubtitleResizing);
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

HRESULT my12doomRenderer::adjust_temp_color(IDirect3DSurface9 *surface_to_adjust, bool left)
{
	if (!surface_to_adjust)
		return E_FAIL;

	// check for size
	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	surface_to_adjust->GetDesc(&desc);

	// create a temp texture, copy the surface to it, render to it, and then copy back again
	HRESULT hr = S_OK;
	double saturation1 = m_saturation1;
	double saturation2 = m_saturation2;

#ifndef no_dual_projector
	if (timeGetTime() - m_last_reset_time > TRAIL_TIME_1 && is_trial_version())
	{
		saturation1 -= (double)(timeGetTime() - m_last_reset_time - TRAIL_TIME_1) / TRAIL_TIME_3;
		saturation2 -= (double)(timeGetTime() - m_last_reset_time - TRAIL_TIME_1) / TRAIL_TIME_3;

		saturation1 = max(saturation1, -1);
		saturation2 = max(saturation2, -2);
	}
#endif
	if ( (left && (abs(saturation1-0.5)>0.005 || abs(m_luminance1-0.5)>0.005 || abs(m_hue1-0.5)>0.005 || abs(m_contrast1-0.5)>0.005)) || 
		(!left && (abs(saturation2-0.5)>0.005 || abs(m_luminance2-0.5)>0.005 || abs(m_hue2-0.5)>0.005 || abs(m_contrast2-0.5)>0.005)))
	{
		// creating
		CAutoLock lck(&m_pool_lock);
		CPooledTexture *tex_src;
		CPooledTexture *tex_rt;
		hr = m_pool->CreateTexture(desc.Width, desc.Height, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_src);
		if (FAILED(hr))
			return hr;
		hr = m_pool->CreateTexture(desc.Width, desc.Height, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_rt);
		if (FAILED(hr))
			return hr;
		CComPtr<IDirect3DSurface9> surface_of_tex_src;
		tex_src->texture->GetSurfaceLevel(0, &surface_of_tex_src);
		CComPtr<IDirect3DSurface9> surface_of_tex_rt;
		tex_rt->texture->GetSurfaceLevel(0, &surface_of_tex_rt);

		// copying
		hr = m_Device->StretchRect(surface_to_adjust, NULL, surface_of_tex_src, NULL, D3DTEXF_LINEAR);		//we are using linear filter here, to cover possible coordinate error		

		// vertex
		MyVertex whole_backbuffer_vertex[4];
		whole_backbuffer_vertex[0].x = -0.5f; whole_backbuffer_vertex[0].y = -0.5f;
		whole_backbuffer_vertex[1].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[1].y = -0.5f;
		whole_backbuffer_vertex[2].x = -0.5f; whole_backbuffer_vertex[2].y = m_active_pp.BackBufferHeight-0.5f;
		whole_backbuffer_vertex[3].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[3].y = m_active_pp.BackBufferHeight-0.5f;

		// rendering
		CComPtr<IDirect3DPixelShader9> ps;
		hr = m_Device->GetPixelShader(&ps);
		hr = m_Device->SetRenderTarget(0, surface_of_tex_rt);
		hr = m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		hr = m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		hr = m_Device->SetTexture( 0, tex_src->texture );
		hr = m_Device->SetPixelShader(m_ps_color_adjust);
		float ps_parameter[4] = {saturation1, m_luminance1, m_hue1, m_contrast1};
		float ps_parameter2[4] = {saturation2, m_luminance2, m_hue2, m_contrast2};
		hr = m_Device->SetPixelShaderConstantF(0, left?ps_parameter:ps_parameter2, 1);

		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );
		hr = m_Device->SetPixelShader(ps);

		// copying back
		hr = m_Device->StretchRect(surface_of_tex_rt, NULL, surface_to_adjust, NULL, D3DTEXF_LINEAR);

		m_pool->DeleteTexture(tex_src);
		m_pool->DeleteTexture(tex_rt);
	}

	return hr;
}

HRESULT my12doomRenderer::resize_surface(IDirect3DSurface9 *src, gpu_sample *src2, IDirect3DSurface9 *dst, RECT *src_rect /* = NULL */, RECT *dst_rect /* = NULL */, resampling_method method /* = bilinear_mipmap_minus_one */)
{
	if ((!src && !src2) || !dst)
		return E_POINTER;

	// RECT caculate;
	D3DSURFACE_DESC desc;
	dst->GetDesc(&desc);
	RECTF d = {0,0,(float)desc.Width, (float)desc.Height};
	if (dst_rect)
	{
		d.left = (float)dst_rect->left;
		d.right = (float)dst_rect->right;
		d.top = (float)dst_rect->top;
		d.bottom = (float)dst_rect->bottom;
	}

	RECTF s = {0};
	if (src_rect)
	{
		s.left = (float)src_rect->left;
		s.right = (float)src_rect->right;
		s.top = (float)src_rect->top;
		s.bottom = (float)src_rect->bottom;
	}
	else if (src)
	{
		src->GetDesc(&desc);
		s.right = desc.Width;
		s.bottom = desc.Height;
	}
	else if (src2)
	{
		s.right = src2->m_width;
		s.bottom = src2->m_height;
	}

	return resize_surface(src, src2, dst, &s, &d, method);
}


HRESULT my12doomRenderer::resize_surface(IDirect3DSurface9 *src, gpu_sample *src2, IDirect3DSurface9 *dst, RECTF *src_rect /* = NULL */, RECTF *dst_rect /* = NULL */, resampling_method method /* = bilinear_mipmap_minus_one */)
{
	if ((!src && !src2) || !dst)
		return E_POINTER;

	CComPtr<IDirect3DTexture9> tex;
	if (src)
	{
		src->GetContainer(IID_IDirect3DTexture9, (void**)&tex);
		if (!tex)
			return E_INVALIDARG;
	}

	// RECT caculate;
	D3DSURFACE_DESC desc;
	dst->GetDesc(&desc);
	RECTF d = {0,0,(float)desc.Width, (float)desc.Height};
	if (dst_rect)
		d = *dst_rect;

	RECTF s = {0};
	if (src_rect)
	{
		s.left = (float)src_rect->left;
		s.right = (float)src_rect->right;
		s.top = (float)src_rect->top;
		s.bottom = (float)src_rect->bottom;
	}
	else if (src)
	{
		src->GetDesc(&desc);
		s.right = desc.Width;
		s.bottom = desc.Height;
	}
	else if (src2)
	{
		s.right = src2->m_width;
		s.bottom = src2->m_height;
	}

	if (src)
	{
		src->GetDesc(&desc);
	}

	else if (src2)
	{
		desc.Width = src2->m_width;
		desc.Height = src2->m_height;
	}

	// basic vertex calculation
	HRESULT hr = E_FAIL;
	float width_s = s.right - s.left;
	float height_s = s.bottom - s.top;
	float width_d = d.right - d.left;
	float height_d = d.bottom - d.top;

	MyVertex direct_vertex[4];
	direct_vertex[0].x = (float)d.left;
	direct_vertex[0].y = (float)d.top;
	direct_vertex[1].x = (float)d.right;
	direct_vertex[1].y = (float)d.top;
	direct_vertex[2].x = (float)d.left;
	direct_vertex[2].y = (float)d.bottom;
	direct_vertex[3].x = (float)d.right;
	direct_vertex[3].y = (float)d.bottom;
	for(int i=0;i <4; i++)
	{
		direct_vertex[i].x -= 0.5f;
		direct_vertex[i].y -= 0.5f;
		direct_vertex[i].z = 1.0f;
		direct_vertex[i].w = 1.0f;
	}

	direct_vertex[0].tu = (float)s.left / desc.Width;
	direct_vertex[0].tv = (float)s.top / desc.Height;
	direct_vertex[1].tu = (float)s.right / desc.Width;
	direct_vertex[1].tv = (float)s.top / desc.Height;
	direct_vertex[2].tu = (float)s.left / desc.Width;
	direct_vertex[2].tv = (float)s.bottom / desc.Height;
	direct_vertex[3].tu = (float)s.right / desc.Width;
	direct_vertex[3].tv = (float)s.bottom / desc.Height;

	// render state
	BOOL alpha_blend = TRUE;
	m_Device->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*)&alpha_blend);
	m_Device->SetPixelShader(NULL);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, alpha_blend );

	// texture and shader
	hr = m_Device->SetTexture( 0, tex );
	IDirect3DPixelShader9 *shader_yuv = NULL;
	GUID format = MEDIASUBTYPE_None;
	if (src2)
	{
		format = src2->m_format;
		if (format == MEDIASUBTYPE_NV12)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_nv12));
			shader_yuv = m_ps_nv12;
		}

		if (format == MEDIASUBTYPE_YUY2)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_yuy2));
			shader_yuv = m_ps_nv12;
		}

		if (format == MEDIASUBTYPE_YV12)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_yv12));
			shader_yuv = m_ps_yv12;
		}

		if (format == MEDIASUBTYPE_RGB32 || format == MEDIASUBTYPE_ARGB32)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_rgb32));
			m_Device->SetTexture(1, NULL);
			shader_yuv = NULL;
		}
	}
	hr = m_Device->SetPixelShader(shader_yuv);

	if (method == lanczos)
	{
		CAutoLock lck(&m_pool_lock);
		CPooledTexture *tmp1 = NULL;
		hr = m_pool->CreateTexture(BIG_TEXTURE_SIZE, BIG_TEXTURE_SIZE, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tmp1);
		if (FAILED(hr))
		{
			safe_delete(tmp1);
			return hr;
		}

		// pass1, X filter
		CComPtr<IDirect3DSurface9> rt1;
		tmp1->get_first_level(&rt1);
		m_Device->SetRenderTarget(0, rt1);
		clear(rt1, D3DCOLOR_ARGB(0,0,0,0));
		MyVertex vertex[4];	
		vertex[0].x = (float)0;
		vertex[0].y = (float)0;
		vertex[1].x = (float)width_d;
		vertex[1].y = (float)0;
		vertex[2].x = (float)0;
		vertex[2].y = (float)height_s;
		vertex[3].x = (float)width_d;
		vertex[3].y = (float)height_s;
		for(int i=0;i <4; i++)
		{
 			vertex[i].x -= 0.5f;
  			vertex[i].y -= 0.5f;
			vertex[i].z = 1.0f;
			vertex[i].w = 1.0f;
		}

		vertex[0].tu = (float)s.left / desc.Width;
		vertex[0].tv = (float)s.top / desc.Height;
		vertex[1].tu = (float)s.right / desc.Width;
		vertex[1].tv = (float)s.top / desc.Height;
		vertex[2].tu = (float)s.left / desc.Width;
		vertex[2].tv = (float)s.bottom / desc.Height;
		vertex[3].tu = (float)s.right / desc.Width;
		vertex[3].tv = (float)s.bottom / desc.Height;

		float ps[8] = {abs((float)width_d/width_s), abs((float)height_d/height_s), desc.Width, desc.Height,
						desc.Width/2, desc.Height, abs((float)width_d/(width_s/2)), abs((float)height_d/(height_s/2))};
		ps[0] = ps[0] > 1 ? 1 : ps[0];
		ps[1] = ps[1] > 1 ? 1 : ps[1];
		ps[6] = ps[6] > 1 ? 1 : ps[6];
		ps[7] = ps[7] > 1 ? 1 : ps[7];
		m_Device->SetPixelShaderConstantF(0, ps, 2);


		// shader
		IDirect3DPixelShader9 * lanczos_shader = m_lanczosX;
		if (format == MEDIASUBTYPE_YV12)
			lanczos_shader = m_lanczosX_YV12;
		else if (format == MEDIASUBTYPE_NV12 || format == MEDIASUBTYPE_YUY2)
			lanczos_shader = m_lanczosX_NV12;
		else if (format == MEDIASUBTYPE_RGB32 || format == MEDIASUBTYPE_ARGB32)
			lanczos_shader = m_lanczosX;
		if (width_s != width_d)
			m_Device->SetPixelShader(lanczos_shader);
		else
			m_Device->SetPixelShader(shader_yuv);

		// render state and render
		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

		// pass2, Y filter
		m_Device->SetRenderTarget(0, dst);
		vertex[0].x = (float)d.left;
		vertex[0].y = (float)d.top;
		vertex[1].x = (float)d.right;
		vertex[1].y = (float)d.top;
		vertex[2].x = (float)d.left;
		vertex[2].y = (float)d.bottom;
		vertex[3].x = (float)d.right;
		vertex[3].y = (float)d.bottom;
		for(int i=0;i <4; i++)
		{
			vertex[i].x -= 0.5f;
 			vertex[i].y -= 0.5f;
			vertex[i].z = 1.0f;
			vertex[i].w = 1.0f;
		}

		vertex[0].tu = (float)0 / BIG_TEXTURE_SIZE;
		vertex[0].tv = (float)0 / BIG_TEXTURE_SIZE;
		vertex[1].tu = (float)width_d / BIG_TEXTURE_SIZE;
		vertex[1].tv = (float)0 / BIG_TEXTURE_SIZE;
		vertex[2].tu = (float)0 / BIG_TEXTURE_SIZE;
		vertex[2].tv = (float)height_s / BIG_TEXTURE_SIZE;
		vertex[3].tu = (float)width_d / BIG_TEXTURE_SIZE;
		vertex[3].tv = (float)height_s / BIG_TEXTURE_SIZE;

		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, alpha_blend );
		ps[1] = (float)height_d/height_s;
		ps[1] = ps[1] > 0 ? ps[1] : -ps[1];
		ps[1] = ps[1] > 1 ? 1 : ps[1];
		ps[2] = ps[3] = BIG_TEXTURE_SIZE;
		m_Device->SetPixelShaderConstantF(0, ps, 1);
		if (height_s != height_d)
			m_Device->SetPixelShader(m_lanczosY);
		else
			m_Device->SetPixelShader(NULL);

		hr = m_Device->SetTexture( 0, tmp1->texture );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

		safe_delete(tmp1);

	}
	else if (method == lanczos_onepass)
	{
		// pass0, 2D filter, rather slow, a little better quality
		m_Device->SetRenderTarget(0, dst);

		// shader
		IDirect3DPixelShader9 * lanczos_shader = m_lanczos;
		if (format == MEDIASUBTYPE_YV12)
			lanczos_shader = m_lanczos_YV12;
		else if (format == MEDIASUBTYPE_NV12 || format == MEDIASUBTYPE_YUY2)
			lanczos_shader = m_lanczos_NV12;
		else if (format == MEDIASUBTYPE_RGB32 || format == MEDIASUBTYPE_ARGB32)
			lanczos_shader = m_lanczos;
		if ((height_s != height_d || width_s != width_d)
			&& (IDirect3DPixelShader9*)m_lanczos)
		{
			m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			float ps[4] = {abs((float)width_d/width_s), abs((float)height_d/height_s), desc.Width, desc.Height};
			ps[0] = ps[0] > 1 ? 1 : ps[0];
			ps[1] = ps[1] > 1 ? 1 : ps[1];
			m_Device->SetPixelShaderConstantF(0, ps, 1);
			m_Device->SetPixelShader(lanczos_shader);
		}
		else
			hr = m_Device->SetPixelShader(shader_yuv);

		// render state and go
		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, alpha_blend );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, direct_vertex, sizeof(MyVertex));
		hr = m_Device->SetPixelShader(NULL);
	}
	else if (method == bilinear_mipmap_minus_one || method == bilinear_mipmap || method == bilinear_no_mipmap)
	{
		if (src2 && method != bilinear_no_mipmap)
		{
			// nearly all D3D9 cards doesn't support D3DUSAGE_AUTOGENMIPMAP
			// so if we need to use MIPMAP, then we need convert to RGB32 first;
			FAIL_RET(src2->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, NULL, NULL, m_last_reset_time));
			m_Device->SetTexture(0, src2->m_tex_gpu_RGB32->texture);
			shader_yuv = NULL;
		}

		float mip_lod = (method == bilinear_mipmap_minus_one) ?  -1.0f : 0.0f;
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&mip_lod );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, (method == bilinear_no_mipmap) ? D3DTEXF_NONE : D3DTEXF_LINEAR );

		m_Device->SetRenderTarget(0, dst);
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		m_Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
		m_Device->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);					// set to border mode, to remove a single half-line
		hr = m_Device->SetPixelShader(shader_yuv);
		hr = m_Device->SetStreamSource( 0, NULL, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, direct_vertex, sizeof(MyVertex));
	}

	m_Device->SetPixelShader(NULL);

	return S_OK;
}

HRESULT my12doomRenderer::render(bool forced)
{
	if(m_output_mode == pageflipping)
		return S_FALSE;

	SetEvent(m_render_event);

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
	_this->m_render_thread_id = GetCurrentThreadId();

	int l = timeGetTime();
	while (timeGetTime() - l < 0 && !_this->m_render_thread_exit)
		Sleep(1);

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	//SetThreadAffinityMask(GetCurrentThread(), 1);
	while(!_this->m_render_thread_exit)
	{
		if (_this->m_output_mode != pageflipping && GPUIdle)
		{
			if (_this->m_dsr0->m_State == State_Running && timeGetTime() - _this->m_last_frame_time < 333)
			{
				if (WaitForSingleObject(_this->m_render_event, 1) != WAIT_TIMEOUT)
				{

					//_this->m_Device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);
					//_this->render_nolock(true);
					//_this->m_Device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME);
					_this->render_nolock(true);

				}
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
		}
	}
	
	return S_OK;
}

// dx9 helper functions
HRESULT my12doomRenderer::load_image(int id /*= -1*/, bool forced /* = false */)
{

	CAutoLock lck(&m_packet_lock);
	if (!m_sample2render_1 && !m_sample2render_2)
		return S_FALSE;

	int l = timeGetTime();

	gpu_sample *sample1 = m_sample2render_1;
	gpu_sample *sample2 = m_sample2render_2;

	bool dual_stream = sample1 && sample2;
	input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
	bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;
	CLSID format = sample1->m_format;
	bool topdown = sample1->m_topdown;

	HRESULT hr = S_OK;
	if (sample1)
	{
// 		FAIL_RET(sample1->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time));
		if (need_detect) sample1->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
	}
	if (sample2)
	{
// 		FAIL_RET(sample2->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time));
	}


	CAutoLock rendered_lock(&m_rendered_packet_lock);
	safe_delete(m_last_rendered_sample1);
	safe_delete(m_last_rendered_sample2);
	m_last_rendered_sample1 = m_sample2render_1;
	m_last_rendered_sample2 = m_sample2render_2;
	m_sample2render_1 = NULL;
	m_sample2render_2 = NULL;
	

	// stereo test
	if (need_detect)
	{
		int this_frame_type = 0;
		if (S_OK == m_last_rendered_sample1->get_strereo_test_result(m_Device, &this_frame_type))
		{
			if (this_frame_type == side_by_side)
				m_sbs ++;
			else if (this_frame_type == top_bottom)
				m_tb ++;
			else if (this_frame_type == mono2d)
				m_normal ++;
		}

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
			if (false)
			set_device_state(need_reset_object);
		}
	}

	static int n = timeGetTime();
	//printf("load and convert time:%dms, presant delta: %dms(%.02f circle)\n", timeGetTime()-l, timeGetTime()-n, (double)(timeGetTime()-n) / (1000/60));
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

	bool dual_stream = (m_dsr0->is_connected() && m_dsr1->is_connected()) || m_remux_mode;
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

HRESULT my12doomRenderer::calculate_movie_position(RECT *position)
{
	if (!position)
		return E_POINTER;

	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};

	// swap width and height for vertical orientation
	if (m_display_orientation == vertical)
	{
		tar.right ^= tar.bottom;
		tar.bottom ^= tar.right;
		tar.right ^= tar.bottom;
	}

	// half width/height for sbs/tb output mode
	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

	double active_aspect = get_active_aspect();
	int delta_w = (int)(tar.right - tar.bottom * active_aspect + 0.5);
	int delta_h = (int)(tar.bottom - tar.right  / active_aspect + 0.5);
	if (delta_w > 0)
	{
		// letterbox left and right (default), or vertical fill(vertical is already full)
		if (m_aspect_mode == aspect_letterbox || m_aspect_mode == aspect_vertical_fill)
		{
			tar.left += delta_w/2;
			tar.right -= delta_w/2;
		}
		else if (m_aspect_mode == aspect_horizontal_fill)
		{
			// extent horizontally, top and bottom cut
			// (delta_h < 0)
			tar.top += delta_h/2;
			tar.bottom -= delta_h/2;
		}
		else	// stretch mode, do nothing
		{
		}
	}
	else if (delta_h > 0)
	{
		// letterbox top and bottome (default)
		if (m_aspect_mode == aspect_letterbox || m_aspect_mode == aspect_horizontal_fill)
		{
			tar.top += delta_h/2;
			tar.bottom -= delta_h/2;
		}
		else if (m_aspect_mode == aspect_vertical_fill)
		{
			// extent vertically, top and bottom cut
			// (delta_w < 0)
			tar.left += delta_w/2;
			tar.right -= delta_w/2;
		}
		else	// stretch mode, do nothing
		{
		}
	}

	// offsets
	int tar_width = tar.right-tar.left;
	int tar_height = tar.bottom - tar.top;
	tar.left += (LONG)(tar_width * m_movie_offset_x);
	tar.right += (LONG)(tar_width * m_movie_offset_x);
	tar.top += (LONG)(tar_height * m_movie_offset_y);
	tar.bottom += (LONG)(tar_height * m_movie_offset_y);

	*position = tar;
	return S_OK;
}

HRESULT my12doomRenderer::calculate_subtitle_position(RECTF *postion, bool left_eye)
{
	if (!postion)
		return E_POINTER;

	RECT tar;
	calculate_movie_position(&tar);

	int pic_width = tar.right - tar.left;
	int pic_height = tar.bottom - tar.top;

	float left = tar.left + pic_width * (m_bmp_fleft - (left_eye ? 0 :m_bmp_parallax));
	float width = pic_width * m_bmp_fwidth;
	float top = tar.top + pic_height * m_bmp_ftop;
	float height = pic_height * m_bmp_fheight;

	postion->left = left;
	postion->right = left + width;
	postion->top = top;
	postion->bottom = top + height;

	return E_NOTIMPL;
}


HRESULT my12doomRenderer::generate_mask()
{
	HRESULT hr;
	if (m_output_mode != masking)
		return S_FALSE;

	if (!m_tex_mask)
		return VFW_E_NOT_CONNECTED;

	CComPtr<IDirect3DTexture9> mask_cpu;
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &mask_cpu, NULL));


	CAutoLock lck(&m_frame_lock);
	D3DLOCKED_RECT locked;
	FAIL_RET(mask_cpu->LockRect(0, &locked, NULL, NULL));

	BYTE *dst = (BYTE*) locked.pBits;

	if (m_mask_mode == row_interlace)
	{
		// init row mask texture
		D3DCOLOR one_line[BIG_TEXTURE_SIZE];
		for(DWORD i=0; i<BIG_TEXTURE_SIZE; i++)
			one_line[i] = i%2 == 0 ? 0 : D3DCOLOR_ARGB(255,255,255,255);

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == line_interlace)
	{
		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memset(dst, i%2 == 0 ? 255 : 0 ,m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == checkboard_interlace)
	{
		// init row mask texture
		D3DCOLOR one_line[BIG_TEXTURE_SIZE];
		for(DWORD i=0; i<BIG_TEXTURE_SIZE; i++)
			one_line[i] = i%2 == 0 ? 0 : D3DCOLOR_ARGB(255,255,255,255);

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (i%2), m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == subpixel_row_interlace)
	{
		D3DCOLOR one_line[BIG_TEXTURE_SIZE];
		for(DWORD i=0; i<BIG_TEXTURE_SIZE; i++)
			one_line[i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,0,255) : D3DCOLOR_ARGB(255,0,255,0);

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == subpixel_45_interlace)
	{
		D3DCOLOR one_line[6][BIG_TEXTURE_SIZE];
		for(DWORD i=0; i<BIG_TEXTURE_SIZE; i++)
		{
			one_line[0][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,255,255) : D3DCOLOR_ARGB(255,0,0,0);
			one_line[1][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,255,0) : D3DCOLOR_ARGB(255,0,0,255);
			one_line[2][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,0,0) : D3DCOLOR_ARGB(255,0,255,255);
			one_line[3][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,0,0,0) : D3DCOLOR_ARGB(255,255,255,255);
			one_line[4][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,0,0,255) : D3DCOLOR_ARGB(255,255,255,0);
			one_line[5][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,0,255,255) : D3DCOLOR_ARGB(255,255,0,0);
		}

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line[(i+m_mask_parameter)%6], m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}

	FAIL_RET(mask_cpu->UnlockRect(0));
	FAIL_RET(m_Device->UpdateTexture(mask_cpu, m_tex_mask));

	return hr;
}
HRESULT my12doomRenderer::set_fullscreen(bool full)
{
	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &m_d3ddm );
	m_new_pp.BackBufferFormat       = m_d3ddm.Format;

	mylog("mode:%dx%d@%dHz, format:%d.\n", m_d3ddm.Width, m_d3ddm.Height, m_d3ddm.RefreshRate, m_d3ddm.Format);

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
		m_new_pp.BackBufferWidth = m_d3ddm.Width;
		m_new_pp.BackBufferHeight = m_d3ddm.Height;
		m_new_pp.FullScreen_RefreshRateInHz = m_d3ddm.RefreshRate;

		if (m_output_mode == hd3d && m_HD3DStereoModesCount > 0)
			HD3DSetStereoFullscreenPresentParameters();

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

HRESULT my12doomRenderer::pump()
{
	BOOL success = FALSE;
	RECT rect;
	if (m_hWnd)
	{
		GetWindowRect(m_hWnd, &m_window_rect);
		success = GetClientRect(m_hWnd, &rect);
		if (success && rect.right > 0 && rect.bottom > 0)
		if (m_active_pp.BackBufferWidth != rect.right-rect.left || m_active_pp.BackBufferHeight != rect.bottom - rect.top)
		{
			if (m_active_pp.Windowed)
			set_device_state(need_resize_back_buffer);
		}
	}

	if (m_hWnd2)
	{
		success = GetClientRect(m_hWnd2, &rect);
		if (success && rect.right > 0 && rect.bottom > 0)
		if (m_active_pp2.BackBufferWidth != rect.right-rect.left || m_active_pp2.BackBufferHeight != rect.bottom - rect.top)
			if (m_active_pp.Windowed)
			set_device_state(need_resize_back_buffer);
	}

	return handle_device_state();
}


HRESULT my12doomRenderer::NV3D_notify(WPARAM wparam)
{
	BOOL actived = LOWORD(wparam);
	WORD separation = HIWORD(wparam);
	
	if (actived)
	{
		m_nv3d_actived = true;
		mylog("actived!\n");
	}
	else
	{
		m_nv3d_actived = false;
		if (m_output_mode == NV3D)
			set_device_state(need_resize_back_buffer);
		mylog("deactived!\n");
	}


	return S_OK;
}

HRESULT my12doomRenderer::set_input_layout(int layout)
{
	m_input_layout = (input_layout_types)layout;
	if(false)
	{
		set_device_state(need_reset_object);
		handle_device_state();
	}
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_output_mode(int mode)
{
	if (m_output_mode == mode)
		return S_OK;

	HRESULT hr;

	if (mode == intel3d && m_intel_caps.ulNumEntries <= 0)
		return E_NOINTERFACE;

	if (m_output_mode != intel3d && mode == intel3d)
		FAIL_RET(intel_switch_to_3d());

	if (m_output_mode == intel3d && mode != intel3d)
		FAIL_RET(intel_switch_to_2d());

	if (mode == hd3d)
		FAIL_RET(HD3DMatchResolution());

	m_output_mode = (output_mode_types)(mode % output_mode_types_max);

	m_pageflipping_start = -1;
	set_device_state(need_resize_back_buffer);

	return S_OK;
}

HRESULT my12doomRenderer::set_mask_mode(int mode)
{
	m_mask_mode = (mask_mode_types)(mode % mask_mode_types_max);
	generate_mask();
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_parameter(int parameter)
{
	m_mask_parameter = parameter;
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

	reload_image();

	if (m_output_mode == pageflipping)
		terminate_render_thread();
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
int my12doomRenderer::get_mask_parameter()
{
	return m_mask_parameter;
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
		m_movie_offset_x = offset;
	else if (dimention == 2)
		m_movie_offset_y = offset;
	else
		return E_INVALIDARG;

	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_aspect_mode(int mode)
{
	if (m_aspect_mode == mode)
		return S_OK;

	m_aspect_mode = (aspect_mode_types)mode;
	if (m_output_mode == pageflipping)
		terminate_render_thread();
	repaint_video();
	if (m_output_mode == pageflipping)
		create_render_thread();
	return S_OK;

}

HRESULT my12doomRenderer::set_vsync(bool on)
{
	on = m_output_mode == pageflipping ? true : false;
	
	if (m_vertical_sync != on)
	{
		m_vertical_sync = on;

		m_new_pp.PresentationInterval   = m_vertical_sync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

		set_device_state(need_reset);
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_aspect(double aspect)
{
	m_forced_aspect = aspect;
	if (m_output_mode == pageflipping)
		terminate_render_thread();
	repaint_video();
	if (m_output_mode == pageflipping)
		create_render_thread();
	return S_OK;
}
double my12doomRenderer::get_movie_pos(int dimention)
{
	if (dimention == 1)
		return m_movie_offset_x;
	else if (dimention == 2)
		return m_movie_offset_y;
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
	if (m_dsr0->m_State != State_Running && timeGetTime() - m_last_frame_time < 333 && m_output_mode != pageflipping)
		render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_bmp(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop, bool gpu_shadow)
{
	if (m_tex_bmp == NULL)
		return VFW_E_WRONG_STATE;

	if (m_device_state >= device_lost)
		return S_FALSE;			// TODO : of source it's not SUCCESS

	if (data == NULL)
	{
		m_has_subtitle = false;
	}

	else
	{
		m_has_subtitle = true;
		m_gpu_shadow = gpu_shadow;

		m_bmp_fleft = fleft;
		m_bmp_ftop = ftop;
		m_bmp_fwidth = fwidth;
		m_bmp_fheight = fheight;
		m_bmp_width = width;
		m_bmp_height = height;


		{
			CAutoLock lck2(&m_bmp_lock);
			BYTE *src = (BYTE*)data;
			BYTE *dst = (BYTE*) m_bmp_locked_rect.pBits;
			for(int y=0; y<min(BIG_TEXTURE_SIZE,height); y++)
			{
				memset(dst, 0, m_bmp_locked_rect.Pitch);
				memcpy(dst, src, width*4);
				dst += m_bmp_locked_rect.Pitch;
				src += width*4;
			}
			memset(dst, 0, m_bmp_locked_rect.Pitch * (BIG_TEXTURE_SIZE-min(BIG_TEXTURE_SIZE,height)));
			m_bmp_changed = true;
		}

		//m_vertex_changed = true;
		repaint_video();
	}

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

HRESULT my12doomRenderer::set_bmp_parallax(double offset)
{
	if (m_bmp_parallax != offset)
	{
		m_bmp_parallax = offset;
		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_parallax(double parallax)
{
	if (m_parallax != parallax)
	{
		m_parallax = parallax;
		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::intel_get_caps()
{
	HRESULT hr;
	m_intel_s3d = CreateIGFXS3DControl();
	if (!m_intel_s3d)
		return E_UNEXPECTED;
	memset(&m_intel_caps, 0 , sizeof(m_intel_caps));
	FAIL_FALSE(m_intel_s3d->GetS3DCaps(&m_intel_caps));

	if (m_intel_caps.ulNumEntries <= 0)
		return S_FALSE;
	return S_OK;
}

HRESULT my12doomRenderer::intel_get_caps(IGFX_S3DCAPS *caps)
{
	if (!caps)
		return E_POINTER;

	HRESULT hr;
	FAIL_RET(intel_get_caps());

	memcpy(caps, &m_intel_caps, sizeof(IGFX_S3DCAPS));
	return hr;
}

HRESULT my12doomRenderer::intel_switch_to_2d()
{
	if (!m_intel_s3d || m_intel_2d_mode.ulResWidth == 0)
		return E_UNEXPECTED;

	HRESULT hr ;

	FAIL_RET(m_intel_s3d->SwitchTo2D(&m_intel_2d_mode));

	memset(&m_intel_2d_mode, 0, sizeof(m_intel_2d_mode));
	memset(&m_intel_active_3d_mode, 0, sizeof(m_intel_active_3d_mode));

	return S_OK;
}

HRESULT my12doomRenderer::intel_switch_to_3d()
{
	HRESULT hr;
	if (!m_intel_s3d)
		return E_UNEXPECTED;

	// save current mode first
	memset(&m_intel_2d_mode, 0 , sizeof(m_intel_2d_mode));
	D3DDISPLAYMODE current;
	FAIL_RET(m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &current));
	m_intel_2d_mode.ulResWidth = current.Width;
	m_intel_2d_mode.ulResHeight = current.Height;
	m_intel_2d_mode.ulRefreshRate = current.RefreshRate;

	hr = intel_get_caps();
	if (hr != S_OK)
		return E_FAIL;

	// TODO: select modes
	memset(&m_intel_active_3d_mode, 0, sizeof(m_intel_active_3d_mode));

	// resolution match
	for(int i=0; i<m_intel_caps.ulNumEntries; i++)
	{
		if (m_intel_caps.S3DSupportedModes[i].ulResWidth == current.Width && 
			m_intel_caps.S3DSupportedModes[i].ulResHeight == current.Height)
		{
			m_intel_active_3d_mode.ulResWidth = current.Width;
			m_intel_active_3d_mode.ulResHeight = current.Height;
			break;
		}
	}

	// return on match fail
	if (m_intel_active_3d_mode.ulResWidth == 0)
		return E_RESOLUTION_MISSMATCH;

	// select max refresh rate
	for(int i=0; i<m_intel_caps.ulNumEntries; i++)
	{
		if (m_intel_caps.S3DSupportedModes[i].ulResWidth == current.Width && 
			m_intel_caps.S3DSupportedModes[i].ulResHeight == current.Height)
		{
			m_intel_active_3d_mode.ulRefreshRate = max(m_intel_active_3d_mode.ulRefreshRate, m_intel_caps.S3DSupportedModes[i].ulRefreshRate);
		}

		if (m_intel_caps.S3DSupportedModes[i].ulRefreshRate == current.RefreshRate)
		{
			m_intel_active_3d_mode.ulRefreshRate = current.RefreshRate;
			break;
		}
	}

	// return on match fail
	if (m_intel_active_3d_mode.ulRefreshRate == 0)
		return E_RESOLUTION_MISSMATCH;

	hr = m_intel_s3d->SwitchTo2D(&m_intel_active_3d_mode);
	//m_intel_active_3d_mode.ulRefreshRate = 24;
	hr = m_intel_s3d->SwitchTo3D(&m_intel_active_3d_mode);

	return S_OK;
}

HRESULT my12doomRenderer::intel_d3d_init()
{
	HRESULT hr;

	hr = myDXVA2CreateDirect3DDeviceManager9(&m_resetToken, &m_d3d_manager);

	if (m_d3d_manager)
		hr = m_d3d_manager->ResetDevice(m_Device, m_resetToken);

	return S_OK;
}
HRESULT my12doomRenderer::intel_delete_rendertargets()
{
	m_overlay_swap_chain = NULL;
	m_intel_VP_right = NULL;
	m_intel_VP_left = NULL;

	return S_OK;
}
HRESULT my12doomRenderer::intel_create_rendertargets()
{
	if (m_output_mode != intel3d)
		return S_FALSE;

	HRESULT hr;

	// swap chain
	D3DPRESENT_PARAMETERS pp2 = m_active_pp;
	pp2.BackBufferWidth = min(pp2.BackBufferWidth, m_intel_active_3d_mode.ulResWidth);
	pp2.BackBufferHeight = min(pp2.BackBufferHeight, m_intel_active_3d_mode.ulResHeight);
	pp2.SwapEffect = D3DSWAPEFFECT_OVERLAY;
	pp2.MultiSampleType = D3DMULTISAMPLE_NONE;
	FAIL_RET(m_Device->CreateAdditionalSwapChain(&pp2, &m_overlay_swap_chain));

	// some present parameter
	DXVA2_VideoDesc g_VideoDesc;
	memset(&g_VideoDesc, 0, sizeof(g_VideoDesc));
	DXVA2_ExtendedFormat format =   {           // DestFormat
		DXVA2_SampleProgressiveFrame,           // SampleFormat
		DXVA2_VideoChromaSubsampling_MPEG2,     // VideoChromaSubsampling
		DXVA_NominalRange_0_255,                // NominalRange
		DXVA2_VideoTransferMatrix_BT709,        // VideoTransferMatrix
		DXVA2_VideoLighting_bright,             // VideoLighting
		DXVA2_VideoPrimaries_BT709,             // VideoPrimaries
		DXVA2_VideoTransFunc_709                // VideoTransferFunction            
	};
	DXVA2_AYUVSample16 color = {         
		0x8000,          // Cr
		0x8000,          // Cb
		0x1000,          // Y
		0xffff           // Alpha
	};

	// video desc
	memcpy(&g_VideoDesc.SampleFormat, &format, sizeof(DXVA2_ExtendedFormat));
	g_VideoDesc.SampleWidth                         = 0;
	g_VideoDesc.SampleHeight                        = 0;    
	g_VideoDesc.InputSampleFreq.Numerator           = 60;
	g_VideoDesc.InputSampleFreq.Denominator         = 1;
	g_VideoDesc.OutputFrameFreq.Numerator           = 60;
	g_VideoDesc.OutputFrameFreq.Denominator         = 1;

	// blt param
	memset(&m_BltParams, 0, sizeof(m_BltParams));
	m_BltParams.TargetFrame = 0;
	memcpy(&m_BltParams.DestFormat, &format, sizeof(DXVA2_ExtendedFormat));
	memcpy(&m_BltParams.BackgroundColor, &color, sizeof(DXVA2_AYUVSample16));  

	// sample
	m_Sample.Start = 0;
	m_Sample.End = 1;
	m_Sample.SampleFormat = format;
	m_Sample.PlanarAlpha.Fraction = 0;
	m_Sample.PlanarAlpha.Value = 1;   


	// CreateVideoProcessor

	CComPtr<IDirectXVideoProcessorService> g_dxva_service;
	FAIL_RET(myDXVA2CreateVideoService(m_Device, IID_IDirectXVideoProcessorService, (void**)&g_dxva_service));
	FAIL_RET(m_intel_s3d->SetDevice(m_d3d_manager));
	FAIL_RET(m_intel_s3d->SelectLeftView());
	FAIL_RET( g_dxva_service->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice,
		&g_VideoDesc,
		D3DFMT_X8R8G8B8,
		1,
		&m_intel_VP_left));

	FAIL_RET(m_intel_s3d->SelectRightView());
	FAIL_RET(g_dxva_service->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice,
		&g_VideoDesc,
		D3DFMT_X8R8G8B8,
		1,
		&m_intel_VP_right));

	return S_OK;
}
HRESULT my12doomRenderer::intel_render_frame(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface)
{
	if (!m_overlay_swap_chain)
		return E_FAIL;

	CComPtr<IDirect3DSurface9> back_buffer;
	m_overlay_swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);

	RECT rect = {0,0,min(m_active_pp.BackBufferWidth, m_intel_active_3d_mode.ulResWidth), min(m_active_pp.BackBufferHeight, m_intel_active_3d_mode.ulResHeight)};

	m_BltParams.TargetRect = rect;
	m_Sample.SrcRect = rect;
	m_Sample.DstRect = rect;
	m_Sample.SrcSurface = left_surface;


	HRESULT hr;
	hr = m_intel_VP_left->VideoProcessBlt(back_buffer, &m_BltParams, &m_Sample, 1, NULL);
	if (FAILED(hr))
		set_device_state(need_resize_back_buffer);

	m_Sample.SrcSurface = right_surface;

	hr = m_intel_VP_right->VideoProcessBlt(back_buffer, &m_BltParams, &m_Sample, 1, NULL);

	// 	g_pd3dDevice->EndScene();
	// 	hr = g_overlay_swap_chain->Present(&dest, &dest, NULL, NULL, D3DPRESENT_UPDATECOLORKEY);


	return S_OK;
}

// AMD HD3D functions
HRESULT my12doomRenderer::HD3D_one_time_init()
{
	m_HD3Dlineoffset = 0;
	m_HD3DStereoModesCount = 0;

	HRESULT hr = m_Device->CreateOffscreenPlainSurface(10, 10, (D3DFORMAT)FOURCC_AQBS, D3DPOOL_DEFAULT, &m_HD3DCommSurface, NULL);
	if( FAILED( hr ) )
	{
		mylog("CreateOffscreenPlainSurface(FOURCC_AQBS) FAILED\r\n");
		goto amd_hd3d_fail;
	}
	else
	{
		mylog("CreateOffscreenPlainSurface(FOURCC_AQBS) OK\r\n");
	}

	// Send the command to the driver using the temporary surface
	// TO enable stereo
	hr = HD3DSendStereoCommand(ATI_STEREO_ENABLESTEREO, NULL, 0, 0, 0);
	if( FAILED( hr ) )
	{
		mylog("SendStereoCommand(ATI_STEREO_ENABLESTEREO) FAILED\r\n");
		m_HD3DCommSurface = NULL;
		goto amd_hd3d_fail;
	}
	else
	{
		mylog("SendStereoCommand(ATI_STEREO_ENABLESTEREO) OK\r\n");
	}


	// See what stereo modes are available
	ATIDX9GETDISPLAYMODES displayModeParams;
	displayModeParams.dwNumModes    = 0;
	displayModeParams.pStereoModes = NULL;

	//Send stereo command to get the number of available stereo modes.
	hr = HD3DSendStereoCommand(ATI_STEREO_GETDISPLAYMODES, (BYTE *)(&displayModeParams),
		sizeof(ATIDX9GETDISPLAYMODES), 0, 0);  
	if( FAILED( hr ) )
	{
		mylog("ATI_STEREO_GETDISPLAYMODES count FAILED\r\n");
		m_HD3DCommSurface = NULL;
		goto amd_hd3d_fail;
	}

	//Send stereo command to get the list of stereo modes
	if(displayModeParams.dwNumModes)
	{
		displayModeParams.pStereoModes = m_HD3DStereoModes;

		hr = HD3DSendStereoCommand(ATI_STEREO_GETDISPLAYMODES, (BYTE *)(&displayModeParams), 
			sizeof(ATIDX9GETDISPLAYMODES), 0, 0);
		if( FAILED( hr ) )
		{
			mylog("ATI_STEREO_GETDISPLAYMODES modes FAILED\r\n");

			m_HD3DCommSurface = NULL;
			goto amd_hd3d_fail;
		}
		m_HD3DStereoModesCount = displayModeParams.dwNumModes;
	}

	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE mode = m_HD3DStereoModes[i];
		mylog("Stereo Mode: %dx%d@%dfps, %dformat\r\n", mode.Width, mode.Height, mode.RefreshRate, mode.Format);
	}

	m_HD3DCommSurface = NULL;
	return S_OK;

amd_hd3d_fail:
	m_HD3DCommSurface = NULL;
	return S_FALSE;		// yes, always success, 
}

HRESULT my12doomRenderer::HD3D_restore_objects()
{
	mylog("1");

	m_HD3Dlineoffset = 0;
	HRESULT hr = m_Device->CreateOffscreenPlainSurface(10, 10, (D3DFORMAT)FOURCC_AQBS, D3DPOOL_DEFAULT, &m_HD3DCommSurface, NULL);
	if( FAILED( hr ) )
	{
		mylog("FOURCC_AQBS FAIL");
		return S_FALSE;
	}

	mylog("2");
	//Retrieve the line offset
	hr = HD3DSendStereoCommand(ATI_STEREO_GETLINEOFFSET, (BYTE *)(&m_HD3Dlineoffset), sizeof(DWORD), 0, 0);
	if( FAILED( hr ) )
	{
		mylog("ATI_STEREO_GETLINEOFFSET FAIL=%d\r\n", m_HD3Dlineoffset);
		return S_FALSE;
	}
	mylog("3");

	// see if lineOffset is valid
	mylog("lineoffset=%d\r\n", m_HD3Dlineoffset);

	return S_OK;
}

HRESULT my12doomRenderer::HD3D_invalidate_objects()
{
	mylog("m_HD3DCommSurface = NULL");
	m_HD3DCommSurface = NULL;
	return S_OK;
}

HRESULT my12doomRenderer::HD3DSetStereoFullscreenPresentParameters()
{
	D3DDISPLAYMODE current_mode;
	m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &current_mode);

	// find direct match
	int res_x = 0;
	int res_y = 0;
	int refresh = 0;
	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
		if (this_mode.Width == current_mode.Width && this_mode.Height == current_mode.Height)
		{
			res_x = this_mode.Width;
			res_y = this_mode.Height;
			break;
		}
	}
	// if no match, then we choose highest mode
	if (res_x == 0 || res_y == 0)
	{
		for(int i=0; i<m_HD3DStereoModesCount; i++)
		{
			D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
			if (this_mode.Width * this_mode.Height > res_x*res_y)
			{
				res_x = this_mode.Width;
				res_y = this_mode.Height;
			}
		}

	}

	// find direct matched refresh rate
	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
		if (this_mode.Width == res_x && this_mode.Height == res_y && this_mode.RefreshRate == current_mode.RefreshRate)
		{
			refresh = this_mode.RefreshRate;
			break;
		}
	}


	// no match ? use highest refresh rate
	if (refresh == 0)
	{
		for(int i=0; i<m_HD3DStereoModesCount; i++)
		{
			D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
			if (this_mode.Width == res_x && this_mode.Height == res_y)
				refresh = max(refresh, this_mode.RefreshRate);
		}
	}


	m_new_pp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
	m_new_pp.FullScreen_RefreshRateInHz = refresh;
	m_new_pp.BackBufferWidth = res_x;
	m_new_pp.BackBufferHeight = res_y;
	m_new_pp.BackBufferFormat = D3DFMT_X8R8G8B8;

	return S_OK;
}


HRESULT my12doomRenderer::HD3DMatchResolution()
{
	D3DDISPLAYMODE current_mode;
	m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &current_mode);

	if (m_HD3DStereoModesCount <= 0)
		return E_NOINTERFACE;

	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
		if (this_mode.Width == current_mode.Width && this_mode.Height == current_mode.Height)
			return S_OK;
	}

	return E_RESOLUTION_MISSMATCH;
}

HRESULT my12doomRenderer::HD3DGetAvailable3DModes(D3DDISPLAYMODE *modes, int *count)
{
	if (NULL == count)
		return E_POINTER;

	if( m_HD3DStereoModesCount <= 0 )
	{
		*count = 0;
		return S_OK;
	}

	for(int i=0; i<m_HD3DStereoModesCount && i<*count; i++)
		modes[i] = m_HD3DStereoModes[i];

	*count = m_HD3DStereoModesCount;
	return S_OK;
}

HRESULT my12doomRenderer::HD3DSendStereoCommand(ATIDX9STEREOCOMMAND stereoCommand, BYTE *pOutBuffer, 
							 DWORD dwOutBufferSize, BYTE *pInBuffer, 
							 DWORD dwInBufferSize)
{
	if (!m_HD3DCommSurface)
		return E_FAIL;

	HRESULT hr;
	ATIDX9STEREOCOMMPACKET *pCommPacket;
	D3DLOCKED_RECT lockedRect;

	hr = m_HD3DCommSurface->LockRect(&lockedRect, 0, 0);
	if(FAILED(hr))
	{
		return hr;
	}
	pCommPacket = (ATIDX9STEREOCOMMPACKET *)(lockedRect.pBits);
	pCommPacket->dwSignature = 'STER';
	pCommPacket->pResult = &hr;
	pCommPacket->stereoCommand = stereoCommand;
	if (pOutBuffer && !dwOutBufferSize)
	{
		return hr;
	}
	pCommPacket->pOutBuffer = pOutBuffer;
	pCommPacket->dwOutBufferSize = dwOutBufferSize;
	if (pInBuffer && !dwInBufferSize)
	{
		return hr;
	}
	pCommPacket->pInBuffer = pInBuffer;
	pCommPacket->dwInBufferSize = dwInBufferSize;
	m_HD3DCommSurface->UnlockRect();
	return hr;
}

HRESULT my12doomRenderer::HD3DDrawStereo(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface, IDirect3DSurface9 *back_buffer)
{
	m_Device->SetRenderTarget(0, back_buffer);

	// draw left
	HRESULT hr = m_Device->StretchRect(left_surface, NULL, back_buffer, NULL, D3DTEXF_LINEAR);

	// update the quad buffer with the right render target
	D3DVIEWPORT9 viewPort;
	viewPort.X = 0;
	viewPort.Y = m_HD3Dlineoffset;
	viewPort.Width = m_active_pp.BackBufferWidth;
	viewPort.Height = m_active_pp.BackBufferHeight;
	viewPort.MinZ = 0;
	viewPort.MaxZ = 1;
	hr = m_Device->SetViewport(&viewPort);

	mylog("lineoffset = %d, right_surface = %08x\r\n, hr = %08x", m_HD3Dlineoffset, right_surface, hr);


	// set the right quad buffer as the destination for StretchRect
	DWORD dwEye = ATI_STEREO_RIGHTEYE;
	HD3DSendStereoCommand(ATI_STEREO_SETDSTEYE, NULL, 0, (BYTE *)&dwEye, sizeof(dwEye));
	m_Device->StretchRect(right_surface, NULL, back_buffer, NULL, D3DTEXF_LINEAR);

	// restore the destination
	dwEye = ATI_STEREO_LEFTEYE;
	HD3DSendStereoCommand(ATI_STEREO_SETDSTEYE, NULL, 0, (BYTE *)&dwEye, sizeof(dwEye));

	// RenderPixelID();
	struct Vertex
	{
		FLOAT		x, y, z, w;
		D3DCOLOR	diffuse;
	};

	Vertex verts[2];

	verts[0].x = 0;
	verts[0].z = 0.5f;
	verts[0].w = 1;
	verts[1].x = 1;
	verts[1].z = 0.5f;
	verts[1].w = 1;

	viewPort.X = 0;
	viewPort.Width = m_active_pp.BackBufferWidth;
	viewPort.Height = m_active_pp.BackBufferHeight;
	viewPort.MinZ = 0;
	viewPort.MaxZ = 1;

	IDirect3DDevice9 *pd3dDevice = m_Device;

	// save state
	DWORD zEnable;
	pd3dDevice->GetRenderState(D3DRS_ZENABLE, &zEnable);

	pd3dDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DMATRIX identityMatrix;
	//D3DXMatrixIdentity
	myMatrixIdentity(&identityMatrix);
	pd3dDevice->SetTransform(D3DTS_VIEW, &identityMatrix);
	pd3dDevice->SetTransform(D3DTS_WORLD, &identityMatrix);
	D3DMATRIX orthoMatrix;
	//D3DXMatrixOrthoRH;
	myMatrixOrthoRH(&orthoMatrix, (float)m_active_pp.BackBufferWidth, (float)m_active_pp.BackBufferHeight, 0.0, 1.0f);
	pd3dDevice->SetTransform(D3DTS_PROJECTION, &orthoMatrix);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	float pointSize = 1.0f;
	//pd3dDevice->SetRenderTarget(0, pBackBufferSurface);
	pd3dDevice->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&pointSize));
	pd3dDevice->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pd3dDevice->SetVertexShader(NULL);
	pd3dDevice->SetPixelShader(NULL);

	// draw right eye
	viewPort.Y = m_HD3Dlineoffset;
	verts[0].y = (float)m_HD3Dlineoffset;
	verts[1].y = (float)m_HD3Dlineoffset;
	pd3dDevice->SetViewport(&viewPort);
	verts[0].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 0, 0, 0);
	verts[1].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 255, 255, 255);
	pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 2, verts, sizeof(Vertex));

	// draw left eye
	viewPort.Y = 0;
	pd3dDevice->SetViewport(&viewPort);
	verts[0].y = 0;
	verts[1].y = 0;
	verts[0].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 255, 255, 255);
	verts[1].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 0, 0, 0);
	pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 2, verts, sizeof(Vertex));

	// restore state
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, zEnable);
	return S_OK;
}

// helper functions

HRESULT myMatrixIdentity(D3DMATRIX *matrix)
{
	if (!matrix)
		return E_POINTER;

	memset(matrix, 0 , sizeof(D3DMATRIX));
	matrix->_11 =
	matrix->_22 =
	matrix->_33 =
	matrix->_44 = 1.0f;

	return S_OK;
}

HRESULT myMatrixOrthoRH(D3DMATRIX *matrix, float w, float h, float zn, float zf)
{
	if (!matrix)
		return E_POINTER;


	memset(matrix, 0 , sizeof(D3DMATRIX));
	matrix->_11 = 2.0f / w;
	matrix->_22 = 2.0f / h;
	matrix->_33 = 1 / (zn-zf);
	matrix->_43 = zn/ (zn-zf);
	matrix->_44 = .0f;

	return S_OK;
}

IDirect3DTexture9* helper_get_texture(gpu_sample *sample, helper_sample_format format)
{
	if (!sample)
		return NULL;

	CPooledTexture *texture = NULL;

	if (format == helper_sample_format_rgb32) texture = sample->m_tex_gpu_RGB32;
	if (format == helper_sample_format_y) texture = sample->m_tex_gpu_Y;
	if (format == helper_sample_format_yv12) texture = sample->m_tex_gpu_YV12_UV;
	if (format == helper_sample_format_nv12) texture = sample->m_tex_gpu_NV12_UV;
	if (format == helper_sample_format_yuy2) texture = sample->m_tex_gpu_YUY2_UV;

	if (!texture)
		return NULL;

	return texture->texture;
}

void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
	typedef struct tagTHREADNAME_INFO {
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

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

float frac(float f)
{
	double tmp;
	return modf(f, &tmp);
}
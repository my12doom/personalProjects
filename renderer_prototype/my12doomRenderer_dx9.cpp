// Direct3D9 part of my12doom renderer

#include "my12doomRenderer.h"
#include <dxva.h>
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
#include "PixelShaders/lanczos.h"
#include "PixelShaders/blur.h"
#include "PixelShaders/blur2.h"
#include "PixelShaders/LanczosX.h"
#include "PixelShaders/LanczosY.h"
#include "3dvideo.h"
#include <dvdmedia.h>
#include <math.h>
#include <assert.h>
#include "..\ZBuffer\stereo_test.h"
#include "..\dwindow\global_funcs.h"

#pragma comment (lib, "igfx_s3dcontrol.lib")
#pragma comment (lib, "comsupp.lib")
#pragma comment (lib, "dxva2.lib")
#pragma comment (lib, "nvapi.lib")

#define FAIL_RET(x) {hr=x; if(FAILED(hr)){return hr;}}
#define FAIL_FALSE(x) {hr=x; if(FAILED(hr)){return S_FALSE;}}
#define FAIL_SLEEP_RET(x) {hr=x; if(FAILED(hr)){Sleep(1); return hr;}}
#define safe_delete(x) if(x){delete x;x=NULL;}
#define safe_decommit(x) if((x))(x)->decommit()
HRESULT myMatrixOrthoRH(D3DMATRIX *matrix, float w, float h, float zn, float zf);
HRESULT myMatrixIdentity(D3DMATRIX *matrix);

#ifndef DEBUG
#define printf
#endif

int lockrect_surface = 0;
int lockrect_texture = 0;
__int64 lockrect_texture_cycle = 0;
const int BIG_TEXTURE_SIZE = 2048;
AutoSetting<BOOL> GPUIdle(L"GPUIdle", true, REG_DWORD);
AutoSetting<int> MovieResizing(L"MovieResampling", bilinear_mipmap_minus_one, REG_DWORD);
AutoSetting<int> SubtitleResizing(L"SubtitleResampling", bilinear_mipmap_minus_one, REG_DWORD);


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

float frac(float f)
{
	double tmp;
	return modf(f, &tmp);
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
	OutputDebugStringA(tmp2);
#endif
	return S_OK;
}

//#define printf
//#define mylog


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
	m_bmp_offset_x = 0.0;
	m_bmp_offset_y = 0.0;
	m_source_aspect = 1.0;
	m_forced_aspect = -1;
	m_aspect_mode = aspect_letterbox;

	// input / output
	m_input_layout = input_layout_auto;
	m_swapeyes = false;
	m_mask_mode = row_interlace;
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
	m_bmp_offset = 0;
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
	else if (m_dsr0->m_format == MEDIASUBTYPE_RGB32)
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
		loaded_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
		int l2 = timeGetTime();
		if (need_detect) loaded_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, g_VertexBuffer);
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
		CComPtr<IDirect3DSurface9> surface;
		m_swap1->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &surface);
		D3DSURFACE_DESC desc;

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

	if (!m_recreating_dshow_renderer && GetCurrentThreadId() == m_render_thread_id)
	{
		// warning : thread safe problem here
		D3DSURFACE_DESC desc;
		memset(&desc, 0, sizeof(desc));
		if (m_tex_rgb_left) m_tex_rgb_left->GetLevelDesc(0, &desc);
		bool dual_stream = (m_dsr0->is_connected() && m_dsr1->is_connected()) || m_remux_mode;
		input_layout_types input = get_active_input_layout();
		if (dual_stream && (m_lVidWidth != desc.Width || m_lVidHeight != desc.Height))
			set_device_state(need_reset_object);

		if (!dual_stream)
		{
			if (input == side_by_side && (m_lVidWidth/2 != desc.Width || m_lVidHeight != desc.Height))
				set_device_state(need_reset_object);
			if (input == top_bottom && (m_lVidWidth != desc.Width || m_lVidHeight /2 != desc.Height))
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
		/*
		terminate_render_thread();
		CAutoLock lck(&m_frame_lock);
		FAIL_SLEEP_RET(invalidate_objects());
		FAIL_SLEEP_RET(restore_objects());
		m_device_state = fine;
		*/

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
		m_vertex_changed = true;
		m_device_state = fine;

		// clear DEFAULT pool
		{
			CAutoLock lck(&m_pool_lock);
			if (m_pool)
				m_pool->DestroyPool(D3DPOOL_DEFAULT);
		}

		render_nolock(true);

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
			m_active_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
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
	m_lanczosY.invalid();
	m_lanczos.invalid();

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
	m_vs_subtitle = NULL;
	m_ps_color_adjust = NULL;
	m_ps_bmp_lanczos = NULL;
	m_ps_bmp_blur = NULL;

	// textures
	m_tex_rgb_left = NULL;
	m_tex_rgb_right = NULL;
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

	// vertex buffers
	g_VertexBuffer = NULL;
	m_vertex_subtitle = NULL;

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
	m_lanczosY.set_source(m_Device, g_code_lanczosY, sizeof(g_code_lanczosY), true, (DWORD*)m_key);
	m_lanczos.set_source(m_Device, g_code_lanczos, sizeof(g_code_lanczos), true, (DWORD*)m_key);

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
	FAIL_RET( m_Device->CreateTexture(m_pass1_width, m_pass1_height, 1, D3DUSAGE_RENDERTARGET | use_mipmap, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_tex_rgb_left, NULL));
	FAIL_RET( m_Device->CreateTexture(m_pass1_width, m_pass1_height, 1, D3DUSAGE_RENDERTARGET | use_mipmap, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_tex_rgb_right, NULL));
	fix_nv3d_bug();
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

	// clear m_tex_rgb_left & right
	CComPtr<IDirect3DSurface9> rgb_surface;
	FAIL_RET(m_tex_rgb_left->GetSurfaceLevel(0, &rgb_surface));
	clear(rgb_surface);
	rgb_surface = NULL;
	FAIL_RET(m_tex_rgb_right->GetSurfaceLevel(0, &rgb_surface));
	clear(rgb_surface);

	// vertex
	FAIL_RET(m_Device->CreateVertexBuffer( sizeof(m_vertices), D3DUSAGE_WRITEONLY, FVF_Flags, D3DPOOL_DEFAULT, &g_VertexBuffer, NULL ));
	FAIL_RET(m_Device->CreateVertexBuffer( sizeof(m_vertices), D3DUSAGE_WRITEONLY, FVF_Flags_subtitle, D3DPOOL_DEFAULT, &m_vertex_subtitle, NULL ));
	m_vertex_changed = true;

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

	m_Device->CreateVertexShader((DWORD*)g_code_vs_subtitle, &m_vs_subtitle);

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
	{
		CAutoLock lck(&m_packet_lock);
		CAutoLock rendered_lock(&m_rendered_packet_lock);
		safe_delete(m_last_rendered_sample1);
		safe_delete(m_last_rendered_sample2);

		m_sample2render_1 = m_last_rendered_sample1;
		m_sample2render_2 = m_last_rendered_sample2;
	}

	return S_OK;
}


#define JIF(x) {if(FAILED(hr=x))goto clearup;}

HRESULT my12doomRenderer::render_nolock(bool forced)
{
	// device state check and restore
	if (FAILED(handle_device_state()))
		return E_FAIL;

	//if (m_d3d_query) m_d3d_query->Issue(D3DISSUE_BEGIN);

	if (!(m_PC_level&PCLEVELTEST_TESTED))
		test_PC_level();

	// image loading and idle check
	if (load_image() != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!m_Device)
		return E_FAIL;

	static int last_render_time = timeGetTime();
	static int last_nv3d_fix_time = timeGetTime();

	HRESULT hr;

	CAutoLock lck(&m_frame_lock);

	if (!m_tex_rgb_left)
		return VFW_E_NOT_CONNECTED;

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
	//if(false)
	{
		bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();
		input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
		bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;

		CAutoLock lck(&m_queue_lock);
		for(POSITION pos_left = m_left_queue.GetHeadPosition(); pos_left; pos_left = m_left_queue.Next(pos_left))
		{
			gpu_sample *left_sample = m_left_queue.Get(pos_left);
			left_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
			if (need_detect) left_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, g_VertexBuffer);
		}
		for(POSITION pos_right = m_right_queue.GetHeadPosition(); pos_right; pos_right = m_right_queue.Next(pos_right))
		{
			gpu_sample *right_sample = m_right_queue.Get(pos_right);
			right_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
			if (need_detect) right_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, g_VertexBuffer);
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
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = m_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = m_tex_rgb_right->GetSurfaceLevel(0, &right_surface);

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

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );

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

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );

		// set render target to swap chain2
		if (m_swap2)
		{
			CComPtr<IDirect3DSurface9> back_buffer2;
			m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);

			clear(back_buffer2);

			hr = m_Device->SetRenderTarget(0, back_buffer2);
			m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			m_Device->SetPixelShader(m_ps_iz3d_front);
			hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
			hr = m_Device->SetFVF( FVF_Flags );
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );
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


	if (m_output_mode == intel3d && m_overlay_swap_chain)
	{
		RECT rect= {0, 0, min(m_intel_active_3d_mode.ulResWidth,m_active_pp.BackBufferWidth), 
			min(m_active_pp.BackBufferHeight, m_active_pp.BackBufferHeight)};
		hr = m_overlay_swap_chain->Present(&rect, &rect, NULL, NULL, D3DPRESENT_UPDATECOLORKEY);

		//mylog("%08x\n", hr);
	}


// 	UINT64 timing=0;
// 	if (m_d3d_query)
// 	{
// 		m_d3d_query->Issue(D3DISSUE_END);
// 		// Force the driver to execute the commands from the command buffer.
// 		// Empty the command buffer and wait until the GPU is idle.
// 		//while( == S_FALSE);
// 		LARGE_INTEGER l1, l2;
// 		QueryPerformanceCounter(&l1);
// 		hr = m_d3d_query->GetData( &timing, 
// 			sizeof(timing), D3DGETDATA_FLUSH );
// 		if (timing)
// 			printf("GPUIdle:%d.\n", (int)timing);
// 
// 		QueryPerformanceCounter(&l2);
// 
// 		//printf("Query time: %d cycle.\n", l2.QuadPart - l1.QuadPart);
// 
// 	}





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





	FAIL_RET(calculate_vertex());
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

HRESULT my12doomRenderer::draw_movie_lanczos(IDirect3DSurface9 *surface, bool left_eye)
{
	// assume draw_movie() handles pointer and not connected issues, so no more check
	MyVertex *p = m_vertices + (left_eye ? vertex_pass2_main : vertex_pass2_main_r);
	RECTF target = {p[0].x+0.5, p[0].y+0.5, p[3].x+0.5, p[3].y+0.5};
	CComPtr<IDirect3DSurface9> src;
	if (left_eye)
		m_tex_rgb_left->GetSurfaceLevel(0, &src);
	else
		m_tex_rgb_right->GetSurfaceLevel(0, &src);

	return resize_surface(src, surface, false, NULL, &target);
}

HRESULT my12doomRenderer::draw_movie_mip(IDirect3DSurface9 *surface, bool left_eye)
{
	// assume draw_movie() handles pointer and not connected issues, so no more check
	HRESULT hr = E_FAIL;

	float mip_lod = -1.0f;
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&mip_lod );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );


	m_Device->SetRenderTarget(0, surface);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_Device->SetTexture( 0, left_eye ? m_tex_rgb_left : m_tex_rgb_right );
	m_Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	m_Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
	m_Device->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);					// set to border mode, to remove a single half-line
	hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, left_eye ? vertex_pass2_main : vertex_pass2_main_r, 2 );

	return S_OK;
}

HRESULT my12doomRenderer::draw_movie(IDirect3DSurface9 *surface, bool left_eye)
{
	if (!surface)
		return E_POINTER;

	if (!m_dsr0->is_connected() && m_uidrawer)
	{
		m_last_reset_time = timeGetTime();
		return m_uidrawer->draw_nonmovie_bg(surface, left_eye);
	}

	switch((int)MovieResizing)
	{
	case bilinear_mipmap_minus_one:
		return draw_movie_mip(surface, left_eye);
	case lanczos:
		return draw_movie_lanczos(surface, left_eye);
	}

	assert(0);
	return draw_movie_mip(surface, left_eye);
}

HRESULT my12doomRenderer::draw_bmp_lanczos(IDirect3DSurface9 *surface, bool left_eye)
{
	// assume draw_movie() handles pointer and not connected issues, so no more check
	HRESULT hr = E_FAIL;

	CAutoLock lck(&m_pool_lock);
	CPooledTexture *tmp1 = NULL;
	hr = m_pool->CreateTexture(BIG_TEXTURE_SIZE, BIG_TEXTURE_SIZE, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tmp1);
	if (FAILED(hr))
	{
		safe_delete(tmp1);
		return hr;
	}

	// movie picture position
	float active_aspect = get_active_aspect();
	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_display_orientation == vertical)
	{
		tar.right %= tar.bottom;
		tar.bottom %= tar.right;
		tar.right %= tar.bottom;
	}
	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

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

	float pic_left = (float)tar.left / m_active_pp.BackBufferWidth;
	float pic_width = (float)(tar.right - tar.left) / m_active_pp.BackBufferWidth;
	float pic_top = (float)tar.top / m_active_pp.BackBufferHeight;
	float pic_height = (float)(tar.bottom - tar.top) / m_active_pp.BackBufferHeight;

	float left = pic_left + pic_width * m_bmp_fleft;
	float width = pic_width * m_bmp_fwidth;
	float top = pic_top + pic_height * m_bmp_ftop;
	float height = pic_height * m_bmp_fheight;

	// lanczos resize
	float left_in_pixel = left * m_active_pp.BackBufferWidth;
	float top_in_pixel = top * m_active_pp.BackBufferHeight;
	float right_in_pixel = (left+width) * m_active_pp.BackBufferWidth;
	float bottom_in_pixel = (top+height) * m_active_pp.BackBufferHeight;
	float width_in_pixel = right_in_pixel - left_in_pixel;
	float height_in_pixel = bottom_in_pixel - top_in_pixel;

	static int last_top = 0;
	static int last_bottom = 0;
	if (top_in_pixel != last_top || bottom_in_pixel != last_bottom)
		mylog("top,bottom:%d,%d\n", int(top * m_active_pp.BackBufferHeight * 100), int((top+height) * m_active_pp.BackBufferHeight * 100));
	last_top = top_in_pixel;
	last_bottom = bottom_in_pixel;

	CComPtr<IDirect3DSurface9> bmp_surf;
	m_tex_bmp->GetSurfaceLevel(0, &bmp_surf);
	CComPtr<IDirect3DSurface9> rt1;
	tmp1->get_first_level(&rt1);
	RECTF resize_s = {0,0,m_bmp_width, m_bmp_height};
	RECTF resize_d = {frac(left_in_pixel), frac(top_in_pixel),
		frac(left_in_pixel)+width_in_pixel,
		frac(top_in_pixel)+height_in_pixel};
	resize_surface(bmp_surf, rt1, false, &resize_s, &resize_d);

	// final drawing
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_Device->SetRenderTarget(0, surface);

	// reset shader
	hr = m_Device->SetVertexShader(NULL);
	hr = m_Device->SetPixelShader(NULL);

	MyVertex vertex[4];	
	vertex[0].x = (float)floor(left_in_pixel);
	vertex[0].y = (float)floor(top_in_pixel);
	vertex[1].x = (float)ceil(right_in_pixel);
	vertex[1].y = (float)floor(top_in_pixel);
	vertex[2].x = (float)floor(left_in_pixel);
	vertex[2].y = (float)ceil(bottom_in_pixel);
	vertex[3].x = (float)ceil(right_in_pixel);
	vertex[3].y = (float)ceil(bottom_in_pixel);

	for(int i=0;i <4; i++)
	{
		vertex[i].x -= 0.5f;
		vertex[i].y -= 0.5f;
		vertex[i].z = 1.0f;
		vertex[i].w = 1.0f;
	}

	vertex[0].tu = (float)floor(resize_d.left) / BIG_TEXTURE_SIZE;
	vertex[0].tv = (float)floor(resize_d.top) / BIG_TEXTURE_SIZE;
	vertex[1].tu = (float)ceil(resize_d.right) / BIG_TEXTURE_SIZE;
	vertex[1].tv = (float)floor(resize_d.top) / BIG_TEXTURE_SIZE;
	vertex[2].tu = (float)floor(resize_d.left) / BIG_TEXTURE_SIZE;
	vertex[2].tv = (float)ceil(resize_d.bottom) / BIG_TEXTURE_SIZE;
	vertex[3].tu = (float)ceil(resize_d.right) / BIG_TEXTURE_SIZE;
	vertex[3].tv = (float)ceil(resize_d.bottom) / BIG_TEXTURE_SIZE;

	m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	hr = m_Device->SetTexture( 0, tmp1->texture);
	hr = m_Device->SetFVF( FVF_Flags );

	if (!left_eye)
	{
		for(int i=0; i<4; i++)
			vertex[i].x -= floor(m_bmp_offset*pic_width * m_active_pp.BackBufferWidth+0.5);
	}

	if (m_gpu_shadow)
	{
		// draw shadow
		for(int i=0; i<4; i++)
		{
			vertex[i].x += floor(0.001*pic_width * m_active_pp.BackBufferWidth+0.5);
			vertex[i].y += floor(0.001*pic_height * m_active_pp.BackBufferHeight+0.5);
		}
		hr = m_Device->SetPixelShader(m_ps_bmp_blur);
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

		// draw main
		for(int i=0; i<4; i++)
		{
			vertex[i].x -= floor(0.001*pic_width * m_active_pp.BackBufferWidth+0.5);
			vertex[i].y -= floor(0.001*pic_height * m_active_pp.BackBufferHeight+0.5);
		}
		hr = m_Device->SetPixelShader(NULL);
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));
	}
	else
	{
		// draw main
		hr = m_Device->SetPixelShader(NULL);
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));
	}

	safe_delete(tmp1);

	return S_OK;
}

HRESULT my12doomRenderer::draw_bmp_mip(IDirect3DSurface9 *surface, bool left_eye)
{
	// assume draw_movie() handles pointer and not connected issues, so no more check
	HRESULT hr = E_FAIL;

	m_Device->SetRenderTarget(0, surface);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	float mip_lod = -1.0f;
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&mip_lod );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetTexture( 0, m_tex_bmp );
	hr = m_Device->SetStreamSource( 0, m_vertex_subtitle, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags_subtitle );


	// movie picture position
	float active_aspect = get_active_aspect();
	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

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

	float pic_left = (float)tar.left / m_active_pp.BackBufferWidth;
	float pic_width = (float)(tar.right - tar.left) / m_active_pp.BackBufferWidth;
	float pic_top = (float)tar.top / m_active_pp.BackBufferHeight;
	float pic_height = (float)(tar.bottom - tar.top) / m_active_pp.BackBufferHeight;

	// config subtitle position
	float left = pic_left + pic_width * m_bmp_fleft;
	float width = pic_width * m_bmp_fwidth;
	float top = pic_top + pic_height * m_bmp_ftop;
	float height = pic_height * m_bmp_fheight;
	float cfg_main[8] = {left*2-1, top*-2+1, width*2, height*-2,
		0, 0, (float)m_bmp_width/BIG_TEXTURE_SIZE, (float)m_bmp_height/BIG_TEXTURE_SIZE};
	float cfg_shadow[8] = {(left+0.001)*2-1, (top+0.001)*-2+1, width*2, height*-2,
		0, 0, (float)m_bmp_width/BIG_TEXTURE_SIZE, (float)m_bmp_height/BIG_TEXTURE_SIZE};

	if (!left_eye)
	{
		cfg_main[0] -= m_bmp_offset*pic_width * 2;
		cfg_shadow[0] -= m_bmp_offset*pic_width * 2;
	}
	hr = m_Device->SetVertexShader(m_vs_subtitle);

	// draw shadow
	if (m_gpu_shadow)
	{
		hr = m_Device->SetPixelShader(m_ps_bmp_blur);
		hr = m_Device->SetVertexShaderConstantF(0, cfg_shadow, 2);
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_bmp, 2 );
	}

	// draw main
	hr = m_Device->SetPixelShader(NULL);
	hr = m_Device->SetVertexShaderConstantF(0, cfg_main, 2);
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_bmp, 2 );

	// reset shader
	hr = m_Device->SetVertexShader(NULL);
	hr = m_Device->SetPixelShader(NULL);

	return S_OK;
}

HRESULT my12doomRenderer::draw_bmp(IDirect3DSurface9 *surface, bool left_eye)
{
	if (m_display_orientation != horizontal)
		return E_NOTIMPL;

	if (!surface)
		return E_POINTER;

	if (!m_has_subtitle)
		return S_FALSE;

	switch((int)SubtitleResizing)
	{
	case bilinear_mipmap_minus_one:
		return draw_bmp_mip(surface, left_eye);
	case lanczos:
		return draw_bmp_lanczos(surface, left_eye);
	}

	assert(0);
	return E_UNEXPECTED;
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

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );
		hr = m_Device->SetPixelShader(ps);

		// copying back
		hr = m_Device->StretchRect(surface_of_tex_rt, NULL, surface_to_adjust, NULL, D3DTEXF_LINEAR);

		m_pool->DeleteTexture(tex_src);
		m_pool->DeleteTexture(tex_rt);
	}

	return hr;
}

HRESULT my12doomRenderer::resize_surface(IDirect3DSurface9 *src, IDirect3DSurface9 *dst, RECT *src_rect /* = NULL */, RECT *dst_rect /* = NULL */)
{
	if (!src || !dst)
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

	src->GetDesc(&desc);
	RECTF s = {0,0,(float)desc.Width, (float)desc.Height};
	if (src_rect)
	{
		s.left = (float)src_rect->left;
		s.right = (float)src_rect->right;
		s.top = (float)src_rect->top;
		s.bottom = (float)src_rect->bottom;
	}

	return resize_surface(src, dst, false, &s, &d);
}


HRESULT my12doomRenderer::resize_surface(IDirect3DSurface9 *src, IDirect3DSurface9 *dst, bool one_pass, RECTF *src_rect /* = NULL */, RECTF *dst_rect /* = NULL */)
{
	if (!src || !dst)
		return E_POINTER;

	CComPtr<IDirect3DTexture9> tex;
	src->GetContainer(IID_IDirect3DTexture9, (void**)&tex);
	if (!tex)
		return E_INVALIDARG;

	// RECT caculate;
	D3DSURFACE_DESC desc;
	dst->GetDesc(&desc);
	RECTF d = {0,0,(float)desc.Width, (float)desc.Height};
	if (dst_rect)
		d = *dst_rect;

	src->GetDesc(&desc);
	RECTF s = {0,0,(float)desc.Width, (float)desc.Height};
	if (src_rect)
		s = *src_rect;


	// temp texture creation
	HRESULT hr = E_FAIL;
	CAutoLock lck(&m_pool_lock);
	CPooledTexture *tmp1 = NULL;
	hr = m_pool->CreateTexture(BIG_TEXTURE_SIZE, BIG_TEXTURE_SIZE, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tmp1);
	if (FAILED(hr))
	{
		safe_delete(tmp1);
		return hr;
	}

	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
// 	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
// 	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	// 
	float width_s = s.right - s.left;
	float height_s = s.bottom - s.top;
	float width_d = d.right - d.left;
	float height_d = d.bottom - d.top;

	if (!one_pass)
	{
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

		vertex[0].tu = (float)0 / desc.Width;
		vertex[0].tv = (float)0 / desc.Height;
		vertex[1].tu = (float)(width_s) / desc.Width;
		vertex[1].tv = (float)0 / desc.Height;
		vertex[2].tu = (float)0 / desc.Width;
		vertex[2].tv = (float)(height_s) / desc.Height;
		vertex[3].tu = (float)(width_s) / desc.Width;
		vertex[3].tv = (float)(height_s) / desc.Height;

		float ps[4] = {abs((float)width_d/width_s), abs((float)height_d/height_s), desc.Width, desc.Height};
		ps[0] = ps[0] > 1 ? 1 : ps[0];
		ps[1] = ps[1] > 1 ? 1 : ps[1];
		m_Device->SetPixelShaderConstantF(0, ps, 1);
		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		if (width_s != width_d && (IDirect3DPixelShader9*)m_lanczosX)
		{
			m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			m_Device->SetPixelShader(m_lanczosX);
		}
		hr = m_Device->SetTexture( 0, tex );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

		// pass2, Y filter
		m_Device->SetRenderTarget(0, dst);
		//m_Device->Clear(0L, (D3DRECT*)dst_rect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0L );
		clear(dst, D3DCOLOR_ARGB(0,0,0,0));
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
		if (height_s != height_d && (IDirect3DPixelShader9*)m_lanczosY)
		{
			ps[0] = (float)height_d/height_s;
			ps[0] = ps[0] > 0 ? ps[0] : -ps[0];
			ps[0] = ps[0] > 1 ? 1 : ps[0];
			ps[2] = ps[3] = BIG_TEXTURE_SIZE;
			m_Device->SetPixelShaderConstantF(0, ps, 1);
			m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			m_Device->SetPixelShader(m_lanczosY);
		}
		hr = m_Device->SetTexture( 0, tmp1->texture );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));
	}
	else
	{
		// pass0, 2D filter, rather slow, a little better quality
		m_Device->SetRenderTarget(0, dst);
		clear(dst, D3DCOLOR_ARGB(0,0,0,0));
		MyVertex vertex[4];
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

		vertex[0].tu = (float)s.left / desc.Width;
		vertex[0].tv = (float)s.top / desc.Height;
		vertex[1].tu = (float)s.right / desc.Width;
		vertex[1].tv = (float)s.top / desc.Height;
		vertex[2].tu = (float)s.left / desc.Width;
		vertex[2].tv = (float)s.bottom / desc.Height;
		vertex[3].tu = (float)s.right / desc.Width;
		vertex[3].tv = (float)s.bottom / desc.Height;

		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		if ((height_s != height_d || width_s != width_d)
			&& (IDirect3DPixelShader9*)m_lanczos)
		{
			m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			float ps[4] = {abs((float)width_d/width_s), abs((float)height_d/height_s), desc.Width, desc.Height};
			ps[0] = ps[0] > 1 ? 1 : ps[0];
			ps[1] = ps[1] > 1 ? 1 : ps[1];
			m_Device->SetPixelShaderConstantF(0, ps, 1);
			m_Device->SetPixelShader(m_lanczos);
		}
		else
			hr = m_Device->SetPixelShader(NULL);
		hr = m_Device->SetTexture( 0, tex );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));
		hr = m_Device->SetPixelShader(NULL);
	}
safe_delete(tmp1);

	return S_OK;
}

HRESULT my12doomRenderer::render(bool forced)
{
	if(m_output_mode == pageflipping)
		return S_FALSE;

	SetEvent(m_render_event);

	//CAutoLock lck(&m_frame_lock);
	//if (m_Device)
	//	return render_nolock(forced);
	//else
	//	return E_FAIL;
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
			//Sleep(33+(rand()%18));
			//Sleep(40+(rand()%24));
			//Sleep(50);
			//Sleep(33);

			/*
			LARGE_INTEGER start, current, frequency;
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&start);

			do 
			{
				QueryPerformanceCounter(&current);
				Sleep(0);
			} while (current.QuadPart - start.QuadPart < (frequency.QuadPart/29.15));
			*/

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
	if (!sample1)
		return E_POINTER;

	bool dual_stream = sample1 && sample2;
	input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
	bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;
	CLSID format = sample1->m_format;
	bool topdown = sample1->m_topdown;

	// try convert to RGB32 first
	HRESULT hr;
	CComPtr<IDirect3DSurface9> s1;
	CComPtr<IDirect3DSurface9> s2;
	CComPtr<IDirect3DSurface9> s3;
	if (sample1)
	{
		FAIL_RET(sample1->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time));
		if (need_detect) sample1->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, g_VertexBuffer);
		sample1->m_tex_gpu_RGB32->get_first_level(&s1);
		if (sample1->m_tex_stereo_test)
			sample1->m_tex_stereo_test->get_first_level(&s3);
	}
	if (sample2)
	{
		FAIL_RET(sample2->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time));
		sample2->m_tex_gpu_RGB32->get_first_level(&s2);
	}


	CAutoLock rendered_lock(&m_rendered_packet_lock);
	safe_delete(m_last_rendered_sample1);
	safe_delete(m_last_rendered_sample2);
	m_last_rendered_sample1 = m_sample2render_1;
	m_last_rendered_sample2 = m_sample2render_2;
	m_sample2render_1 = NULL;
	m_sample2render_2 = NULL;
	



	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = m_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = m_tex_rgb_right->GetSurfaceLevel(0, &right_surface);
	IDirect3DSurface9 *t1 = !m_swapeyes ? left_surface : right_surface;
	IDirect3DSurface9 *t2 = m_swapeyes ? left_surface : right_surface;
	if (dual_stream)
	{
		m_Device->StretchRect(s1, NULL, t1, NULL, D3DTEXF_LINEAR);
		m_Device->StretchRect(s2, NULL, t2, NULL, D3DTEXF_LINEAR);
	}
	else
	{

		RECT rect_left = {0,0,m_lVidWidth/2, m_lVidHeight};
		RECT rect_right = {m_lVidWidth/2,0,m_lVidWidth, m_lVidHeight};
		RECT rect_top = {0,0,m_lVidWidth, m_lVidHeight/2};
		RECT rect_bottom = {0,m_lVidHeight/2,m_lVidWidth, m_lVidHeight};

		if (input == side_by_side)
		{
			hr = m_Device->StretchRect(s1, &rect_left, t1, NULL, D3DTEXF_LINEAR);
			hr = m_Device->StretchRect(s1, &rect_right, t2, NULL, D3DTEXF_LINEAR);
		}
		else if (input == top_bottom)
		{
			hr = m_Device->StretchRect(s1, &rect_top, t1, NULL, D3DTEXF_LINEAR);
			hr = m_Device->StretchRect(s1, &rect_bottom, t2, NULL, D3DTEXF_LINEAR);
		}
		else if (input == mono2d)
		{
			hr = m_Device->StretchRect(s1, NULL, t1, NULL, D3DTEXF_LINEAR);
			hr = m_Device->StretchRect(s1, NULL, t2, NULL, D3DTEXF_LINEAR);
		}
	}

	m_backuped = false;

	// deinterlace on needed
	// we are using Blending method for now, it's very simple ,just StretchRect to a half height surface and StretchRect back, with BiLinear resizing
	if (m_forced_deinterlace)
	{
		m_Device->StretchRect(left_surface, NULL, m_deinterlace_surface, NULL, D3DTEXF_LINEAR);
		m_Device->StretchRect(m_deinterlace_surface, NULL, left_surface, NULL, D3DTEXF_LINEAR);
		m_Device->StretchRect(right_surface, NULL, m_deinterlace_surface, NULL, D3DTEXF_LINEAR);
		m_Device->StretchRect(m_deinterlace_surface, NULL, right_surface, NULL, D3DTEXF_LINEAR);
	}

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

enum helper_sample_format
{
	helper_sample_format_rgb32,
	helper_sample_format_yuy2,
	helper_sample_format_y,
	helper_sample_format_yv12,
	helper_sample_format_nv12,
};
IDirect3DTexture9* helper_get_texture(gpu_sample *sample, helper_sample_format format)
{
	if (!sample)
		return NULL;

	CPooledTexture *texture = NULL;
	if (format == helper_sample_format_rgb32) texture = sample->m_tex_RGB32;
	if (format == helper_sample_format_yuy2) texture = sample->m_tex_YUY2;
	if (format == helper_sample_format_y) texture = sample->m_tex_Y;
	if (format == helper_sample_format_yv12) texture = sample->m_tex_YV12_UV;
	if (format == helper_sample_format_nv12) texture = sample->m_tex_NV12_UV;

	if (format == helper_sample_format_rgb32) texture = sample->m_tex_gpu_RGB32;
	if (format == helper_sample_format_yuy2) texture = sample->m_tex_gpu_YUY2;
	if (format == helper_sample_format_y) texture = sample->m_tex_gpu_Y;
	if (format == helper_sample_format_yv12) texture = sample->m_tex_gpu_YV12_UV;
	if (format == helper_sample_format_nv12) texture = sample->m_tex_gpu_NV12_UV;

	if (!texture)
		return NULL;

	return texture->texture;
}

HRESULT my12doomRenderer::calculate_vertex()
{
	int l = timeGetTime();
	if (!m_vertex_changed)
		return S_FALSE;

	double active_aspect = get_active_aspect();

	CAutoLock lck(&m_frame_lock);

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
		tmp[1].x = m_lVidWidth-0.5f; tmp[1].y = -0.5f;
		tmp[2].x = -0.5f; tmp[2].y = m_lVidHeight-0.5f;
		tmp[3].x =  m_lVidWidth-0.5f; tmp[3].y =m_lVidHeight-0.5f;
	}

	// pass2-3 coordinate
	// main window coordinate
	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_display_orientation == vertical)
	{
		tar.right ^= tar.bottom;
		tar.bottom ^= tar.right;
		tar.right ^= tar.bottom;
	}

	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

	// ui zone
	MyVertex *ui = m_vertices + vertex_ui;
	ui[0].x = -0.5f; ui[0].y = tar.bottom-30-0.5f;
	ui[1].x = tar.right-0.5f; ui[1].y = ui[0].y;
	ui[2].x = ui[0].x; ui[2].y = ui[0].y + 30;
	ui[3].x = ui[1].x; ui[3].y = ui[1].y + 30;
	ui[0].tu = 0; ui[0].tv = (1024-30)/1024.0f;
	ui[1].tu = (float)(tar.right-1)/BIG_TEXTURE_SIZE; ui[1].tv = ui[0].tv;
	ui[2].tu = 0; ui[2].tv = ui[0].tv + (30-1)/1024.0f;
	ui[3].tu = ui[1].tu; ui[3].tv = ui[1].tv + (30-1)/1024.0f;

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


	int tar_width = tar.right-tar.left;
	int tar_height = tar.bottom - tar.top;
	tar.left += (LONG)(tar_width * m_bmp_offset_x);
	tar.right += (LONG)(tar_width * m_bmp_offset_x);
	tar.top += (LONG)(tar_height * m_bmp_offset_y);
	tar.bottom += (LONG)(tar_height * m_bmp_offset_y);

	MyVertex *pass2_main = m_vertices + vertex_pass2_main;

	if (m_display_orientation == horizontal)
	{
		pass2_main[0].x = tar.left-0.5f; pass2_main[0].y = tar.top-0.5f;
		pass2_main[1].x = tar.right-0.5f; pass2_main[1].y = tar.top-0.5f;
		pass2_main[2].x = tar.left-0.5f; pass2_main[2].y = tar.bottom-0.5f;
		pass2_main[3].x = tar.right-0.5f; pass2_main[3].y = tar.bottom-0.5f;
	}
	else
	{
		pass2_main[0].x = tar.top-0.5f; pass2_main[0].y = (float)m_active_pp.BackBufferHeight - tar.left-0.5f;
		pass2_main[1].x = tar.top-0.5f; pass2_main[1].y = (float)m_active_pp.BackBufferHeight - tar.right-0.5f;
		pass2_main[2].x = tar.bottom-0.5f; pass2_main[2].y = (float)m_active_pp.BackBufferHeight - tar.left-0.5f;
		pass2_main[3].x = tar.bottom-0.5f; pass2_main[3].y = (float)m_active_pp.BackBufferHeight - tar.right-0.5f;
	}

	MyVertex *pass2_main_r = m_vertices + vertex_pass2_main_r;
	memcpy(pass2_main_r, pass2_main, sizeof(MyVertex) * 4);

	if (m_parallax > 0)
	{
		// cut right edge of right eye and left edge of left eye
		pass2_main[0].tu += m_parallax;
		pass2_main[2].tu += m_parallax;
		pass2_main_r[1].tu -= m_parallax;
		pass2_main_r[3].tu -= m_parallax;

	}
	else if (m_parallax < 0)
	{
		// cut left edge of right eye and right edge of left eye
		pass2_main_r[0].tu += abs(m_parallax);
		pass2_main_r[2].tu += abs(m_parallax);
		pass2_main[1].tu -= abs(m_parallax);
		pass2_main[3].tu -= abs(m_parallax);
	}

	MyVertex *bmp = m_vertices + vertex_bmp;
	tar_width = tar.right-tar.left;
	tar_height = tar.bottom - tar.top;
	bmp[0] = pass2_main[0];
	bmp[1] = pass2_main[1];
	bmp[2] = pass2_main[2];
	bmp[3] = pass2_main[3];
	//bmp[0].x += m_bmp_fleft * tar_width; bmp[0].y += m_bmp_ftop * tar_height;
	//bmp[1] = bmp[0]; bmp[1].x += m_bmp_fwidth * tar_width;
	//bmp[3] = bmp[1]; bmp[3].y += m_bmp_fheight* tar_height;
	//bmp[2] = bmp[3]; bmp[2].x -= m_bmp_fwidth * tar_width;

	bmp[0].x = 0; bmp[0].y = 0;
	bmp[1].x = 1; bmp[1].y = 0;
	bmp[2].x = 0; bmp[2].y = 1;
	bmp[3].x = 1; bmp[3].y = 1;

	//bmp[0].tu = 0; bmp[0].tv = 0;
	//bmp[1].tu = (m_bmp_width-1)/BIG_TEXTURE_SIZE.0f; bmp[1].tv = 0;
	//bmp[2].tu = 0; bmp[2].tv = (m_bmp_height-1)/1024.0f;
	//bmp[3].tu = (m_bmp_width-1)/BIG_TEXTURE_SIZE.0f; bmp[3].tv = (m_bmp_height-1)/1024.0f;
	bmp[0].tu = 0; bmp[0].tv = 0;
	bmp[1].tu = 1; bmp[1].tv = 0;
	bmp[2].tu = 0; bmp[2].tv = 1;
	bmp[3].tu = 1; bmp[3].tv = 1;

	MyVertex *bmp2 = m_vertices + vertex_bmp2;
	for(int i=0; i<4; i++)
	{
		bmp2[i] = bmp[i];
		//bmp2[i].x -= tar_width * (m_bmp_offset);
	}

	MyVertex *pass3 = m_vertices + vertex_pass3;
	pass3[0].x = -0.5f; pass3[0].y = -0.5f;
	pass3[1].x = m_active_pp.BackBufferWidth-0.5f; pass3[1].y = -0.5f;
	pass3[2].x = -0.5f; pass3[2].y = m_active_pp.BackBufferHeight-0.5f;
	pass3[3].x = m_active_pp.BackBufferWidth-0.5f; pass3[3].y = m_active_pp.BackBufferHeight-0.5f;

	// second window coordinate
	// not used
	tar.left = tar.top = 0;
	tar.right = m_active_pp2.BackBufferWidth;
	tar.bottom = m_active_pp2.BackBufferHeight;
	delta_w = (int)(tar.right - tar.bottom * active_aspect + 0.5);
	delta_h = (int)(tar.bottom - tar.right  / active_aspect + 0.5);
	if (delta_w > 0)
	{
		// letterbox at left and right

		tar.left += delta_w/2;
		tar.right -= delta_w/2;
	}
	else if (delta_h > 0)
	{
		// letterbox at top and bottom
		tar.top += delta_h/2;
		tar.bottom -= delta_h/2;
	}

	tar_width = tar.right-tar.left;
	tar_height = tar.bottom - tar.top;

	// movie position offset
	tar.left += (LONG)(tar_width * m_bmp_offset_x);
	tar.right += (LONG)(tar_width * m_bmp_offset_x);
	tar.top += (LONG)(tar_height * m_bmp_offset_y);
	tar.bottom += (LONG)(tar_height * m_bmp_offset_y);

	MyVertex *test_sbs = m_vertices + vertex_test_sbs;
	test_sbs[0].x = -0.5f; test_sbs[0].y = -0.5f;
	test_sbs[1].x = stereo_test_texture_size/2-0.5f; test_sbs[1].y = -0.5f;
	test_sbs[2].x = -0.5f; test_sbs[2].y = test_sbs[0].y + stereo_test_texture_size;
	test_sbs[3].x = stereo_test_texture_size/2-0.5f; test_sbs[3].y = test_sbs[1].y + stereo_test_texture_size;
	test_sbs[0].tu = 0; test_sbs[0].tv = 0;
	test_sbs[1].tu = 1.0f; test_sbs[1].tv = 0;
	test_sbs[2].tu = 0; test_sbs[2].tv = 1.0f;
	test_sbs[3].tu = 1.0f; test_sbs[3].tv = 1.0f;

	MyVertex *test_tb = m_vertices + vertex_test_tb;
	for(int i=0; i<4; i++)
	{
		test_tb[i] = test_sbs[i];
		test_tb[i].x += stereo_test_texture_size/2;
	}

	if (!g_VertexBuffer)
		return S_FALSE;

	int l2 = timeGetTime();

	void *pVertices = NULL;
	g_VertexBuffer->Lock( 0, sizeof(m_vertices), (void**)&pVertices, NULL );
	memcpy( pVertices, m_vertices, sizeof(m_vertices) );
	g_VertexBuffer->Unlock();
	m_vertex_subtitle->Lock( 0, sizeof(m_vertices), (void**)&pVertices, NULL );
	memcpy( pVertices, m_vertices, sizeof(m_vertices) );
	m_vertex_subtitle->Unlock();

	mylog("calculate_vertex(): calculate = %dms, Lock()=%dms.\n", l2-l, timeGetTime()-l2);
	m_vertex_changed = false;
	return S_OK;
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
	set_device_state(need_reset_object);
	handle_device_state();
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

	m_vertex_changed = true;
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
	m_vertex_changed = true;
	repaint_video();
	if (m_output_mode == pageflipping)
		create_render_thread();
	return S_OK;

}

HRESULT my12doomRenderer::set_aspect(double aspect)
{
	m_forced_aspect = aspect;
	m_vertex_changed = true;
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

HRESULT my12doomRenderer::set_bmp_offset(double offset)
{
	if (m_bmp_offset != offset)
	{
		m_bmp_offset = offset;
		m_vertex_changed = true;
		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_parallax(double parallax)
{
	if (m_parallax != parallax)
	{
		m_parallax = parallax;
		m_vertex_changed = true;
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

	hr = DXVA2CreateDirect3DDeviceManager9(&m_resetToken, &m_d3d_manager);
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
	//pp2.BackBufferWidth = m_intel_active_3d_mode.ulResWidth;
	//pp2.BackBufferHeight = m_intel_active_3d_mode.ulResHeight;
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
	FAIL_RET(DXVA2CreateVideoService(m_Device, IID_IDirectXVideoProcessorService, (void**)&g_dxva_service));
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


gpu_sample::~gpu_sample()
{
	safe_delete(m_tex_RGB32);
	safe_delete(m_tex_YUY2);
	safe_delete(m_tex_Y);
	safe_delete(m_tex_YV12_UV);
	safe_delete(m_tex_NV12_UV);

	safe_delete(m_tex_gpu_RGB32);
	safe_delete(m_tex_gpu_YUY2);
	safe_delete(m_tex_gpu_Y);
	safe_delete(m_tex_gpu_YV12_UV);
	safe_delete(m_tex_gpu_NV12_UV);

	safe_delete(m_tex_stereo_test);
	safe_delete(m_tex_stereo_test_cpu);

	safe_delete(m_surf_YV12);
	safe_delete(m_surf_NV12);
	safe_delete(m_surf_YUY2);

	safe_delete(m_surf_gpu_YV12);
	safe_delete(m_surf_gpu_NV12);
	safe_delete(m_surf_gpu_YUY2);
}

CCritSec g_gpu_lock;
HRESULT gpu_sample::commit()
{
// 	LARGE_INTEGER counter1, counter2, fre;
// 	QueryPerformanceCounter(&counter1);
// 	QueryPerformanceFrequency(&fre);

	if (m_prepared_for_rendering)
		return S_FALSE;

	if (m_tex_RGB32) 
		m_tex_RGB32->Unlock();

	if (m_tex_YUY2) 
		m_tex_YUY2->Unlock();

	if (m_tex_Y)
		m_tex_Y->Unlock();

	if (m_tex_YV12_UV)
		m_tex_YV12_UV->Unlock();

	if (m_tex_NV12_UV) 
		m_tex_NV12_UV->Unlock();

	if (m_surf_YV12)
		m_surf_YV12->Unlock();
	if (m_surf_NV12)
		m_surf_NV12->Unlock();
	if (m_surf_YUY2)
		m_surf_YUY2->Unlock();

	HRESULT hr = S_OK;

	JIF( m_allocator->CreateTexture(m_width, m_height, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_gpu_RGB32));

	if (m_format == MEDIASUBTYPE_YUY2)
	{
		JIF( m_allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,	&m_tex_gpu_YUY2));
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{
		JIF( m_allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
		JIF( m_allocator->CreateTexture(m_width/2, m_height/2, NULL, D3DFMT_A8L8,D3DPOOL_DEFAULT,	&m_tex_gpu_NV12_UV));
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		JIF( m_allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
		JIF( m_allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_YV12_UV));
	}

	if (m_tex_RGB32) JIF(m_allocator->UpdateTexture(m_tex_RGB32, m_tex_gpu_RGB32));
	if (m_tex_YUY2) JIF(m_allocator->UpdateTexture(m_tex_YUY2, m_tex_gpu_YUY2));
	if (m_tex_Y) JIF(m_allocator->UpdateTexture(m_tex_Y, m_tex_gpu_Y));
	if (m_tex_YV12_UV) JIF(m_allocator->UpdateTexture(m_tex_YV12_UV, m_tex_gpu_YV12_UV));
	if (m_tex_NV12_UV) JIF(m_allocator->UpdateTexture(m_tex_NV12_UV, m_tex_gpu_NV12_UV));


	m_prepared_for_rendering = true;
// 	QueryPerformanceCounter(&counter2);
// 
// 	mylog("prepare_rendering() cost %d cycle(%.3fms).\n", (int)(counter2.QuadPart - counter1.QuadPart), (double)(counter2.QuadPart-counter1.QuadPart)/fre.QuadPart);

	return S_OK;

clearup:
	decommit();
	mylog("%08x commit() fail", this);
	return E_FAIL;
}

HRESULT gpu_sample::decommit()
{
	if (!m_prepared_for_rendering)
		return S_FALSE;

	safe_delete(m_tex_gpu_RGB32);
	safe_delete(m_tex_gpu_YUY2);
	safe_delete(m_tex_gpu_Y);
	safe_delete(m_tex_gpu_YV12_UV);
	safe_delete(m_tex_gpu_NV12_UV);
	safe_delete(m_tex_stereo_test);
	safe_delete(m_surf_gpu_YV12);
	safe_delete(m_surf_gpu_NV12);
	safe_delete(m_surf_gpu_YUY2);

	m_converted = m_prepared_for_rendering = false;
	return S_OK;
}

HRESULT gpu_sample::convert_to_RGB32(IDirect3DDevice9 *device, IDirect3DPixelShader9 *ps_yv12, IDirect3DPixelShader9 *ps_nv12, IDirect3DPixelShader9 *ps_yuy2,
									 IDirect3DVertexBuffer9 *vb, int time)
{
	if (!m_ready)
		return VFW_E_WRONG_STATE;

	if (m_converted)
		return S_FALSE;

	HRESULT hr = commit();
	if (FAILED(hr))
		return hr;

	if (m_format == MEDIASUBTYPE_RGB32)
		return S_FALSE;

	if (m_StretchRect)
	{
		CComPtr<IDirect3DSurface9> rgb_surf;
		m_tex_gpu_RGB32->get_first_level(&rgb_surf);
		if (m_surf_YV12)
			hr = device->StretchRect(m_surf_YV12->surface, NULL, rgb_surf, NULL, D3DTEXF_LINEAR);
		if (m_surf_NV12)
			hr = device->StretchRect(m_surf_NV12->surface, NULL, rgb_surf, NULL, D3DTEXF_LINEAR);
		if (m_surf_YUY2)
			hr = device->StretchRect(m_surf_YUY2->surface, NULL, rgb_surf, NULL, D3DTEXF_LINEAR);
	}
	else
	{
		CComPtr<IDirect3DSurface9> rt;
		m_tex_gpu_RGB32->get_first_level(&rt);
		device->SetRenderTarget(0, rt);

		device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		hr = device->SetPixelShader(NULL);
		if (m_format == MEDIASUBTYPE_YV12) hr = device->SetPixelShader(ps_yv12);
		if (m_format == MEDIASUBTYPE_NV12) hr = device->SetPixelShader(ps_nv12);
		if (m_format == MEDIASUBTYPE_YUY2) hr = device->SetPixelShader(ps_yuy2);
		float rect_data[8] = {m_width, m_height, m_width/2, m_height, (float)time/100000, (float)timeGetTime()/100000};
		hr = device->SetPixelShaderConstantF(0, rect_data, 2);
		hr = device->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		CComPtr<IDirect3DSurface9> left_surface;
		CComPtr<IDirect3DSurface9> right_surface;

		// drawing
		hr = device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = device->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = device->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_NONE );

		hr = device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

		hr = device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = device->SetStreamSource( 0, vb, 0, sizeof(MyVertex) );
		hr = device->SetFVF( FVF_Flags );

		hr = device->SetTexture( 0, m_format == MEDIASUBTYPE_YUY2 ? m_tex_gpu_YUY2->texture : m_tex_gpu_Y->texture );
		if (m_format == MEDIASUBTYPE_YV12 || m_format == MEDIASUBTYPE_NV12)
			hr = device->SetTexture( 1, m_format == MEDIASUBTYPE_NV12 ? m_tex_gpu_NV12_UV->texture : m_tex_gpu_YV12_UV->texture);
		hr = device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );
	}

	if (SUCCEEDED(hr))
		m_converted = true;
	return hr;
}

HRESULT gpu_sample::do_stereo_test(IDirect3DDevice9 *device, IDirect3DPixelShader9 *shader_sbs, IDirect3DPixelShader9 *shader_tb, IDirect3DVertexBuffer9 *vb)
{
	if (m_cpu_stereo_tested)
		return S_FALSE;

	if (!device || !shader_sbs || !shader_tb ||!vb)
		return E_POINTER;

	if (m_tex_stereo_test)
		return S_FALSE;

	HRESULT hr;
	if (FAILED( hr = m_allocator->CreateTexture(stereo_test_texture_size, stereo_test_texture_size, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_stereo_test)))
		return hr;

	CComPtr<IDirect3DSurface9> rt;
	m_tex_stereo_test->get_first_level(&rt);
	device->SetRenderTarget(0, rt);

	device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	hr = device->SetRenderState(D3DRS_LIGHTING, FALSE);
	hr = device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;

	// drawing
	hr = device->SetStreamSource( 0, vb, 0, sizeof(MyVertex) );
	hr = device->SetFVF( FVF_Flags );


	hr = device->SetTexture( 0, m_tex_gpu_RGB32->texture );
	hr = device->SetPixelShader(shader_sbs);
	hr = device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_test_sbs, 2 );
	hr = device->SetPixelShader(shader_tb);
	hr = device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_test_tb, 2 );

	//hr = device->EndScene();

	mylog("test:%x:%d\n", this, timeGetTime());

	return S_OK;
}

HRESULT gpu_sample::get_strereo_test_result(IDirect3DDevice9 *device, int *out)
{
	if (m_cpu_stereo_tested)
	{
		*out = m_cpu_tested_result;
		return m_cpu_tested_result == input_layout_auto ? S_FALSE : S_OK;
	}

	if (!m_tex_stereo_test)
		return E_FAIL;	//call do_stereo_test() first

	HRESULT hr;
	HRESULT rtn = S_FALSE;
	bool do_transition = false;
	if (!m_tex_stereo_test_cpu)
	{
		if (FAILED( hr = m_allocator->CreateTexture(stereo_test_texture_size, stereo_test_texture_size, NULL, D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,	&m_tex_stereo_test_cpu)))
			return hr;

		do_transition = true;
	}

	m_tex_stereo_test_cpu->Unlock();

	mylog("get:%x:%d\n", this, timeGetTime());

	CComPtr<IDirect3DSurface9> gpu;
	CComPtr<IDirect3DSurface9> cpu;
	m_tex_stereo_test->get_first_level(&gpu);
	m_tex_stereo_test_cpu->get_first_level(&cpu);

	if (do_transition)
	{
		int l = timeGetTime();
		hr = device->GetRenderTargetData(gpu, cpu);
		if (timeGetTime()-l>3) mylog("GetRenderTargetData() cost %dms.\n", timeGetTime()-l);
	}

	D3DLOCKED_RECT locked;
	hr = m_tex_stereo_test_cpu->texture->LockRect(0, &locked, NULL, D3DLOCK_READONLY);
	if (FAILED(hr))
		return hr;

	lockrect_surface ++;

	BYTE* src = (BYTE*)locked.pBits;
	double average1 = 0;
	double average2 = 0;
	double delta1 = 0;
	double delta2 = 0;
	for(int y=0; y<stereo_test_texture_size; y++)
		for(int x=0; x<stereo_test_texture_size; x++)
		{
			double &average = x<stereo_test_texture_size/2 ?average1:average2;
			average += src[2];
			src += 4;
		}

	average1 /= stereo_test_texture_size/2*stereo_test_texture_size;
	average2 /= stereo_test_texture_size/2*stereo_test_texture_size;

	src = (BYTE*)locked.pBits;
	for(int y=0; y<stereo_test_texture_size; y++)
		for(int x=0; x<stereo_test_texture_size; x++)
		{
			double &average = x<stereo_test_texture_size/2 ?average1:average2;
			double &tdelta = x<stereo_test_texture_size/2 ? delta1 : delta2;

			int delta = abs(src[2] - average);
			tdelta += delta * delta;
			src += 4;
		}


	delta1 = sqrt((double)delta1)/(stereo_test_texture_size/2*stereo_test_texture_size-1);
	delta2 = sqrt((double)delta2)/(stereo_test_texture_size/2*stereo_test_texture_size-1);

	double times = 0;
	double var1 = average1 * delta1;
	double var2 = average2 * delta2;
	if ( (var1 > 0.001 && var2 > 0.001) || (var1>var2*10000) || (var2>var1*10000))
		times = var1 > var2 ? var1 / var2 : var2 / var1;

	printf("%f - %f, %f - %f, %f - %f, %f\r\n", average1, average2, delta1, delta2, var1, var2, times);

	if (times > 31.62/2)		// 10^1.5
	{
		mylog("stereo(%s).\r\n", var1 > var2 ? "tb" : "sbs");
		rtn = S_OK;
		*out = var1>var2 ? top_bottom : side_by_side;
	}
	else if ( 1.0 < times && times < 4.68 )
	{
		//m_normal ++;
		mylog("normal.\r\n");
		rtn = S_OK;
		*out = mono2d;
	}
	else
	{
		rtn = S_FALSE;
		mylog("unkown.\r\n");
	}
	m_tex_stereo_test_cpu->texture->UnlockRect(0);

	return rtn;
}

bool gpu_sample::is_ignored_line(int line)
{
	if (line == 1 || line == 136 || line == 272 || line == 408 || line == 544 || line == 680 || line == 816 || line == 952)
		return true;
	return false;
}

gpu_sample::gpu_sample(IMediaSample *memory_sample, CTextureAllocator *allocator, int width, int height, CLSID format,
					   bool topdown_RGB32, bool do_cpu_test, bool remux_mode, D3DPOOL pool, DWORD PC_LEVEL)
{
	//CAutoLock lck(&g_gpu_lock);
	m_allocator = allocator;
	m_interlace_flags = 0;
	m_tex_RGB32 = m_tex_YUY2 = m_tex_Y = m_tex_YV12_UV = m_tex_NV12_UV = NULL;
	m_tex_gpu_RGB32 = m_tex_gpu_YUY2 = m_tex_gpu_Y = m_tex_gpu_YV12_UV = m_tex_gpu_NV12_UV = NULL;
	m_surf_YV12 = m_surf_NV12 = m_surf_YUY2 = NULL;
	m_surf_gpu_YV12 = m_surf_gpu_NV12 = m_surf_gpu_YUY2 = NULL;
	m_tex_stereo_test = m_tex_stereo_test_cpu = NULL;
	m_width = width;
	m_height = height;
	m_ready = false;
	m_format = format;
	m_topdown = topdown_RGB32;
	m_prepared_for_rendering = false;
	m_converted = false;
	m_cpu_stereo_tested = false;
	m_cpu_tested_result = input_layout_auto;		// means unknown
	HRESULT hr;
	CComQIPtr<IMediaSample2, &IID_IMediaSample2> I2(memory_sample);
	if (!allocator || !memory_sample)
		goto clearup;

	if (remux_mode && format != MEDIASUBTYPE_YUY2)
		goto clearup;

	// time stamp
	memory_sample->GetTime(&m_start, &m_end);

	// interlace flags
	if (I2)
	{
		AM_SAMPLE2_PROPERTIES prop;
		if (SUCCEEDED(I2->GetProperties(sizeof(prop), (BYTE*) &prop)))
		{
			m_interlace_flags = prop.dwTypeSpecificFlags;
		}

		//printf("sample interlace_flag: %02x.\n", m_interlace_flags);
	}


	int l = timeGetTime();

	m_ready = true;
	//return;

	m_StretchRect = false;
	if (m_format == MEDIASUBTYPE_YUY2)
	{
		if ((PC_LEVEL & PCLEVELTEST_TESTED) && (PC_LEVEL &PCLEVELTEST_YUY2))
		{
			m_StretchRect = true;
			JIF( allocator->CreateOffscreenSurface(m_width, m_height, (D3DFORMAT)MAKEFOURCC('Y','U','Y','2'), D3DPOOL_DEFAULT, &m_surf_YUY2));
		}
		else
		{
			JIF( allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_A8R8G8B8,pool,	&m_tex_YUY2));
// 			JIF( allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,	&m_tex_gpu_YUY2));
		}
	}

	else if (m_format == MEDIASUBTYPE_RGB32)
	{
		JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_A8R8G8B8,pool,	&m_tex_RGB32));
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{
		if ((PC_LEVEL & PCLEVELTEST_TESTED) && (PC_LEVEL &PCLEVELTEST_NV12))
		{
			m_StretchRect = true;
			JIF( allocator->CreateOffscreenSurface(m_width, m_height, (D3DFORMAT)MAKEFOURCC('N','V','1','2'), D3DPOOL_DEFAULT, &m_surf_NV12));
		}
		else
		{
			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,pool,	&m_tex_Y));
			JIF( allocator->CreateTexture(m_width/2, m_height/2, NULL, D3DFMT_A8L8,pool,	&m_tex_NV12_UV));
// 			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
// 			JIF( allocator->CreateTexture(m_width/2, m_height/2, NULL, D3DFMT_A8L8,D3DPOOL_DEFAULT,	&m_tex_gpu_NV12_UV));
		}
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		if ((PC_LEVEL & PCLEVELTEST_TESTED) && (PC_LEVEL &PCLEVELTEST_YV12))
		{
			m_StretchRect = true;
			JIF( allocator->CreateOffscreenSurface(m_width, m_height, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &m_surf_YV12));
		}
		else
		{
			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,pool,	&m_tex_Y));
			JIF( allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_L8,pool,	&m_tex_YV12_UV));
// 			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
// 			JIF( allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_YV12_UV));
		}
	}

	m_pool = pool;
	m_ready = true;


	int l2 = timeGetTime();

	// data loading
	BYTE * src;
	BYTE * dst;
	memory_sample->GetPointer(&src);
	if (m_format == MEDIASUBTYPE_RGB32)
	{
		for(int i=0; i<m_height; i++)
		{
			D3DLOCKED_RECT &d3dlr = m_tex_RGB32->locked_rect;
			memory_sample->GetPointer(&src);
			src += m_width*4*(topdown_RGB32?m_height-i-1:i);
			dst = (BYTE*)d3dlr.pBits + d3dlr.Pitch*i;

			memcpy(dst, src, m_width*4);
		}
		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<DWORD>(src, width, height, (int*)&m_cpu_tested_result);
	}

	else if (m_format == MEDIASUBTYPE_YUY2)
	{
		// loading YUY2 image as one ARGB half width texture
		D3DLOCKED_RECT d3dlr;
		if (m_StretchRect)
			d3dlr = m_surf_YUY2->locked_rect;
		else
			d3dlr = m_tex_YUY2->locked_rect;
		dst = (BYTE*)d3dlr.pBits;
		int j = 0;
		for(int i=0; i<m_height; i++)
		{
			memcpy(dst, src, m_width*2);
			src += m_width*2;
			dst += d3dlr.Pitch;

			j++;
			if (is_ignored_line(j) && height == 1080 && remux_mode)
			{
				j++;
				src += m_width*2;
			}

		}

		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<WORD>(src, width, height, (int*)&m_cpu_tested_result);

	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{

		if (m_StretchRect)
		{
			D3DLOCKED_RECT &d3dlr = m_surf_NV12->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height*3/2; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}
		}

		else
		{
			// loading NV12 image as one L8 texture and one A8L8 texture
			// load Y
			D3DLOCKED_RECT &d3dlr = m_tex_Y->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}

			// load UV
			D3DLOCKED_RECT &d3dlr2 = m_tex_NV12_UV->locked_rect;
			dst = (BYTE*)d3dlr2.pBits;
			for(int i=0; i<m_height/2; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr2.Pitch;
			}
		}

		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<BYTE>(src, width, height, (int*)&m_cpu_tested_result);
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		if (m_StretchRect)
		{
			D3DLOCKED_RECT &d3dlr = m_surf_YV12->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width/2);
				src += m_width/2;
				dst += d3dlr.Pitch/2;
			}
		}

		else
		{
			// loading YV12 image as two L8 texture
			// load Y
			D3DLOCKED_RECT &d3dlr = m_tex_Y->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}

			// load UV
			D3DLOCKED_RECT &d3dlr2 = m_tex_YV12_UV->locked_rect;
			dst = (BYTE*)d3dlr2.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width/2);
				src += m_width/2;
				dst += d3dlr2.Pitch;
			}
		}

		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<BYTE>(src, width, height, (int*)&m_cpu_tested_result);
	}

	m_cpu_stereo_tested = do_cpu_test;


	//prepare_rendering();
	//if (timeGetTime() - l > 5)
		//mylog("load():createTexture time:%d ms, load data to GPU cost %d ms.\n", l2- l, timeGetTime()-l2);

	return;

clearup:
	safe_delete(m_tex_RGB32);
	safe_delete(m_tex_YUY2);
	safe_delete(m_tex_Y);
	safe_delete(m_tex_YV12_UV);
	safe_delete(m_tex_NV12_UV);
}

my12doom_auto_shader::my12doom_auto_shader()
{
	m_has_key = false;
	m_device = NULL;
	m_data = NULL;
	m_key = (DWORD*)malloc(32);
}

HRESULT my12doom_auto_shader::set_source(IDirect3DDevice9 *device, const DWORD *data, int datasize, bool is_ps, DWORD *aes_key)
{
	if (!device || !data)
		return E_POINTER;

	if (m_data) free(m_data);
	m_data = NULL;

	m_device = device;
	m_is_ps = is_ps;
	m_has_key = false;
	m_data = (DWORD *) malloc(datasize);
	m_datasize = datasize;
	memcpy(m_data, data, datasize);
	if (aes_key)
	{
		memcpy(m_key, aes_key, 32);
		m_has_key = true;
	}

	invalid();
	return S_OK;
}

my12doom_auto_shader::~my12doom_auto_shader()
{
	if (m_data) free(m_data);
	free(m_key);
	m_data = NULL;
}

HRESULT my12doom_auto_shader::invalid()
{
	m_ps = NULL;
	m_vs = NULL;

	return S_OK;
}

HRESULT my12doom_auto_shader::restore()
{
	DWORD * data = m_data;
	if (m_has_key)
	{
		data = (DWORD*)malloc(m_datasize);
		memcpy(data, m_data, m_datasize);

		AESCryptor aes;
		aes.set_key((unsigned char*)m_key, 256);
		for(int i=0; i<(m_datasize/16*16)/4; i+=4)
			aes.decrypt((unsigned char*)(data+i), (unsigned char*)(data+i));
	}
	HRESULT hr = E_FAIL;
	if (m_is_ps)
		hr = m_device->CreatePixelShader(data, &m_ps);
	else 
		hr = m_device->CreateVertexShader(data, &m_vs);

	if (m_has_key)
		free(data);

	return hr;
}

my12doom_auto_shader::operator IDirect3DPixelShader9*()
{
	if (!m_is_ps)
		return NULL;
	if (!m_ps)
		restore();
	return m_ps;
}

my12doom_auto_shader::operator IDirect3DVertexShader9*()
{
	if (m_is_ps)
		return NULL;
	if (!m_vs)
		restore();
	return m_vs;
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
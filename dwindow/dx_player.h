#pragma once
// COM & DShow
#include <atlbase.h>
#include <dshow.h>

// renderer
#include "UnifyRenderer.h"


#include "streams.h"
#include "dwindow.h"
#include "bar.h"
#include "CSubtitle.h"
#include "srt\srt_parser.h"
#include "..\mySplitter\filter.h"
#include "PGS\PGSParser.h"

#define MKV_NO_TRACK -1
#define MKV_ALL_TRACK -2
#define MKV_TRACK_NUMBER(x) (0x01 << x)
#define MKV_FIRST_TRACK MKV_TRACK_NUMBER(0)

#define FILTER_MODE_FAIL 0
#define FILTER_MODE_MONO 1
#define FILTER_MODE_STEREO 2

DEFINE_GUID(CLSID_HaaliSimple, 
			0x8F43B7D9, 0x9D6B, 0x4F48, 0xBE, 0x18, 0x4D, 0x78, 0x7C, 0x79, 0x5E, 0xEA);

// {0A68C3B5-9164-4A54-AFAF-995B2FF0E0D4}
DEFINE_GUID(CLSID_E3DSource, 
			0x0A68C3B5, 0x9164, 0x4A54, 0xAF, 0xAF, 0x99, 0x5B, 0x2F, 0xF0, 0xE0, 0xD4);

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// {FBCFD6AF-B25F-4a6d-AE56-5B5303F1A40E}
DEFINE_GUID(CLSID_3dvSource, 
			0xfbcfd6af, 0xb25f, 0x4a6d, 0xae, 0x56, 0x5b, 0x53, 0x3, 0xf1, 0xa4, 0xe);

static const GUID CLSID_SSIFSource = { 0x916e4c8d, 0xe37f, 0x4fd4, { 0x95, 0xf6, 0xa4, 0x4e, 0x51, 0x46, 0x2e, 0xdf } };


class dx_window : public IDWindowFilterCB, public dwindow
{
public:
	dx_window(RECT screen1, RECT screen2, HINSTANCE hExe);
	~dx_window();
	// load functions
	HRESULT start_loading();
	HRESULT load_subtitle(wchar_t *pathname, bool reset = true);
	HRESULT load_file(wchar_t *pathname, int audio_track = MKV_FIRST_TRACK, int video_track = MKV_ALL_TRACK);			// for multi stream mkv
	HRESULT load_BD3D_file(wchar_t *pathname);
	HRESULT load_mkv_file(wchar_t *pathname, int audio_track = MKV_FIRST_TRACK, int video_track = MKV_ALL_TRACK);			// for multi stream mkv
	HRESULT load_REMUX_file(wchar_t *pathname);																				// for PD10 demuxer and mvc decoder
	HRESULT load_E3D_file(wchar_t *pathname, int audio_track = MKV_FIRST_TRACK, int video_track = MKV_ALL_TRACK);			// for multi stream mkv
	HRESULT load_3dv_file(wchar_t *pathname, int audio_track = MKV_FIRST_TRACK, int video_track = MKV_ALL_TRACK);			// for multi stream mkv
	HRESULT end_loading();
	HRESULT set_PD10(bool PD10 = false);

	// subtitle control functions
	HRESULT select_font(bool show_dlg = true);
	HRESULT set_subtitle_pos(double center_x, double bottom_y);
	HRESULT set_subtitle_offset(int offset);

	// image control functions
	HRESULT set_revert(bool revert);
	HRESULT set_letterbox(double delta);	// 1.0 = top, -1.0 = bottom, 0 = center

	// show property page for PD10 demuxer
	HRESULT show_PD10_demuxer_config();

	// play control functions
	HRESULT play();
	HRESULT pause();
	HRESULT stop();
	HRESULT seek(int time);
	HRESULT tell(int *time);
	HRESULT total(int *time);
	HRESULT set_volume(double volume);			// 0 - 1.0 = 0% - 100%
	HRESULT get_volume(double *volume);
	bool is_closed();
	POINT m_mouse;

	// filter callback function
	HRESULT SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn);

	// error reporting vars and functions
	HRESULT log_line(wchar_t *format, ...);
	wchar_t *m_log;

	// PGS test
	PGSParser m_pgs;
protected:
	// image control vars
	HWND m_video1;
	HWND m_video2;
	HWND id_to_video(int id);
	int video_to_id(HWND video);

	int m_mirror1;
	int m_mirror2;			// 0x0:no mirror, 0x1 mirror horizontal, 0x2 mirror vertical, 0x3(0x3=0x1|0x2) mirror both
	bool m_revert;
	double m_letterbox_delta;

	// helper function and vars
	HRESULT CrackPD10(IBaseFilter *filter);
	HRESULT calculate_movie_rect(RECT *source, RECT *client, RECT *letterbox, bool ui);
	HRESULT paint_letterbox(int id, RECT letterbox);
	bool m_demuxer_config_active;
	static DWORD WINAPI property_page_thread(LPVOID lpParame);
	bool m_select_font_active;
	static DWORD WINAPI select_font_thread(LPVOID lpParame);


	// window handler
	LRESULT on_display_change();
	LRESULT on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT on_sys_command(int id, WPARAM wParam, LPARAM lParam);	// don't block WM_SYSCOMMAND
	LRESULT on_getminmaxinfo(int id, MINMAXINFO *info);
	LRESULT on_move(int id, int x, int y);
	LRESULT on_mouse_move(int id, int x, int y);
	LRESULT on_nc_mouse_move(int id, int x, int y);
	LRESULT on_mouse_down(int id, int button, int x, int y);
	//LRESULT on_mouse_up(int id, int button, int x, int y);
	LRESULT on_double_click(int id, int button, int x, int y);
	LRESULT on_key_down(int id, int key);
	//LRESULT on_close(int id);
	//LRESULT on_display_change();
	LRESULT on_paint(int id, HDC hdc);
	LRESULT on_timer(int id);
	LRESULT on_size(int id, int type, int x, int y);
	LRESULT on_init_dialog(int id, WPARAM wParam, LPARAM lParam);		// buged
	// end window handler

	// helper
	int hwnd_to_id(HWND hwnd);


	// directshow etc. core part
	bar_drawer m_bar;
	CCritSec m_draw_sec;
	CCritSec m_seek_sec;
	bool m_show_ui;
	HRESULT show_ui(bool show);
	HRESULT draw_ui();

	HRESULT on_dshow_event();		//"on move window"
	HRESULT update_video_pos();		//"on move window"
	HRESULT init_direct_show();
	HRESULT exit_direct_show();
	HRESULT init_D3D9();
	HRESULT exit_D3D9();
	HRESULT draw_text(wchar_t *text);
	HRESULT end_loading_sidebyside(IPin **pin1, IPin **pin2);
	HRESULT end_loading_double_stream(IPin **pin1, IPin **pin2);
	HRESULT end_loading_step2(IPin *pin1, IPin *pin2);
	bool m_loading;

	// basic directshow vars
	DWORD rot_id;
	CComPtr<IGraphBuilder>		m_gb;
	CComPtr<IMediaSeeking>		m_ms;
	CComPtr<IMediaControl>		m_mc;
	CComPtr<IBasicAudio>		m_ba;

	// renderer
	int m_lastCBtime;
	CSubtitleRenderer *m_srenderer;
	CUnifyRenderer *m_renderer1;
	CUnifyRenderer *m_renderer2;
	BYTE *m_subtile_bitmap;

	// VMR filter vars
	/*
	CComPtr<IVMRWindowlessControl9> m_vmr1c;
	CComPtr<IVMRWindowlessControl9> m_vmr2c;
	CComPtr<IVMRMixerBitmap9>	m_vmr1bmp;
	CComPtr<IVMRMixerBitmap9>	m_vmr2bmp;
	*/

	// my filters
	CComPtr<IDWindowExtender> m_stereo;
	CComPtr<IDWindowExtender> m_mono1;
	CComPtr<IDWindowExtender> m_mono2;
	//CComPtr<IBaseFilter> m_demuxer;//pd10
	int m_filter_mode;
	bool m_PD10;
	HRESULT create_myfilter();

	// subtitle control
	srt_parser m_srt;
	wchar_t m_last_subtitle[5000];
	HRESULT draw_subtitle();
	double m_subtitle_center_x;
	double m_subtitle_bottom_y;
	int m_subtitle_offset;

	// font
	HFONT m_font;
	DWORD m_font_color;
	LONG lFontPointSize;//   = 60;
	TCHAR szFontName[100];// = L"ËÎÌו";
	TCHAR szFontStyle[32];// = L"Regular";
	// end font

	// subtitle D3D & Surface
	CComPtr<IDirect3D9>			m_D3D;
	CComPtr<IDirect3DDevice9>	m_D3Ddevice;   
	CComPtr<IDirect3DSurface9>	m_text_surface;
	int m_text_surface_width;
	int m_text_surface_height;
};
#pragma once
// COM & DShow
#include <atlbase.h>
#include <dshow.h>
#include <InitGuid.h>

// renderer
#include "streams.h"
#include "..\renderer_prototype\my12doomRenderer.h"
#include "dwindow.h"
#include "bar.h"

// other
#include "global_funcs.h"
#include "CSubtitle.h"
#include "srt\srt_parser.h"
#include "..\lrtb\mySink.h"
#include "PGS\PGSParser.h"
#include "DShowSubtitleRenderer.h"
#include "..\my12doomSource\src\filters\parser\MpegSplitter\IOffsetMetadata.h"

#define _AFX
#define __AFX_H__
#include <atlcoll.h>

// some definition
#define MKV_NO_TRACK -1
#define MKV_ALL_TRACK -2
#define MKV_TRACK_NUMBER(x) (0x01 << x)
#define MKV_FIRST_TRACK MKV_TRACK_NUMBER(0)

#define FILTER_MODE_FAIL 0
#define FILTER_MODE_MONO 1
#define FILTER_MODE_STEREO 2

#define WM_LOADFILE (WM_USER + 5)

class subtitle_file_handler
{
public:
	bool actived/* = false*/;
	wchar_t m_pathname[MAX_PATH];
	CSubtitleRenderer *m_renderer;
	subtitle_file_handler(const wchar_t *pathname);
	~subtitle_file_handler();
};

class dx_player : public Imy12doomRendererCallback, public dwindow
{
public:
	dx_player(RECT screen1, RECT screen2, HINSTANCE hExe);
	~dx_player();

	// load functions
	bool m_reset_load_done;
	HRESULT reset();								// unload all video and subtitle files
	HRESULT start_loading();
	HRESULT reset_and_loadfile(const wchar_t *pathname, bool stop);
	HRESULT load_audiotrack(const wchar_t *pathname);
	HRESULT load_subtitle(const wchar_t *pathname, bool reset = true);
	HRESULT load_file(const wchar_t *pathname, bool non_mainfile = false, int audio_track = MKV_FIRST_TRACK, int video_track = MKV_ALL_TRACK);			// for multi stream mkv
	HRESULT end_loading();

	// subtitle control functions
	HRESULT set_subtitle_pos(double center_x, double bottom_y);
	HRESULT set_subtitle_offset(int offset);

	// image control functions
	HRESULT set_revert(bool revert);
	HRESULT set_letterbox(double delta);	// 1.0 = top, -1.0 = bottom, 0 = center

	// play control functions
	HRESULT play();
	HRESULT pause();
	HRESULT stop();
	HRESULT seek(int time);
	HRESULT tell(int *time);
	HRESULT total(int *time);
	HRESULT set_volume(double volume);			// 0 - 1.0 = 0% - 100%, linear
	HRESULT get_volume(double *volume);
	HRESULT show_mouse(bool show)
	{
		GetCursorPos(&m_mouse);
		return __super::show_mouse(show);
	}
	bool is_closed();
	HRESULT toggle_fullscreen();
	POINT m_mouse;

	// error reporting vars and functions
	HRESULT log_line(wchar_t *format, ...);
	wchar_t *m_log;

protected:
	bool m_file_loaded /*= false*/;
	// image control vars
	HINSTANCE m_hexe;
	//AutoSetting<bool> m_always_show_right/*(L"AlwaysShowRight", false)*/;
	AutoSetting<double> m_aspect;/*(L"AlwaysShowRight", false)*/;

	int m_mirror1;
	int m_mirror2;			// 0x0:no mirror, 0x1 mirror horizontal, 0x2 mirror vertical, 0x3(0x3=0x1|0x2) mirror both
	bool m_revert;
	double m_letterbox_delta;
	double m_parallax;

	int init_done_flag;

	// helper function and vars
	HRESULT CrackPD10(IBaseFilter *filter);
	HRESULT calculate_movie_rect(RECT *source, RECT *client, RECT *letterbox, bool ui);
	HRESULT paint_letterbox(int id, RECT letterbox);
	bool m_select_font_active;
	static DWORD WINAPI select_font_thread(LPVOID lpParame);
	HRESULT reset_and_loadfile_internal(const wchar_t *pathname);
	bool m_reset_and_load;
	bool m_stop_after_load;
	wchar_t m_file_to_load[MAX_PATH];


	// window handler
	LRESULT on_display_change();
	LRESULT on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT on_sys_command(int id, WPARAM wParam, LPARAM lParam);	// don't block WM_SYSCOMMAND
	LRESULT on_command(int id, WPARAM wParam, LPARAM lParam);
	LRESULT on_getminmaxinfo(int id, MINMAXINFO *info);
	LRESULT on_move(int id, int x, int y);
	LRESULT on_mouse_move(int id, int x, int y);
	LRESULT on_nc_mouse_move(int id, int x, int y);
	LRESULT on_mouse_down(int id, int button, int x, int y);
	LRESULT on_mouse_up(int id, int button, int x, int y);
	LRESULT on_double_click(int id, int button, int x, int y);
	LRESULT on_key_down(int id, int key);
	//LRESULT on_close(int id);
	//LRESULT on_display_change();
	LRESULT on_paint(int id, HDC hdc);
	LRESULT on_timer(int id);
	LRESULT on_size(int id, int type, int x, int y);
	LRESULT on_init_dialog(int id, WPARAM wParam, LPARAM lParam);		// buged
	LRESULT on_dropfile(int id, int count, wchar_t **filenames);
	LRESULT on_idle_time();	// S_FALSE = Sleep(1)
	// end window handler

	// helper
	HRESULT enable_audio_track(int track);
	HRESULT enable_subtitle_track(int track);
	HRESULT list_audio_track(HMENU submenu);
	HRESULT list_subtitle_track(HMENU submenu);
	HRESULT debug_list_filters();

	// filter callback function
	HRESULT SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn);

	// directshow etc. core part
	bar_drawer m_bar_drawer;
	CCritSec m_draw_sec;
	CCritSec m_seek_sec;
	bool m_show_ui;
	AutoSetting<double> m_volume;	// = 1.0
	HRESULT show_ui(bool show);
	HRESULT draw_ui();

	HRESULT on_dshow_event();		//"on move window"
	HRESULT init_direct_show();
	HRESULT exit_direct_show();

	// basic directshow vars
	DWORD rot_id;
	CComPtr<IGraphBuilder>		m_gb;
	CComPtr<IMediaSeeking>		m_ms;
	CComPtr<IMediaControl>		m_mc;
	CComPtr<IBasicAudio>		m_ba;

	// renderer and input layout and output mode
	my12doomRenderer *m_renderer1;
	AutoSetting<DWORD> m_input_layout; /* = input_layout_auto*/
	AutoSetting<DWORD> m_output_mode;  /* = anaglyph*/
	AutoSetting<DWORD> m_mask_mode;	   /* = row_interlace */
	AutoSetting<DWORD> m_anaglygh_left_color;	   /* = row_interlace */
	AutoSetting<DWORD> m_anaglygh_right_color;	   /* = row_interlace */

	// subtitle control
	CComPtr<IOffsetMetadata> m_offset_metadata;
	CSubtitleRenderer *m_srenderer;
	DShowSubtitleRenderer m_grenderer;
	CAutoPtrList<subtitle_file_handler> m_external_subtitles;
	CCritSec m_subtitle_sec;
	int m_lastCBtime;
	HRESULT draw_subtitle();
	double m_subtitle_center_x;
	double m_subtitle_bottom_y;
	int m_user_offset;
	int m_internel_offset;
	AutoSetting<int> m_subtitle_latency;
	AutoSetting<double> m_subtitle_ratio;

	// font
	HFONT m_font;
	AutoSetting<DWORD> m_font_color/*L"FontColor", 0x00ffffff)*/;
	AutoSetting<LONG> m_lFontPointSize/*(L"FontSize", 40)*/;
	AutoSetting<bool> m_display_subtitle;
	AutoSettingString m_FontName/*(L"Font", L"Arial")*/;
	AutoSettingString m_FontStyle/*(L"FontStyle", L"Regular")*/;
	HRESULT select_font(bool show_dlg);
	// end font

};
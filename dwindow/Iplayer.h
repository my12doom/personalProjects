#include <Windows.h>

// some definition
#define LOADFILE_NO_TRACK -1
#define LOADFILE_ALL_TRACK -2
#define LOADFILE_TRACK_NUMBER(x) (0x01 << x)
#define LOADFILE_FIRST_TRACK LOADFILE_TRACK_NUMBER(0)
#define WM_LOADFILE (WM_USER + 5)


class Iplayer
{
public:
	// load functions
	virtual HRESULT reset() PURE;								// unload all video and subtitle files
	virtual HRESULT start_loading() PURE;
	virtual HRESULT reset_and_loadfile(const wchar_t *pathname, bool stop) PURE;
	virtual HRESULT load_audiotrack(const wchar_t *pathname) PURE;
	virtual HRESULT load_subtitle(const wchar_t *pathname, bool reset = true) PURE;
	virtual HRESULT load_file(const wchar_t *pathname, bool non_mainfile = false, 
		int audio_track = LOADFILE_FIRST_TRACK, int video_track = LOADFILE_ALL_TRACK) PURE;			// for multi stream mkv
	virtual HRESULT end_loading() PURE;

	// subtitle control functions
	virtual HRESULT set_subtitle_pos(double center_x, double bottom_y) PURE;
	virtual HRESULT set_subtitle_offset(int offset) PURE;

	// image control functions
	virtual HRESULT set_revert(bool revert) PURE;
	virtual HRESULT set_letterbox(double delta) PURE;	// 1.0 = top, -1.0 = bottom, 0 = center

	// playlist
	virtual HRESULT play_next_file() PURE;

	// play control functions
	virtual HRESULT play() PURE;
	virtual HRESULT pause() PURE;
	virtual HRESULT stop() PURE;
	virtual HRESULT seek(int time) PURE;
	virtual HRESULT tell(int *time) PURE;
	virtual HRESULT total(int *time) PURE;
	virtual HRESULT set_volume(double volume) PURE;			// 0 - 1.0 = 0% - 100%, linear
	virtual HRESULT get_volume(double *volume) PURE;
	virtual bool is_playing() PURE;
	virtual HRESULT show_mouse(bool show) PURE;
	virtual bool is_closed() PURE;
	virtual HRESULT toggle_fullscreen() PURE;
	virtual HRESULT set_output_mode(int mode) PURE;
	virtual HRESULT set_theater(HWND owner) PURE;
	virtual HRESULT popup_menu(HWND owner) PURE;
	virtual bool is_fullsceen(int window_id) PURE;
	virtual HWND get_window(int window_id) PURE;

	// error reporting vars and functions
	virtual HRESULT log_line(wchar_t *format, ...) PURE;

	// helper variables
	wchar_t *m_log;
	bool m_file_loaded /*= false*/;
	POINT m_mouse;
	bool m_reset_load_done;

};

#pragma once
extern "C" {
#include <ass/ass.h>
};
#include "CSubtitle.h"

class LibassRendererCore: public CSubtitleRenderer
{
public:
	LibassRendererCore();
	~LibassRendererCore();

	// CSubtitleRenderer
	virtual HRESULT load_file(wchar_t *filename);												//maybe you don't need this?
	virtual HRESULT load_index(void *data, int size);
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek(){return S_OK;}														// to provide dshow support
	virtual HRESULT set_font_color(DWORD newcolor){return E_NOTIMPL;};
	virtual HRESULT set_font(HFONT newfont){return E_NOTIMPL;};									// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.

	// LibassRenderer 's own function for fontconfig..
	static HRESULT load_fonts();	// will create a thread to load fonts, use fonts_loaded() to get loading progress
	static HRESULT fonts_loaded();	// S_OK = loaded, S_FALSE = loading / loading not started / load fail

protected:

	ASS_Renderer *m_ass_renderer;
	ASS_Track *m_track;
};

class LibassRenderer: public CSubtitleRenderer
{
public:
	LibassRenderer();
	~LibassRenderer();

	// CSubtitleRenderer
	virtual HRESULT load_file(wchar_t *filename);												//maybe you don't need this?
	virtual HRESULT load_index(void *data, int size);
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek(){return S_OK;}														// to provide dshow support
	virtual HRESULT set_font_color(DWORD newcolor){return E_NOTIMPL;};
	virtual HRESULT set_font(HFONT newfont){return E_NOTIMPL;};									// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.

protected:

	LibassRendererCore *m_ass_renderer;
	void *m_command_queue;
};
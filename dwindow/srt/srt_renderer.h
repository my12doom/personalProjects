#pragma once
#include "..\CSubtitle.h"
#include "srt_parser.h"

class CsrtRenderer : public CSubtitleRenderer
{
public:
	CsrtRenderer();
	~CsrtRenderer();
	virtual HRESULT load_file(wchar_t *filename);	//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();		// just clear current incompleted data, to support dshow seeking.

	HRESULT select_font(bool show_dlg = true);
protected:
	srt_parser m_srt;
	wchar_t m_last_found[1024];

	// font
	HFONT m_font;
	DWORD m_font_color;
	LONG m_lFontPointSize;//   = 60;
	wchar_t m_FontName[100];// = L"ËÎÌו";
	wchar_t m_FontStyle[32];// = L"Regular";
	// end font
};

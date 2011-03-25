#pragma once
#include "..\CSubtitle.h"
#include "PGSParser.h"

class PGSRenderer: public CSubtitleRenderer
{
public:
	PGSRenderer();
	virtual HRESULT load_file(wchar_t *filename);	//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();	// to provide dshow support
	virtual HRESULT select_font(bool show_dlg = true){return E_NOTIMPL;}
protected:
	int m_last_found;
	PGSParser m_parser;
};
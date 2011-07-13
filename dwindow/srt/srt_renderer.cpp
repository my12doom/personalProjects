#include "srt_renderer.h"

CsrtRenderer::CsrtRenderer(HFONT font, DWORD fontcolor)
{
	m_font = font;
	m_font_color = fontcolor;
	reset();
}

HRESULT CsrtRenderer::set_font_color(DWORD newcolor)
{
	m_font_color = newcolor;
	return S_OK;
}

HRESULT CsrtRenderer::set_font(HFONT newfont)
{
	m_font = newfont;
	return S_OK;
}


HRESULT CsrtRenderer::load_file(wchar_t *filename)
{
	return m_srt.load(filename);
}

HRESULT CsrtRenderer::reset()
{
	m_srt.init(20480, 2048*1024);
	m_last_found[0] = NULL;
	return S_OK;
}

HRESULT CsrtRenderer::seek()
{
	m_last_found[0] = NULL;
	return S_OK;
}

HRESULT CsrtRenderer::add_data(BYTE *data, int size, int start, int end)
{
	char *p1 = (char*)malloc(size+1);
	memcpy(p1, data, size);
	p1[size] = NULL;
	wchar_t *p2 = (wchar_t*)malloc(size*2+2);
	MultiByteToWideChar(CP_UTF8, 0, (char*)p1, size+1, p2, size+1);
	HRESULT hr = m_srt.direct_add_subtitle(p2, start, end);

	free(p1);
	free(p2);

	return hr;
}

HRESULT CsrtRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/* =-1 */)
{
	if (NULL == out)
		return E_POINTER;

	wchar_t found[1024];
	m_srt.get_subtitle(time, time, found);
	if ((wcscmp(found, m_last_found) == 0) && (last_time != -1))
	{
		out->data = NULL;
		return S_FALSE;
	}

	else
	{
		wcscpy(m_last_found, found);
		// rendering
		if (found[0] == NULL)
		{
			out->width_pixel = out->height_pixel = 0;
			out->width = out->height = 0;
			out->data = NULL;
			return S_OK;
		}

		HDC hdc = GetDC(NULL);
		HDC hdcBmp = CreateCompatibleDC(hdc);

		HFONT hOldFont = (HFONT) SelectObject(hdcBmp, m_font);

		RECT rect = {0,0,1920,1080};
		DrawTextW(hdcBmp, found, (int)wcslen(found), &rect, DT_CENTER | DT_CALCRECT | DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_EDITCONTROL);
		out->pixel_type = out->pixel_type_RGB;
		out->height_pixel = rect.bottom - rect.top;
		out->width_pixel  = rect.right - rect.left;
		out->width = (double)out->width_pixel/1920;
		out->height = (double)out->height_pixel/1080;
		out->aspect = (double)16/9;
		out->delta = 0;

		out->left = 0.5 - out->width/2;
		out->top = 0.95 - out->height;

		HBITMAP hbm = CreateCompatibleBitmap(hdc, out->width_pixel, out->height_pixel);

		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

		RECT rcText;
		SetRect(&rcText, 0, 0, out->width_pixel, out->height_pixel);
		SetBkColor(hdcBmp, RGB(0, 0, 0));					// Pure black background
		SetTextColor(hdcBmp, RGB(255, 255, 255));			// white text for alpha

		DrawTextW(hdcBmp, found, (int)wcslen(found), &rect, DT_CENTER);

		out->data = (BYTE *) malloc(out->width_pixel * out->height_pixel * 4);
		GetBitmapBits(hbm, out->width_pixel * out->height_pixel * 4, out->data);


		BYTE *data = (BYTE*)out->data;
		DWORD *data_dw = (DWORD*)out->data;
		unsigned char color_r = (BYTE)(m_font_color       & 0xff);
		unsigned char color_g = (BYTE)((m_font_color>>8)  & 0xff);
		unsigned char color_b = (BYTE)((m_font_color>>16) & 0xff);
		DWORD color = (color_r<<16) | (color_g <<8) | (color_b);// reverse big endian

		for(int i=0; i<out->width_pixel * out->height_pixel; i++)
		{
			unsigned char alpha = data[i*4+2];
			data_dw[i] = color;
			data[i*4+3] = alpha;
		}

		DeleteObject(SelectObject(hdcBmp, hbmOld));
		SelectObject(hdc, hOldFont);
		DeleteObject(hbm);
		DeleteDC(hdcBmp);
		ReleaseDC(NULL, hdc);

		return S_OK;
	}
}
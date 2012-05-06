#include "libass_renderer.h"
#include "..\libass\charset.h"
#include <Windows.h>
#include <streams.h>

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)


#pragma comment(lib, "libass.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libass.a")
#pragma comment(lib, "libfreetype.a")
#pragma comment(lib, "libfontconfig.a")
#pragma comment(lib, "libexpat.a")

ASS_Library *g_ass_library = NULL;
CCritSec g_libass_cs;

LibassRenderer::LibassRenderer()
{
	m_ass_renderer = NULL;
	m_track = NULL;

	CAutoLock lck(&g_libass_cs);
	if (NULL == g_ass_library)
		g_ass_library = ass_library_init();

	if (!g_ass_library) 
	{
		printf("ass_library_init failed!\n");
		return;
	}

	reset();
}
LibassRenderer::~LibassRenderer()
{
	CAutoLock lck(&g_libass_cs);

	if (m_track)
		ass_free_track(m_track);

	if (m_ass_renderer)
		ass_renderer_done(m_ass_renderer);
}
HRESULT LibassRenderer::load_file(wchar_t *filename)
{
	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer)
		return E_FAIL;

	if (m_track)
		ass_free_track(m_track);

	FILE * f = _wfopen(filename, L"rb");
	fseek(f, 0, SEEK_END);
	int file_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *src = (char*)malloc(file_size);
	char *utf8 = (char*)malloc(file_size*3);
	fread(src, 1, file_size, f);
	fclose(f);

	int utf8_size = ConvertToUTF8(src, file_size, utf8, file_size*3);

	m_track = ass_read_memory(g_ass_library, utf8, utf8_size, NULL);

	free(src);
	free(utf8);

	return m_track ? S_OK : E_FAIL;
}
HRESULT LibassRenderer::load_index(void *data, int size)
{
	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer)
		return E_FAIL;

	if (m_track)
		ass_free_track(m_track);

	m_track = ass_new_track(g_ass_library);

	ass_process_codec_private(m_track, (char*)data, size);

	return S_OK;
}
HRESULT LibassRenderer::add_data(BYTE *data, int size, int start, int end)
{
	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer || !m_track)
		return E_FAIL;

	ass_process_chunk(m_track, (char*)data, size, start, end-start);

	return S_OK;
}
HRESULT LibassRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/*=-1*/)			// get subtitle on a time point, 
																									// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
{
	if (NULL == out)
		return E_POINTER;

	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer || !m_track)
		return E_FAIL;

	memset(out, 0, sizeof(rendered_subtitle));

	ASS_Image *img = NULL;
	ASS_Image *p = NULL;
	int changed = 0;
	p = img = ass_render_frame(m_ass_renderer, m_track, time, &changed);

	if (!img)
		return S_OK;

	if (changed == 0 && last_time>=0)
		return S_FALSE;

	RECT rect = {0};
	if (p)
	{
		rect.left = p->dst_x;
		rect.right = rect.left + p->w;
		rect.top = p->dst_y;
		rect.bottom = rect.top + p->h;
	}

	while (p)
	{
		rect.left = min(rect.left, p->dst_x);
		rect.top = min(rect.top, p->dst_y);
		rect.right = max(rect.right, p->dst_x + p->w);
		rect.bottom = max(rect.bottom, p->dst_y + p->h);

		p = p->next;
	}

	out->height_pixel = rect.bottom - rect.top;
	out->width_pixel  = rect.right - rect.left;
	out->width = (double)out->width_pixel/1920;
	out->height = (double)out->height_pixel/1080;
	out->aspect = 16.0/9.0;
	out->data = (BYTE *) malloc(out->width_pixel * out->height_pixel * 4);
	memset(out->data, 0, out->width_pixel * out->height_pixel * 4);
	out->left = (double)rect.left/1920;
	out->top = (double)rect.top/1080;

	// blending
	while (img) 
	{
		int x, y;
		unsigned char opacity = 255 - _a(img->color);
		unsigned char a = _a(img->color);
		unsigned char r = _r(img->color);
		unsigned char g = _g(img->color);
		unsigned char b = _b(img->color);

		unsigned char *src;
		unsigned char *dst;

		src = img->bitmap;
		dst = out->data + (img->dst_y - rect.top) * out->width_pixel*4 + (img->dst_x-rect.left) * 4;

		for (y = 0; y < img->h; ++y) {
			for (x = 0; x < img->w; ++x) {
				unsigned k = ((unsigned) src[x]) * opacity / 255;
				dst[x * 4] = (k * b + (255 - k) * dst[x * 4]) / 255;
				dst[x * 4 + 1] = (k * g + (255 - k) * dst[x * 4 + 1]) / 255;
				dst[x * 4 + 2] = (k * r + (255 - k) * dst[x * 4 + 2]) / 255;
				dst[x * 4 + 3] = (k * 255 + (255 - k) * dst[x * 4 + 3]) / 255;
			}
			src += img->stride;
			dst += out->width_pixel*4;
		}

		img = img->next;
	}

	return S_OK;
}
HRESULT LibassRenderer::reset()
{
	CAutoLock lck(&g_libass_cs);

	if (m_track)
		ass_free_track(m_track);

	if (m_ass_renderer)
		ass_renderer_done(m_ass_renderer);

	m_ass_renderer = ass_renderer_init(g_ass_library);
	if (!m_ass_renderer)
	{
		printf("ass_renderer_init failed!\n");
		return E_FAIL;
	}

	ass_set_frame_size(m_ass_renderer, 1920, 1080);
	ass_set_font_scale(m_ass_renderer, 1.0);
	ass_set_fonts(m_ass_renderer, "Arial", "Sans", 1, "Z:\\fonts.conf", 1);

	return S_OK;
}

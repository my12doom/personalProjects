// DirectShow part of my12doom renderer

#include "my12doomRenderer.h"
#include <dvdmedia.h>

my12doomRenderer::my12doomRenderer(LPUNKNOWN pUnk,HRESULT *phr, HWND hwnd1 /* = NULL */, HWND hwnd2 /* = NULL */)
                                  : CBaseVideoRenderer(__uuidof(CLSID_my12doomRenderer),
                                    NAME("Texture Renderer"), pUnk, phr)
{
    if (phr)
        *phr = S_OK;
	m_data = NULL;

	timeBeginPeriod(1);
	// aspect and offset
	offset_x = 0.0;
	offset_y = 0.0;
	source_aspect = 0.0;

	// window
	g_hWnd = hwnd1;
	g_hWnd2 = hwnd2;

	// input / output
	g_swapeyes = false;
	output_mode = mono;
	input_layout = mono2d;
	mask_mode = row_interlace;
	m_color1 = D3DCOLOR_XRGB(255, 0, 0);
	m_color2 = D3DCOLOR_XRGB(0, 255, 255);

	// thread
	m_device_threadid = GetCurrentThreadId();
	m_device_state = need_create;
	m_render_thread = INVALID_HANDLE_VALUE;
	m_render_thread_exit = false;

	// D3D
	g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );

}

my12doomRenderer::~my12doomRenderer()
{
	shutdown();
	if (m_data)
		free(m_data);
	timeEndPeriod(1);
}

HRESULT my12doomRenderer::CheckMediaType(const CMediaType *pmt)
{
    HRESULT   hr = E_FAIL;
    VIDEOINFO *pvi=0;

    CheckPointer(pmt,E_POINTER);

    if( *pmt->FormatType() != FORMAT_VideoInfo && *pmt->FormatType() != FORMAT_VideoInfo2)
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();

    if(IsEqualGUID( *pmt->Type(),    MEDIATYPE_Video)  &&
       IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_YV12))
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT my12doomRenderer::SetMediaType(const CMediaType *pmt)
{
    // Retrive the size of this media type
	BITMAPINFOHEADER *pbih = NULL;
	if (*pmt->FormatType() == FORMAT_VideoInfo)
		pbih = &((VIDEOINFOHEADER*)pmt->Format())->bmiHeader;
	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
		pbih = &((VIDEOINFOHEADER2*)pmt->Format())->bmiHeader;

	if (!pbih)
		return E_FAIL;

	m_lVidWidth = pbih->biWidth;
	m_lVidHeight = pbih->biHeight;

	source_aspect = (double)m_lVidWidth / m_lVidHeight;
	if (source_aspect> 2.425)
		source_aspect /= 2;
	else if (source_aspect< 1.2125)
		source_aspect *= 2;


	if (m_data)
		free(m_data);
	m_data = (BYTE*)malloc(m_lVidWidth * m_lVidHeight * 3 / 2);
	memset(m_data, 0, m_lVidWidth * m_lVidHeight);
	memset(m_data + m_lVidWidth * m_lVidHeight, 128, m_lVidWidth * m_lVidHeight/2);
	m_data_changed = false;

    return S_OK;
}

HRESULT my12doomRenderer::DoRenderSample( IMediaSample * pSample )
{
    BYTE  *pBmpBuffer;
    pSample->GetPointer( &pBmpBuffer );

	{
		CAutoLock lck(&m_data_lock);
		m_data_changed = true;
		memcpy(m_data, pBmpBuffer, pSample->GetActualDataLength());

	}

	if (output_mode != pageflipping)
		render(true);

	return S_OK;
}

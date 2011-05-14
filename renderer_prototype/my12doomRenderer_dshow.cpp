// DirectShow part of my12doom renderer

#include "my12doomRenderer.h"

my12doomRenderer::my12doomRenderer(LPUNKNOWN pUnk,HRESULT *phr, HWND hwnd1 /* = NULL */, HWND hwnd2 /* = NULL */)
                                  : CBaseVideoRenderer(__uuidof(CLSID_my12doomRenderer),
                                    NAME("Texture Renderer"), pUnk, phr)
{
    if (phr)
        *phr = S_OK;
	m_data = NULL;

	g_hWnd = hwnd1;
	g_hWnd2 = hwnd2;

	m_device_threadid = GetCurrentThreadId();
	m_device_state = need_create;
	m_color1 = D3DCOLOR_XRGB(255, 0, 0);
	m_color2 = D3DCOLOR_XRGB(0, 255, 255);
	m_render_thread = INVALID_HANDLE_VALUE;
	m_render_thread_exit = false;
}


//-----------------------------------------------------------------------------
// CTextureRenderer destructor
//-----------------------------------------------------------------------------
my12doomRenderer::~my12doomRenderer()
{
	shutdown();
	if (m_data)
		free(m_data);
}


//-----------------------------------------------------------------------------
// CheckMediaType: This method forces the graph to give us an R8G8B8 video
// type, making our copy to texture memory trivial.
//-----------------------------------------------------------------------------
HRESULT my12doomRenderer::CheckMediaType(const CMediaType *pmt)
{
    HRESULT   hr = E_FAIL;
    VIDEOINFO *pvi=0;

    CheckPointer(pmt,E_POINTER);

    if( *pmt->FormatType() != FORMAT_VideoInfo )
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();

    if(IsEqualGUID( *pmt->Type(),    MEDIATYPE_Video)  &&
       IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_YV12))
    {
        hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------------------
// SetMediaType: Graph connection has been made.
//-----------------------------------------------------------------------------
HRESULT my12doomRenderer::SetMediaType(const CMediaType *pmt)
{
    // Retrive the size of this media type
    VIDEOINFO *pviBmp;                      // Bitmap info header
    pviBmp = (VIDEOINFO *)pmt->Format();
	m_lVidWidth = pviBmp->bmiHeader.biWidth;
	m_lVidHeight = pviBmp->bmiHeader.biHeight;

	if (m_data)
		free(m_data);
	m_data = (BYTE*)malloc(m_lVidWidth * m_lVidHeight * 3 / 2);
	memset(m_data, 0, m_lVidWidth * m_lVidHeight);
	memset(m_data + m_lVidWidth * m_lVidHeight, 128, m_lVidWidth * m_lVidHeight/2);
	m_data_changed = false;

    return S_OK;
}

HRESULT render( int time = 1, bool forced = false);
extern enum output_mode_types
{
	NV3D, masking, anaglyph, mono, pageflipping, output_mode_types_max, safe_interlace_row, safe_interlace_line
} output_mode;

//-----------------------------------------------------------------------------
// DoRenderSample: A sample has been delivered. Copy it to the texture.
//-----------------------------------------------------------------------------
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

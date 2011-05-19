// DirectShow part of my12doom renderer

#include "my12doomRenderer.h"
#include <dvdmedia.h>

// base
STDMETHODIMP DRendererInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	((DBaseVideoRenderer*)m_pFilter)->m_thisstream = tStart;
	return __super::NewSegment(tStart, tStop, dRate);
}

CBasePin * DBaseVideoRenderer::GetPin(int n)
{
	// copied from __super, and changed only the input pin creation class
	CAutoLock cObjectCreationLock(&m_ObjectCreationLock);

	// Should only ever be called with zero
	ASSERT(n == 0);

	if (n != 0) {
		return NULL;
	}

	// Create the input pin if not already done so

	if (m_pInputPin == NULL) {

		// hr must be initialized to NOERROR because
		// CRendererInputPin's constructor only changes
		// hr's value if an error occurs.
		HRESULT hr = NOERROR;

		m_pInputPin = new DRendererInputPin(this,&hr,L"In");
		if (NULL == m_pInputPin) {
			return NULL;
		}

		if (FAILED(hr)) {
			delete m_pInputPin;
			m_pInputPin = NULL;
			return NULL;
		}
	}
	return m_pInputPin;
}


// renderer dshow part
my12doomRenderer::my12doomRenderer(LPUNKNOWN pUnk,HRESULT *phr, HWND hwnd1 /* = NULL */, HWND hwnd2 /* = NULL */)
                                  : DBaseVideoRenderer(__uuidof(CLSID_my12doomRenderer),
                                    NAME("Texture Renderer"), pUnk, phr)
{
    if (phr)
        *phr = S_OK;
	m_data = NULL;

	timeBeginPeriod(1);

	// callback
	m_cb = NULL;

	// aspect and offset
	m_offset_x = 0.0;
	m_offset_y = 0.0;
	m_source_aspect = 0.0;

	// window
	m_hWnd = hwnd1;
	m_hWnd2 = hwnd2;

	// input / output
	m_swapeyes = false;
	m_output_mode = mono;
	m_input_layout = mono2d;
	m_mask_mode = row_interlace;
	m_color1 = D3DCOLOR_XRGB(255, 0, 0);
	m_color2 = D3DCOLOR_XRGB(0, 255, 255);

	// thread
	m_device_threadid = GetCurrentThreadId();
	m_device_state = need_create;
	m_render_thread = INVALID_HANDLE_VALUE;
	m_render_thread_exit = false;

	// D3D
	m_D3D = Direct3DCreate9( D3D_SDK_VERSION );

	// ui & bitmap
	m_showui = false;
}

my12doomRenderer::~my12doomRenderer()
{
	shutdown();
	if (m_data)
		free(m_data);
	timeEndPeriod(1);
	m_D3D = NULL;

	m_D3D = NULL;
}

HRESULT my12doomRenderer::CheckMediaType(const CMediaType *pmt)
{
    HRESULT   hr = E_FAIL;
    VIDEOINFO *pvi=0;

    CheckPointer(pmt,E_POINTER);

    if( *pmt->FormatType() != FORMAT_VideoInfo && *pmt->FormatType() != FORMAT_VideoInfo2)
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();

	GUID subtype = *pmt->Subtype();
    if(*pmt->Type() == MEDIATYPE_Video  &&
       (subtype == MEDIASUBTYPE_YV12 || subtype ==  MEDIASUBTYPE_NV12 || subtype == MEDIASUBTYPE_YUY2))
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT my12doomRenderer::SetMediaType(const CMediaType *pmt)
{
    // Retrive the size of this media type
	if (*pmt->FormatType() == FORMAT_VideoInfo)
	{
		VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)pmt->Format();
		m_lVidWidth = vihIn->bmiHeader.biWidth;
		m_lVidHeight = vihIn->bmiHeader.biHeight;
		m_source_aspect = (double)m_lVidWidth / m_lVidHeight;

	}

	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->Format();
		m_lVidWidth = vihIn->bmiHeader.biWidth;
		m_lVidHeight = vihIn->bmiHeader.biHeight;
		m_source_aspect = (double)vihIn->dwPictAspectRatioX / vihIn->dwPictAspectRatioY;
	}

	else
		return E_FAIL;

	if (m_source_aspect> 2.425)
	{
		m_source_aspect /= 2;
		m_input_layout = side_by_side;
	}
	else if (m_source_aspect< 1.2125)
	{
		m_source_aspect *= 2;
		m_input_layout = top_bottom;
	}
	else
		m_input_layout = mono2d;

	m_format = *pmt->Subtype();

	if (m_data)
		free(m_data);

	if (m_format  == MEDIASUBTYPE_YUY2)
	{
		m_data = (BYTE*)malloc(m_lVidWidth * m_lVidHeight * 2);
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = i%2 ? 128 : 0;
		for(int i=0; i<m_lVidHeight; i++)
			memcpy(m_data + m_lVidWidth*2*i, one_line, m_lVidWidth*2);
	}
	else
	{
		m_data = (BYTE*)malloc(m_lVidWidth * m_lVidHeight * 3 / 2);
		memset(m_data, 0, m_lVidWidth * m_lVidHeight);
		memset(m_data + m_lVidWidth * m_lVidHeight, 128, m_lVidWidth * m_lVidHeight/2);
	}
	m_data_changed = true;

    return S_OK;
}

HRESULT my12doomRenderer::DoRenderSample( IMediaSample * pSample )
{
    BYTE  *pBmpBuffer;
    pSample->GetPointer( &pBmpBuffer );

	REFERENCE_TIME start, end;
	pSample->GetTime(&start, &end);
	if (m_cb)
		m_cb->SampleCB(start + m_thisstream, end + m_thisstream, pSample);

	{
		CAutoLock lck(&m_data_lock);
		m_data_changed = true;
		int size = pSample->GetActualDataLength();
		memcpy(m_data, pBmpBuffer, m_format == MEDIASUBTYPE_YUY2 ? m_lVidWidth * m_lVidHeight * 2 : m_lVidWidth * m_lVidHeight * 3 / 2);
	}

	if (m_output_mode != pageflipping)
		render(true);

	return S_OK;
}

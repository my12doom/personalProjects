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
my12doomRendererDShow::my12doomRendererDShow(LPUNKNOWN pUnk,HRESULT *phr, my12doomRenderer *owner, int id)
: DBaseVideoRenderer(__uuidof(CLSID_my12doomRenderer),NAME("Texture Renderer"), pUnk, phr),
m_owner(owner),
m_id(id)
{
    if (phr)
        *phr = S_OK;
	m_data = NULL;

	timeBeginPeriod(1);

}

my12doomRendererDShow::~my12doomRendererDShow()
{
	if (m_data)
		free(m_data);
	timeEndPeriod(1);
}

HRESULT my12doomRendererDShow::CheckMediaType(const CMediaType *pmt)
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
        hr = m_owner->CheckMediaType(pmt, m_id);
    }

    return hr;
}

HRESULT my12doomRendererDShow::SetMediaType(const CMediaType *pmt)
{
	int width = m_owner->m_lVidWidth;
	int height= m_owner->m_lVidHeight;

	m_format = *pmt->Subtype();

	if (m_data)
		free(m_data);

	if (m_format  == MEDIASUBTYPE_YUY2)
	{
		m_data = (BYTE*)malloc(width * height * 2);
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = i%2 ? 128 : 0;
		for(int i=0; i<height; i++)
			memcpy(m_data + width*2*i, one_line, width*2);
	}
	else
	{
		m_data = (BYTE*)malloc(width * height * 3 / 2);
		memset(m_data, 0, width * height);
		memset(m_data + width * height, 128, width * height/2);
	}
	m_data_changed = true;

    return S_OK;
}

HRESULT	my12doomRendererDShow::BreakConnect()
{
	m_owner->BreakConnect(m_id);
	return __super::BreakConnect();
}

HRESULT my12doomRendererDShow::CompleteConnect(IPin *pRecievePin)
{
	m_owner->CompleteConnect(pRecievePin, m_id);
	return __super::CompleteConnect(pRecievePin);
}

bool my12doomRendererDShow::is_connected()
{
	if (!m_pInputPin)
		return false;
	return m_pInputPin->IsConnected() ? true : false;
}

HRESULT my12doomRendererDShow::DoRenderSample( IMediaSample * pSample )
{
    BYTE  *pBmpBuffer;
    pSample->GetPointer( &pBmpBuffer );

	REFERENCE_TIME start, end;
	pSample->GetTime(&start, &end);

	{
		CAutoLock lck(&m_data_lock);
		m_data_changed = true;
		int size = pSample->GetActualDataLength();
		size = min(size,  m_format == MEDIASUBTYPE_YUY2 ? m_owner->m_lVidWidth * m_owner->m_lVidHeight * 2 : m_owner->m_lVidWidth * m_owner->m_lVidHeight * 3 / 2);
		memcpy(m_data, pBmpBuffer, size);
	}

	m_owner->DataArrive(m_id, pSample);

	return S_OK;
}

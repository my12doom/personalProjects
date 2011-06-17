// DirectShow part of my12doom renderer

#include "my12doomRenderer.h"
#include <dvdmedia.h>

// base
STDMETHODIMP DRendererInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	((DBaseVideoRenderer*)m_pFilter)->NewSegment(tStart, tStop, dRate);
	return __super::NewSegment(tStart, tStop, dRate);
}
STDMETHODIMP DRendererInputPin::BeginFlush()
{
	((DBaseVideoRenderer*)m_pFilter)->BeginFlush();
	return __super::BeginFlush();
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


HRESULT DBaseVideoRenderer::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_thisstream = tStart;
	return S_OK;
}

HRESULT DBaseVideoRenderer::BeginFlush()
{
	return __super::BeginFlush();
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
	CAutoLock lck(&m_data_lock);
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
       (subtype == MEDIASUBTYPE_YV12 || subtype ==  MEDIASUBTYPE_NV12 || subtype == MEDIASUBTYPE_YUY2 || subtype == MEDIASUBTYPE_RGB32))
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

	CAutoLock lck(&m_data_lock);
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
	else if (m_format == MEDIASUBTYPE_RGB32)
	{
		m_data = (BYTE*)malloc(width * height * 4);
		memset(m_data, 0, width * height * 4);
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
HRESULT my12doomRendererDShow::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{	
	IMediaSample *tmp;
	CAutoLock lck(&m_owner->m_queue_lock);
	while(m_owner->m_left_queue.GetCount())
	{
		tmp = m_owner->m_left_queue.RemoveTail();
		tmp->Release();
	}
	while(m_owner->m_right_queue.GetCount())
	{
		tmp = m_owner->m_right_queue.RemoveTail();
		tmp->Release();
	}

	return __super::NewSegment(tStart, tStop, dRate);
}

bool my12doomRendererDShow::is_connected()
{
	if (!m_pInputPin)
		return false;
	return m_pInputPin->IsConnected() ? true : false;
}

HRESULT my12doomRendererDShow::DoRenderSample( IMediaSample * pSample )
{
	REFERENCE_TIME start, end;
	pSample->GetTime(&start, &end);


	m_time = start + m_thisstream;
	m_owner->DataArrive(m_id, pSample);

	return S_OK;
}

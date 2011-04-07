#include "GenericSubtitleRenderer.h"
#include "global_funcs.h"

CGenericSubtitleRenderer::CGenericSubtitleRenderer(HFONT font, DWORD fontcolor)
{
	m_srenderer = NULL;
	m_font = font;
	m_font_color = fontcolor;

	HRESULT hr;
	m_sink = new mySink(NULL, &hr);
	m_sink->QueryInterface(IID_IBaseFilter, (void**)&m_filter);
	m_sink->SetCallback(this);
}

CGenericSubtitleRenderer::~CGenericSubtitleRenderer()
{
	BreakConnectCB();
}

HRESULT CGenericSubtitleRenderer::BreakConnectCB()
{
	CAutoLock lck1(&m_subtitle_sec);
	if (m_srenderer) delete m_srenderer;
	m_srenderer = NULL;

	return S_OK;
}

HRESULT CGenericSubtitleRenderer::SampleCB(IMediaSample *sample)
{
	REFERENCE_TIME start, end;
	sample->GetTime(&start, &end);
	BYTE *p = NULL;
	sample->GetPointer(&p);

	//CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		m_srenderer->add_data(p, sample->GetActualDataLength(), (start+m_subtitle_seg_time)/10000, (end+m_subtitle_seg_time)/10000);

	return S_OK;
}

HRESULT CGenericSubtitleRenderer::CheckMediaTypeCB(const CMediaType *inType)
{
	GUID majorType = *inType->Type();
	GUID subType = *inType->Subtype();
	if (majorType != MEDIATYPE_Subtitle)
		return VFW_E_INVALID_MEDIA_TYPE;
	typedef struct SUBTITLEINFO 
	{
		DWORD dwOffset; // size of the structure/pointer to codec init data
		CHAR IsoLang[4]; // three letter lang code + terminating zero
		WCHAR TrackName[256]; // 256 bytes ought to be enough for everyone
	} haali_format;
	haali_format *format = (haali_format*)inType->pbFormat;
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		delete m_srenderer;
	m_srenderer = NULL;
	if (subType == MEDIASUBTYPE_PGS || 
		(subType == GUID_NULL &&inType->FormatLength()>=520 && wcsstr(format->TrackName, L"SUP")))
		m_srenderer = new PGSRenderer();
	else if (subType == MEDIASUBTYPE_UTF8)
		m_srenderer = new CsrtRenderer(m_font, m_font_color);
	else
		return VFW_E_INVALID_MEDIA_TYPE;

	return S_OK;
}

HRESULT CGenericSubtitleRenderer::NewSegmentCB(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (m_srenderer)
		m_srenderer->seek();

	m_subtitle_seg_time = tStart;
	return S_OK;
}

IBaseFilter *CGenericSubtitleRenderer::GetFilter()
{
	return m_filter;
}

CSubtitleRenderer *CGenericSubtitleRenderer::GetSubtitleRenderer()
{
	CAutoLock lck(&m_subtitle_sec);
	return m_srenderer;
}

HRESULT CGenericSubtitleRenderer::set_font(HFONT newfont)
{
	m_font = newfont;
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		return m_srenderer->set_font(newfont);
	else
		return S_OK;
}

HRESULT CGenericSubtitleRenderer::set_font_color(DWORD newcolor)
{
	m_font_color = newcolor;
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		return m_srenderer->set_font_color(newcolor);
	else
		return S_OK;
}
#pragma once

#include <streams.h>
#include "..\dwindow\runnable.h"
#include "openHevcWrapper.h"
#include <list>

class CHEVCDecoder;

// {44BDBC26-FA2C-497b-965C-9BCBEC7FCC56}
DEFINE_GUID(CLSID_OpenHEVCDecoder, 0x44bdbc26, 0xfa2c, 0x497b, 0x96, 0x5c, 0x9b, 0xcb, 0xec, 0x7f, 0xcc, 0x56);

class CHEVCDecoder : CTransformFilter
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IDWindowExtenderMono and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Overrriden from CSplitFilter base class
	HRESULT Receive(IMediaSample *pSample);
	HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// you know what
	CHEVCDecoder(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CHEVCDecoder();

	// dshow events
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

protected:
	GUID m_subtype;

	int m_width;
	int m_height;
	int m_frame_length;

	CCritSec m_samples_cs;
	OpenHevc_Handle h;
	REFERENCE_TIME m_pts;
};
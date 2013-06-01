#pragma once

#include <streams.h>
#include "..\dwindow\runnable.h"
#include <list>

class CJ2kDecoder;

// {7FB9DA30-61D1-4c42-AB65-069568CEC361}
DEFINE_GUID(CLSID_my12doomJ2KDecoder, 
			0x7fb9da30, 0x61d1, 0x4c42, 0xab, 0x65, 0x6, 0x95, 0x68, 0xce, 0xc3, 0x61);

typedef struct
{
	IMediaSample *sample;
	int fn;
} sample_entry;

class J2KWorker : public Irunnable
{
public:
	J2KWorker(IMediaSample *pIn, IMediaSample *pOut, CJ2kDecoder *owner, int fn);
	~J2KWorker(){}

	void run();

	IMediaSample *m_in;
	IMediaSample *m_out;
	CJ2kDecoder *m_owner;
	int m_fn;
};

class CJ2kDecoder : CTransformFilter
{
public:
	friend class J2KWorker;
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
	CJ2kDecoder(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CJ2kDecoder();

	// dshow events
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

protected:
	GUID m_subtype;

	int m_width;
	int m_height;

	int m_fn_delivered;
	int m_fn_recieved;

	thread_pool *m_workers;
	std::list<sample_entry> m_completed_samples;

	CCritSec m_samples_cs;
};
#pragma once
#include <streams.h>		// Based on CTransformFilter

class CQTransformOutputPin : public CTransformOutputPin
{
protected:
	COutputQueue *m_pOutputQueue;  // Streams data to the peer pin
public:
	CQTransformOutputPin(
        TCHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName)
		:CTransformOutputPin(pObjectName, pTransformFilter, phr, pName), m_pOutputQueue(NULL){}
#ifdef UNICODE
    CQTransformOutputPin(
        CHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName)
		:CTransformOutputPin(pObjectName, pTransformFilter, phr, pName), m_pOutputQueue(NULL){}
#endif
    ~CQTransformOutputPin();

	// Used to create output queue objects
	HRESULT Active();
	HRESULT Inactive();
	HRESULT Deliver(IMediaSample *pMediaSample);
	HRESULT DeliverEndOfStream();
	HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
	HRESULT DeliverNewSegment(
		REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);
};

class AM_NOVTABLE CQTransformFilter : public CTransformFilter
{
public:
    CQTransformFilter(TCHAR *name, LPUNKNOWN punk, REFCLSID clsid)
		:CTransformFilter(name, punk, clsid){};
#ifdef UNICODE
    CQTransformFilter(CHAR * name, LPUNKNOWN punk, REFCLSID clsid)
		:CTransformFilter(name, punk, clsid){};
#endif
	CBasePin * GetPin(int n);
	virtual HRESULT Receive(IMediaSample *pSample);

	// CTransformFilter's bug fix
	STDMETHODIMP Pause(){CAutoLock lock_it(m_pLock);return CTransformFilter::Pause();}
	STDMETHODIMP Stop(){CAutoLock lock_it(m_pLock);return CTransformFilter::Stop();}
};
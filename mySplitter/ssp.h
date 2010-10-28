#include <streams.h>
#include <dvdmedia.h>
#include "filter.h"
#include "image.h"

class CDWindowSSP : public CTransformFilter
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IYV12MonoMixer and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// CTransformFilter Bug FIX
	STDMETHODIMP Pause(){CAutoLock lock_it(m_pLock);return CTransformFilter::Pause();}
	STDMETHODIMP Stop(){CAutoLock lock_it(m_pLock);return CTransformFilter::Stop();}

	// Overrriden from CSplitFilter base class
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
    HRESULT BreakConnect(PIN_DIRECTION dir);

	// IBaseFilter::JoinFilter
	STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);

private:
	CComPtr<IBaseFilter> m_demuxer;
	HANDLE h_F11_thread;
	static DWORD WINAPI F11Thread(LPVOID lpParame);
	static DWORD WINAPI default_thread(LPVOID lpParame);		//reset graph to default thread
	bool my12doom_found;
	int m_image_x;
	int m_image_y;
	int m_left;
	BYTE *m_image_buffer;
	CCritSec m_DWindowSSPLock;
	CDWindowSSP(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CDWindowSSP();

	// watermark
	int m_frm;
	unsigned char *m_mask;
	int m_mask_width;
	int m_mask_height;
	void prepare_mask();
};
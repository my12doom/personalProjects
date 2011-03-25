#include <atlbase.h>
#include <streams.h>
#include <dvdmedia.h>
#include "filter.h"
#include "image.h"

class CDWindowSSP : public CTransformFilter, public IPropertyBag
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// IPropertyBag
	STDMETHODIMP Read(LPCOLESTR pszPropName,	VARIANT *pVar,	IErrorLog *pErrorLog)
	{
		return S_OK;
	}
	STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar)
	{
		return S_OK;
	}

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
	//HRESULT StartStreaming();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);

	// IBaseFilter::JoinFilter
	STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);

protected:
	bool m_mvc_actived;
	HRESULT find_and_active_mvc();
	HRESULT modules_check();

private:
	REFERENCE_TIME m_t;
	bool my12doom_found;
	int m_image_x;
	int m_image_y;
	BYTE *m_image_buffer;
	bool m_buffer_has_data;
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
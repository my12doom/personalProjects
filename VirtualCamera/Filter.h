#include <streams.h>
#include <initguid.h>
extern "C" const GUID CLSID_VirtualCamera;

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

const REFERENCE_TIME FPS_30 = UNITS / 30;
const REFERENCE_TIME FPS_20 = UNITS / 20;
const REFERENCE_TIME FPS_10 = UNITS / 10;
const REFERENCE_TIME FPS_5  = UNITS / 5;
const REFERENCE_TIME FPS_4  = UNITS / 4;
const REFERENCE_TIME FPS_3  = UNITS / 3;
const REFERENCE_TIME FPS_2  = UNITS / 2;
const REFERENCE_TIME FPS_1  = UNITS / 1;

const REFERENCE_TIME rtDefaultFrameLength = FPS_10;

class CVCamPin;

class CVCam : public CSource
{
public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFilterGraph *GetGraph() {return m_pGraph;}
	
private:
    CVCam(LPUNKNOWN lpunk, HRESULT *phr);
	CVCamPin *m_pPin;

};


class CVCamPin : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
protected:
    CVCam *m_pParent;
	HCURSOR m_hCursor;
	
    int m_FramesWritten;				// To track where we are in the file
    BOOL m_bZeroMemory;                 // Do we need to clear the buffer?
    CRefTime m_rtSampleTime;	        // The time stamp for each sample
	
    int m_iFrameNumber;
    const REFERENCE_TIME m_rtFrameLength;
	
    RECT m_rScreen;                     // Rect containing entire screen coordinates
	
    int m_iImageHeight;                 // The current image height
    int m_iImageWidth;                  // And current image width
    int m_iRepeatTime;                  // Time in msec between frames
    int m_nCurrentBitDepth;             // Screen bit depth
	
    CMediaType m_MediaType;
    CCritSec m_cSharedState;            // Protects our internal state
    CImageDisplay m_Display;            // Figures out our media type for us
	
public:
	
    CVCamPin(HRESULT *phr, CSource *pFilter);
    ~CVCamPin();
	
    // Override the version that offers exactly one media type
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT FillBuffer(IMediaSample *pSample);
    
    // Set the agreed media type and set up the necessary parameters
    HRESULT SetMediaType(const CMediaType *pMediaType);
	
    // Support multiple display formats
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	
    // Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    }




	//////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv){
		if(riid == _uuidof(IAMStreamConfig))
			*ppv = (IAMStreamConfig*)this;
		else if(riid == _uuidof(IKsPropertySet))
			*ppv = (IKsPropertySet*)this;
		else
			return CSourceStream::QueryInterface(riid, ppv);
		
		AddRef();
		return S_OK;
	}
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
	STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	//////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

};
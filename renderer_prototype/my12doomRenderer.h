#include <streams.h>

struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_TextureRenderer;

class CTextureRenderer : public CBaseVideoRenderer
{
public:
	CTextureRenderer(LPUNKNOWN pUnk,HRESULT *phr);
	~CTextureRenderer();

public:
	HRESULT CheckMediaType(const CMediaType *pmt );     // Format acceptable?
	HRESULT SetMediaType(const CMediaType *pmt );       // Video format notification
	HRESULT DoRenderSample(IMediaSample *pMediaSample); // New video sample

	STDMETHODIMP Run(REFERENCE_TIME tStart){m_state = State_Running; return __super::Run(tStart);}
	STDMETHODIMP Pause(){m_state = State_Paused; return __super::Pause();}
	STDMETHODIMP Stop(){m_state = State_Stopped; return __super::Stop();}

	CCritSec m_data_lock;
	BYTE *m_data;
	bool m_data_changed;
	LONG m_lVidWidth;   // Video width
	LONG m_lVidHeight;  // Video Height

	OAFilterState m_state;
};
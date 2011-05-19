#include "..\mysplitter\QTransfrm.h"

class C2to1InputPin: public CTransformInputPin
{
public:
	C2to1InputPin(LPCTSTR pObjectName, CTransformFilter *pTransformFilter, HRESULT * phr, LPCWSTR pName, int id)
		:CTransformInputPin(pObjectName, pTransformFilter, phr, pName), m_id(id){}
#ifdef UNICODE
	C2to1InputPin(LPCSTR pObjectName, CTransformFilter *pTransformFilter, HRESULT * phr, LPCWSTR pName, int id)
		:CTransformInputPin(pObjectName, pTransformFilter, phr, pName), m_id(id){}
#endif
	STDMETHODIMP Receive(IMediaSample * pSample);
protected:
	int m_id;
};

class C2to1Filter: public CTransformFilter
{
public:
	friend class C2to1InputPin;
	C2to1Filter(TCHAR *name, LPUNKNOWN punk, REFCLSID clsid)
		:CTransformFilter(name, punk, clsid){};
#ifdef UNICODE
	C2to1Filter(CHAR * name, LPUNKNOWN punk, REFCLSID clsid)
		:CTransformFilter(name, punk, clsid){};
#endif


	virtual int GetPinCount();
	virtual CBasePin * GetPin(int n);
	STDMETHODIMP FindPin(LPCWSTR Id, __deref_out IPin **ppPin);



	virtual HRESULT Receive(IMediaSample *pSample, int id);
	virtual HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut, int id);
	HRESULT InitializeOutputSample(IMediaSample *pSample, __deref_out IMediaSample **ppOutSample, int id);
protected:
	CTransformInputPin *m_pInput2;
};
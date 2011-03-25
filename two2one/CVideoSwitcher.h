#include <atlbase.h>
#include <streams.h>
#include <dvdmedia.h>
#include "two2one.h"

class CVideoSwitcher : public C2to1Filter
{
public:
	CVideoSwitcher(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CVideoSwitcher();

	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// Transform!
	virtual HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut, int id);

protected:
	int m_in_x;
	int m_in_y;
	int m_out_x;	// not sbs, is 1920 or 1280
	int m_out_y;
};
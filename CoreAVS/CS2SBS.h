#include <atlbase.h>
#include <streams.h>
#include <dvdmedia.h>
#include "..\two2one\two2one.h"

class packet
{
public:
	packet(IMediaSample *sample)
	{
		m_data = (BYTE*) malloc(sample->GetActualDataLength());
		sample->GetTime(&start, &end);
		BYTE *src;
		sample->GetPointer(&src);
		memcpy(m_data, src, sample->GetActualDataLength());
	}
	~packet()
	{
		free(m_data);
	}

	REFERENCE_TIME start, end;
	BYTE *m_data;
};

class CS2SBS : public C2to1Filter
{
public:
	CS2SBS(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CS2SBS();

	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// Transform!
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	virtual HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut, int id);

protected:

	CGenericList<packet> m_left_queue;
	CGenericList<packet> m_right_queue;

	int m_in_x;
	int m_in_y;

	REFERENCE_TIME m_stream_time;
};
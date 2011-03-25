#include <stdio.h>
#include <initguid.h>
#include <streams.h>
#include "3dvDemuxer.h"

// {FBCFD6AF-B25F-4a6d-AE56-5B5303F1A40E}
DEFINE_GUID(CLSID_my12doomSource, 
			0xfbcfd6af, 0xb25f, 0x4a6d, 0xae, 0x56, 0x5b, 0x53, 0x3, 0xf1, 0xa4, 0xe);

class my12doomSourceStream;

class my12doomSource : public CSource, public IFileSourceFilter
{
public:
	// IUnkown
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// IFileSourceFilter
	HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, __in_opt const AM_MEDIA_TYPE *pmt);
	HRESULT STDMETHODCALLTYPE GetCurFile(__out LPOLESTR *ppszFileName, __out_opt AM_MEDIA_TYPE *pmt);
protected:
	C3dvDemuxer *m_demuxer;
	WCHAR m_curfile[MAX_PATH];
private:
	my12doomSource(LPUNKNOWN lpunk, HRESULT *phr);
	~my12doomSource();
};

class my12doomSourceStream : public CSourceStream, public CSourceSeeking
{
public:
	my12doomSourceStream(HRESULT *phr, my12doomSource *pParent, LPCWSTR pPinName, stream_3dv *stream);
	~my12doomSourceStream();
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// plots a ball into the supplied video frame
	HRESULT FillBuffer(IMediaSample *pms);

	// Ask for buffers of the size appropriate to the agreed media type
	HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,	ALLOCATOR_PROPERTIES *pProperties);

	// Set the agreed media type, and set up the necessary ball parameters
	HRESULT SetMediaType(const CMediaType *pMediaType);

	// Because we calculate the ball there is no reason why we
	// can't calculate it in any one of a set of formats...
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);

	// Resets the stream time to zero
	HRESULT OnThreadCreate(void);

	// Quality control notifications sent to us
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	// CSourceSeeking
	HRESULT OnThreadStartPlay();
	HRESULT ChangeStart();
	HRESULT ChangeStop();
	HRESULT ChangeRate();
	STDMETHODIMP SetRate(double);

protected:
	bool m_need_I_frame;
	stream_3dv *m_stream;
	int m_frame_number;
	int m_max_buffer_size;
	int m_type;
	CCritSec m_cSharedState;            // Lock on m_rtSampleTime
	void UpdateFromSeek();
	BOOL            m_bDiscontinuity; // If true, set the discontinuity flag.
	REFERENCE_TIME m_stream_start;
};
#include <streams.h>
#include "filter.h"
#include "splitbase.h"

class CDWindowExtenderMono : public CTransformFilter, public IDWindowExtender
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IDWindowExtenderMono and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Overrriden from CSplitFilter base class
	HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming();

	// Implementation of  the custom IDWindowExtender interface
	HRESULT STDMETHODCALLTYPE SetCallback(IDWindowFilterCB * cb);
	HRESULT STDMETHODCALLTYPE SetMode(int mode, int extend);					// config before connect
	HRESULT STDMETHODCALLTYPE GetLetterboxHeight(int *max_delta);
	HRESULT STDMETHODCALLTYPE SetLetterbox(int delta);							// delta = top - bottom

private:
	// filter control
	CCritSec m_config_sec;

	// callback related vars
	REFERENCE_TIME m_this_stream_start;
	IDWindowFilterCB *m_cb;

	// image vars and funcs
	int m_letterbox_total;
	int m_letterbox_top;
	int m_letterbox_bottom;

	int m_in_x;
	int m_in_y;
	int m_out_x;
	int m_out_y;
	int m_image_x;
	int m_image_y;
	int m_byte_per_4_pixel;
	GUID m_subtype;
	bool m_topdown;
	HRESULT init_image();
	HRESULT Transform_YV12(IMediaSample * pIn, IMediaSample *pOut);
	HRESULT Transform_YUY2(IMediaSample * pIn, IMediaSample *pOut);

	// no split mode, mode == direct copy
	int m_mode;// = DWindowFilter_CUT_MODE_AUTO
	int m_extend;

	// you know what
	CDWindowExtenderMono(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CDWindowExtenderMono();
};


class CDWindowExtenderStereo : public CSplitFilter, public IDWindowExtender
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IYV12StereoMixer and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Overrriden from CSplitFilter base class
	HRESULT Split(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckSplit(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	// Implementation of  the custom IYV12StereoMixer interface
	HRESULT STDMETHODCALLTYPE SetCallback(IDWindowFilterCB * cb);
	HRESULT STDMETHODCALLTYPE SetMode(int mode, int extend);				// config before connect
	HRESULT STDMETHODCALLTYPE GetLetterboxHeight(int *max_delta);
	HRESULT STDMETHODCALLTYPE SetLetterbox(int delta);						// delta = top - bottom

private:
	// filter control
	CCritSec m_config_sec;

	// callback related vars
	REFERENCE_TIME m_this_stream_start;
	IDWindowFilterCB *m_cb;

	// image vars and funcs
	int m_left;				// for seq split
	int m_letterbox_total;
	int m_letterbox_top;
	int m_letterbox_bottom;
	int m_in_x;
	int m_in_y;
	int m_out_x;
	int m_out_y;
	int m_image_x;
	int m_image_y;
	int m_byte_per_4_pixel;
	GUID m_subtype;
	bool m_topdown;
	HRESULT init_image();
	HRESULT Split_YV12(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2);
	HRESULT Split_YUY2(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2);

	// PD10 timecode fix and frame buffer
	BYTE *m_frame_buffer;
	REFERENCE_TIME m_TimeStart;
	REFERENCE_TIME m_TimeEnd;
	REFERENCE_TIME m_MediaStart;
	REFERENCE_TIME m_MediaEnd;

	// split mode
	int m_mode;
	int m_extend;

	// watermark
	int m_frm;
	unsigned char *m_mask;
	int m_mask_width;
	int m_mask_height;
	void prepare_mask();

	// you know what
	CDWindowExtenderStereo(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CDWindowExtenderStereo();
};
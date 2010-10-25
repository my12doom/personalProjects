#include <streams.h>
#include <dvdmedia.h>
#include "filter.h"
#include "image.h"
#include "splitbase.h"

class CYV12StereoMixer : public CSplitFilter, public IYV12Mixer, public image_processor_stereo
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

	// Implementation of  the custom IYV12StereoMixer interface
	HRESULT STDMETHODCALLTYPE SetCallback(IDWindowFilterCB * cb);
	HRESULT STDMETHODCALLTYPE SetMode(int mode, int extend, int max_mask_buffer = -1);	// 必须在input连接之前设定
	HRESULT STDMETHODCALLTYPE SetColor(DWORD color);									// 左右交换，下一帧生效
	HRESULT STDMETHODCALLTYPE Revert();													// 左右交换，下一帧生效
	HRESULT STDMETHODCALLTYPE GetLetterboxHeight(int *max_delta);						// 获得黑边总高度
	HRESULT STDMETHODCALLTYPE SetLetterbox(int delta);									// 设定上黑边比下黑边宽多少，可正可负，下一帧生效
	HRESULT STDMETHODCALLTYPE SetMask(unsigned char *mask,								//
		int width, int height,															//单侧，另一侧自动复制
		int left, int top)	;															//
	HRESULT STDMETHODCALLTYPE SetMaskPos(int left, int top,	int offset);				// 位置也是以单侧为参考系，颜色为AYUV（0-255）空间
																						// offset为右眼-左眼，可正可负

private:
	REFERENCE_TIME m_this_stream_start;

	CCritSec m_YV12StereoMixerLock;
	bool m_revert;
	IDWindowFilterCB *m_cb;

	CYV12StereoMixer(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CYV12StereoMixer();
};
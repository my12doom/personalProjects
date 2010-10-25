#include <windows.h>
#include "filter.h"
#include "extender.h"

CDWindowExtenderMono::CDWindowExtenderMono(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_DWindowMono)
{
	m_letterbox_total = m_letterbox_top = m_letterbox_bottom = 0;
	m_in_x = 0;
	m_in_y = 0;
	m_out_x = 0;
	m_out_y = 0;
	m_image_x = 0;
	m_image_y = 0;
	m_mode = DWindowFilter_CUT_MODE_AUTO;
	m_extend = DWindowFilter_EXTEND_NONE;
	m_byte_per_4_pixel = 0;
	m_subtype = GUID_NULL;
	m_topdown = false;
	m_cb = NULL;
} 

CDWindowExtenderMono::~CDWindowExtenderMono() 
{
}

// CreateInstance
// Provide the way for COM to create a DWindowExtenderMono object
CUnknown *CDWindowExtenderMono::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CDWindowExtenderMono *pNewObject = new CDWindowExtenderMono(NAME("DWindow Extender Mono"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

// NonDelegatingQueryInterface
// Reveals IYV12Mixer
STDMETHODIMP CDWindowExtenderMono::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == __uuidof(IDWindowExtender)) 
		return GetInterface((IDWindowExtender *) this, ppv);

	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

// CheckInputType
HRESULT CDWindowExtenderMono::CheckInputType(const CMediaType *mtIn)
{
	if (m_mode != DWindowFilter_CUT_MODE_AUTO)
		return VFW_E_INVALID_MEDIA_TYPE;

	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;
	if( *mtIn->FormatType() == FORMAT_VideoInfo && 
		(subtypeIn == MEDIASUBTYPE_YV12 || subtypeIn == MEDIASUBTYPE_YUY2))
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER*)mtIn->Format())->bmiHeader;
		m_subtype = subtypeIn;
		m_in_x = pbih->biWidth;
		m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && 
		(subtypeIn == MEDIASUBTYPE_YV12 || subtypeIn == MEDIASUBTYPE_YUY2))

	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_subtype = subtypeIn;
		m_in_x = pbih->biWidth;
		m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	if (hr == S_OK)
	{
		if (m_in_y < 0)
		{
			m_topdown = true;
			m_in_y = -m_in_y;
		}

		hr = init_image();

		if (m_subtype == MEDIASUBTYPE_YV12)
			m_byte_per_4_pixel = 6;
		else if (m_subtype == MEDIASUBTYPE_YUY2)
			m_byte_per_4_pixel = 8;
		else if (m_subtype == MEDIASUBTYPE_RGB32)
			m_byte_per_4_pixel = 16;
	}

	return hr;
}

// CheckTransform
// Check a Transform can be done between these formats
// DecideBufferSize
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
HRESULT CDWindowExtenderMono::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == subtypeOut && subtypeIn == m_subtype)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}

HRESULT CDWindowExtenderMono::init_image()
{
	m_image_x = m_out_x = m_in_x;
	m_image_y = m_out_y = m_in_y;

	//计算目标高度
	int out_y_extended = m_image_y;
	double aspect_x = -1, aspect_y = -1;
	if (m_extend == DWindowFilter_EXTEND_NONE)
		aspect_x = aspect_y = -1;
	else if (m_extend == DWindowFilter_EXTEND_43)
		aspect_x = 4, aspect_y = 3;
	else if (m_extend == DWindowFilter_EXTEND_54)
		aspect_x = 5, aspect_y = 4;
	else if (m_extend == DWindowFilter_EXTEND_169)
		aspect_x = 16,aspect_y = 9;
	else if (m_extend == DWindowFilter_EXTEND_1610)
		aspect_x = 16,aspect_y =10;
	else if (m_extend > 5 && m_extend < 0xf0000) //DWindowFilter_EXTEND_CUSTOM
	{
		aspect_x = (m_extend -5) & 0xff;
		aspect_y = ((m_extend -5) & 0xff00) >> 8;
	}
	else if ((m_extend & 0xff0000) == 0xf0000)		 //DWindowFilter_EXTEND_TO
		out_y_extended = m_extend & 0xffff;
	else if ((m_extend & 0xf00000) == 0xf00000)		 //DWindowFilter_EXTEND_CUSTOM_DECIMAL
	{
		aspect_x = (double) (m_extend & 0xfffff) / 100000;
		aspect_y = 1;
	}

	if (aspect_x > 0 && aspect_y > 0)
		out_y_extended = (int)(m_image_x * aspect_y / aspect_x);

	//计算黑边高度
	if (m_out_y >= out_y_extended)
	{
		m_extend = 0;
		m_letterbox_top = m_letterbox_bottom = m_letterbox_total = 0;
		return S_OK;
	}
	m_letterbox_total = out_y_extended - m_out_y;					//黑边总高度
	m_letterbox_top = m_letterbox_bottom = m_letterbox_total /2;	//默认为上下等高

	m_out_y = out_y_extended;

	if (m_out_x % 2 )
		m_out_x ++;

	if (m_out_y % 2 )
		m_out_y ++;

	return S_OK;
}

// GetMediaType
// Returns the supported media types in order of preferred  types (starting with iPosition=0)
HRESULT CDWindowExtenderMono::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;

	// Do we have more items to offer
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;


	// get input dimensions
	CMediaType *inMediaType = &m_pInput->CurrentMediaType();
	if (*inMediaType->FormatType() == FORMAT_VideoInfo)
	{
		VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)inMediaType->Format();
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetFormatType(&FORMAT_VideoInfo);
		pMediaType->SetSubtype(&m_subtype);
		pMediaType->SetSampleSize(m_out_x*m_out_y*m_byte_per_4_pixel/4);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER *vihOut = (VIDEOINFOHEADER *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER));
		RECT zero = {0,0,m_out_x,m_out_y};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->bmiHeader.biWidth = m_out_x ;
		vihOut->bmiHeader.biHeight = m_out_y;
		if (m_topdown) vihOut->bmiHeader.biHeight = -m_out_y;

		vihOut->bmiHeader.biXPelsPerMeter = 1 ;
		vihOut->bmiHeader.biYPelsPerMeter = 1;
	}
	else if (*inMediaType->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)inMediaType->Format();
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetFormatType(&FORMAT_VideoInfo2);
		pMediaType->SetSubtype(&m_subtype);
		pMediaType->SetSampleSize(m_out_x*m_out_y*m_byte_per_4_pixel/4);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));
		RECT zero = {0,0,0,0};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->bmiHeader.biWidth = m_out_x ;
		vihOut->bmiHeader.biHeight = m_out_y;
		if (m_topdown) vihOut->bmiHeader.biHeight = -m_out_y;

		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源

		// compute aspect
		int aspect_x = m_out_x;
		int aspect_y = m_out_y;
compute_aspect:
		for(int i=2; i<100; i++)
		{
			if (aspect_x % i == 0 && aspect_y % i == 0)
			{
				aspect_x /= i;
				aspect_y /= i;
				goto compute_aspect;
			}
		}
		vihOut->dwPictAspectRatioX = aspect_x;
		vihOut->dwPictAspectRatioY = aspect_y;
	}

	return NOERROR;
}

HRESULT CDWindowExtenderMono::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock configlock(&m_config_sec);
	REFERENCE_TIME TimeStart, TimeEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);
	if (m_cb)
		m_cb->SampleCB(TimeStart+m_this_stream_start, TimeEnd+m_this_stream_start, pIn);		// callback

	LONGLONG MediaStart, MediaEnd;
	pIn->GetMediaTime(&MediaStart,&MediaEnd);

	HRESULT hr = S_OK;

	if (pOut)
	{
		if (m_subtype == MEDIASUBTYPE_YV12)
			hr = Transform_YV12(pIn, pOut);
		else if (m_subtype == MEDIASUBTYPE_YUY2)
			hr = Transform_YUY2(pIn, pOut);

		pOut->SetTime(&TimeStart, &TimeEnd);
		pOut->SetMediaTime(&MediaStart,&MediaEnd);
		pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
		pOut->SetPreroll(pIn->IsPreroll() == S_OK);
		pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
	}

	return hr;
}



HRESULT CDWindowExtenderMono::Transform_YV12(IMediaSample * pIn, IMediaSample *pOut)
{

	unsigned char *pSrc = 0;
	pIn->GetPointer((unsigned char **)&pSrc);
	unsigned char *pSrcV = pSrc + m_in_x*m_in_y;
	unsigned char *pSrcU = pSrcV+ (m_in_x/2)*(m_in_y/2);
	int stride = pOut->GetSize() *2/3 / m_out_y;

	unsigned char *pDst = 0;
	pOut->GetPointer(&pDst);

	unsigned char *pDstV = pDst + stride*m_out_y;
	unsigned char *pDstU = pDstV+ (stride/2)*(m_out_y/2);



	if (stride < m_out_x)
		return E_UNEXPECTED;

	// letterbox top
	if (m_letterbox_top > 0)
	{
		memset(pDst, 0, m_letterbox_top * stride);
		memset(pDstV, 128, (m_letterbox_top/2) * stride/2);
		memset(pDstU, 128, (m_letterbox_top/2) * stride/2);
	}

	// copy image
	if (m_image_y > 0)
	{
		int line_dst = m_letterbox_top;
		for(int y = 0; y<m_image_y; y++)
		{
			//copy y
			memcpy(pDst+(line_dst + y)*stride, pSrc+y*m_in_x, m_out_x);
		}

		for(int y = 0; y<m_image_y/2; y++)
		{
			//copy v
			memcpy(pDstV +(line_dst/2+ y)*stride/2, pSrcV+y*m_in_x/2, m_out_x/2);

			//copy u
			memcpy(pDstU +(line_dst/2 +y)*stride/2, pSrcU+y*m_in_x/2, m_out_x/2);
		}
	}

	// letterbox bottom
	if (m_letterbox_bottom > 0)
	{
		int line = m_letterbox_top + m_image_y;
		memset(pDst +stride*line, 0, m_letterbox_bottom * stride);
		memset(pDstV+(stride/2)*(line/2), 128, (m_letterbox_bottom/2) * stride/2);
		memset(pDstU+(stride/2)*(line/2), 128, (m_letterbox_bottom/2) * stride/2);
	}

	return S_OK;
}

HRESULT CDWindowExtenderMono::Transform_YUY2(IMediaSample * pIn, IMediaSample *pOut)
{
	// one line of letterbox
	static DWORD one_line_letterbox[16384];
	if (one_line_letterbox[0] != 0x80008000)
		for (int i=0; i<16384; i++)
			one_line_letterbox[i] = 0x80008000;

	// basic infos
	int letterbox_top = m_letterbox_top;
	int letterbox_bottom = m_letterbox_bottom;
	if (m_topdown)
	{
		letterbox_top = m_letterbox_bottom;
		letterbox_bottom = m_letterbox_top;
	}

	// pointers
	BYTE *p_pin;
	BYTE *p_out;
	pIn->GetPointer(&p_pin);
	pOut->GetPointer(&p_out);
	LONG data_size = pIn->GetActualDataLength();
	LONG out_size = pOut->GetActualDataLength();
	const int byte_per_pixel = 2;
	int stride = out_size / m_out_y / byte_per_pixel;
	if (stride < m_out_x)
		return E_UNEXPECTED;

	// letterbox top
	for (int y=0; y<letterbox_top; y++)
		memcpy(p_out + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);
	p_out += stride * letterbox_top * byte_per_pixel;

	// image
	for (int y=0; y<m_image_y; y++)
		memcpy(p_out+y * stride*byte_per_pixel, 
			   p_pin + m_in_x * y*byte_per_pixel,  m_out_x*byte_per_pixel);
	p_out += stride * m_image_y * byte_per_pixel;

	// letterbox bottom
	for (int y=0; y<letterbox_bottom; y++)
		memcpy(p_out + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);

	return S_OK;
}

HRESULT CDWindowExtenderMono::StartStreaming()
{
	CComQIPtr<IBaseFilter, &IID_IBaseFilter> pbase(this);
	FILTER_INFO fi;
	pbase->QueryFilterInfo(&fi);

	if (NULL == fi.pGraph)
		return 0;

	CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> pMS(fi.pGraph);
	pMS->GetCurrentPosition(&m_this_stream_start);

	fi.pGraph->Release();

	return 0;
}

HRESULT CDWindowExtenderMono::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_out_x * m_out_y * m_byte_per_4_pixel / 4;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;


	return NOERROR;
}

//////////////////////////////////////////////////////////////////////////
// IYV12MonoMixer
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CDWindowExtenderMono::SetCallback(IDWindowFilterCB * cb)
{
	CAutoLock cAutolock(&m_config_sec);

	m_cb = cb;
	return NOERROR;

}
HRESULT STDMETHODCALLTYPE CDWindowExtenderMono::SetMode(int mode, int extend)
{
	CAutoLock cAutolock(&m_config_sec);
	// ignore any cut mode..
	m_extend = extend;
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CDWindowExtenderMono::GetLetterboxHeight(int *max_delta)
{
	CAutoLock cAutolock(&m_config_sec);

	if (max_delta == NULL)
		return E_POINTER;

	*max_delta = m_letterbox_total;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDWindowExtenderMono::SetLetterbox(int delta)								// 设定上黑边比下黑边宽多少，可正可负，下一帧生效
{
	CAutoLock cAutolock(&m_config_sec);

	if (m_extend == 0)
		return NOERROR;

	if (delta > m_letterbox_total)
		delta = m_letterbox_total;
	if (delta < - m_letterbox_total)
		delta = -m_letterbox_total;

	int remain = (m_letterbox_total - delta) / 2;
	m_letterbox_top = remain + delta;
	m_letterbox_bottom = remain;

	return NOERROR;
}

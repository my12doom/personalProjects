#include <windows.h>
#include "filter_mono.h"

//////////////////////////////////////////////////////////////////////////
// Most of this code is 'embraced and extended from the DSWIzard filter wizard 
// generated code. My main contribution is the indentation and the resizing algorithm
// rep.movsd@gmail.com
//////////////////////////////////////////////////////////////////////////

CYV12MonoMixer::CYV12MonoMixer(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_YV12MonoMixer)
{
	m_cb = NULL;
} 

CYV12MonoMixer::~CYV12MonoMixer() 
{
	free_masks();
}

// CreateInstance
// Provide the way for COM to create a YV12MonoMixer object
CUnknown *CYV12MonoMixer::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CYV12MonoMixer *pNewObject = new CYV12MonoMixer(NAME("YV12MonoMixer"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

// NonDelegatingQueryInterface
// Reveals IYV12Mixer
STDMETHODIMP CYV12MonoMixer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == __uuidof(IYV12Mixer)) 
		return GetInterface((IYV12Mixer *) this, ppv);

	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CYV12MonoMixer::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	REFERENCE_TIME TimeStart, TimeEnd;

	pIn->GetTime(&TimeStart, &TimeEnd);

	if (m_cb)
		m_cb->SampleCB(TimeStart+m_this_stream_start, TimeEnd+m_this_stream_start, pIn);		// callback

	unsigned char *pSrc = 0;
	pIn->GetPointer((unsigned char **)&pSrc);
	unsigned char *pSrcV = pSrc + m_in_x*m_in_y;
	unsigned char *pSrcU = pSrcV+ (m_in_x/2)*(m_in_y/2);

	LONGLONG MediaStart, MediaEnd;
	pIn->GetMediaTime(&MediaStart,&MediaEnd);

	if (pOut)
	{
		int stride = pOut->GetSize() *2/3 / m_out_y;

		unsigned char *pDst = 0;
		pOut->GetPointer(&pDst);

		unsigned char *pDstV = pDst + stride*m_out_y;
		unsigned char *pDstU = pDstV+ (stride/2)*(m_out_y/2);

		//从inner复制到out(显存)，不要读取显存，左半

		if (stride >= m_out_x)
		{
			// letterbox top
			if (m_letterbox_top > 0)
			{
				memset(pDst, 0, m_letterbox_top * stride);
				memset(pDstV, 128, (m_letterbox_top/2) * stride/2);
				memset(pDstU, 128, (m_letterbox_top/2) * stride/2);
			}

			// free_zone_top
			if (m_free_zone_top > 0)
			{
				int line_dst = m_letterbox_top;
				for(int y = 0; y<m_free_zone_top; y++)
				{
					//copy y
					memcpy(pDst+(line_dst + y)*stride, pSrc+y*m_in_x, m_out_x);
				}

				for(int y = 0; y<m_free_zone_top/2; y++)
				{
					//copy v
					memcpy(pDstV +(line_dst/2+ y)*stride/2, pSrcV+y*m_in_x/2, m_out_x/2);

					//copy u
					memcpy(pDstU +(line_dst/2 +y)*stride/2, pSrcU+y*m_in_x/2, m_out_x/2);
				}
			}

			// mix_zone
			if (m_mix_zone  > 0)
			{
				int line_dst = m_letterbox_top + m_free_zone_top;
				int line_src = m_free_zone_top;

				int line_start = max(0, -m_mask_pos_top);

				// out = input * mask_n(i) + mask(i) * color
				//Y
				for (int y=0; y<m_mix_zone; y++)		
				{
					for (int x=0; x<m_out_x; x++)
					{
						pDst[(y+line_dst) * stride + x] = (pSrc[(y+line_src)*m_in_x+x] * m_mask_n[(y+line_start)*m_out_x + x] 
						+ m_mask_y[(y+line_start)*m_out_x + x]) /255;
					}
				}

				// V
				for (int y=0/2; y<m_mix_zone/2; y++)
				{
					for (int x=0; x<m_out_x/2; x++)
					{
						pDstV[(y+line_dst/2) * stride/2+x] = (pSrcV[(y+line_src/2) * m_in_x/2 + x]  
						*m_mask_uv_n[(y+line_start/2)*m_out_x/2+x]
						+ m_mask_v[(y+line_start/2)*m_out_x/2 +x]) /255;
					}
				}

				// U
				for (int y=0/2; y<m_mix_zone/2; y++)
				{
					for (int x=0; x<m_out_x/2; x++)
					{
						pDstU[(y+line_dst/2) * stride/2+x] = (pSrcU[(y+line_src/2) * m_in_x/2 + x]  
						*m_mask_uv_n[(y+line_start/2)*m_out_x/2+x]
						+ m_mask_u[(y+line_start/2)*m_out_x/2 +x]) /255;
					}
				}

			}

			// free_zone_bottom
			if (m_free_zone_bottom > 0)
			{
				int line_dst = m_letterbox_top + m_free_zone_top + m_mix_zone;
				int line_src = m_free_zone_top + m_mix_zone;
				for(int y = 0; y<m_free_zone_bottom; y++)
				{
					//copy y
					memcpy(pDst+(line_dst + y)*stride, pSrc+(line_src+y)*m_in_x, m_out_x);
				}

				for(int y = 0; y<m_free_zone_bottom/2; y++)
				{
					//copy v
					memcpy(pDstV +(line_dst/2+ y)*stride/2, pSrcV+(line_src/2+y)*m_in_x/2, m_out_x/2);

					//copy u
					memcpy(pDstU +(line_dst/2 +y)*stride/2, pSrcU+(line_src/2+y)*m_in_x/2, m_out_x/2);
				}
			}

			// letterbox bottom
			if (m_letterbox_bottom > 0)
			{
				int line = m_letterbox_top + m_free_zone_top + m_mix_zone + m_free_zone_bottom;
				memset(pDst +stride*line, 0, m_letterbox_bottom * stride);
				memset(pDstV+(stride/2)*(line/2), 128, (m_letterbox_bottom/2) * stride/2);
				memset(pDstU+(stride/2)*(line/2), 128, (m_letterbox_bottom/2) * stride/2);
			}

			// possible mask on letterbox
			// 黑边+字幕区
			{
				int m_letterbox_mask_start = -1;
				int m_letterbox_mask_end = -1;
				int m_letterbox_mask_src_line = 0;
				//上部时
				if (m_mask_pos_top < 0 )
				{
					m_letterbox_mask_start = m_letterbox_top + m_mask_pos_top;
					m_letterbox_mask_end = m_letterbox_mask_start + m_mask_height;
					m_letterbox_mask_src_line = 0;
					for (int y=max(0,m_letterbox_mask_start);
						y<m_letterbox_mask_end && y<m_letterbox_top; y++)
					{
						memcpy(pDst + y*stride, m_mask_letterbox_y + (y-m_letterbox_mask_start+m_letterbox_mask_src_line) * m_out_x, m_out_x);
					}
					for (int y=max(0,m_letterbox_mask_start/2);
						y<m_letterbox_mask_end/2 && y<m_letterbox_top/2; y++)
					{
						memcpy(pDstV + y*stride/2, 
							m_mask_letterbox_v + (y-m_letterbox_mask_start/2+m_letterbox_mask_src_line/2) * m_out_x/2, m_out_x/2);
					}
					for (int y=max(0,m_letterbox_mask_start/2);
						y<m_letterbox_mask_end/2 && y<m_letterbox_top/2; y++)
					{
						memcpy(pDstU + y*stride/2, 
							m_mask_letterbox_u + (y-m_letterbox_mask_start/2+m_letterbox_mask_src_line/2) * m_out_x/2, m_out_x/2);
					}
				}
				//下部时
				else if (m_mask_pos_top + m_mask_height > m_image_y)
				{
					m_letterbox_mask_start = m_letterbox_top + m_mask_pos_top;
					m_letterbox_mask_end = m_letterbox_top + m_mask_pos_top + m_mask_height;
					m_letterbox_mask_src_line = m_mask_height -( m_letterbox_mask_end - m_letterbox_mask_start);
					for (int y=max(m_letterbox_top + m_image_y,m_letterbox_mask_start);
						y<m_letterbox_mask_end && y<m_out_y; y++)
					{
						memcpy(pDst + y*stride, 
							m_mask_letterbox_y + (y-m_letterbox_mask_start+m_letterbox_mask_src_line) * m_out_x, m_out_x);
					}
					for (int y=max(m_letterbox_top + m_image_y,m_letterbox_mask_start)/2;
						y<m_letterbox_mask_end/2 && y<m_out_y/2; y++)
					{
						memcpy(pDstV + y*stride/2, 
							m_mask_letterbox_v + (y-m_letterbox_mask_start/2+m_letterbox_mask_src_line/2) * m_out_x/2, m_out_x/2);
					}
					for (int y=max(m_letterbox_top + m_image_y,m_letterbox_mask_start)/2;
						y<m_letterbox_mask_end/2 && y<m_out_y/2; y++)
					{
						memcpy(pDstU + y*stride/2, 
							m_mask_letterbox_u + (y-m_letterbox_mask_start/2+m_letterbox_mask_src_line/2) * m_out_x/2, m_out_x/2);
					}
				}
			}
		}
		else
			return E_UNEXPECTED;


		pOut->SetTime(&TimeStart, &TimeEnd);
		pOut->SetMediaTime(&MediaStart,&MediaEnd);
		pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
		pOut->SetPreroll(pIn->IsPreroll() == S_OK);
		pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
	}
	return NOERROR;
}

// CheckInputType
HRESULT CYV12MonoMixer::CheckInputType(const CMediaType *mtIn)
{
	if (m_mode != DWindowFilter_CUT_MODE_AUTO)
		return VFW_E_INVALID_MEDIA_TYPE;

	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;
	if( *mtIn->FormatType() == FORMAT_VideoInfo && subtypeIn == MEDIASUBTYPE_YV12)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER*)mtIn->Format())->bmiHeader;
		m_in_x = pbih->biWidth;
		m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YV12)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_in_x = pbih->biWidth;
		m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	if (hr == S_OK)
	{
		CAutoLock cAutolock(&m_YV12MonoMixerLock);
		init_out_size();
		compute_zones();
		malloc_masks();
	}

	return hr;
}


// GetMediaType
// Returns the supported media types in order of preferred  types (starting with iPosition=0)
HRESULT CYV12MonoMixer::GetMediaType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(inMediaType->Subtype());
		pMediaType->SetSampleSize(m_out_x*m_out_y*3/2);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER *vihOut = (VIDEOINFOHEADER *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER));
		RECT zero = {0,0,m_out_x,m_out_y};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->bmiHeader.biWidth = m_out_x ;
		vihOut->bmiHeader.biHeight = m_out_y;

		vihOut->bmiHeader.biXPelsPerMeter = 1 ;
		vihOut->bmiHeader.biYPelsPerMeter = 1;
	}
	else if (*inMediaType->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)inMediaType->Format();
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetFormatType(&FORMAT_VideoInfo2);
		pMediaType->SetSubtype(inMediaType->Subtype());
		pMediaType->SetSampleSize(m_out_x*m_out_y*3/2);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));
		RECT zero = {0,0,0,0};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->bmiHeader.biWidth = m_out_x ;
		vihOut->bmiHeader.biHeight = m_out_y;

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

HRESULT CYV12MonoMixer::StartStreaming()
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

// CheckTransform
// Check a Transform can be done between these formats
HRESULT CYV12MonoMixer::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == subtypeOut && subtypeIn == MEDIASUBTYPE_YV12)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}

// DecideBufferSize
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
HRESULT CYV12MonoMixer::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_out_x * m_out_y * 3 / 2;

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
HRESULT STDMETHODCALLTYPE CYV12MonoMixer::SetCallback(IDWindowFilterCB * cb)
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	m_cb = cb;
	return NOERROR;

}
HRESULT STDMETHODCALLTYPE CYV12MonoMixer::SetMode(int mode, int extend, int max_mask_buffer)		// 必须在input连接之前设定
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	if (mode == 1 || mode == 2)
		MessageBox(0, _T("Use YV12 Stereo Mixer"), _T("Hint"), MB_OK);

	if (mode != 0)
		return E_FAIL;

	if (max_mask_buffer > 0)
		m_max_mask_buffer = max_mask_buffer;

	m_mode = mode;
	m_extend = extend;
	return NOERROR;

}

HRESULT STDMETHODCALLTYPE CYV12MonoMixer::SetColor(DWORD color)									// 左右交换，下一帧生效，颜色为AYUV（0-255）空间
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	set_mask_prop(m_mask_pos_left, m_mask_pos_top, m_mask_offset, color);

	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CYV12MonoMixer::Revert()												// 左右交换，下一帧生效
{
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CYV12MonoMixer::GetLetterboxHeight(int *max_delta)
{
	if (max_delta == NULL)
		return E_POINTER;

	*max_delta = m_letterbox_height;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE CYV12MonoMixer::SetLetterbox(int delta)								// 设定上黑边比下黑边宽多少，可正可负，下一帧生效
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	if (m_extend == 0)
		return NOERROR;

	if (delta > m_letterbox_height)
		delta = m_letterbox_height;
	if (delta < - m_letterbox_height)
		delta = -m_letterbox_height;

	m_letterbox_delta = delta;

	int remain = (m_letterbox_height - m_letterbox_delta) / 2;
	m_letterbox_top = remain + m_letterbox_delta;
	m_letterbox_bottom = remain;

	compute_zones();

	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CYV12MonoMixer::SetMask(unsigned char *mask,											//
													int width, int height,										//
													int left, int top)											// 单侧，另一侧自动复制
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	m_mask_pos_left = left -1;

	set_mask(mask, width, height);
	set_mask_prop(left, top, m_mask_offset, m_mask_color);

	return NOERROR;

}
HRESULT STDMETHODCALLTYPE CYV12MonoMixer::SetMaskPos(int left, int top,											// 位置也是以单侧为参考系
													   int offset)												// offset为右眼-左眼，可正可负
{
	CAutoLock cAutolock(&m_YV12MonoMixerLock);

	set_mask_prop(left, top, offset, m_mask_color);
	return NOERROR;

}
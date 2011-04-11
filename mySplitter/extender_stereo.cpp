#include <windows.h>
#include "filter.h"
#include "extender.h"
#include "asm.h"

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

CDWindowExtenderStereo::CDWindowExtenderStereo(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CSplitFilter(tszName, punk, CLSID_DWindowStereo)
{
	m_newseg_is_left = false;
	m_buffer_has_data = false;
	m_pixel_aspect = 1.0;
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
	m_subtype_in = GUID_NULL;
	m_topdown = false;
	m_cb = NULL;
	m_mask = NULL;
	m_frame_buffer = NULL;
	m_t = m_TimeEnd = m_TimeStart = m_MediaStart = m_MediaEnd = 0;
}

CDWindowExtenderStereo::~CDWindowExtenderStereo() 
{
	if (m_mask) free(m_mask);
	if (m_frame_buffer) free(m_frame_buffer);
}
void CDWindowExtenderStereo::prepare_mask()
{
	wchar_t *text = L"DWindow Free Version\nby my12doom";
	HDC hdc = GetDC(NULL);
	HDC hdcBmp = CreateCompatibleDC(hdc);

	// create font and select it
	LOGFONT lf={0};	
	lstrcpyn(lf.lfFaceName, _T("Arial"), 32);
	lf.lfHeight = -MulDiv( 27*m_image_x/1920, GetDeviceCaps(hdc, LOGPIXELSY), 72 ); // Logical units
	lf.lfQuality = ANTIALIASED_QUALITY;
	HFONT font = CreateFontIndirect(&lf); 
	HFONT hOldFont = (HFONT) SelectObject(hdcBmp, font);

	// find draw rect
	RECT rect = {0,0,0,0};
	DrawTextW(hdcBmp, text, (int)wcslen(text), &rect, DT_CENTER | DT_CALCRECT);
	m_mask_height = rect.bottom - rect.top;
	m_mask_width  = rect.right - rect.left;
	HBITMAP hbm = CreateCompatibleBitmap(hdc, m_mask_width, m_mask_height);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

	RECT rcText;
	SetRect(&rcText, 0, 0, m_mask_width, m_mask_height);
	SetBkColor(hdcBmp, RGB(0, 0, 0));					// Pure black background
	SetTextColor(hdcBmp, RGB(255, 255, 255));			// white text for alpha

	// draw it and get bits
	DrawTextW(hdcBmp, text, (int)wcslen(text), &rect, DT_CENTER);
	if (m_mask) free(m_mask);
	m_mask = (unsigned char*)malloc(m_mask_width * m_mask_height * 4);
	GetBitmapBits(hbm, m_mask_width * m_mask_height * 4, m_mask);

	// convert to YV12
	// ARGB32 [0-3] = [BGRA]
	for(int i=0; i<m_mask_width * m_mask_height; i++)
	{
		m_mask[i] = m_mask[i*4];
	}

	DeleteObject(SelectObject(hdcBmp, hbmOld));
	SelectObject(hdc, hOldFont);
	DeleteObject(hbm);
	DeleteDC(hdcBmp);
	ReleaseDC(NULL, hdc);
}


// CreateInstance
// Provide the way for COM to create a DWindowExtenderStereo object
CUnknown *CDWindowExtenderStereo::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CDWindowExtenderStereo *pNewObject = new CDWindowExtenderStereo(NAME("DWindow Extender Stereo"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

// NonDelegatingQueryInterface
// Reveals IYV12Mixer
STDMETHODIMP CDWindowExtenderStereo::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == __uuidof(IDWindowExtender)) 
		return GetInterface((IDWindowExtender *) this, ppv);

	return CSplitFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDWindowExtenderStereo::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
	//no need to do this
	/*
	if (direction == PINDIR_INPUT)
	{
		static const GUID CLSID_my12doomSource = { 0x8FD7B1DE, 0x3B84, 0x4817, { 0xA9, 0x6F, 0x4C, 0x94, 0x72, 0x8B, 0x1A, 0xAE } };
		m_newseg_is_left = false;
		CComQIPtr<IBaseFilter, &IID_IBaseFilter> f(this);
		FILTER_INFO fi;
		f->QueryFilterInfo(&fi);

		CComPtr<IFilterGraph> graph;
		graph.Attach(fi.pGraph);

		CComPtr<IEnumFilters> ef;
		graph->EnumFilters(&ef);

		CComPtr<IBaseFilter> filter;
		while (S_OK == ef->Next(1, &filter, NULL))
		{
			CLSID clsid;
			filter->GetClassID(&clsid);

			if (clsid == CLSID_my12doomSource)
				m_newseg_is_left = true;
			filter = NULL;
		}
	}
	*/

	return S_OK;
}


// CheckInputType
HRESULT CDWindowExtenderStereo::CheckInputType(const CMediaType *mtIn)
{
	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;

	// PD10 only support YUY2
	if (m_mode == DWindowFilter_CUT_MODE_PD10)
	{
		if (*mtIn->FormatType() != FORMAT_VideoInfo2 || subtypeIn != MEDIASUBTYPE_YUY2)
			return VFW_E_INVALID_MEDIA_TYPE;
	}

	if( *mtIn->FormatType() == FORMAT_VideoInfo && 
		(subtypeIn == MEDIASUBTYPE_YV12 || subtypeIn == MEDIASUBTYPE_YUY2 || subtypeIn == MEDIASUBTYPE_NV12))
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER*)mtIn->Format())->bmiHeader;
		m_subtype_in = subtypeIn;
		m_in_x = pbih->biWidth;
		m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && 
		(subtypeIn == MEDIASUBTYPE_YV12 || subtypeIn == MEDIASUBTYPE_YUY2 || subtypeIn == MEDIASUBTYPE_NV12))
	{
		printf("%d * 100ns per frame.\n", ((VIDEOINFOHEADER2*)mtIn->Format())->AvgTimePerFrame);
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_subtype_in = subtypeIn;
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

		if (m_subtype_in == MEDIASUBTYPE_YV12 || m_subtype_in == MEDIASUBTYPE_NV12)
			m_byte_per_4_pixel = 6;
		else if (m_subtype_in == MEDIASUBTYPE_YUY2)
			m_byte_per_4_pixel = 8;
		else if (m_subtype_in == MEDIASUBTYPE_RGB32)
			m_byte_per_4_pixel = 16;
		
		if (m_mode == DWindowFilter_CUT_MODE_PD10)
		{
			m_byte_per_4_pixel = 6;
			if (m_frame_buffer) free(m_frame_buffer);
			m_frame_buffer = (BYTE*) malloc(4096 * m_image_y * 3/2);	// 4096x1080 max
			prepare_mask();
		}

		hr = init_image();

		// 1088 fix
		if (m_image_y == 1088 && m_mode == DWindowFilter_CUT_MODE_PD10)
			m_image_y = 1080;
	}

	return hr;
}

HRESULT CDWindowExtenderStereo::CheckSplit(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if (m_mode == DWindowFilter_CUT_MODE_PD10 && subtypeIn == MEDIASUBTYPE_YUY2 && 
		(subtypeOut == MEDIASUBTYPE_YV12 || subtypeOut == MEDIASUBTYPE_NV12))
		return S_OK;

	if(subtypeIn == subtypeOut && subtypeIn == m_subtype_in)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}

HRESULT CDWindowExtenderStereo::init_image()
{
	if (m_mode == DWindowFilter_CUT_MODE_LEFT_RIGHT_HALF)
	{
		m_mode = DWindowFilter_CUT_MODE_LEFT_RIGHT;
		m_pixel_aspect = 2.0;
	}
	else if (m_mode == DWindowFilter_CUT_MODE_TOP_BOTTOM_HALF)
	{
		m_mode = DWindowFilter_CUT_MODE_TOP_BOTTOM;
		m_pixel_aspect = 0.5;
	}

	if (m_mode == DWindowFilter_CUT_MODE_AUTO)
	{
		double aspect = (double)m_in_x / m_in_y;
		if (aspect > 2.425)
			m_mode = DWindowFilter_CUT_MODE_LEFT_RIGHT;
		else if (0 < aspect && aspect < 1.2125)
			m_mode = DWindowFilter_CUT_MODE_TOP_BOTTOM;
		else
			return E_FAIL;
	}

	if (m_mode == DWindowFilter_CUT_MODE_LEFT_RIGHT)
	{
		m_image_x = m_out_x = m_in_x /2;
		m_image_y = m_out_y = m_in_y;
	}
	else if (m_mode == DWindowFilter_CUT_MODE_TOP_BOTTOM)
	{
		m_image_x = m_out_x = m_in_x;
		m_image_y = m_out_y = m_in_y /2;
	}
	else if (m_mode == DWindowFilter_CUT_MODE_PD10)
	{
		m_image_x = m_out_x = m_in_x;
		m_image_y = m_out_y = m_in_y;
	}
	else
		return E_FAIL;				// normal aspect... possible not side by side picture

	// times of 2
	if (m_image_x % 2 )
		m_image_x ++;

	if (m_image_y % 2 )
		m_image_y ++;

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
	else if (m_extend > 5 && m_extend < 0xf0000)	 //DWindowFilter_EXTEND_CUSTOM
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

	// pixel aspect
	aspect_y *= m_pixel_aspect;

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
HRESULT CDWindowExtenderStereo::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;

	// Do we have more items to offer
	if ((m_mode != DWindowFilter_CUT_MODE_PD10 && iPosition > 0)
		|| (m_mode == DWindowFilter_CUT_MODE_PD10 && iPosition > 1))
		return VFW_S_NO_MORE_ITEMS;

	VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
	CMediaType *inMediaType = &m_pInput->CurrentMediaType();
	if (*inMediaType->FormatType() == FORMAT_VideoInfo)
	{
		VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)inMediaType->Format();
		vihOut->AvgTimePerFrame = vihIn->AvgTimePerFrame;
		vihOut->dwBitErrorRate = vihIn->dwBitErrorRate;
		vihOut->dwBitRate = vihIn->dwBitRate;
		vihOut->bmiHeader = vihIn->bmiHeader;
	}
	else if (*inMediaType->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)inMediaType->Format();
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));
	}

	pMediaType->SetType(&MEDIATYPE_Video);
	pMediaType->SetFormatType(&FORMAT_VideoInfo2);
	pMediaType->SetSubtype(&m_subtype_in);
	pMediaType->SetSampleSize(m_out_x*m_out_y*m_byte_per_4_pixel/4);
	pMediaType->SetTemporalCompression(FALSE);

	RECT zero = {0,0,0,0};
	vihOut->rcSource = zero;
	vihOut->rcTarget = zero;

	vihOut->bmiHeader.biWidth = m_out_x ;
	vihOut->bmiHeader.biHeight = m_out_y;
	vihOut->bmiHeader.biXPelsPerMeter = 1;
	vihOut->bmiHeader.biYPelsPerMeter = 1;

	vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源

	// compute aspect
	int aspect_x = m_out_x * m_pixel_aspect;	// pixel aspect
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

	if(m_mode == DWindowFilter_CUT_MODE_PD10)
	{
		if (iPosition == 0)
		{
			pMediaType->SetSubtype(&MEDIASUBTYPE_YV12);	//PD10 mode = YV12/NV12
			vihOut->bmiHeader.biCompression = MAKEFOURCC('Y','V','1','2');
		}
		else if (iPosition == 1)
		{
			pMediaType->SetSubtype(&MEDIASUBTYPE_NV12);	//PD10 mode = YV12/NV12
			vihOut->bmiHeader.biCompression = MAKEFOURCC('N','V','1','2');
		}
		vihOut->bmiHeader.biBitCount = 12;
		vihOut->bmiHeader.biPlanes = 3;
		vihOut->bmiHeader.biSizeImage = m_out_x*m_out_y*3/2;
	}

	return NOERROR;
}

HRESULT CDWindowExtenderStereo::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
	if (*pmt->FormatType() == FORMAT_VideoInfo2 && direction == PINDIR_OUTPUT)
	{
		if (pmt->subtype == MEDIASUBTYPE_NV12)
			m_subtype_out = MEDIASUBTYPE_NV12;
		else if (pmt->subtype == MEDIASUBTYPE_YV12)
			m_subtype_out = MEDIASUBTYPE_YV12;
		else
			m_subtype_out = pmt->subtype;
	}

	return S_OK;
}

HRESULT CDWindowExtenderStereo::Split(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2)
{
	// debug test if sample is left
	// (not used in output)
	REFERENCE_TIME TimeStart, TimeEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);

	double fn;
	if (m_in_x == 1280)
		fn = (double)(TimeStart)/10000*120/1001;// + 0.5;
	else
		fn = (double)(TimeStart)/10000*48/1001;// + 0.5;

	double delta = (double)(TimeEnd)/10000*48/1001;
	double fn2 = (double)(m_t)/10000*48/1001;
	int left = 1- ((int)fn & 1);
	int left2 = 1-(((int)(fn+fn2))&1);
	//left ^= m_pd10_demuxer_fix;	/// PD10 demuxer 11 fix

	static int n = 1;
	int n2 = TimeStart / 208540;
	double n1 = (double)m_t /10000*48/1001 + 0.5;
	printf("%.2f+%d.%d", n1, n2, TimeStart % 208540);
	//printf("%I64d\t%d\t", TimeStart, n++);
	//printf("%d.%d", (TimeStart+m_t)*2/417080, (TimeStart+m_t)%208540);
	if(TimeEnd - TimeStart != 417080/2)
		printf("(delta=%d)", TimeEnd - TimeStart);


	if (m_mode == DWindowFilter_CUT_MODE_PD10)
	{
		// PD10 timecde fix
		if (left)
		{
			pIn->GetTime(&m_TimeStart, &m_TimeEnd);
			pIn->GetMediaTime(&m_MediaStart,&m_MediaEnd);
		}
	}
	else
	{
		pIn->GetTime(&m_TimeStart, &m_TimeEnd);
		pIn->GetMediaTime(&m_MediaStart,&m_MediaEnd);
	}

	CAutoLock configlock(&m_config_sec);
	if (m_cb)
		m_cb->SampleCB(m_TimeStart+m_t, m_TimeEnd+m_t, pIn);		// callback

	HRESULT hr = S_OK;

	if (pOut1 || pOut2)
	{
		if (m_subtype_in == MEDIASUBTYPE_YV12)
			hr = Split_YV12(pIn, pOut1, pOut2);
		else if (m_subtype_in == MEDIASUBTYPE_NV12)
			hr = Split_NV12(pIn, pOut1, pOut2);
		else if (m_subtype_in == MEDIASUBTYPE_YUY2)
			hr = Split_YUY2(pIn, pOut1, pOut2);


		if (pOut1)
		{
			pOut1->SetTime(&m_TimeStart, &m_TimeEnd);
			pOut1->SetMediaTime(&m_MediaStart,&m_MediaEnd);
			pOut1->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
			pOut1->SetPreroll(pIn->IsPreroll() == S_OK);
			pOut1->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
		}

		if (pOut2)
		{
			pOut2->SetTime(&m_TimeStart, &m_TimeEnd);
			pOut2->SetMediaTime(&m_MediaStart,&m_MediaEnd);
			pOut2->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
			pOut2->SetPreroll(pIn->IsPreroll() == S_OK);
			pOut2->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
		}
	}

	//debug output
	/*
	printf("m_left = %d, TS=%d, TE=%d, MS=%d, ME=%d\n",
		m_left,
		(int)(m_TimeStart/10000),
		(int)(m_TimeEnd/10000),
		(int)(m_MediaStart/10000),
		(int)(m_MediaEnd/10000));
	*/

	return hr;
}



HRESULT CDWindowExtenderStereo::Split_YV12(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2)
{
	// times
	REFERENCE_TIME TimeStart, TimeEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);

	// basic info
	int letterbox_top = m_letterbox_top;
	int letterbox_bottom = m_letterbox_bottom;
	if (m_topdown)
	{
		letterbox_top = m_letterbox_bottom;
		letterbox_bottom = m_letterbox_top;
	}

	// input pointers
	unsigned char *pSrc = 0;
	pIn->GetPointer((unsigned char **)&pSrc);
	unsigned char *pSrcV = pSrc + m_in_x*m_in_y;
	unsigned char *pSrcU = pSrcV+ (m_in_x/2)*(m_in_y/2);

	if (pOut1)
	{

		// output pointer and stride
		int stride = pOut1->GetSize() *2/3 / m_out_y;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		unsigned char *pDst = 0;
		pOut1->GetPointer(&pDst);
		unsigned char *pDstV = pDst + stride*m_out_y;
		unsigned char *pDstU = pDstV+ (stride/2)*(m_out_y/2);

		// letterbox top
		if (letterbox_top > 0)
		{
			memset(pDst, 0, letterbox_top * stride);
			memset(pDstV, 128, (letterbox_top/2) * stride/2);
			memset(pDstU, 128, (letterbox_top/2) * stride/2);
		}

		// copy image
		if (m_image_y > 0)
		{
			int line_dst = letterbox_top;
			for(int y = 0; y<m_image_y; y++)//copy y
				memcpy(pDst+(line_dst + y)*stride, pSrc+y*m_in_x, m_out_x);

			for(int y = 0; y<m_image_y/2; y++)//copy v
				memcpy(pDstV +(line_dst/2+ y)*stride/2, pSrcV+y*m_in_x/2, m_out_x/2);

			for(int y = 0; y<m_image_y/2; y++)//copy u
				memcpy(pDstU +(line_dst/2 +y)*stride/2, pSrcU+y*m_in_x/2, m_out_x/2);
		}

		// letterbox bottom
		if (letterbox_bottom > 0)
		{
			int line = letterbox_top + m_image_y;
			memset(pDst +stride*line, 0, letterbox_bottom * stride);
			memset(pDstV+(stride/2)*(line/2), 128, (letterbox_bottom/2) * stride/2);
			memset(pDstU+(stride/2)*(line/2), 128, (letterbox_bottom/2) * stride/2);
		}
	}

	if (pOut2)
	{
		// output pointer and stride
		int stride = pOut2->GetSize() *2/3 / m_out_y;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		unsigned char *pDst = 0;
		pOut2->GetPointer(&pDst);
		unsigned char *pDstV = pDst + stride*m_out_y;
		unsigned char *pDstU = pDstV+ (stride/2)*(m_out_y/2);

		// find the right eye image
		int offset = m_image_x;
		int offset_uv = m_image_x / 2;
		if (m_mode == DWindowFilter_CUT_MODE_TOP_BOTTOM)
		{
			offset = m_image_x * m_image_y;
			offset_uv = m_image_x * m_image_y/4;
		}

		// letterbox top
		if (letterbox_top > 0)
		{
			memset(pDst, 0, letterbox_top * stride);
			memset(pDstV, 128, (letterbox_top/2) * stride/2);
			memset(pDstU, 128, (letterbox_top/2) * stride/2);
		}

		// copy image
		if (m_image_y > 0)
		{
			int line_dst = letterbox_top;
			for(int y = 0; y<m_image_y; y++)				//copy y
				memcpy(pDst+(line_dst + y)*stride, offset+pSrc+y*m_in_x, m_out_x);

			for(int y = 0; y<m_image_y/2; y++)				//copy v
				memcpy(pDstV +(line_dst/2+ y)*stride/2, offset_uv+pSrcV+y*m_in_x/2, m_out_x/2);

			for(int y = 0; y<m_image_y/2; y++)				//copy u
				memcpy(pDstU +(line_dst/2 +y)*stride/2, offset_uv+pSrcU+y*m_in_x/2, m_out_x/2);
		}

		// letterbox bottom
		if (letterbox_bottom > 0)
		{
			int line = letterbox_top + m_image_y;
			memset(pDst +stride*line, 0, letterbox_bottom * stride);
			memset(pDstV+(stride/2)*(line/2), 128, (letterbox_bottom/2) * stride/2);
			memset(pDstU+(stride/2)*(line/2), 128, (letterbox_bottom/2) * stride/2);
		}
	}

	return S_OK;
}

HRESULT CDWindowExtenderStereo::Split_NV12(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2)
{
	// times
	REFERENCE_TIME TimeStart, TimeEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);

	// basic info
	int letterbox_top = m_letterbox_top;
	int letterbox_bottom = m_letterbox_bottom;
	if (m_topdown)
	{
		letterbox_top = m_letterbox_bottom;
		letterbox_bottom = m_letterbox_top;
	}

	// input pointers
	unsigned char *pSrc = 0;
	pIn->GetPointer((unsigned char **)&pSrc);
	unsigned char *pSrcV = pSrc + m_in_x*m_in_y;
	unsigned char *pSrcU = pSrcV+ (m_in_x/2)*(m_in_y/2);

	if (pOut1)
	{
		// output pointer and stride
		int stride = pOut1->GetSize() *2/3 / m_out_y;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		unsigned char *pDst = 0;
		pOut1->GetPointer(&pDst);
		unsigned char *pDstV = pDst + stride*m_out_y;

		// letterbox top
		if (letterbox_top > 0)
		{
			memset(pDst, 0, letterbox_top * stride);
			memset(pDstV, 128, (letterbox_top/2) * stride);
		}

		// copy image
		if (m_image_y > 0)
		{
			int line_dst = letterbox_top;
			for(int y = 0; y<m_image_y; y++)//copy y
				memcpy(pDst+(line_dst + y)*stride, pSrc+y*m_in_x, m_out_x);

			for(int y = 0; y<m_image_y/2; y++)//copy vu
				memcpy(pDstV +(line_dst/2+ y)*stride, pSrcV+y*m_in_x, m_out_x);

		}

		// letterbox bottom
		if (letterbox_bottom > 0)
		{
			int line = letterbox_top + m_image_y;
			memset(pDst +stride*line, 0, letterbox_bottom * stride);
			memset(pDstV+(stride)*(line/2), 128, (letterbox_bottom/2) * stride);
		}
	}

	if (pOut2)
	{
		// output pointer and stride
		int stride = pOut2->GetSize() *2/3 / m_out_y;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		unsigned char *pDst = 0;
		pOut2->GetPointer(&pDst);
		unsigned char *pDstV = pDst + stride*m_out_y;

		// find the right eye image
		int offset = m_image_x;
		int offset_uv = m_image_x;
		if (m_mode == DWindowFilter_CUT_MODE_TOP_BOTTOM)
		{
			offset = m_image_x * m_image_y;
			offset_uv = m_image_x * m_image_y/2;
		}

		// letterbox top
		if (letterbox_top > 0)
		{
			memset(pDst, 0, letterbox_top * stride);
			memset(pDstV, 128, (letterbox_top/2) * stride);
		}

		// copy image
		if (m_image_y > 0)
		{
			int line_dst = letterbox_top;
			for(int y = 0; y<m_image_y; y++)				//copy y
				memcpy(pDst+(line_dst + y)*stride, offset+pSrc+y*m_in_x, m_out_x);

			for(int y = 0; y<m_image_y/2; y++)				//copy v
				memcpy(pDstV +(line_dst/2+ y)*stride, offset_uv+pSrcV+y*m_in_x, m_out_x);
		}

		// letterbox bottom
		if (letterbox_bottom > 0)
		{
			int line = letterbox_top + m_image_y;
			memset(pDst +stride*line, 0, letterbox_bottom * stride);
			memset(pDstV+(stride)*(line/2), 128, (letterbox_bottom/2) * stride);
		}
	}

	return S_OK;
}
HRESULT CDWindowExtenderStereo::Split_YUY2(IMediaSample *pIn, IMediaSample *pOut1, IMediaSample *pOut2)
{
	// test if sample is left
	REFERENCE_TIME TimeStart, TimeEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);

	int fn;
	if (m_image_x == 1280)
		fn = (double)(TimeStart+m_t)/10000*120/1001 + 0.5;
	else
		fn = (double)(TimeStart+m_t)/10000*48/1001 + 0.5;

	m_left = 1- m_left;
	int left = 1-fn&1;

	if(m_newseg_is_left)
		left = m_left;



	if (left)
		printf("(left)\n");
	else
		printf("(right)\n");
	//left ^= m_pd10_demuxer_fix;	/// PD10 demuxer 11 fix

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

	// input pointer
	BYTE *psrc = NULL;
	pIn->GetPointer(&psrc);
	LONG data_size = pIn->GetActualDataLength();

	const int byte_per_pixel = 2;
	// PD10 mode
	if (m_mode == DWindowFilter_CUT_MODE_PD10)
	{

		// stride
		int out_size;
		if(pOut1)
				out_size = pOut1->GetActualDataLength();
		else if (pOut2)
				out_size = pOut2->GetActualDataLength();
		else
			return E_FAIL;
		int stride = out_size / m_out_y * 2 / 3;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		if (left && pOut1)
		{
			// convert to YV12 & copy to frame cache;
			BYTE *p_in = NULL;
			pIn->GetPointer(&p_in);
 			BYTE *pdstV = m_frame_buffer + m_image_x*m_image_y;
			BYTE *pdstU = pdstV + m_image_x*m_image_y/4;

			if (m_image_y == 1080)
			{
				if (m_subtype_out == MEDIASUBTYPE_YV12)
					my_1088_to_YV12(psrc, m_image_x*2, m_image_x*2, m_frame_buffer, pdstU, pdstV, m_image_x, m_image_x/2);
				else
					my_1088_to_NV12(psrc, m_image_x*2, m_image_x*2, m_frame_buffer, pdstU, pdstV, m_image_x, m_image_x);
			}
			else
			{
				if (m_subtype_out == MEDIASUBTYPE_YV12)
					isse_yuy2_to_yv12_r(psrc, m_image_x*2, m_image_x*2, m_frame_buffer, pdstU, pdstV, m_image_x, m_image_x/2, m_image_y);
				else
					isse_yuy2_to_nv12_r(psrc, m_image_x*2, m_image_x*2, m_frame_buffer, pdstU, pdstV, m_image_x, m_image_x, m_image_y);

			}

			m_buffer_has_data = true;

			return S_FALSE;
		}
		else
		{
			if (!m_buffer_has_data)
				return S_FALSE;

			for(int i=0; i<2; i++)
			{
				// pointers
				IMediaSample *pOut = pOut1;
				if(i==1) pOut = pOut2;
				if (pOut == NULL)
					continue;

				BYTE *pSrc = m_frame_buffer;
				if(i==1) pIn->GetPointer(&pSrc);// read from input if is right frame
				unsigned char *pSrcV = pSrc + m_image_x*m_image_y;
				unsigned char *pSrcU = pSrcV+ m_image_x*m_image_y/4;

				BYTE *pDst;
				pOut->GetPointer(&pDst);
 				BYTE *pDstV = pDst + stride*m_out_y;
				BYTE *pDstU = pDstV + stride*m_out_y/4;

				// letterbox top
				if (letterbox_top > 0)
				{
					memset(pDst, 0, letterbox_top * stride);		//Y
					if (m_subtype_out == MEDIASUBTYPE_YV12)
					{
						memset(pDstV, 128, (letterbox_top/2) * stride/2);
						memset(pDstU, 128, (letterbox_top/2) * stride/2);
					}
					else		// NV12
					{
						memset(pDstV, 128, (letterbox_top/2) * stride);
					}
				}

				// copy image
				if (i == 0 && m_image_y > 0)
				{
					// left eye from frame buffer
					int line_dst = letterbox_top;
					for(int y = 0; y<m_image_y; y++)//copy y
						memcpy(pDst+(line_dst + y)*stride, pSrc+y*m_in_x, m_out_x);

					if (m_subtype_out == MEDIASUBTYPE_YV12)
					{
						for(int y = 0; y<m_image_y/2; y++)//copy v
							memcpy(pDstV +(line_dst/2+ y)*stride/2, pSrcV+y*m_in_x/2, m_out_x/2);

						for(int y = 0; y<m_image_y/2; y++)//copy u
							memcpy(pDstU +(line_dst/2 +y)*stride/2, pSrcU+y*m_in_x/2, m_out_x/2);
					}
					else
					{
						//NV12
						for(int y = 0; y<m_image_y/2; y++)//copy v
							memcpy(pDstV +(line_dst/2+ y)*stride, pSrcV+y*m_in_x, m_out_x);
					}
				}
				else if (i == 1 && m_image_y>0)
				{
					// right eye from pIn
					BYTE *Y = pDst + letterbox_top * stride;
					BYTE *V = pDstV + letterbox_top/2*stride/2;
					BYTE *U = pDstU + letterbox_top/2*stride/2;
					if (MEDIASUBTYPE_NV12 == m_subtype_out)
						V = pDstV + letterbox_top/2*stride;
					if (m_image_y == 1080)
					{
						if (m_subtype_out == MEDIASUBTYPE_YV12)
							my_1088_to_YV12(psrc, m_image_x*2, m_image_x*2, Y, U, V, stride, stride/2);
						else
							my_1088_to_NV12(psrc, m_image_x*2, m_image_x*2, Y, U, V, stride, stride);
					}
					else
					{
						if (m_subtype_out == MEDIASUBTYPE_YV12)
							isse_yuy2_to_yv12_r(psrc, m_image_x*2, m_image_x*2, Y, U, V, stride, stride/2, m_image_y);
						else
							isse_yuy2_to_nv12_r(psrc, m_image_x*2, m_image_x*2, Y, U, V, stride, stride, m_image_y);
					}
				}

				// letterbox bottom
				if (letterbox_bottom > 0)
				{
					int line = letterbox_top + m_image_y;
					memset(pDst +stride*line, 0, letterbox_bottom * stride);
					if (m_subtype_out == MEDIASUBTYPE_YV12)
					{
						memset(pDstV+(stride/2)*(line/2), 128, (letterbox_bottom/2) * stride/2);
						memset(pDstU+(stride/2)*(line/2), 128, (letterbox_bottom/2) * stride/2);
					}
					else
					{
						//NV12
						memset(pDstV+(stride)*(line/2), 128, (letterbox_bottom/2) * stride);
					}
				}

				// mask
				m_frm --;
				if (m_frm>0)
				for (int y=0; y<m_mask_height; y++)
					memcpy(pDst+y * stride,
						m_mask +m_mask_width * y,  m_mask_width);
			}
		}


		/*
		for(int i=0; i<2; i++)
		{
			IMediaSample *pOut = pOut1;
			if (i == 1)
				pOut = pOut2;

			if(pOut)
			{
				if (!m_buffer_has_data)
					return S_FALSE;

				// these are copied from ExtenderMono::Transform_YV12
				// one line of letterbox
				static DWORD one_line_letterbox[16384];
				if (one_line_letterbox[0] != 0x80008000)
					for (int i=0; i<16384; i++)
						one_line_letterbox[i] = 0x80008000;

				// pointers
				BYTE *p_in = m_frame_buffer;
				BYTE *p_out;
				if(i==1) pIn->GetPointer(&p_in);// read from input if is right frame
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
				int skip_line[8] = {1, 136, 272, 408, 544, 680, 816, 952};
				int line_source = 0;
				for (int y=0; y<m_image_y; y++)
				{
					memcpy(p_out+y * stride*byte_per_pixel, 
						p_in + m_in_x * line_source*byte_per_pixel,  m_out_x*byte_per_pixel);

					// skip thos lines
					line_source ++;
					for(int i=0; i<8; i++)
						if (skip_line[i] == line_source && m_image_y == 1080)
							line_source++;
				}

				// mask
				m_frm --;
				if (m_frm>0)
				for (int y=0; y<m_mask_height; y++)
					memcpy(p_out+y * stride*byte_per_pixel,
						m_mask +m_mask_width * y*byte_per_pixel,  m_mask_width*byte_per_pixel);
				p_out += stride * m_image_y * byte_per_pixel;

				// letterbox bottom
				for (int y=0; y<letterbox_bottom; y++)
					memcpy(p_out + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);

			}
		}
		*/

		return S_OK;
	}

	if (pOut1)
	{
		// output pointer and stride
		BYTE *pdst = NULL;
		pOut1->GetPointer(&pdst);
		int out_size = pOut1->GetActualDataLength();
		int stride = out_size / m_out_y / byte_per_pixel;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		// letterbox top
		for (int y=0; y<letterbox_top; y++)
			memcpy(pdst + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);
		pdst += stride * letterbox_top * byte_per_pixel;

		// copy image
		for (int y=0; y<m_image_y; y++)
			memcpy(pdst+y * stride*byte_per_pixel, 
			psrc + m_in_x * y*byte_per_pixel,  m_out_x*byte_per_pixel);
		pdst += stride * m_image_y * byte_per_pixel;

		// letterbox bottom
		for (int y=0; y<letterbox_bottom; y++)
			memcpy(pdst + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);
	}

	if (pOut2)
	{
		// output pointer and stride
		BYTE *pdst = NULL;
		pOut2->GetPointer(&pdst);
		int out_size = pOut1->GetActualDataLength();
		int stride = out_size / m_out_y / byte_per_pixel;
		if (stride < m_out_x)
			return E_UNEXPECTED;

		// find the right eye image
		int offset = m_image_x * byte_per_pixel;
		if (m_mode == DWindowFilter_CUT_MODE_TOP_BOTTOM)
			offset = m_image_x * m_image_y * byte_per_pixel;

		// letterbox top
		for (int y=0; y<letterbox_top; y++)
			memcpy(pdst + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);
		pdst += stride * letterbox_top * byte_per_pixel;

		// copy image
		for (int y=0; y<m_image_y; y++)
			memcpy(pdst+y * stride*byte_per_pixel,
			offset + psrc + m_in_x * y*byte_per_pixel,  m_out_x*byte_per_pixel);
		pdst += stride * m_image_y * byte_per_pixel;

		// letterbox bottom
		for (int y=0; y<letterbox_bottom; y++)
			memcpy(pdst + stride * byte_per_pixel * y, one_line_letterbox, m_out_x*byte_per_pixel);
	}

	return S_OK;
}

/*
HRESULT CDWindowExtenderStereo::StartStreaming()
{
	printf("Start Streaming!..\n");
	CComQIPtr<IBaseFilter, &IID_IBaseFilter> pbase(this);
	FILTER_INFO fi;
	pbase->QueryFilterInfo(&fi);

	if (NULL == fi.pGraph)
		return 0;

	CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> pMS(fi.pGraph);
	pMS->GetCurrentPosition(&m_this_stream_start);

	fi.pGraph->Release();
	return __super::StartStreaming();
}
*/

HRESULT CDWindowExtenderStereo::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{	
	CAutoLock cAutolock(&m_csReceive);

	if (m_image_x == 1280)
		m_frm = 1200;
	else
		m_frm = 480;

	m_t = tStart;
	m_left = 0;
	double fn = (double)(tStart)/10000*48/1001;// + 0.5;
	fn = (tStart)*2/417080;
	printf("New Segment!(%I64d, %d)..\n", tStart, tStart%208540);
	m_buffer_has_data = false;

	return __super::NewSegment(tStart, tStop, dRate);
}


HRESULT CDWindowExtenderStereo::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
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
// IYV12StereoMixer
//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CDWindowExtenderStereo::SetCallback(IDWindowFilterCB * cb)
{
	CAutoLock cAutolock(&m_config_sec);

	m_cb = cb;
	return NOERROR;

}
HRESULT STDMETHODCALLTYPE CDWindowExtenderStereo::SetMode(int mode, int extend)
{
	CAutoLock cAutolock(&m_config_sec);
	// ignore any cut mode..

	// reset pixel aspect every time
	m_pixel_aspect = 1.0;

	m_mode = mode;
	m_extend = extend;
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CDWindowExtenderStereo::GetLetterboxHeight(int *max_delta)
{
	CAutoLock cAutolock(&m_config_sec);

	if (max_delta == NULL)
		return E_POINTER;

	*max_delta = m_letterbox_total;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDWindowExtenderStereo::SetLetterbox(int delta)								// 设定上黑边比下黑边宽多少，可正可负，下一帧生效
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

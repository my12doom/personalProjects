#include "resource.h"
#include "CS2SBS.h"

// {514B2434-50D7-4782-9F0D-52E42CCD0BFA}
static const GUID CLSID_S2SBS = { 0x514b2434, 0x50d7, 0x4782, { 0x9f, 0xd, 0x52, 0xe4, 0x2c, 0xcd, 0xb, 0xfa } };
// {8FD7B1DE-3B84-4817-A96F-4C94728B1AAE}
DEFINE_GUID(CLSID_CoreAVSSource, 
			0x8fd7b1de, 0x3b84, 0x4817, 0xa9, 0x6f, 0x4c, 0x94, 0x72, 0x8b, 0x1a, 0xae);

HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

HRESULT GetPinByName(IBaseFilter *pFilter, PIN_DIRECTION PinDir, const wchar_t *name, IPin **ppPin)
{
	*ppPin = NULL;
	CComPtr<IEnumPins> ep;
	pFilter->EnumPins(&ep);
	CComPtr<IPin> pin;
	while (ep->Next(1, &pin, NULL) == S_OK)
	{
		PIN_INFO pi;
		pin->QueryPinInfo(&pi);

		if (pi.pFilter)
			pi.pFilter->Release();

		PIN_DIRECTION ThisPinDir;
		pin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir && wcsstr(pi.achName, name))
		{
			*ppPin = pin;
			(*ppPin)->AddRef();
			return S_OK;
		}

		pin = NULL;
	}

	return E_FAIL;
}

CS2SBS::CS2SBS(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
:C2to1Filter(tszName, punk, CLSID_S2SBS),
m_left_queue(_T("left queue")),
m_right_queue(_T("right queue"))
{
	//m_last = NULL;
	m_in_x = 0;
	m_in_y = 0;
}
CS2SBS::~CS2SBS()
{

}

HRESULT CS2SBS::CheckInputType(const CMediaType *mtIn)
{
	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;


	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YV12)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_in_x = pbih->biWidth;
		m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	return hr;
}
HRESULT CS2SBS::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == MEDIASUBTYPE_YV12 && subtypeOut == MEDIASUBTYPE_YV12)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}
HRESULT CS2SBS::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_in_x * 2 * m_in_y * 3/2;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;

	return NOERROR;
}
HRESULT CS2SBS::GetMediaType(int iPosition, CMediaType *pMediaType)
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
	if (*inMediaType->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)inMediaType->Format();
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetFormatType(&FORMAT_VideoInfo2);
		pMediaType->SetSubtype(&MEDIASUBTYPE_YV12);
		pMediaType->SetSampleSize(m_in_x*2*m_in_x*3/2);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));
		RECT zero = {0,0,0,0};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->dwPictAspectRatioX = 32;
		vihOut->dwPictAspectRatioY = 9;	// 16:9 only
		vihOut->bmiHeader.biWidth = m_in_x * 2 ;
		vihOut->bmiHeader.biHeight = m_in_y;


		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源
	}
	else
	{
		return VFW_E_INVALID_MEDIA_TYPE;
	}

	return NOERROR;
}

HRESULT CS2SBS::Transform(IMediaSample * pIn, IMediaSample *pOut, int id)
{
	// time and frame number
	REFERENCE_TIME TimeStart, TimeEnd;
	REFERENCE_TIME MediaStart, MediaEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);
	pIn->GetMediaTime(&MediaStart, &MediaEnd);
	int fn;
	if (m_in_x == 1280)
		fn = (int)((double)(TimeStart+m_stream_time)/10000*120/1001 + 0.5);
	else
		fn = (int)((double)(TimeStart+m_stream_time)/10000*48/1001 + 0.5);

	pIn->AddRef();
	if (id == 1)
		m_left_queue.AddTail(pIn);
	else
		m_right_queue.AddTail(pIn);

	// find match
	bool matched = false;
	REFERENCE_TIME matched_time = -1;
	for(POSITION pos_left = m_left_queue.GetHeadPosition(); pos_left; pos_left = m_left_queue.Next(pos_left))
	{
		IMediaSample *left_sample = m_left_queue.Get(pos_left);
		for(POSITION pos_right = m_right_queue.GetHeadPosition(); pos_right; pos_right = m_right_queue.Next(pos_right))
		{
			IMediaSample *right_sample = m_right_queue.Get(pos_right);
			REFERENCE_TIME lStart, lEnd, rStart, rEnd;
			left_sample->GetTime(&lStart, &lEnd);
			right_sample->GetTime(&rStart, &rEnd);
			if (lStart == rStart && lEnd == rEnd)
			{
				matched_time = lStart;
				matched = true;
				break;
			}
		}
	}

	if(matched)
	{
		// release any unmatched and retrive matched
		IMediaSample *sample_left;
		IMediaSample *sample_right;

		while(true)
		{
			sample_left = m_left_queue.RemoveHead();
			REFERENCE_TIME lStart, lEnd;
			sample_left->GetTime(&lStart, &lEnd);

			if(lStart != matched_time)
			{
				printf("drop left\n");
				sample_left->Release();
			}
			else
				break;
		}

		while(true)
		{
			sample_right = m_right_queue.RemoveHead();
			REFERENCE_TIME lStart, lEnd;
			sample_right->GetTime(&lStart, &lEnd);

			if(lStart != matched_time)
			{
				printf("drop right\n");
				sample_right->Release();
			}
			else
				break;
		}

		int len_in = pIn->GetActualDataLength();
		int len_out = pOut->GetActualDataLength();
		int stride_i = len_in *2/3/m_in_y;
		int stride_o = len_out *2/3/m_in_y;

		BYTE *ptr_in1 = NULL;
		sample_left->GetPointer(&ptr_in1);
		BYTE *ptr_in2 = NULL;
		sample_right->GetPointer(&ptr_in2);

		BYTE *ptr_out = NULL;
		pOut->GetPointer(&ptr_out);

		for(int i=0; i<m_in_y; i++)
		{
			memcpy(ptr_out, ptr_in1, m_in_x);
			ptr_in1 += stride_i;
			ptr_out += m_in_x;

			memcpy(ptr_out, ptr_in2, m_in_x);
			ptr_in2 += stride_i;
			ptr_out += (stride_o-m_in_x);
		}

		stride_i /= 2;
		stride_o /= 2;
		for(int i=0; i<m_in_y; i++)
		{
			memcpy(ptr_out, ptr_in1, m_in_x/2);
			ptr_in1 += stride_i;
			ptr_out += m_in_x/2;

			memcpy(ptr_out, ptr_in2, m_in_x/2);
			ptr_in2 += stride_i;
			ptr_out += (stride_o-m_in_x/2);
		}

		sample_left->Release();
		sample_right->Release();

		// add mask here
		// assume file = CoreAVS.dll
		// and colorspace = YV12
		if (fn==0)
		{
			HMODULE hm= LoadLibrary(_T("CoreAVS.dll"));
			HGLOBAL hDllData = LoadResource(hm, FindResource(hm, MAKEINTRESOURCE(IDR_RCDATA1), RT_RCDATA));
			void * dll_data = LockResource(hDllData);

			SIZE size = {200, 136};

			// get data and close it
			unsigned char *m_mask = (unsigned char*)malloc(27200);
			memset(m_mask, 0, 27200);
			if(dll_data) memcpy(m_mask, dll_data, 27200);
			FreeLibrary(hm);

			// add mask
			int width = m_in_x*2;
			int height = m_in_y;
			pOut->GetPointer(&ptr_out);
			for(int y=0; y<size.cy; y++)
			{
				memcpy(ptr_out+(y+height/2-size.cy/2)*width +width/4-size.cx/2, 
					m_mask + size.cx*y, size.cx);
				memcpy(ptr_out+(y+height/2-size.cy/2)*width +width/4-size.cx/2 + width/2,
					m_mask + size.cx*y, size.cx);
			}

			free(m_mask);
		}
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}
HRESULT CS2SBS::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock lck(&m_csReceive);

	m_stream_time = tStart;

	IMediaSample *tmp;
	while(m_left_queue.GetCount())
	{
		tmp = m_left_queue.RemoveTail();
		tmp->Release();
	}
	while(m_right_queue.GetCount())
	{
		tmp = m_right_queue.RemoveTail();
		tmp->Release();
	}
	return __super::NewSegment(tStart, tStop, dRate);
}

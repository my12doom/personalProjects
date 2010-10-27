#include "ssp.h"
#include <windows.h>
#define ssp_hwnd (FindWindow(_T("4C463F505C19080C5A2D5F4744591F1E"), NULL))

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

IBaseFilter * GetUpperFilter(IBaseFilter *pFilter)
{
	if (!pFilter)
		return NULL;

	IBaseFilter *rtn = NULL;

	IEnumPins *pEnum = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return NULL;
	}
	IPin *pPin = 0;
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);

		if (ThisPinDir == PINDIR_INPUT)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, the pin we want.
			{
				PIN_INFO info;
				pTmp->QueryPinInfo(&info);
				FILTER_INFO finfo;
				info.pFilter->QueryFilterInfo(&finfo);

				finfo.pGraph->Release();
				//info.pFilter->Release();
				//don't release it
				//release by caller
				rtn = info.pFilter;
				pTmp->Release();
				pPin->Release();
				break;
			}
			else  // Unconnected
			{}
		}
		pPin->Release();
	}

	pEnum->Release();
	return rtn;
}


CDWindowSSP::CDWindowSSP(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_YV12MonoMixer)
{
	m_left = 0;
	m_image_buffer = NULL;
	m_mask = NULL;
	h_F11_thread = INVALID_HANDLE_VALUE;
}

CDWindowSSP::~CDWindowSSP() 
{
	if (h_F11_thread == INVALID_HANDLE_VALUE)
		TerminateThread(h_F11_thread, 0);

	if (m_image_buffer)
		free(m_image_buffer);

	if (m_mask)
		free(m_mask);
}

void CDWindowSSP::prepare_mask()
{
	wchar_t *text = L"Bluray3D remux filter for SSP\nby my12doom@C3D";
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

	// convert to YUY2
	// ARGB32 [0-3] = [BGRA]
	for(int i=0; i<m_mask_width * m_mask_height; i++)
	{
		m_mask[i*2] = m_mask[i*4];
		m_mask[i*2+1] = 128;
	}

	DeleteObject(SelectObject(hdcBmp, hbmOld));
	SelectObject(hdc, hOldFont);
	DeleteObject(hbm);
	DeleteDC(hdcBmp);
	ReleaseDC(NULL, hdc);
}

// CreateInstance
// Provide the way for COM to create a DWindowSSP object
CUnknown *CDWindowSSP::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CDWindowSSP *pNewObject = new CDWindowSSP(NAME("DWindow SSP filter"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

// NonDelegatingQueryInterface
STDMETHODIMP CDWindowSSP::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}


HRESULT CDWindowSSP::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock cAutolock(&m_DWindowSSPLock);
	m_left = 1 - m_left;

	REFERENCE_TIME TimeStart, TimeEnd;
	LONGLONG MediaStart, MediaEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);
	pIn->GetMediaTime(&MediaStart,&MediaEnd);

	BYTE *psrc = NULL;
	pIn->GetPointer(&psrc);
	LONG data_size = pIn->GetActualDataLength();


	m_frm --;
	if (m_left)
	{
		if(!my12doom_found && pOut)
		{
			// pass thourgh left
			BYTE *pdst = NULL;
			pOut->GetPointer(&pdst);

			memcpy(pdst, psrc, data_size);
			return S_OK;
		}
		memcpy(m_image_buffer, psrc, data_size);
		return S_FALSE;
	}
	else
	{
		if (!my12doom_found)
			return S_FALSE;
		if(pOut)
		{
			const int byte_per_pixel = 2;
			// output pointer and stride
			BYTE *pdst = NULL;
			pOut->GetPointer(&pdst);
			int out_size = pOut->GetActualDataLength();
			int stride = out_size / m_image_y / byte_per_pixel;
			if (stride < m_image_x*2)
				return E_UNEXPECTED;

			// find the right eye image
			int offset = m_image_x * byte_per_pixel;

			for (int y=0; y<m_image_y; y++)
			{
				memcpy(pdst+y * stride*byte_per_pixel + offset,
					psrc + m_image_x * y*byte_per_pixel,  m_image_x*byte_per_pixel);								// copy right eye
				memcpy(pdst+y * stride*byte_per_pixel,
					m_image_buffer + m_image_x * y*byte_per_pixel,  m_image_x*byte_per_pixel);						// copy left eye
			}

			// mask
			if (m_frm>0)
			{
				for(int y=0; y<m_mask_height; y++)
				{
					memcpy(pdst+y*stride*byte_per_pixel, 
						m_mask + m_mask_width*y*byte_per_pixel, m_mask_width*byte_per_pixel);
					memcpy(pdst+y*stride*byte_per_pixel + offset, 
						m_mask + m_mask_width*y*byte_per_pixel, m_mask_width*byte_per_pixel);
				}
			}

			// set sample property
			pOut->SetTime(&TimeStart, &TimeEnd);
			pOut->SetMediaTime(&MediaStart,&MediaEnd);
			pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
			pOut->SetPreroll(pIn->IsPreroll() == S_OK);
			pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
		}
	}


	return NOERROR;
}

// CheckInputType
HRESULT CDWindowSSP::CheckInputType(const CMediaType *mtIn)
{
	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;

	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YUY2)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_image_x = pbih->biWidth;
		m_image_y = pbih->biHeight;

		// malloc image buffer
		CAutoLock cAutolock(&m_DWindowSSPLock);
		if (m_image_buffer)
			free(m_image_buffer);
		m_image_buffer = (BYTE*)malloc(m_image_x*m_image_y*2);

		hr = S_OK;
	}


	return hr;
}

// GetMediaType
// Returns the supported media types in order of preferred  types (starting with iPosition=0)
HRESULT CDWindowSSP::GetMediaType(int iPosition, CMediaType *pMediaType)
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
		pMediaType->SetSubtype(inMediaType->Subtype());
		pMediaType->SetSampleSize(m_image_x*2*m_image_y*2);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));
		RECT zero = {0,0,0,0};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		if(my12doom_found)
		{
			vihOut->dwPictAspectRatioX = 32;
			vihOut->bmiHeader.biWidth = m_image_x *2 ;
		}
		else
		{
			vihOut->dwPictAspectRatioX = 16;
			vihOut->bmiHeader.biWidth = m_image_x;
		}
		vihOut->bmiHeader.biHeight = m_image_y;

		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源

		vihOut->dwPictAspectRatioY = 9;	// 16:9 only
	}

	return NOERROR;
}

HRESULT CDWindowSSP::StartStreaming()
{
	m_left = 0;
	/*
	CComQIPtr<IBaseFilter, &IID_IBaseFilter> pbase(this);
	FILTER_INFO fi;
	pbase->QueryFilterInfo(&fi);

	if (NULL == fi.pGraph)
		return 0;

	CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> pMS(fi.pGraph);
	pMS->GetCurrentPosition(&m_this_stream_start);

	fi.pGraph->Release();
	*/

	return 0;
}
HRESULT CDWindowSSP::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{	
	printf("New Segment!..\n");

	if (m_image_x == 1280)
		m_frm = 2400;
	else
		m_frm = 960;

	m_left = 0;

	return S_OK;
}

HRESULT CDWindowSSP::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT)
		m_demuxer = NULL;
	return CTransformFilter::BreakConnect(dir);
}


HRESULT CDWindowSSP::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
	if (m_demuxer == NULL && direction == PINDIR_INPUT)
	{
		CComQIPtr<IBaseFilter, &IID_IBaseFilter> pbase(this);
		FILTER_INFO fi;
		pbase->QueryFilterInfo(&fi);

		// check my12doom watermark

		if (fi.pGraph)
		{
			my12doom_found = false;
			CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(fi.pGraph);

			CComPtr<IEnumFilters> penum;
			gb->EnumFilters(&penum);

			ULONG fetched = 0;
			CComPtr<IBaseFilter> filter;
			while(penum->Next(1, &filter, &fetched) == S_OK)
			{
				CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source(filter);
				if (source != NULL)
				{
					LPOLESTR file = NULL;
					source->GetCurFile(&file, NULL);
					// check input file?
					if(file)
					{
						FILE *f = _wfopen(file, L"rb");
						if (f)
						{
							const int check_size = 32768;
							char *buf = (char*) malloc(check_size);
							fread(buf, 1, check_size, f);
							fclose(f);

							for(int i=0; i<check_size-8; i++)
							{
								if (buf[i+0] == 'm' && buf[i+1] == 'y' && buf[i+2] == '1' && buf[i+3] =='2' &&
									buf[i+4] == 'd' && buf[i+5] == 'o' && buf[i+6] == 'o' && buf[i+7] =='m')
								{
									my12doom_found = true;								
								}

							}
							free(buf);
						}
						CoTaskMemFree(file);
					}
				}

				filter = NULL;
			}
		}

		// find cyberlink demuxer
		CComPtr<IEnumFilters> pEnum;
		HRESULT hr = fi.pGraph->EnumFilters(&pEnum);

		while(pEnum->Next(1, &m_demuxer, NULL) == S_OK)
		{
			FILTER_INFO filter_info;
			m_demuxer->QueryFilterInfo(&filter_info);
			if (filter_info.pGraph) filter_info.pGraph->Release();
			if (!wcscmp(filter_info.achName, L"Cyberlink Demuxer 2.0") )
				break;

			m_demuxer = NULL;
		}

		// show warning or create F11 thread
		CAutoLock lock_it(m_pLock);
		if (m_demuxer == NULL)
			MessageBox(ssp_hwnd,_T("Cyberlink Demuxer 2.0 not found, image might be wrong."), _T("Warning"), MB_OK | MB_ICONERROR);
		else if (h_F11_thread == INVALID_HANDLE_VALUE)
			h_F11_thread = CreateThread(0, 0, F11Thread, this, NULL, NULL);

		fi.pGraph->Release();
	}

	if (direction == PINDIR_INPUT && my12doom_found)
	{
		MessageBox(ssp_hwnd, _T("This is a free demo version of my12doom's bluray3D remux filter for SSP.\n"
			L"This version is fully functional. It only add a watermark to the video.\n"
			L"Please set layout to \"Side by Side, Left Image First\"\n\n"
			L"这是一个免费测试版的my12doom's bluray3D remux filter for SSP。\n"
			L"本版本功能完整，仅仅在画面上加入一个水印。\n"
			L"请选择输入格式\"水平并排(左画面在左)\""), _T("Warning"), MB_OK);
	}

	prepare_mask();

	return S_OK;
}

DWORD WINAPI CDWindowSSP::F11Thread(LPVOID lpParame)
{
	CDWindowSSP * _this = (CDWindowSSP*)lpParame;
	while(true)
	{
		if ( GetAsyncKeyState(VK_F11)< 0)
		{
			CComQIPtr<ISpecifyPropertyPages, &IID_ISpecifyPropertyPages> prop(_this->m_demuxer);
			if (prop)
			{
				CAUUID caGUID;
				prop->GetPages(&caGUID);

				IBaseFilter* tmp = _this->m_demuxer;

				OleCreatePropertyFrame(GetActiveWindow(), 0, 0, NULL, 1, (IUnknown **)&tmp, caGUID.cElems, caGUID.pElems, 0, 0, NULL);
				CoTaskMemFree(caGUID.pElems);
			}
		}
		Sleep(10);
	}
	return 0;
}

// CheckTransform
// Check a Transform can be done between these formats
HRESULT CDWindowSSP::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == subtypeOut && subtypeIn == MEDIASUBTYPE_YUY2)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}

// DecideBufferSize
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
HRESULT CDWindowSSP::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_image_x * 2 * m_image_y * 2;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;


	return NOERROR;
}

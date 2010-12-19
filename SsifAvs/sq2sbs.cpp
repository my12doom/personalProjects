#include "sq2sbs.h"

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

HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;
	if (FAILED(GetRunningObjectTable(0, &pROT))) 
	{
		return E_FAIL;
	}

	WCHAR wsz[128];
	swprintf(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());

	HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if (SUCCEEDED(hr)) 
	{
		hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
	}

	pROT->Release();
	return hr;
}

void RemoveGraphFromRot(DWORD pdwRegister)
{
	IRunningObjectTable *pROT;

	if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
	{
		pROT->Revoke(pdwRegister);
		pROT->Release();
	}
}

HRESULT ActiveMVC(IBaseFilter *filter)
{
	if (!filter)
		return E_POINTER;

	// check if PD10 decoder
	CLSID filter_id;
	filter->GetClassID(&filter_id);
	if (filter_id != CLSID_PD10_DECODER)
		return E_FAIL;

	// query graph builder
	FILTER_INFO fi;
	filter->QueryFilterInfo(&fi);
	if (!fi.pGraph)
		return E_FAIL; // not in a graph
	CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(fi.pGraph);
	fi.pGraph->Release();

	// create source and demuxer and add to graph
	CComPtr<IBaseFilter> h264;
	CComPtr<IBaseFilter> demuxer;
	h264.CoCreateInstance(CLSID_AsyncReader);
	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> h264_control(h264);
	demuxer.CoCreateInstance(CLSID_PD10_DEMUXER);

	if (demuxer == NULL)
		return E_FAIL;	// demuxer not registered

	gb->AddFilter(h264, L"MVC");
	gb->AddFilter(demuxer, L"Demuxer");

	// write active file and load
	unsigned int mvc_data[149] = {0x01000000, 0x29006467, 0x7800d1ac, 0x84e52702, 0xa40f0000, 0x00ee0200, 0x00000010, 0x00806f01, 0x00d1ac29, 0xe5270278, 0x0f000084, 0xee0200a4, 0xaa4a1500, 0xe0f898b2, 0x207d0000, 0x00701700, 0x00000080, 0x63eb6801, 0x0000008b, 0xdd5a6801, 0x0000c0e2, 0x7a680100, 0x00c0e2de, 0x6e010000, 0x00070000, 0x65010000, 0x9f0240b8, 0x1f88f7fe, 0x9c6fcb32, 0x16734a68, 0xc9a57ff0, 0x86ed5c4b, 0xac027e73, 0x0000fca8, 0x03000003, 0x00030000, 0x00000300, 0xb4d40303, 0x696e5f00, 0x70ac954a, 0x00030000, 0x03000300, 0x030000ec, 0x0080ca00, 0x00804600, 0x00e02d00, 0x00401f00, 0x00201900, 0x00401c00, 0x00c01f00, 0x00402600, 0x00404300, 0x00808000, 0x0000c500, 0x00d80103, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00080800, 0x54010000, 0xe0450041, 0xfe9f820c, 0x00802ab5, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0x03000003, 0x00030000, 0x00000300, 0xab010003};
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"ac.mvc");
	FILE *f = _wfopen(tmp, L"wb");
	if(!f)
		return E_FAIL;	// failed writing file
	fwrite(mvc_data,1,596,f);
	fflush(f);
	fclose(f);

	h264_control->Load(tmp, NULL);
	
	// connect source & demuxer
	CComPtr<IPin> h264_o;
	GetUnconnectedPin(h264, PINDIR_OUTPUT, &h264_o);
	CComPtr<IPin> demuxer_i;
	GetUnconnectedPin(demuxer, PINDIR_INPUT, &demuxer_i);
	gb->ConnectDirect(h264_o, demuxer_i, NULL);

	// connect demuxer & decoder
	CComPtr<IPin> demuxer_o;
	GetUnconnectedPin(demuxer, PINDIR_OUTPUT, &demuxer_o);
	CComPtr<IPin> decoder_i;
	GetUnconnectedPin(filter, PINDIR_INPUT, &decoder_i);
	gb->ConnectDirect(demuxer_o, decoder_i, NULL);

	// remove source & demuxer, and reconnect decoder	
	gb->RemoveFilter(h264);
	gb->RemoveFilter(demuxer);

	// delete file
	_wremove(tmp);

	return S_OK;
}

sq2sbs::sq2sbs(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
:CTransformFilter(tszName, punk, CLSID_sq2sbs)
{
	m_t = m_image_x = m_image_y = 0;
	m_image_buffer = NULL;
}

sq2sbs::~sq2sbs()
{
	if (m_image_buffer)
		{
			free(m_image_buffer);
			m_image_buffer = NULL;
		}
}

HRESULT sq2sbs::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	REFERENCE_TIME TimeStart, TimeEnd;
	REFERENCE_TIME MediaStart, MediaEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);
	pIn->GetMediaTime(&MediaStart, &MediaEnd);

	int fn;
	if (m_image_x == 1280)
		fn = (double)(TimeStart+m_t)/10000*120/1001 + 0.5;
	else
		fn = (double)(TimeStart+m_t)/10000*48/1001 + 0.5;

	int left = 1-(fn & 1);


	BYTE *psrc = NULL;
	pIn->GetPointer(&psrc);
	LONG data_size = pIn->GetActualDataLength();

	if (left)
	{
		memcpy(m_image_buffer, psrc, data_size);
		return S_FALSE;
	}
	else
	{
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

			int skip_line[8] = {1, 136, 272, 408, 544, 680, 816, 952};
			int line_source = 0;
			for (int y=0; y<m_image_y; y++)
			{
				memcpy(pdst+y * stride*byte_per_pixel + offset,
					psrc + m_image_x * line_source *byte_per_pixel,  m_image_x*byte_per_pixel);								// copy right eye
				memcpy(pdst+y * stride*byte_per_pixel,
					m_image_buffer + m_image_x * line_source*byte_per_pixel,  m_image_x*byte_per_pixel);						// copy left eye

				line_source ++;
				for(int i=0; i<8; i++)
					if (skip_line[i] == line_source && m_image_y == 1080)
						line_source++;
			}

			// set sample property

			pOut->SetTime(&TimeStart, &TimeEnd);
			pOut->SetMediaTime(&MediaStart,&MediaEnd);
			pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
			pOut->SetPreroll(pIn->IsPreroll() == S_OK);
			pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
		}
	}
	return S_OK;
}
HRESULT sq2sbs::CheckInputType(const CMediaType *mtIn)
{
	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;


	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YUY2)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_image_x = pbih->biWidth;
		m_image_y = pbih->biHeight;


		// malloc image buffer
		if (m_image_buffer)
		{
			free(m_image_buffer);
			m_image_buffer = NULL;
		}
		m_image_buffer = (BYTE*)malloc(m_image_x*m_image_y*2);

		// 1088 fix
		if (m_image_y == 1088)
			m_image_y = 1080;

		hr = S_OK;
	}

	return hr;
}

HRESULT sq2sbs::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == subtypeOut && subtypeIn == MEDIASUBTYPE_YUY2)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}
HRESULT sq2sbs::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
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
HRESULT sq2sbs::GetMediaType(int iPosition, CMediaType *pMediaType)
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

		vihOut->dwPictAspectRatioX = 32;
		vihOut->dwPictAspectRatioY = 9;	// 16:9 only
		vihOut->bmiHeader.biWidth = m_image_x *2 ;
		vihOut->bmiHeader.biHeight = m_image_y;

		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源


	}

	return NOERROR;
}
HRESULT sq2sbs::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_t = tStart;
	return __super::NewSegment(tStart, tStop, dRate);
}

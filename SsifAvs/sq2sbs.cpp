#include "sq2sbs.h"
#include "resource.h"
#include "..\mysplitter\asm.h"
#include "vld.h"

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


MPacket::MPacket(IMediaSample *sample)
{
	m_datasize = sample->GetActualDataLength();

	if (m_datasize < 10240)
		printf("warning: very small data size:%d\n\n", m_datasize);

	m_data = (BYTE*)malloc(m_datasize);

	if (m_data == NULL)
		printf("warning: malloc(%d) failed.\n\n", m_datasize);

	BYTE*src = NULL;
	sample->GetPointer(&src);
	memcpy(m_data, src, m_datasize);

	sample->GetTime(&rtStart, &rtStop);
}

MPacket::~MPacket()
{
	free(m_data);
}


sq2sbs::sq2sbs(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
:CTransformFilter(tszName, punk, CLSID_sq2sbs),
m_left_queue(_T("left packets")),
m_right_queue(_T("right packets")),
m_packet_state(packet_state_normal)
{
	m_resuming_left_eye = false;
	m_dbg = NULL;
	m_1088fix = false;
	m_flushing_fn = -1;
	m_this_stream_start = m_in_x = m_in_y = 0;
}

void sq2sbs::debug_print(const char *err, ...)
{
	if (m_dbg == NULL)
		return;

	char out[MAX_PATH + 256];

	va_list valist;

	va_start(valist,err);
	wvsprintfA(out, err, valist);
	va_end (valist);
	fprintf(m_dbg, out);
	fflush(m_dbg);
}
void sq2sbs::debug_print(const wchar_t *err, ...)
{
	if (m_dbg == NULL)
		return;

	wchar_t out[MAX_PATH + 256];

	va_list valist;

	va_start(valist,err);
	wvsprintfW(out, err, valist);
	va_end (valist);
	fwprintf(m_dbg, out);
	fflush(m_dbg);
}

sq2sbs::~sq2sbs()
{
	if (m_dbg)
	{
		fprintf(m_dbg, "end of debug.\r\n");
		fclose(m_dbg);
	}
}

HRESULT sq2sbs::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	// time and frame number
	REFERENCE_TIME TimeStart, TimeEnd;
	REFERENCE_TIME MediaStart, MediaEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);
	pIn->GetMediaTime(&MediaStart, &MediaEnd);
	int fn;
	if (m_in_x == 1280)
		fn = (int)((double)(TimeStart+m_this_stream_start)/10000*120/1001 + 0.5);
	else
		fn = (int)((double)(TimeStart+m_this_stream_start)/10000*48/1001 + 0.5);

	debug_print("got frame %d,queue(%d,%d)\r\n", fn, m_left_queue.GetCount(), m_right_queue.GetCount());

	int left = 0;
	if (m_last_fn != -1 && fn != m_last_fn+1 && m_packet_state == packet_state_normal)
	{
		debug_print("lost sync\r\n");
		m_packet_state = packet_state_lost_sync;
		m_flushing_fn = fn;
		fn = m_last_fn + 1;
		left = 0;
	}

	else if (m_packet_state == packet_state_lost_sync)
	{
		if (fn == m_flushing_fn +1 && ((fn&1) == 1) )	// m_flushing_fn == real last fn
		{
			debug_print("lost right eye packets, start new segment.\r\n");
			// new segment
			// TODO: maintain sync
			MPacket *last_packet = m_right_queue.RemoveTail();
			ClearQueue();
			POSITION pos = m_left_queue.AddTail(last_packet);
			left = 0;
			m_packet_state = packet_state_normal;
		}
		else if (fn == m_last_fn +1)
		{
			debug_print("resync.\r\n");
			left = 1;
			m_packet_state = packet_state_normal;
			fn = m_last_fn + 1;
		}
		else if(fn == m_flushing_fn)
		{
			debug_print("right eye flushing\r\n");
			left = 0;
			m_packet_state = packet_state_flushing_R3;
			fn = m_last_fn + 1;
		}
		else
		{
			debug_print("lost left and right eye packets, start new segment.\r\n");
			// new segment
			// TODO: even X or odd Y
			// TODO: maintain sync
			ClearQueue();
			left = 1;
			m_packet_state = packet_state_normal;
		}
	}

	else if (m_packet_state == packet_state_flushing_R3)
	{
		debug_print("3rd right eye flushing packet.\r\n");
		fn = m_last_fn + 1;
		left = 0;
		m_packet_state = packet_state_flushing_R4;
	}

	else if (m_packet_state == packet_state_flushing_R4)
	{
		debug_print("4th right eye flushing packet.\r\n");
		fn = m_last_fn + 1;
		left = 0;
		m_packet_state = packet_state_flushing_L1;
	}

	else if (m_packet_state == packet_state_flushing_L1)
	{
		debug_print("1st left eye flushing packet, reversing right eye order.\r\n");
		//reverse
		MPacket *last_right_packet = m_right_queue.RemoveTail();
		POSITION pos = m_right_queue.AddHead(last_right_packet);

		fn = m_last_fn + 1;
		left = 1;
		m_packet_state = packet_state_flushing_L2;
	}

	else if (m_packet_state == packet_state_flushing_L2)
	{
		debug_print("2nd left eye flushing packet.\r\n");
		fn = m_last_fn + 1;
		left = 1;
		m_packet_state = packet_state_flushing_L3;
	}

	else if (m_packet_state == packet_state_flushing_L3)
	{
		debug_print("3rd left eye flushing packet.\r\n");
		fn = m_last_fn + 1;
		left = 1;
		m_packet_state = packet_state_normal;
	}
	
	else if (m_packet_state == packet_state_normal)
	{
		left = 1- (fn & 0x1);
	}
	else
	{
		debug_print("unkown packet\r\n");
	}

	m_last_fn = fn;

	MPacket *tmp = new MPacket(pIn);
	tmp->fn = fn;
	if (left)
	{
		debug_print("adding to left, queue(%d,%d).\r\n", m_left_queue.GetCount(), m_right_queue.GetCount());
		POSITION pos = m_left_queue.AddTail(tmp);
	}
	else
	{
		debug_print("adding to right, queue(%d,%d).\r\n", m_left_queue.GetCount(), m_right_queue.GetCount());
		POSITION pos = m_right_queue.AddTail(tmp);
	}

	if (m_left_queue.GetCount()>0 && m_right_queue.GetCount()>0 && m_packet_state == packet_state_normal)
	{
		// pointer and stride
		BYTE *pdst = NULL;
		pOut->GetPointer(&pdst);
		int out_size = pOut->GetActualDataLength();
		int stride = out_size *2 /3/ m_out_y ;
		if (stride < m_in_x*2)
			return E_UNEXPECTED;
		BYTE *pdstV = pdst + stride*m_out_y;
		BYTE *pdstU = pdstV + stride*m_out_y/4;
		BYTE *pdst_r = pdst + m_in_x;
		BYTE *pdst_r_V = pdstV + m_in_x/2;
		BYTE *pdst_r_U = pdstU + m_in_x/2;

		CAutoPtr<MPacket> left_packet(m_left_queue.RemoveHead());
		BYTE *psrc_l = left_packet->m_data;
		//left_packet->GetPointer(&psrc_l);
		//BYTE *psrc_l_V = psrc_l + m_image_x*m_image_y;
		//BYTE *psrc_l_U = psrc_l_V + m_image_x*m_image_y/4;

		CAutoPtr<MPacket> right_packet(m_right_queue.RemoveHead());
		BYTE *psrc_r = right_packet->m_data;
		//right_packet->GetPointer(&psrc_r);
		//BYTE *psrc_r_V = psrc_r + m_image_x*m_image_y;
		//BYTE *psrc_r_U = psrc_r_V + m_image_x*m_image_y/4;
		if (m_1088fix)
		{
			my_1088_to_YV12(psrc_l, m_in_x*2, m_in_x*2, pdst, pdstU, pdstV, stride, stride/2);
			my_1088_to_YV12(psrc_r, m_in_x*2, m_in_x*2, pdst_r, pdst_r_U, pdst_r_V, stride, stride/2);
		}
		else
		{
			isse_yuy2_to_yv12_r(psrc_l, m_in_x*2, m_in_x*2, pdst, pdstU, pdstV, stride, stride/2, m_in_y);
			isse_yuy2_to_yv12_r(psrc_r, m_in_x*2, m_in_x*2, pdst_r, pdst_r_U, pdst_r_V, stride, stride/2, m_in_y);
		}


		// add mask here
		// assume file = ssifavs.dll
		// and colorspace = YV12
		if (fn==1)
		{
			HMODULE hm= LoadLibrary(_T("ssifavs.dll"));
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
			int height = m_out_y;
			for(int y=0; y<size.cy; y++)
			{
				memcpy(pdst+(y+height/2-size.cy/2)*width +width/4-size.cx/2, 
					m_mask + size.cx*y, size.cx);
				memcpy(pdst+(y+height/2-size.cy/2)*width +width/4-size.cx/2 + width/2,
					m_mask + size.cx*y, size.cx);
			}

			free(m_mask);
		}
		pOut->SetTime(&left_packet->rtStart, &left_packet->rtStop);
		debug_print("deliver\r\n");
		return S_OK;
	}
	else
	{
		debug_print("not enough data\r\n");
		return S_FALSE;
	}
}
HRESULT sq2sbs::CheckInputType(const CMediaType *mtIn)
{
	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;


	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YUY2)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_out_x = m_in_x = pbih->biWidth;
		m_out_y = m_in_y = pbih->biHeight;

		// 1088 fix
		if (m_in_y == 1088)
		{
			m_out_y = 1080;
			m_1088fix = true;
		}

		hr = S_OK;
	}

	return hr;
}

HRESULT sq2sbs::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == MEDIASUBTYPE_YUY2 && subtypeOut == MEDIASUBTYPE_YV12)
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
	pProperties->cbBuffer = m_in_x * 2 * m_out_y * 3/2;

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
		pMediaType->SetSubtype(&MEDIASUBTYPE_YV12);
		pMediaType->SetSampleSize(m_out_x*2*m_out_y*3/2);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));
		RECT zero = {0,0,0,0};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->dwPictAspectRatioX = 32;
		vihOut->dwPictAspectRatioY = 9;	// 16:9 only
		vihOut->bmiHeader.biWidth = m_out_x *2 ;
		vihOut->bmiHeader.biHeight = m_out_y;


		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源


	}

	return NOERROR;
}
HRESULT sq2sbs::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	// TODO: flush to ensure sync
	CAutoLock lck(&m_csReceive);

	debug_print("new segment!\r\n");
	ClearQueue();
	m_last_fn = -1;
	m_flushing_fn = -1;
	m_this_stream_start = tStart;
	return __super::NewSegment(tStart, tStop, dRate);
}
void sq2sbs::ClearQueue()
{
	while(m_left_queue.GetCount())
	{
		delete m_left_queue.RemoveTail();
	}
	while(m_right_queue.GetCount())
	{
		delete m_right_queue.RemoveTail();
	}
}

#include "ssp.h"
#include <windows.h>

#include "..\libchecksum\libchecksum.h"
#include "..\SsifSource\src\filters\parser\MpegSplitter\mvc.h"

#define ssp_hwnd (FindWindow(_T("4C463F505C19080C5A2D5F4744591F1E"), NULL))

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

// {854408FE-B7CB-4881-A8AF-5E17FB42AC24}
DEFINE_GUID(CLSID_SSP_TRANFORMER, 
                        0x854408FE, 0xB7CB, 0x4881, 0xA8, 0xAF, 0x5E, 0x17, 0xFB, 0x42, 0xac, 0x24);

// {55DA30FC-F16B-49FC-BAA5-AE59FC65F82D}
DEFINE_GUID(CLSID_haali_source,
						0x55DA30FC, 0xF16B, 0x49FC, 0xBA, 0xA5, 0xAE, 0x59, 0xFC, 0x65, 0xF8, 0x2D);

// {DBF9000E-F08C-4858-B769-C914A0FBB1D7}
DEFINE_GUID(CLSID_ffdshow_subtitle,
						0xDBF9000E, 0xF08C, 0x4858, 0xB7, 0x69, 0xC9, 0x14, 0xA0, 0xFB, 0xB1, 0xD7);

// helper functions
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

HRESULT GetConnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
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
			if (SUCCEEDED(hr)) // Connected, this is the pin we want. 
			{
				pTmp->Release();
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
			else  // Unconnected, not the pin we want.
			{
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

HRESULT GetUpperFilter(IBaseFilter *pFilter, IBaseFilter **pOut)
{
	if (!pFilter || !pOut)
		return E_POINTER;

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
				*pOut = info.pFilter;
				pTmp->Release();
				pPin->Release();
				pEnum->Release();
				return S_OK;
			}
			else  // Unconnected
			{}
		}
		pPin->Release();
	}

	pEnum->Release();
	return NULL;
}

HRESULT FindPin(IBaseFilter *filter, IPin **out, wchar_t *pin_name, int connected=-1)// -1 = don't check connect, 0=un, 1=connected
{
	HRESULT hr = E_FAIL;

	if (!filter || !out)
		return E_POINTER;

	CComPtr<IEnumPins> penum;
	filter->EnumPins(&penum);

	CComPtr<IPin> pin;
	ULONG fetched = 0;
	while (penum->Next(1, &pin, &fetched) == S_OK)
	{
		PIN_INFO pi;
		pin->QueryPinInfo(&pi);
		if(pi.pFilter) pi.pFilter->Release();

		if(wcsstr(pi.achName, pin_name))
		{
			if (connected == -1)
			{
				hr = S_OK;
				*out = pin;
				(*out)->AddRef();
				break;
			}

			CComPtr<IPin> connected;
			if (SUCCEEDED(pin->ConnectedTo(&connected)) && connected)
			{
				hr = S_OK;
				*out = pin;
				(*out)->AddRef();
				break;
			}

			connected = NULL;
			if (FAILED(pin->ConnectedTo(&connected)) && !connected)
			{
				hr = S_OK;
				*out = pin;
				(*out)->AddRef();
				break;
			}

		}

		pin = NULL;
	}

	return hr;
}

HRESULT GetTopmostFilter(IBaseFilter *filter, IBaseFilter **pOut)
{
	if (!filter || !pOut)
		return E_FAIL;

	CComPtr<IBaseFilter> cur = filter;
	CComPtr<IBaseFilter> up;

	while(true)
	{
		up = NULL;
		GetUpperFilter(cur, &up);

		if(!up)
		{
			*pOut = cur;
			(*pOut)->AddRef();
			return S_OK;
		}

		cur = NULL;
		cur = up;
	}

	return E_FAIL;
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
	GetConnectedPin(filter, PINDIR_INPUT, &decoder_i);
	CComPtr<IPin> decoder_up;
	decoder_i->ConnectedTo(&decoder_up);
	gb->Disconnect(decoder_i);
	gb->Disconnect(decoder_up);
	gb->ConnectDirect(demuxer_o, decoder_i, NULL);

	// remove source & demuxer, and reconnect decoder
	
	gb->RemoveFilter(h264);
	gb->RemoveFilter(demuxer);
	gb->ConnectDirect(decoder_up, decoder_i, NULL);

	// delete file
	_wremove(tmp);

	return S_OK;
}

CDWindowSSP::CDWindowSSP(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_YV12MonoMixer)
{
	m_buffer_has_data = false;
	m_t = 0;
	my12doom_found = false;
	m_image_buffer = NULL;
	m_mask = NULL;
	h_F11_thread = INVALID_HANDLE_VALUE;
}

CDWindowSSP::~CDWindowSSP() 
{
	if (h_F11_thread != INVALID_HANDLE_VALUE)
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

	REFERENCE_TIME TimeStart, TimeEnd;
	LONGLONG MediaStart, MediaEnd;
	pIn->GetTime(&TimeStart, &TimeEnd);
	pIn->GetMediaTime(&MediaStart,&MediaEnd);

	int fn;
	if (m_image_x == 1280)
		fn = (double)(TimeStart+m_t)/10000*120/1001 + 0.5;
	else
		fn = (double)(TimeStart+m_t)/10000*48/1001 + 0.5;

	int left = 1-(fn & 1);

	BYTE *psrc = NULL;
	pIn->GetPointer(&psrc);
	LONG data_size = pIn->GetActualDataLength();

	if(!my12doom_found && pOut)
	{
		// pass thourgh
		BYTE *pdst = NULL;
		pOut->GetPointer(&pdst);

		memcpy(pdst, psrc, data_size);
		return S_OK;
	}

	if (m_image_x == 1280)
		if( m_frm < -36000)
			m_frm = 2400;
	else
		if(m_frm < -14400)
			m_frm = 960;

	m_frm --;
	if (left)
	{
		m_buffer_has_data = true;
		memcpy(m_image_buffer, psrc, data_size);
		return S_FALSE;
	}
	else
	{
		if(pOut)
		{
			if (!m_buffer_has_data)
				return S_FALSE;

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

	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YUY2 && my12doom_found)
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
			vihOut->dwPictAspectRatioY = 9;	// 16:9 only
			vihOut->bmiHeader.biWidth = m_image_x *2 ;
		}
		else
		{

			// compute aspect
			int aspect_x = m_image_x;
			int aspect_y = m_image_y;
			for(int i=2; i<100; i++)
			{
				if (aspect_x % i == 0 && aspect_y % i == 0)
				{
					aspect_x /= i;
					aspect_y /= i;
				}
			}
			vihOut->dwPictAspectRatioX = aspect_x;
			vihOut->dwPictAspectRatioY = aspect_y;
			vihOut->bmiHeader.biWidth = m_image_x;
		}
		vihOut->bmiHeader.biHeight = m_image_y;

		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源

	}

	return NOERROR;
}

/*
HRESULT CDWindowSSP::StartStreaming()
{
	return __super::StartStreaming();
}
*/
HRESULT CDWindowSSP::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutolock(&m_csReceive);
	printf("New Segment!..\n");

	if (m_image_x == 1280)
		m_frm = 2400;
	else
		m_frm = 960;

	m_t = tStart;

	m_buffer_has_data = false;


	return CTransformFilter::NewSegment(tStart, tStop, dRate);
}

HRESULT CDWindowSSP::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT)
		m_demuxer = NULL;
	return CTransformFilter::BreakConnect(dir);
}

STDMETHODIMP CDWindowSSP::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
	if (pGraph)
	{
		// check creation
		CComPtr<IBaseFilter> demuxer_test;
		CComPtr<IBaseFilter> decoder_test;
		demuxer_test.CoCreateInstance(CLSID_PD10_DEMUXER);
		decoder_test.CoCreateInstance(CLSID_PD10_DECODER);

		if (demuxer_test == NULL || decoder_test == NULL)
		{
			MessageBoxW(ssp_hwnd, L"some modules not registered, try use DWindow Launcher to fix it.\n\n"
				L"某些组件没有注册，请使用DWindow注册组件",L"Warning", MB_OK | MB_ICONERROR);
			return E_FAIL;
		}

		demuxer_test = NULL;

		// check decoder version
		if (decoder_test != NULL)
		{
			decoder_test = NULL;
			HKEY hkeyFilter=0;
			DWORD dwSize=MAX_PATH;
			BYTE pbFilename[MAX_PATH];
			int rc = RegOpenKey(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\{D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}\\InprocServer32", &hkeyFilter);
			rc = RegQueryValueEx(hkeyFilter, NULL,  // Read (Default) value
									NULL, NULL, pbFilename, &dwSize);

			TCHAR szFilename[MAX_PATH];
			HRESULT hr = StringCchPrintf(szFilename, NUMELMS(szFilename), TEXT("%s\0"), pbFilename);

			const int filesize = 615792;
			FILE * f = _wfopen(szFilename, L"rb");
			unsigned char *buf = (unsigned char*)malloc(filesize);
			fread(buf, 1, filesize, f);
			fclose(f);

			DWORD sha1[5];
			DWORD sha1_right[5] ={0x597ea85e, 0xd026f966, 0x25a5df89, 0x587b6272, 0xd574a627};
			SHA1Hash((unsigned char*)sha1, buf, filesize);
			RegCloseKey(hkeyFilter);

			for(int i=0; i<5; i++)
			{
				if (sha1[i] != sha1_right[i])
				{
					MessageBoxW(ssp_hwnd, L"some modules are invalid version, try use DWindow Launcher to fix it.\n\n"
						L"某些组件是错误的版本，请使用DWindow注册正确版本组件",L"Warning", MB_OK | MB_ICONERROR);
					return E_FAIL;
				}
			}
		}
	}

	// join it
	HRESULT hr = CTransformFilter::JoinFilterGraph(pGraph, pName);


	if (pGraph)
	{
		// check input file signature
		bool PD10_found = false;
		my12doom_found = false;
		CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(pGraph);

		CComPtr<IEnumFilters> penum;
		gb->EnumFilters(&penum);
		ULONG fetched = 0;
		CComPtr<IBaseFilter> filter;
		while(penum->Next(1, &filter, &fetched) == S_OK)
		{
			CLSID filter_id;
			filter->GetClassID(&filter_id);

			// test if need active
			if (filter_id == CLSID_PD10_DECODER)
			{
				PD10_found = true;
				bool this_decoder_is_remux = false;
				CComPtr<IBaseFilter> topmost;
				GetTopmostFilter(filter, &topmost);

				CComQIPtr<IMVC, &IID_IMVC> mvc(topmost);
				CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> source(topmost);
				
				if (mvc != NULL)
				{
					if (mvc->IsMVC() == S_OK)
						this_decoder_is_remux = my12doom_found = true;
				}
				else if (source != NULL)
				{
					// check input file
					LPOLESTR file = NULL;
					source->GetCurFile(&file, NULL);
					if(file)
					{
						int verify_result = verify_file(file);
						if (verify_result == 2)
							this_decoder_is_remux = my12doom_found = true;

						CoTaskMemFree(file);
					}
				}

				if (this_decoder_is_remux)
					ActiveMVC(filter);
			}


			filter = NULL;
		}

		if (!PD10_found)
		MessageBoxW(ssp_hwnd, L"Cyberlink Video Decoder not found.\n"
			L"try use DWindow Launcher to fix it.\n\n"
			L"未发现Cyberlink Video Decoder\n"
			L"请使用DWindow配置工具配置SSP", L"Warning", MB_OK | MB_ICONERROR);

		else if(!my12doom_found)
		MessageBoxW(ssp_hwnd, L"Non-REMUX content detected, switching to compatibility mode\n"
			L"if picture become confused or freezed,\n"
			L"please use DWindow config tool to reset SSP's config\n\n"
			L"检测到非REMUX文件，正在使用兼容模式播放\n"
			L"如果仍出现画面异常或冻结，请用DWindow配置工具恢复SSP默认设置", L"Warning", MB_OK | MB_ICONERROR);


	}

	return hr;
}


HRESULT CDWindowSSP::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
	if (m_demuxer == NULL && direction == PINDIR_INPUT)
	{
		// prepare
		CComQIPtr<IBaseFilter, &IID_IBaseFilter> pbase(this);
		FILTER_INFO fi;
		pbase->QueryFilterInfo(&fi);
		CComPtr<IEnumFilters> pEnum;
		HRESULT hr = fi.pGraph->EnumFilters(&pEnum);
		
		// find cyberlink demuxer
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
			;//MessageBox(ssp_hwnd,_T("Cyberlink Demuxer 2.0 not found, image might be wrong."), _T("Warning"), MB_OK | MB_ICONERROR);
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

	// subtile
	if (direction == PINDIR_OUTPUT)
	{
		// check if ffdshow subtitle
		PIN_INFO recieve_info;
		pReceivePin->QueryPinInfo(&recieve_info);
		CLSID recieve_id;
		recieve_info.pFilter->GetClassID(&recieve_id);
		CComPtr<IBaseFilter> recieve_filter = recieve_info.pFilter;
		recieve_info.pFilter->Release();
		if (recieve_id != CLSID_ffdshow_subtitle)
			goto end_output;

		// prepare graphBuilder...
		CComQIPtr<IBaseFilter, &IID_IBaseFilter> pbase(this);
		FILTER_INFO fi;
		pbase->QueryFilterInfo(&fi);
		CComQIPtr<IGraphBuilder, &IID_IGraphBuilder> gb(fi.pGraph);
		fi.pGraph->Release();

		// go upstream and find unconnected Subtitle pin
		CComPtr<IBaseFilter> filter = pbase;
		CComPtr<IBaseFilter> up;
		while(true)
		{
			// find upstream
			up = NULL;
			GetUpperFilter(filter, &up);

			if(!up)
				break;

			// test find pin and connect
			CComPtr<IPin> subtitle_pin;
			::FindPin(up, &subtitle_pin, L"Subtitle", 0);
			CComPtr<IPin> in_text_pin;
			::FindPin(recieve_filter, &in_text_pin, L"In Text", 0);

			if (subtitle_pin != NULL && in_text_pin != NULL)
				gb->ConnectDirect(subtitle_pin, in_text_pin, NULL);

			// go upstream
			filter = NULL;
			filter = up;
		}
		
		// disconnect SSP's transformer
		filter = NULL;
		CComPtr<IEnumFilters> penum;
		gb->EnumFilters(&penum);
		ULONG fetched = 0;
		while(penum->Next(1, &filter, &fetched) == S_OK)
		{
			CLSID filter_id;
			filter->GetClassID(&filter_id);
			if (filter_id == CLSID_SSP_TRANFORMER)
			{
				CComPtr<IPin> pin1;
				CComPtr<IPin> pin1o;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin1);
				if(pin1 != NULL)
				{
					pin1->ConnectedTo(&pin1o);
					gb->Disconnect(pin1);
					gb->Disconnect(pin1o);
				}


				CComPtr<IPin> pin2;
				CComPtr<IPin> pin2o;
				GetConnectedPin(filter, PINDIR_OUTPUT, &pin2);
				if(pin2 != NULL)
				{
					pin2->ConnectedTo(&pin2o);
					gb->Disconnect(pin2);
					gb->Disconnect(pin2o);
				}

			}

			filter = NULL;
		}
	}
end_output:


	prepare_mask();

	return S_OK;
}


DWORD WINAPI CDWindowSSP::default_thread(LPVOID lpParame)
{

	return 0;
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

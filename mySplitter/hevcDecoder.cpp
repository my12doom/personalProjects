#include <atlbase.h>
#include <InitGuid.h>
#include "hevcDecoder.h"
#include <dvdmedia.h>
#include <stdio.h>
#include <math.h>
#include <tchar.h>

typedef OpenHevc_Handle (__cdecl* pOpenHevcInit)(int nb_pthreads);
typedef const char *(__cdecl *pOpenHevcVersion)(OpenHevc_Handle openHevcHandle);
typedef int  (__cdecl* pOpenHevcDecode)(OpenHevc_Handle openHevcHandle, const unsigned char *buff, int nal_len, int64_t pts);
typedef void (__cdecl* pOpenHevcGetPictureInfo)(OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo);
typedef void (__cdecl* pOpenHevcGetPictureSize2)(OpenHevc_Handle openHevcHandle, OpenHevc_FrameInfo *openHevcFrameInfo);
typedef int  (__cdecl* pOpenHevcGetOutput)(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame *openHevcFrame);
typedef int  (__cdecl* pOpenHevcGetOutputCpy)(OpenHevc_Handle openHevcHandle, int got_picture, OpenHevc_Frame_cpy *openHevcFrame);
typedef void (__cdecl* pOpenHevcSetCheckMD5)(OpenHevc_Handle openHevcHandle, int val);
typedef void (__cdecl* pOpenHevcClose)(OpenHevc_Handle openHevcHandle);
typedef void (__cdecl* pOpenHevcFlush)(OpenHevc_Handle openHevcHandle);

pOpenHevcInit OpenHevcInit;
pOpenHevcVersion OpenHevcVersion;
pOpenHevcDecode OpenHevcDecode;
pOpenHevcGetPictureInfo OpenHevcGetPictureInfo;
pOpenHevcGetPictureSize2 OpenHevcGetPictureSize2;
pOpenHevcGetOutput OpenHevcGetOutput;
pOpenHevcGetOutputCpy OpenHevcGetOutputCpy;
pOpenHevcSetCheckMD5 OpenHevcSetCheckMD5;
pOpenHevcClose OpenHevcClose;
pOpenHevcFlush OpenHevcFlush;

static GUID MEDIASUBTYPE_HM91 = FOURCCMap('19MH');
static GUID MEDIASUBTYPE_HM10 = FOURCCMap('01MH');
static GUID MEDIASUBTYPE_HM11 = FOURCCMap('11MH');
static GUID MEDIASUBTYPE_HM12 = FOURCCMap('21MH');
static GUID MEDIASUBTYPE_HEVC = FOURCCMap('CVEH');

CHEVCDecoder::CHEVCDecoder(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
:CTransformFilter(tszName, punk, CLSID_OpenHEVCDecoder)
,h(NULL)
{
	wchar_t module[MAX_PATH];

	GetModuleFileNameW((HINSTANCE)&__ImageBase, module, MAX_PATH);
	wcscpy((wchar_t*)wcsrchr(module, L'\\'), L"\\libLibOpenHevcWrapper.dll");
	HMODULE h = LoadLibraryW(module);

	if (h)
	{
		OpenHevcInit = (pOpenHevcInit)GetProcAddress(h, "libOpenHevcInit");
		OpenHevcVersion = (pOpenHevcVersion)GetProcAddress(h, "libOpenHevcVersion");
		OpenHevcDecode = (pOpenHevcDecode)GetProcAddress(h, "libOpenHevcDecode");
		OpenHevcGetPictureInfo = (pOpenHevcGetPictureInfo)GetProcAddress(h, "libOpenHevcGetPictureInfo");
		OpenHevcGetPictureSize2 = (pOpenHevcGetPictureSize2)GetProcAddress(h, "libOpenHevcGetPictureSize2");
		OpenHevcGetOutput = (pOpenHevcGetOutput)GetProcAddress(h, "libOpenHevcGetOutput");
		OpenHevcGetOutputCpy = (pOpenHevcGetOutputCpy)GetProcAddress(h, "libOpenHevcGetOutputCpy");
		OpenHevcSetCheckMD5 = (pOpenHevcSetCheckMD5)GetProcAddress(h, "libOpenHevcSetCheckMD5");
		OpenHevcClose = (pOpenHevcClose)GetProcAddress(h, "libOpenHevcClose");
		OpenHevcFlush = (pOpenHevcFlush)GetProcAddress(h, "libOpenHevcFlush");
	}
}

CHEVCDecoder::~CHEVCDecoder() 
{
}


// CreateInstance
// Provide the way for COM to create a DWindowExtenderMono object
CUnknown *CHEVCDecoder::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CHEVCDecoder *pNewObject = new CHEVCDecoder(NAME("DWindow OpenHEVC Decoder"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

STDMETHODIMP CHEVCDecoder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CHEVCDecoder::CheckInputType(const CMediaType *mtIn)
{
	if (mtIn->majortype != MEDIATYPE_Video)
		return VFW_E_INVALID_MEDIA_TYPE;
	GUID subtypeIn = *mtIn->Subtype();
	if( *mtIn->FormatType() == FORMAT_MPEG2_VIDEO)
	
	if(subtypeIn == MEDIASUBTYPE_HM91 || subtypeIn == MEDIASUBTYPE_HM10 || subtypeIn == MEDIASUBTYPE_HM11 || subtypeIn == MEDIASUBTYPE_HM12 || subtypeIn == MEDIASUBTYPE_HEVC)
	{
		BITMAPINFOHEADER *pbih = &((MPEG2VIDEOINFO*)mtIn->Format())->hdr.bmiHeader;

		m_width = pbih->biWidth;
		m_height = abs(pbih->biHeight);

		m_subtype = MEDIASUBTYPE_YV12;
		m_frame_length = ((MPEG2VIDEOINFO*)mtIn->Format())->hdr.AvgTimePerFrame;

		return S_OK;
	}

	return VFW_E_INVALID_MEDIA_TYPE;
}

HRESULT CHEVCDecoder::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();


	return NOERROR;
}

HRESULT CHEVCDecoder::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;

	// Do we have more items to offer
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();
	if (*inMediaType->FormatType() == FORMAT_MPEG2_VIDEO)
	{
		VIDEOINFOHEADER2 *vihIn = &((MPEG2VIDEOINFO*)inMediaType->Format())->hdr;
		pMediaType->SetType(&MEDIATYPE_Video);
		pMediaType->SetFormatType(&FORMAT_VideoInfo2);
		pMediaType->SetSubtype(&m_subtype);
		pMediaType->SetSampleSize(m_width*m_height*4);
		pMediaType->SetTemporalCompression(FALSE);

		VIDEOINFOHEADER2 *vihOut = (VIDEOINFOHEADER2 *)pMediaType->ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memcpy(vihOut, vihIn, sizeof(VIDEOINFOHEADER2));

		RECT zero = {0,0,0,0};
		vihOut->rcSource = zero;
		vihOut->rcTarget = zero;

		vihOut->bmiHeader.biWidth = m_width ;
		vihOut->bmiHeader.biHeight = m_height;
		vihOut->bmiHeader.biBitCount = 12;
		vihOut->bmiHeader.biCompression = '21VY';

		vihOut->bmiHeader.biXPelsPerMeter = 1;
		vihOut->bmiHeader.biYPelsPerMeter = 1;

		vihOut->dwInterlaceFlags = 0;	// 不支持交织图像，如果发现闪瞎狗眼的图像，检查片源
		vihOut->bmiHeader.biSizeImage = m_width*m_height*4;

		// compute aspect
		int aspect_x = m_width;
		int aspect_y = m_height;
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

HRESULT CHEVCDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (h)
		OpenHevcClose(h);

	h = OpenHevcInit(4);
	m_pts = -1;
	const char *v = OpenHevcVersion(h);

	return __super::NewSegment(tStart, tStop, dRate);
}


HRESULT CHEVCDecoder::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_width*m_height*3/2;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;

	return NOERROR;
}

int protected_decoding(OpenHevc_Handle openHevcHandle, const unsigned char *buff, int nal_len, int64_t pts)
{
	__try
	{
		return OpenHevcDecode(openHevcHandle, buff, nal_len, pts);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return -1;
	}
}

HRESULT CHEVCDecoder::Receive(IMediaSample *pIn)
{
	IPin *pin = NULL;
	if (FAILED(m_pOutput->ConnectedTo(&pin)) || !pin)
		return E_UNEXPECTED;
	CComQIPtr<IMemInputPin, &IID_IMemInputPin> mp(pin);

	// Check for other streams and pass them on
	AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA) {
		return mp->Receive(pIn);
	}

	HRESULT hr;

	int frame_decoded = 0;

	BYTE *p = NULL;
	int size = 0;
	REFERENCE_TIME start, end;

	pIn->GetPointer(&p);
	size = pIn->GetActualDataLength();
	pIn->GetTime(&start, &end);

	OpenHevc_Frame openHevcFrame;
	int got_picture;

	while (size>0)
	{
		if (size<4)
			break;
		int seg_size = (p[0] <<24) | (p[1] <<16) | (p[2] <<8) | p[3];
		if (size < seg_size+4)
			break;
		memcpy(p, "\x0\x0\x0\x1", 4);

		got_picture = protected_decoding(h, p, seg_size+4, start);

		p += 4+seg_size;
		size -= 4+seg_size;

		static int t = GetTickCount();
		if (got_picture>0)
		{
			// Set up the output sample
			IMediaSample * pOut;
			hr = InitializeOutputSample(pIn, &pOut);

			if (FAILED(hr)) {
				return hr;
			}

			OpenHevcGetOutput(h, got_picture, &openHevcFrame);
			OpenHevcGetPictureSize2(h, &openHevcFrame.frameInfo);

			if (m_pts < 0)
				m_pts = openHevcFrame.frameInfo.nTimeStamp;
			else
				m_pts += m_frame_length;
			start = m_pts;
			end = start + max(1, m_frame_length);
			pOut->SetTime(&start, &end);
			BYTE *o = NULL;
			pOut->GetPointer(&o);
			int stride = pOut->GetSize() *2/3 / m_height;

			// copy y
			BYTE *in = (BYTE*)openHevcFrame.pvY;
			for(int y=0; y<m_height; y++)
			{
				memcpy(o, in, m_width);
				o += stride;
				in += openHevcFrame.frameInfo.nYPitch;
			}

			in = (BYTE*)openHevcFrame.pvV;
			for(int y=0; y<m_height/2; y++)
			{
				memcpy(o, in, m_width/2);
				o += stride/2;
				in += openHevcFrame.frameInfo.nVPitch;
			}
			in = (BYTE*)openHevcFrame.pvU;
			for(int y=0; y<m_height/2; y++)
			{
				memcpy(o, in, m_width/2);
				o += stride/2;
				in += openHevcFrame.frameInfo.nUPitch;
			}
			frame_decoded++;
			
			hr = mp->Receive(pOut);;
			pOut->Release();
		}
	}

	return S_OK;
}

HRESULT CHEVCDecoder::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	return E_UNEXPECTED;
}

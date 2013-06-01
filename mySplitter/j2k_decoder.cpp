#include <atlbase.h>
#include <InitGuid.h>
#include "j2k_decoder.h"
#include <dvdmedia.h>
#include <stdio.h>
#include <math.h>
#include <tchar.h>
#include <j2k-codec.h>
#pragma comment(lib, "j2k-dynamic.lib")

int DownmixTo8bit(char *out, J2K_Image *img);

CJ2kDecoder::CJ2kDecoder(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
:CTransformFilter(tszName, punk, CLSID_my12doomJ2KDecoder)
,m_workers(NULL)
{
} 

CJ2kDecoder::~CJ2kDecoder() 
{
	if (m_workers)
		delete m_workers;
}


// {7FB9DA30-61D1-4c42-AB65-069568CEC361}
DEFINE_GUID(MEDIASUBTYPE_JPEG2000, 0x43324A4D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// CreateInstance
// Provide the way for COM to create a DWindowExtenderMono object
CUnknown *CJ2kDecoder::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CJ2kDecoder *pNewObject = new CJ2kDecoder(NAME("DWindow JPEG2000 Decoder"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

STDMETHODIMP CJ2kDecoder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CJ2kDecoder::CheckInputType(const CMediaType *mtIn)
{
	if (mtIn->majortype != MEDIATYPE_Video)
		return VFW_E_INVALID_MEDIA_TYPE;
	GUID subtypeIn = *mtIn->Subtype();
	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_JPEG2000 )

	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_subtype = subtypeIn;
		m_width = pbih->biWidth;
		m_height = abs(pbih->biHeight);

		m_subtype = MEDIASUBTYPE_RGB32;

		return S_OK;
	}

	return VFW_E_INVALID_MEDIA_TYPE;
}

HRESULT CJ2kDecoder::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();


	return NOERROR;
}

HRESULT CJ2kDecoder::GetMediaType(int iPosition, CMediaType *pMediaType)
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
	if (*inMediaType->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)inMediaType->Format();
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
		vihOut->bmiHeader.biHeight = -m_height;
		vihOut->bmiHeader.biCompression = 0;

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

HRESULT CJ2kDecoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (m_workers)
		delete m_workers;

	m_workers = new thread_pool(4);
	m_fn_recieved = m_fn_delivered = -1;

	CAutoLock lck(&m_samples_cs);
	for(std::list<sample_entry>::iterator i = m_completed_samples.begin(); i!=m_completed_samples.end(); ++i)
		(*i).sample->Release();

	m_completed_samples.clear();

	return __super::NewSegment(tStart, tStop, dRate);
}


HRESULT CJ2kDecoder::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 8;
	pProperties->cbBuffer = m_width*m_height*4;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;

	return NOERROR;
}

HRESULT CJ2kDecoder::Receive(IMediaSample *pSample)
{
	/*  Check for other streams and pass them on */
	IPin *p = NULL;
	if (FAILED(m_pOutput->ConnectedTo(&p)) || !p)
		return E_UNEXPECTED;
	CComQIPtr<IMemInputPin, &IID_IMemInputPin> mp(p);

	AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA) {
		return mp->Receive(pSample);
		//return m_pOutput->m_pInputPin->Receive(pSample);
	}
	HRESULT hr;
	ASSERT(pSample);
	IMediaSample * pOutSample;

	// If no output to deliver to then no point sending us data

	ASSERT (m_pOutput != NULL) ;

	// Set up the output sample
	hr = InitializeOutputSample(pSample, &pOutSample);

	if (FAILED(hr)) {
		return hr;
	}

	m_fn_recieved++;
	m_workers->submmit(new J2KWorker(pSample, pOutSample, this, m_fn_recieved));



	// search for completed samples

	int sleep = 1;
	int l = GetTickCount();
	while(sleep && GetTickCount() - l < 100)
	{
		{
			if (m_fn_recieved - m_fn_delivered >4)
				sleep = 10;
			else
				sleep = 0;
		}
		Sleep(sleep);
	}

	CAutoLock lck(&m_samples_cs);
	for(std::list<sample_entry>::iterator i = m_completed_samples.begin(); i!=m_completed_samples.end(); ++i)
	{
		if ((*i).fn == m_fn_delivered+1)
		{
			hr = mp->Receive((*i).sample);
			(*i).sample->Release();
			m_completed_samples.erase(i);
			m_fn_delivered++;
			return hr;
		}
	}

	return hr;
}

HRESULT CJ2kDecoder::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	return E_UNEXPECTED;
}


int DownmixTo8bit(char *out, J2K_Image *img)
{
	int w, w2, h, width = img->Width, height = img->Height, bpp, prec, color_map_size;

	unsigned char   *row;
	unsigned short *srow;

	prec = img->Precision;

	if(prec > 8) bpp = img->buffer_bpp / 2; else bpp = img->buffer_bpp;

	if(bpp == 2) return 0; // not supported

	color_map_size = (bpp == 1 ? 256*4 : 0);

	row = img->buffer + width * (height-1) * bpp;
	srow = (unsigned short*)(img->buffer + width * (height-1) * bpp * 2);

	if(bpp==1)
	{
		for(w = 0; w < 256; w++)
		{
			*(DWORD*)out = (w<<16) | (w<<8) | w;
			out +=4;
		}

		for(h = 0; h < height; h++)
		{
			if(prec > 8) // 16-bit
			{
				for(w = 0; w < width; w++)
				{
					*out++ = (srow[w] >> (prec-8));
				} // we need to scale it to 8 bits in BMP
			}
			else
			{
				memcpy(out, row, width);
				out += width;
			}

			if(width&3)
				for(w = 0; w < 4-(width&3); w++)
					*out++ = 0;

			row -= img->buffer_pitch;
			srow -= img->buffer_pitch / 2;
		}
	}
	else
	{
		w2 = width * bpp;

		if(prec > 8) // 16-bit
		{
			for(h = 0; h < height; h++, srow -= img->buffer_pitch / 2)
			{
				for(w = 0; w < w2; w++)
				{
					*out++ = (srow[w] >> (prec-8));
				}

				if(w2&3) for(w = 0; w < 4-(w2&3); w++)
					*out++ = 0;
			}
		}
		else // 8-bit
		{
			for(h = 0; h < height; h++, row -= img->buffer_pitch)
			{
				memcpy(out, row, w2);
				out += w2;

				if(w2&3) for(w = 0; w < 4-(w2&3); w++)
					*out++ = 0;
			}
		}
	}

	return 1;
}//_____________________________________________________________________________


J2KWorker::J2KWorker(IMediaSample *pIn, IMediaSample *pOut, CJ2kDecoder *owner, int fn)
:m_in(pIn)
,m_out(pOut)
,m_owner(owner)
,m_fn(fn)
{

}

void J2KWorker::run()
{
	// the process
	IMediaSample *pSample = m_in;
	IMediaSample *pOutSample = m_out;
	BYTE *src;
	BYTE *dst;

	pSample->GetPointer(&src);
	pOutSample->GetPointer(&dst);

	int outsize = m_owner->m_width*m_owner->m_height*4;
	pOutSample->SetActualDataLength(outsize);

	J2K_CImage img;
	int l = GetTickCount();
	int j2k_err = img.easyDecode(src, pSample->GetActualDataLength(), 0, _T("thread=0,video=0"));
	int dl = GetTickCount()-l;

	printf("time:%d\n", dl);

	DownmixTo8bit((char*)dst, &img);

	REFERENCE_TIME TimeStart, TimeEnd;
	LONGLONG MediaStart, MediaEnd;
	if (SUCCEEDED(pSample->GetMediaTime(&MediaStart,&MediaEnd)))
		pOutSample->SetMediaTime(&MediaStart,&MediaEnd);
	if (SUCCEEDED(pSample->GetTime(&TimeStart, &TimeEnd)))
		pOutSample->SetTime(&TimeStart, &TimeEnd);
	pOutSample->SetSyncPoint(pSample->IsSyncPoint() == S_OK);
	pOutSample->SetPreroll(pSample->IsPreroll() == S_OK);
	pOutSample->SetDiscontinuity(pSample->IsDiscontinuity() == S_OK);

	CAutoLock lck(&m_owner->m_samples_cs);
	sample_entry entry = {m_out, m_fn};
	m_owner->m_completed_samples.push_back(entry);
}
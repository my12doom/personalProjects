#include "CVideoSwitcher.h"

// {514B2434-50D7-4782-9F0D-52E42CCD0BFA}
static const GUID CLSID_S2SBS = { 0x514b2434, 0x50d7, 0x4782, { 0x9f, 0xd, 0x52, 0xe4, 0x2c, 0xcd, 0xb, 0xfa } };

CVideoSwitcher::CVideoSwitcher(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
:C2to1Filter(tszName, punk, CLSID_S2SBS)
{
	m_out_x = m_in_x = 0;
	m_out_y = m_in_y = 0;
}
CVideoSwitcher::~CVideoSwitcher()
{

}

HRESULT CVideoSwitcher::CheckInputType(const CMediaType *mtIn)
{
	GUID subtypeIn = *mtIn->Subtype();
	HRESULT hr = VFW_E_INVALID_MEDIA_TYPE;


	if( *mtIn->FormatType() == FORMAT_VideoInfo2 && subtypeIn == MEDIASUBTYPE_YV12)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)mtIn->Format())->bmiHeader;
		m_out_x = m_in_x = pbih->biWidth;
		m_out_y = m_in_y = pbih->biHeight;

		hr = S_OK;
	}

	return hr;
}
HRESULT CVideoSwitcher::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();

	if(subtypeIn == MEDIASUBTYPE_YV12 && subtypeOut == MEDIASUBTYPE_YV12)
		return S_OK;

	return VFW_E_INVALID_MEDIA_TYPE;
}
HRESULT CVideoSwitcher::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_out_x * m_out_y * 3/2;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;

	return NOERROR;
}
HRESULT CVideoSwitcher::GetMediaType(int iPosition, CMediaType *pMediaType)
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

		vihOut->dwPictAspectRatioX = 16;
		vihOut->dwPictAspectRatioY = 9;	// 16:9 only
		vihOut->bmiHeader.biWidth = m_out_x ;
		vihOut->bmiHeader.biHeight = m_out_y;


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

HRESULT CVideoSwitcher::Transform(IMediaSample * pIn, IMediaSample *pOut, int id)
{
	if (id == 1)
		return S_FALSE;

	int len_in = pIn->GetActualDataLength();
	int len_out = pOut->GetActualDataLength();
	int stride_i = len_in *2/3/m_in_y;
	int stride_o = len_out *2/3/m_out_y;
	BYTE *ptr_in = NULL;
	pIn->GetPointer(&ptr_in);
	BYTE *ptr_out = NULL;
	pOut->GetPointer(&ptr_out);

	for(int i=0; i<m_out_y; i++)
	{
		memcpy(ptr_out, ptr_in, stride_i);
		ptr_in += stride_i;
		ptr_out += stride_o;
	}

	stride_i /= 2;
	stride_o /= 2;
	for(int i=0; i<m_out_y; i++)
	{
		memcpy(ptr_out, ptr_in, stride_i);
		ptr_in += stride_i;
		ptr_out += stride_o;
	}

	return S_OK;
}
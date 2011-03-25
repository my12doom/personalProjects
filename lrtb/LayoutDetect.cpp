//#include <tchar.h>
//#include <Windows.h>
#include "LayoutDetect.h"
#ifndef CLSID_NullRenderer
#include <InitGuid.h>
// {{C1F400A4-3F08-11D3-9F0B-006008039E37}}
DEFINE_GUID(CLSID_NullRenderer, 
			0xC1F400A4, 0X3F08, 0x11D3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37);
#endif


layout_detector::layout_detector(int frames_to_scan, const wchar_t *file):
m_scaned(0),
m_frames_to_scan(frames_to_scan),
sbs_result(new double[frames_to_scan]),
tb_result(new double[frames_to_scan])
{
	CoInitialize(NULL);
	// SSIM block init
	img12_sum_block = (short*)_aligned_malloc(544 * 1920 * 2, 16);

	//these circular buffer will contain the total sum along the corresponding column
	//in the original images and per pixel functions thereoff needed for the simularity measure
	//one line will also be used as a temporary for calculating the sum for the last 8 lines
	img1_sum      = (short *)_aligned_malloc(9 * 3840 * 2, 16);
	img2_sum      = (short *)_aligned_malloc(9 * 3840 * 2, 16);
	img1_sq_sum   = (int *)_aligned_malloc(9 * 3840 * 4, 16);
	img2_sq_sum   = (int *)_aligned_malloc(9 * 3840 * 4, 16);
	img12_mul_sum = (int *)_aligned_malloc(9 * 3840 * 4, 16);

	HRESULT hr;
	CoCreateInstance(CLSID_FilterGraph , NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void **)&m_gb);
	//m_gb.CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC);
	// ROT
	/*
	IMoniker *pMoniker;
	IRunningObjectTable *pROT;
	GetRunningObjectTable(0, &pROT);

	WCHAR wsz[128];
	wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)(IUnknown*)m_gb, GetCurrentProcessId());

	hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if (SUCCEEDED(hr))
	{
		hr = pROT->Register(0, m_gb, pMoniker, &m_ROT);
		pMoniker->Release();
	}
	pROT->Release();
	*/

	m_gb->RenderFile(file, NULL);

	// replace any renderer to NULL renderer
	CComPtr<IEnumFilters> ef;
	CComPtr<IBaseFilter> filter;
	m_gb->EnumFilters(&ef);
	while(ef->Next(1, &filter, NULL) == S_OK)
	{
		FILTER_INFO fi;
		filter->QueryFilterInfo(&fi);
		if (fi.pGraph) fi.pGraph->Release();

		if (wcsstr(fi.achName, L"Video Renderer"))
		{
			// find connected pin and reconnect it to NULL Renderer
			CComPtr<IPin> pin;
			CComPtr<IPin> pinc;
			CComPtr<IEnumPins> ep;
			filter->EnumPins(&ep);
			while (ep->Next(1, &pin, NULL) == S_OK)
			{
				if (pin->ConnectedTo(&pinc) == S_OK)
					break;

				pin = NULL;
			}
			m_gb->RemoveFilter(filter);

			yv12 = new CYV12Dump(NULL, &hr);
			yv12->SetCallback(this);
			CComPtr<IBaseFilter> yv12_base;
			yv12->QueryInterface(IID_IBaseFilter, (void**)&yv12_base);
			m_gb->AddFilter(yv12_base, L"YV12 Dump");
			//yv12->Release();

			pin = NULL;
			yv12_base->FindPin(L"Input", &pin);
			m_gb->ConnectDirect(pinc, pin, NULL);

			break;
		}

		filter = NULL;
	}
reset:
	filter = NULL;
	ef = NULL;
	m_gb->EnumFilters(&ef);
	while(ef->Next(1, &filter, NULL) == S_OK)
	{
		FILTER_INFO fi;
		filter->QueryFilterInfo(&fi);
		if (fi.pGraph) fi.pGraph->Release();

		if (wcsstr(fi.achName, L"Renderer") || wcsstr(fi.achName, L"DirectSound"))
		{
			// find connected pin and reconnect it to NULL Renderer
			CComPtr<IPin> pin;
			CComPtr<IPin> pinc;

			CComPtr<IEnumPins> ep;
			filter->EnumPins(&ep);
			while (ep->Next(1, &pin, NULL) == S_OK)
			{
				if (pin->ConnectedTo(&pinc) == S_OK)
					break;

				pin = NULL;
			}

			m_gb->RemoveFilter(filter);

			CComPtr<IBaseFilter> null;
			null.CoCreateInstance(CLSID_NullRenderer);
			m_gb->AddFilter(null, L"Null");
			pin = NULL;
			null->FindPin(L"In", &pin);
			m_gb->ConnectDirect(pinc, pin, NULL);

			goto reset;
		}

		filter = NULL;
	}

	// set max speed	
	CComQIPtr<IMediaFilter, &IID_IMediaFilter> mf(m_gb);
	mf->SetSyncSource(NULL);

	// go to center of movie
	CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> ms(m_gb);
	REFERENCE_TIME total;
	ms->GetDuration(&total);
	total /= 3;
	ms->SetPositions(&total, AM_SEEKING_AbsolutePositioning, NULL, NULL);

	// run
	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(m_gb);
	mc->Run();
}

layout_detector::~layout_detector()
{
	yv12->SetCallback(NULL);
	m_gb = NULL;
	delete [] sbs_result;
	delete [] tb_result;
	_aligned_free(img12_sum_block);
	_aligned_free(img1_sum);
	_aligned_free(img2_sum);
	_aligned_free(img1_sq_sum);
	_aligned_free(img2_sq_sum);
	_aligned_free(img12_mul_sum);
}

HRESULT layout_detector::SampleCB(int width, int height, IMediaSample *sample)
{
	if(m_scaned >= m_frames_to_scan)
	{
		return S_OK;
	}

	BYTE *p = NULL;
	sample->GetPointer(&p);
	int datasize = sample->GetActualDataLength();
	int stride = datasize / height * 2 / 3;

	BYTE *bottom = p + stride * height/2;
	BYTE *right = p + width/2;

	sbs_result[m_scaned] = image_quality(p, right, stride, stride, width/2, height, true, NULL);
	tb_result[m_scaned++] = image_quality(p, bottom, stride, stride, width, height/2, true, NULL);
	return S_OK;
}

// SSIM stuff
// copied from SSIM avisynth plugin 0.24
#define C1 (double)(64 * 64 * 0.01*255*0.01*255)
#define C2 (double)(64 * 64 * 0.03*255*0.03*255)
inline double layout_detector::similarity(int muX, int muY, int preMuX2, int preMuY2, int preMuXY2) 
{
	int muX2, muY2, muXY, thetaX2, thetaY2, thetaXY;

	muX2 = muX*muX;
	muY2 = muY*muY;
	muXY = muX*muY;

	thetaX2 = 64 * preMuX2 - muX2;
	thetaY2 = 64 * preMuY2 - muY2;
	thetaXY = 64 * preMuXY2 - muXY;

	return (2 * muXY + C1) * (2 * thetaXY + C2) / ((muX2 + muY2 + C1) * (thetaX2 + thetaY2 + C2));
}


double layout_detector::image_quality(const unsigned char *img1, const unsigned char *img2, int stride_img1, int stride_img2, int width, int height, bool luminance, double *oweight)
{
	int widthUV = width/2;
	int stride = stride_img1;
	bool lumimask = true;
	double planeSummedWeights = 0;

	int x, y, x2, y2, img1_block, img2_block, img1_sq_block, img2_sq_block, img12_mul_block, temp;

	double planeQuality, weight, mean;

	short *img1_sum_ptr1, *img1_sum_ptr2;
	short *img2_sum_ptr1, *img2_sum_ptr2;
	int *img1_sq_sum_ptr1, *img1_sq_sum_ptr2;
	int *img2_sq_sum_ptr1, *img2_sq_sum_ptr2;
	int *img12_mul_sum_ptr1, *img12_mul_sum_ptr2;

	planeQuality = 0;
	if(lumimask)
		planeSummedWeights = 0.0f;
	else
		planeSummedWeights = (height - 7) * (width - 7);

	//some prologue for the main loop
	temp = 8 * stride;

	img1_sum_ptr1      = img1_sum + temp;
	img2_sum_ptr1      = img2_sum + temp;
	img1_sq_sum_ptr1   = img1_sq_sum + temp;
	img2_sq_sum_ptr1   = img2_sq_sum + temp;
	img12_mul_sum_ptr1 = img12_mul_sum + temp;

	for (x = 0; x < width; x++) {
		img1_sum[x]      = img1[x];
		img2_sum[x]      = img2[x];
		img1_sq_sum[x]   = img1[x] * img1[x];
		img2_sq_sum[x]   = img2[x] * img2[x];
		img12_mul_sum[x] = img1[x] * img2[x];

		img1_sum_ptr1[x]      = 0;
		img2_sum_ptr1[x]      = 0;
		img1_sq_sum_ptr1[x]   = 0;
		img2_sq_sum_ptr1[x]   = 0;
		img12_mul_sum_ptr1[x] = 0;
	}

	//the main loop
	for (y = 1; y < height; y++) {
		img1 += stride_img1;
		img2 += stride_img2;

		temp = (y - 1) % 9 * stride;

		img1_sum_ptr1      = img1_sum + temp;
		img2_sum_ptr1      = img2_sum + temp;
		img1_sq_sum_ptr1   = img1_sq_sum + temp;
		img2_sq_sum_ptr1   = img2_sq_sum + temp;
		img12_mul_sum_ptr1 = img12_mul_sum + temp;

		temp = y % 9 * stride;

		img1_sum_ptr2      = img1_sum + temp;
		img2_sum_ptr2      = img2_sum + temp;
		img1_sq_sum_ptr2   = img1_sq_sum + temp;
		img2_sq_sum_ptr2   = img2_sq_sum + temp;
		img12_mul_sum_ptr2 = img12_mul_sum + temp;

		for (x = 0; x < width; x++) {
			img1_sum_ptr2[x]      = img1_sum_ptr1[x] + img1[x];
			img2_sum_ptr2[x]      = img2_sum_ptr1[x] + img2[x];
			img1_sq_sum_ptr2[x]   = img1_sq_sum_ptr1[x] + img1[x] * img1[x];
			img2_sq_sum_ptr2[x]   = img2_sq_sum_ptr1[x] + img2[x] * img2[x];
			img12_mul_sum_ptr2[x] = img12_mul_sum_ptr1[x] + img1[x] * img2[x];
		}

		if (y > 6) {
			//calculate the sum of the last 8 lines by subtracting the total sum of 8 lines back from the present sum
			temp = (y + 1) % 9 * stride;

			img1_sum_ptr1      = img1_sum + temp;
			img2_sum_ptr1      = img2_sum + temp;
			img1_sq_sum_ptr1   = img1_sq_sum + temp;
			img2_sq_sum_ptr1   = img2_sq_sum + temp;
			img12_mul_sum_ptr1 = img12_mul_sum + temp;

			for (x = 0; x < width; x++) {
				img1_sum_ptr1[x]      = img1_sum_ptr2[x] - img1_sum_ptr1[x];
				img2_sum_ptr1[x]      = img2_sum_ptr2[x] - img2_sum_ptr1[x];
				img1_sq_sum_ptr1[x]   = img1_sq_sum_ptr2[x] - img1_sq_sum_ptr1[x];
				img2_sq_sum_ptr1[x]   = img2_sq_sum_ptr2[x] - img2_sq_sum_ptr1[x];
				img12_mul_sum_ptr1[x] = img12_mul_sum_ptr2[x] - img12_mul_sum_ptr1[x];
			}

			//here we calculate the sum over the 8x8 block of pixels
			//this is done by sliding a window across the column sums for the last 8 lines
			//each time adding the new column sum, and subtracting the one which fell out of the window
			img1_block      = 0;
			img2_block      = 0;
			img1_sq_block   = 0;
			img2_sq_block   = 0;
			img12_mul_block = 0;

			//prologue, and calculation of simularity measure from the first 8 column sums
			for (x = 0; x < 8; x++) {
				img1_block      += img1_sum_ptr1[x];
				img2_block      += img2_sum_ptr1[x];
				img1_sq_block   += img1_sq_sum_ptr1[x];
				img2_sq_block   += img2_sq_sum_ptr1[x];
				img12_mul_block += img12_mul_sum_ptr1[x];
			}

			if(lumimask)
			{
				y2 = y - 7;
				x2 = 0;

				if (luminance)
				{
					mean = (img2_block + img1_block) / 128.0f;

					if (!(y2%2 || x2%2))
						*(img12_sum_block + y2/2 * widthUV + x2/2) = img2_block + img1_block;
				}
				else
				{
					mean = *(img12_sum_block + y2 * widthUV + x2);
					mean += *(img12_sum_block + y2 * widthUV + x2 + 4);
					mean += *(img12_sum_block + (y2 + 4) * widthUV + x2);
					mean += *(img12_sum_block + (y2 + 4) * widthUV + x2 + 4);

					mean /= 512.0f;
				}

				weight = mean < 40 ? 0.0f :
					(mean < 50 ? (mean - 40.0f) / 10.0f : 1.0f);
				planeSummedWeights += weight;

				planeQuality += weight * similarity(img1_block, img2_block, img1_sq_block, img2_sq_block, img12_mul_block);
			}
			else
				planeQuality += similarity(img1_block, img2_block, img1_sq_block, img2_sq_block, img12_mul_block);

			//and for the rest
			for (x = 8; x < width; x++) {
				img1_block      = img1_block + img1_sum_ptr1[x] - img1_sum_ptr1[x - 8];
				img2_block      = img2_block + img2_sum_ptr1[x] - img2_sum_ptr1[x - 8];
				img1_sq_block   = img1_sq_block + img1_sq_sum_ptr1[x] - img1_sq_sum_ptr1[x - 8];
				img2_sq_block   = img2_sq_block + img2_sq_sum_ptr1[x] - img2_sq_sum_ptr1[x - 8];
				img12_mul_block = img12_mul_block + img12_mul_sum_ptr1[x] - img12_mul_sum_ptr1[x - 8];

				if(lumimask)
				{
					y2 = y - 7;
					x2 = x - 7;

					if (luminance)
					{
						mean = (img2_block + img1_block) / 128.0f;

						if (!(y2%2 || x2%2))
							*(img12_sum_block + y2/2 * widthUV + x2/2) = img2_block + img1_block;
					}
					else
					{
						mean = *(img12_sum_block + y2 * widthUV + x2);
						mean += *(img12_sum_block + y2 * widthUV + x2 + 4);
						mean += *(img12_sum_block + (y2 + 4) * widthUV + x2);
						mean += *(img12_sum_block + (y2 + 4) * widthUV + x2 + 4);

						mean /= 512.0f;
					}

					weight = mean < 40 ? 0.0f :
						(mean < 50 ? (mean - 40.0f) / 10.0f : 1.0f);
					planeSummedWeights += weight;

					planeQuality += weight * similarity(img1_block, img2_block, img1_sq_block, img2_sq_block, img12_mul_block);
				}
				else
					planeQuality += similarity(img1_block, img2_block, img1_sq_block, img2_sq_block, img12_mul_block);
			}
		}
	}

	if (oweight && luminance)
		*oweight = planeSummedWeights / ((width - 7) * (height - 7));
	else if (oweight && !luminance)
		*oweight = planeSummedWeights / ((width*2 - 7) * (height*2 - 7));

	if (planeSummedWeights == 0)
		return 1.0f;
	else
		return planeQuality / planeSummedWeights;
}
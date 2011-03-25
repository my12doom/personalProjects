#include <atlbase.h>
#include <DShow.h>

#include "YV12Dump.h"

class layout_detector : protected IYV12CB
{
public:
	layout_detector(int frames_to_scan, const wchar_t *file);
	~layout_detector();

	double get_result();		// return value is something like x.yyy, x=0 means sbs, x=1 means top-bottom, .yyy = possibility

	int m_scaned;
	double *sbs_result;
	double *tb_result;

protected:
	CYV12Dump *yv12;
	DWORD m_ROT;
	HRESULT SampleCB(int width, int height, IMediaSample *sample);
	CComPtr<IGraphBuilder> m_gb;
	int m_frames_to_scan;

	// SSIM stuff
	short *img12_sum_block;
	short* img1_sum;
	short* img2_sum;
	int* img1_sq_sum;
	int* img2_sq_sum;
	int* img12_mul_sum;
	double image_quality(const unsigned char *img1, const unsigned char *img2, int stride_img1, int stride_img2, int width, int height, bool isY, double *oweight = NULL);
	double similarity(int img1_block, int img2_block, int img1_sq_block, int img2_sq_block, int img12_mul_block);

};
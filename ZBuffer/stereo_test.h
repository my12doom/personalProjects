#pragma once
#include <Windows.h>
#include <math.h>

#ifndef def_input_layout_types
#define def_input_layout_types
enum input_layout_types
{
	side_by_side, top_bottom, mono2d, input_layout_types_max, input_layout_auto
};
#endif

template<typename pixel_type>
HRESULT get_layout(void *src, int width, int height, int *out, int stride = -1)
// stride is also in pixel, like 1280*720 video in VideoCard usually is 1536*720, stride = 1536
{
	if (!out || !src)
		return E_POINTER;

	if (-1 == stride)
		stride = width;

	pixel_type *data = (pixel_type*) ( ((BYTE*)src) + (sizeof(pixel_type) == 4 ? 2 : 0));

	unsigned int avg1 = 0;
	unsigned int avg2 = 0;
	unsigned int var1 = 0;		// variance
	unsigned int var2 = 0;

	int width2 = width / 2;
	int height2 = height / 2;

	const int test_size = 64;
	const int range = (int)(width*0.03);
	const int range2 = (int)(width2*0.03);
	const int range_step = max(1, range/15);
	const int range_step2 = max(1, range2/15);

	// SBS
	for(int y=0; y<test_size; y++)
	for(int x=0; x<test_size; x+=2)
	{
		int sx = x*width2/test_size;
		int sy = y*height/test_size;
		int min_delta = 255;
		int r1 = data[sy*stride+sx] & 0xff;
		int m = min(width, width2+sx+range2);
		for(int i=max(width2, width2+sx-range2); i<=m; i+=range_step2)
		{
			int r2 = data[sy*stride+i] & 0xff;
			int t = abs(r1-r2);
			min_delta = min(min_delta, t);
		}

		avg1 += min_delta*2;
		var1 += min_delta * min_delta*2;
	}

	// TB
	pixel_type * data2 = data + stride * height2;
	for(int y=0; y<test_size; y++)
	for(int x=0; x<test_size; x+=2)
	{
		int sx = x*width/test_size;
		int sy = y*height2/test_size;
		int min_delta = 255;
		int r1 = data[sy*stride+sx] & 0xff;
		int m = min(width, sx+range);
		for(int i=max(0, sx-range); i<=m; i+=range_step)
		{
			int r2 = data2[sy*stride+i] & 0xff;
			int t = abs(r1-r2);
			min_delta = min(min_delta, t);
		}

		avg2 += min_delta*2;
		var2 += min_delta * min_delta*2;
	}

	double d_avg1 = (double)avg1 / (test_size * test_size);
	double d_var1 = (double)var1 / (test_size * test_size);
	d_var1 -= d_avg1*d_avg1;
	d_var1 = sqrt(d_var1);

	double d_avg2 = (double)avg2 / (test_size * test_size);
	double d_var2 = (double)var2 / (test_size * test_size);
	d_var2 -= d_avg2*d_avg2;
	d_var2 = sqrt(d_var2);

	double cal1 = d_avg1 * d_var1;
	double cal2 = d_avg2 * d_var2;

	double times = 0;
	if ( (cal1 > 0.001 && cal2 > 0.001) || (cal1>cal2*10000) || (cal2>cal1*10000))
		times = cal1 > cal2 ? cal1 / cal2 : cal2 / cal1;

// 	printf("sbs:%f - %f - %f\r\n", d_avg1, d_var1, cal1);
// 	printf("tb: %f - %f - %f\r\n", d_avg2, d_var2, cal2);
 	printf("times: %f.\r\n", times);


	if (times > 31.62/2)		// 10^1.5
	{
// 		printf("stereo(%s).\r\n", cal1 > cal2 ? "tb" : "sbs");
		*out = cal1 > cal2 ? top_bottom : side_by_side;
	}
	else if ( 1.0 < times && times < 4.68 )
	{
// 		printf("normal.\r\n");
		*out = mono2d;
	}
	else
	{
// 		printf("unkown.\r\n");
		return S_FALSE;
	}

	return S_OK;
}

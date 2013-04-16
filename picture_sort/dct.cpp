/*
see http://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II
*/


#include "stdafx.h"
#include <math.h>
#include "dct.h"

#define PI 3.1415926535898

double dct_tbl[512][512];
double idct_tbl[512][512];

int dct_init(int size)
{
	for(int k=0; k<size; k++)
	{
		for(int n=0; n<size; n++)
		{
			dct_tbl[k][n]  = cos(PI/size*(n+0.5f) * k);
			idct_tbl[k][n] = cos(PI/size*n*(k+0.5f));
			idct_tbl[k][0] = 0.5;
		}
	}

	return 0;
}

int dct(double *src, double *dst, int size)
{
	if (!src || !dst)
		return -1;

	for(int k=0; k<size; k++)
	{
		double o = 0;

		for(int n=0; n<size; n++)
		{
			o += src[n] * dct_tbl[k][n];
		}

		dst[k] = o;
	}

	return 0;
}

int idct(double *src, double *dst, int size)
{
	if (!src || !dst)
		return -1;

	for(int k=0; k<size; k++)
	{
		double o = 0;

		for(int n=0; n<size; n++)
		{
			o += src[n] * idct_tbl[k][n];
		}

		dst[k] = o*2.0/size;
	}

	return 0;
}

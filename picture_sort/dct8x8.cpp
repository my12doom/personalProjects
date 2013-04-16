#include "stdafx.h"
#include "dct8x8.h"
#include <emmintrin.h>
#define BLOCK_SIZE_8x8 8

#define  _MM_TRANSPOSE8I(a, b, c, d, e, f, g, h){							\
	__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;					\
	__m128i t0, t1, t2, t3, t4, t5, t6, t7;									\
																			\
	tmp0 = _mm_unpacklo_epi16((a), (b));									\
	tmp1 = _mm_unpacklo_epi16((c), (d));									\
	tmp2 = _mm_unpacklo_epi16((e), (f));									\
	tmp3 = _mm_unpacklo_epi16((g), (h));									\
	tmp4 = _mm_unpackhi_epi16((a), (b));									\
	tmp5 = _mm_unpackhi_epi16((c), (d));									\
	tmp6 = _mm_unpackhi_epi16((e), (f));									\
	tmp7 = _mm_unpackhi_epi16((g), (h));									\
																			\
	t0 = _mm_unpacklo_epi32(tmp0, tmp1);									\
	t1 = _mm_unpackhi_epi32(tmp0, tmp1);									\
	t2 = _mm_unpacklo_epi32(tmp2, tmp3);									\
	t3 = _mm_unpackhi_epi32(tmp2, tmp3);									\
	t4 = _mm_unpacklo_epi32(tmp4, tmp5);									\
	t5 = _mm_unpackhi_epi32(tmp4, tmp5);									\
	t6 = _mm_unpacklo_epi32(tmp6, tmp7);									\
	t7 = _mm_unpackhi_epi32(tmp6, tmp7);									\
																			\
	a = _mm_unpacklo_epi64(t0, t2);											\
	b = _mm_unpackhi_epi64(t0, t2);											\
	c = _mm_unpacklo_epi64(t1, t3);											\
	d = _mm_unpackhi_epi64(t1, t3);											\
	e = _mm_unpacklo_epi64(t4, t6);											\
	f = _mm_unpackhi_epi64(t4, t6);											\
	g = _mm_unpacklo_epi64(t5, t7);											\
	h = _mm_unpackhi_epi64(t5, t7);											\
}

void forward8x8(int **block, int **tblock, int pos_y, int pos_x)
{
	int i, ii;  
	int tmp[64];
	int *pTmp = tmp, *pblock;
	int a0, a1, a2, a3;
	int p0, p1, p2, p3, p4, p5 ,p6, p7;
	int b0, b1, b2, b3, b4, b5, b6, b7;

	// Horizontal
	for (i=pos_y; i < pos_y + BLOCK_SIZE_8x8; i++)
	{
		pblock = &block[i][pos_x];
		p0 = *(pblock++);
		p1 = *(pblock++);
		p2 = *(pblock++);
		p3 = *(pblock++);
		p4 = *(pblock++);
		p5 = *(pblock++);
		p6 = *(pblock++);
		p7 = *(pblock  );

		a0 = p0 + p7;
		a1 = p1 + p6;
		a2 = p2 + p5;
		a3 = p3 + p4;

		b0 = a0 + a3;
		b1 = a1 + a2;
		b2 = a0 - a3;
		b3 = a1 - a2;

		a0 = p0 - p7;
		a1 = p1 - p6;
		a2 = p2 - p5;
		a3 = p3 - p4;

		b4 = a1 + a2 + ((a0 >> 1) + a0);
		b5 = a0 - a3 - ((a2 >> 1) + a2);
		b6 = a0 + a3 - ((a1 >> 1) + a1);
		b7 = a1 - a2 + ((a3 >> 1) + a3);

		*(pTmp++) =  b0 + b1;
		*(pTmp++) =  b4 + (b7 >> 2);
		*(pTmp++) =  b2 + (b3 >> 1);
		*(pTmp++) =  b5 + (b6 >> 2);
		*(pTmp++) =  b0 - b1;
		*(pTmp++) =  b6 - (b5 >> 2);
		*(pTmp++) = (b2 >> 1) - b3;                 
		*(pTmp++) = (b4 >> 2) - b7;
	}

	// Vertical 
	for (i=0; i < BLOCK_SIZE_8x8; i++)
	{
		pTmp = tmp + i;
		p0 = *pTmp;
		p1 = *(pTmp += BLOCK_SIZE_8x8);
		p2 = *(pTmp += BLOCK_SIZE_8x8);
		p3 = *(pTmp += BLOCK_SIZE_8x8);
		p4 = *(pTmp += BLOCK_SIZE_8x8);
		p5 = *(pTmp += BLOCK_SIZE_8x8);
		p6 = *(pTmp += BLOCK_SIZE_8x8);
		p7 = *(pTmp += BLOCK_SIZE_8x8);

		a0 = p0 + p7;
		a1 = p1 + p6;
		a2 = p2 + p5;
		a3 = p3 + p4;

		b0 = a0 + a3;
		b1 = a1 + a2;
		b2 = a0 - a3;
		b3 = a1 - a2;

		a0 = p0 - p7;
		a1 = p1 - p6;
		a2 = p2 - p5;
		a3 = p3 - p4;

		b4 = a1 + a2 + ((a0 >> 1) + a0);
		b5 = a0 - a3 - ((a2 >> 1) + a2);
		b6 = a0 + a3 - ((a1 >> 1) + a1);
		b7 = a1 - a2 + ((a3 >> 1) + a3);

		ii = pos_x + i;
		tblock[pos_y    ][ii] =  b0 + b1;
		tblock[pos_y + 1][ii] =  b4 + (b7 >> 2);
		tblock[pos_y + 2][ii] =  b2 + (b3 >> 1);
		tblock[pos_y + 3][ii] =  b5 + (b6 >> 2);
		tblock[pos_y + 4][ii] =  b0 - b1;
		tblock[pos_y + 5][ii] =  b6 - (b5 >> 2);
		tblock[pos_y + 6][ii] = (b2 >> 1) - b3;
		tblock[pos_y + 7][ii] = (b4 >> 2) - b7;
	}
}

void inverse8x8(int **tblock, int **block, int pos_x)
{
#if 0
	int i, ii;
	int tmp[64];
	int *pTmp = tmp, *pblock;
	int a0, a1, a2, a3;
	int p0, p1, p2, p3, p4, p5 ,p6, p7;  
	int b0, b1, b2, b3, b4, b5, b6, b7;

	// Horizontal  
	for (i=0; i < BLOCK_SIZE_8x8; i++)
	{
		pblock = &tblock[i][pos_x];
		p0 = *(pblock++);
		p1 = *(pblock++);
		p2 = *(pblock++);
		p3 = *(pblock++);
		p4 = *(pblock++);
		p5 = *(pblock++);
		p6 = *(pblock++);
		p7 = *(pblock  );

		a0 = p0 + p4;
		a1 = p0 - p4;
		a2 = p6 - (p2 >> 1);
		a3 = p2 + (p6 >> 1);

		b0 =  a0 + a3;
		b2 =  a1 - a2;
		b4 =  a1 + a2;
		b6 =  a0 - a3;

		a0 = -p3 + p5 - p7 - (p7 >> 1);    
		a1 =  p1 + p7 - p3 - (p3 >> 1);    
		a2 = -p1 + p7 + p5 + (p5 >> 1);    
		a3 =  p3 + p5 + p1 + (p1 >> 1);


		b1 =  a0 + (a3>>2);    
		b3 =  a1 + (a2>>2);    
		b5 =  a2 - (a1>>2);
		b7 =  a3 - (a0>>2);                

		*(pTmp++) = b0 + b7;
		*(pTmp++) = b2 - b5;
		*(pTmp++) = b4 + b3;
		*(pTmp++) = b6 + b1;
		*(pTmp++) = b6 - b1;
		*(pTmp++) = b4 - b3;
		*(pTmp++) = b2 + b5;
		*(pTmp++) = b0 - b7;
	}

	//  Vertical 
	for (i=0; i < BLOCK_SIZE_8x8; i++)
	{
		pTmp = tmp + i;
		p0 = *pTmp;
		p1 = *(pTmp += BLOCK_SIZE_8x8);
		p2 = *(pTmp += BLOCK_SIZE_8x8);
		p3 = *(pTmp += BLOCK_SIZE_8x8);
		p4 = *(pTmp += BLOCK_SIZE_8x8);
		p5 = *(pTmp += BLOCK_SIZE_8x8);
		p6 = *(pTmp += BLOCK_SIZE_8x8);
		p7 = *(pTmp += BLOCK_SIZE_8x8);

		a0 =  p0 + p4;
		a1 =  p0 - p4;
		a2 =  p6 - (p2>>1);
		a3 =  p2 + (p6>>1);

		b0 = a0 + a3;
		b2 = a1 - a2;
		b4 = a1 + a2;
		b6 = a0 - a3;

		a0 = -p3 + p5 - p7 - (p7 >> 1);
		a1 =  p1 + p7 - p3 - (p3 >> 1);
		a2 = -p1 + p7 + p5 + (p5 >> 1);
		a3 =  p3 + p5 + p1 + (p1 >> 1);


		b1 =  a0 + (a3 >> 2);
		b7 =  a3 - (a0 >> 2);
		b3 =  a1 + (a2 >> 2);
		b5 =  a2 - (a1 >> 2);

		ii = i + pos_x;
		block[0][ii] = b0 + b7;
		block[1][ii] = b2 - b5;
		block[2][ii] = b4 + b3;
		block[3][ii] = b6 + b1;
		block[4][ii] = b6 - b1;
		block[5][ii] = b4 - b3;
		block[6][ii] = b2 + b5;
		block[7][ii] = b0 - b7;
	}
#endif

#if 1
	{
		__m128i p0, p1, p2, p3, p4, p5, p6, p7;
		__m128i a0, a1, a2, a3, b0, b1, b2, b3, b4, b5, b6, b7;
		// load
		p0 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[0][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[0][pos_x+4])));
		p1 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[1][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[1][pos_x+4])));
		p2 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[2][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[2][pos_x+4])));
		p3 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[3][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[3][pos_x+4])));
		p4 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[4][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[4][pos_x+4])));
		p5 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[5][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[5][pos_x+4])));
		p6 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[6][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[6][pos_x+4])));
		p7 = _mm_packs_epi32(_mm_loadu_si128((__m128i*)(&tblock[7][pos_x])), _mm_loadu_si128((__m128i*)(&tblock[7][pos_x+4])));

		_MM_TRANSPOSE8I(p0, p1, p2, p3, p4, p5, p6, p7);

		// horizontal
		a0 = _mm_add_epi16(p0, p4);
		a1 = _mm_sub_epi16(p0, p4);
		a2 = _mm_srai_epi16(p2, 1);
		a2 = _mm_sub_epi16(p6, a2);
		a3 = _mm_srai_epi16(p6, 1);
		a3 = _mm_add_epi16(a3, p2);

		b0 = _mm_add_epi16(a0, a3);
		b2 = _mm_sub_epi16(a1, a2);
		b4 = _mm_add_epi16(a1, a2);
		b6 = _mm_sub_epi16(a0, a3);

		p0 = _mm_srai_epi16(p7, 1);
		a0 = _mm_sub_epi16(p5, p7);
		a0 = _mm_sub_epi16(a0, p3);
		a0 = _mm_sub_epi16(a0, p0);
		p0 = _mm_srai_epi16(p3, 1);
		a1 = _mm_add_epi16(p1, p7);
		a1 = _mm_sub_epi16(a1, p3);
		a1 = _mm_sub_epi16(a1, p0);
		p0 = _mm_srai_epi16(p5, 1);
		a2 = _mm_sub_epi16(p7, p1);
		a2 = _mm_add_epi16(a2, p5);
		a2 = _mm_add_epi16(a2, p0);
		p0 = _mm_srai_epi16(p1, 1);
		a3 = _mm_add_epi16(p3, p5);
		a3 = _mm_add_epi16(a3, p1);
		a3 = _mm_add_epi16(a3, p0);

		b1 = _mm_srai_epi16(a3, 2);
		b7 = _mm_srai_epi16(a0, 2);
		b3 = _mm_srai_epi16(a2, 2);
		b5 = _mm_srai_epi16(a1, 2);

		b1 = _mm_add_epi16(b1, a0);
		b7 = _mm_sub_epi16(a3, b7);
		b3 = _mm_add_epi16(b3, a1);
		b5 = _mm_sub_epi16(a2, b5);

		p0 = _mm_add_epi16(b0, b7);
		p1 = _mm_sub_epi16(b2, b5);
		p2 = _mm_add_epi16(b4, b3);
		p3 = _mm_add_epi16(b6, b1);
		p4 = _mm_sub_epi16(b6, b1);
		p5 = _mm_sub_epi16(b4, b3);
		p6 = _mm_add_epi16(b2, b5);
		p7 = _mm_sub_epi16(b0, b7);

		_MM_TRANSPOSE8I(p0, p1, p2, p3, p4, p5, p6, p7);

		// vertical
		a0 = _mm_add_epi16(p0, p4);
		a1 = _mm_sub_epi16(p0, p4);
		a2 = _mm_srai_epi16(p2, 1);
		a2 = _mm_sub_epi16(p6, a2);
		a3 = _mm_srai_epi16(p6, 1);
		a3 = _mm_add_epi16(a3, p2);

		b0 = _mm_add_epi16(a0, a3);
		b2 = _mm_sub_epi16(a1, a2);
		b4 = _mm_add_epi16(a1, a2);
		b6 = _mm_sub_epi16(a0, a3);

		p0 = _mm_srai_epi16(p7, 1);
		a0 = _mm_sub_epi16(p5, p7);
		a0 = _mm_sub_epi16(a0, p3);
		a0 = _mm_sub_epi16(a0, p0);
		p0 = _mm_srai_epi16(p3, 1);
		a1 = _mm_add_epi16(p1, p7);
		a1 = _mm_sub_epi16(a1, p3);
		a1 = _mm_sub_epi16(a1, p0);
		p0 = _mm_srai_epi16(p5, 1);
		a2 = _mm_sub_epi16(p7, p1);
		a2 = _mm_add_epi16(a2, p5);
		a2 = _mm_add_epi16(a2, p0);
		p0 = _mm_srai_epi16(p1, 1);
		a3 = _mm_add_epi16(p3, p5);
		a3 = _mm_add_epi16(a3, p1);
		a3 = _mm_add_epi16(a3, p0);

		b1 = _mm_srai_epi16(a3, 2);
		b7 = _mm_srai_epi16(a0, 2);
		b3 = _mm_srai_epi16(a2, 2);
		b5 = _mm_srai_epi16(a1, 2);

		b1 = _mm_add_epi16(b1, a0);
		b7 = _mm_sub_epi16(a3, b7);
		b3 = _mm_add_epi16(b3, a1);
		b5 = _mm_sub_epi16(a2, b5);

		p0 = _mm_add_epi16(b0, b7);
		p1 = _mm_sub_epi16(b2, b5);
		p2 = _mm_add_epi16(b4, b3);
		p3 = _mm_add_epi16(b6, b1);
		p4 = _mm_sub_epi16(b6, b1);
		p5 = _mm_sub_epi16(b4, b3);
		p6 = _mm_add_epi16(b2, b5);
		p7 = _mm_sub_epi16(b0, b7);



		// store
		a0 = _mm_xor_si128(a0, a0);
		b0 = _mm_cmpgt_epi16(a0, p0);
		b1 = _mm_cmpgt_epi16(a0, p1);
		b2 = _mm_cmpgt_epi16(a0, p2);
		b3 = _mm_cmpgt_epi16(a0, p3);
		b4 = _mm_cmpgt_epi16(a0, p4);
		b5 = _mm_cmpgt_epi16(a0, p5);
		b6 = _mm_cmpgt_epi16(a0, p6);
		b7 = _mm_cmpgt_epi16(a0, p7);
		_mm_storeu_si128((__m128i*)&block[0][pos_x], _mm_unpacklo_epi16(p0, b0));  _mm_storeu_si128((__m128i*)&block[0][pos_x+4], _mm_unpackhi_epi16(p0, b0));
		_mm_storeu_si128((__m128i*)&block[1][pos_x], _mm_unpacklo_epi16(p1, b1));  _mm_storeu_si128((__m128i*)&block[1][pos_x+4], _mm_unpackhi_epi16(p1, b1));
		_mm_storeu_si128((__m128i*)&block[2][pos_x], _mm_unpacklo_epi16(p2, b2));  _mm_storeu_si128((__m128i*)&block[2][pos_x+4], _mm_unpackhi_epi16(p2, b2));
		_mm_storeu_si128((__m128i*)&block[3][pos_x], _mm_unpacklo_epi16(p3, b3));  _mm_storeu_si128((__m128i*)&block[3][pos_x+4], _mm_unpackhi_epi16(p3, b3));
		_mm_storeu_si128((__m128i*)&block[4][pos_x], _mm_unpacklo_epi16(p4, b4));  _mm_storeu_si128((__m128i*)&block[4][pos_x+4], _mm_unpackhi_epi16(p4, b4));
		_mm_storeu_si128((__m128i*)&block[5][pos_x], _mm_unpacklo_epi16(p5, b5));  _mm_storeu_si128((__m128i*)&block[5][pos_x+4], _mm_unpackhi_epi16(p5, b5));
		_mm_storeu_si128((__m128i*)&block[6][pos_x], _mm_unpacklo_epi16(p6, b6));  _mm_storeu_si128((__m128i*)&block[6][pos_x+4], _mm_unpackhi_epi16(p6, b6));
		_mm_storeu_si128((__m128i*)&block[7][pos_x], _mm_unpacklo_epi16(p7, b7));  _mm_storeu_si128((__m128i*)&block[7][pos_x+4], _mm_unpackhi_epi16(p7, b7));


	}

#endif
}

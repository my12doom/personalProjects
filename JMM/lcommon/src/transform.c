/*!
 ***************************************************************************
 * \file transform.c
 *
 * \brief
 *    Transform functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Alexis Michael Tourapis
 * \date
 *    01. July 2007
 **************************************************************************
 */

#include "global.h"
#include "transform.h"
#include <emmintrin.h>


void forward4x4(int **block, int **tblock, int pos_y, int pos_x)
{
  int i, ii;  
  int tmp[16];
  int *pTmp = tmp, *pblock;
  int p0,p1,p2,p3;
  int t0,t1,t2,t3;

  // Horizontal
  for (i=pos_y; i < pos_y + BLOCK_SIZE; i++)
  {
    pblock = &block[i][pos_x];
    p0 = *(pblock++);
    p1 = *(pblock++);
    p2 = *(pblock++);
    p3 = *(pblock  );

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    *(pTmp++) =  t0 + t1;
    *(pTmp++) = (t3 << 1) + t2;
    *(pTmp++) =  t0 - t1;    
    *(pTmp++) =  t3 - (t2 << 1);
  }

  // Vertical 
  for (i=0; i < BLOCK_SIZE; i++)
  {
    pTmp = tmp + i;
    p0 = *pTmp;
    p1 = *(pTmp += BLOCK_SIZE);
    p2 = *(pTmp += BLOCK_SIZE);
    p3 = *(pTmp += BLOCK_SIZE);

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    ii = pos_x + i;
    tblock[pos_y    ][ii] = t0 +  t1;
    tblock[pos_y + 1][ii] = t2 + (t3 << 1);
    tblock[pos_y + 2][ii] = t0 -  t1;
    tblock[pos_y + 3][ii] = t3 - (t2 << 1);
  }
}

#define _MM_TRANSPOSE4_PS2(row0, row1, row2, row3) {                 \
	__m128 tmp3, tmp2, tmp1, tmp0;                          \
	\
	tmp0   = _mm_unpacklo_ps((row0), (row1));          \
	tmp1   = _mm_unpacklo_ps((row2), (row3));          \
	tmp2   = _mm_unpackhi_ps((row0), (row1));          \
	tmp3   = _mm_unpackhi_ps((row2), (row3));          \
	\
	(row0) = _mm_movelh_ps(tmp0, tmp1);              \
	(row1) = _mm_movehl_ps(tmp1, tmp0);              \
	(row2) = _mm_movelh_ps(tmp2, tmp3);              \
	(row3) = _mm_movehl_ps(tmp3, tmp2);              \
}


#define  _MM_TRANSPOSE4I(a, b, c, d){				\
	__m128i tmp0, tmp1, tmp2, tmp3;					\
													\
	tmp0 = _mm_unpacklo_epi32((a), (b));			\
	tmp1 = _mm_unpacklo_epi32((c), (d));			\
	tmp2 = _mm_unpackhi_epi32((a), (b));			\
	tmp3 = _mm_unpackhi_epi32((c), (d));			\
													\
	a = _mm_unpacklo_epi64(tmp0, tmp1);				\
	b = _mm_unpackhi_epi64(tmp0, tmp1);				\
	c = _mm_unpacklo_epi64(tmp2, tmp3);				\
	d = _mm_unpackhi_epi64(tmp2, tmp3);				\
}

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

void inverse4x4(int **tblock, int **block, int pos_y, int pos_x)
{

  // Horizontal
#if 0
  int i, ii;  
  int tmp[16];
  int *pTmp = tmp, *pblock;
  int p0,p1,p2,p3;
  int t0,t1,t2,t3;

  for (i = pos_y; i < pos_y + BLOCK_SIZE; i++)
  {
    pblock = &tblock[i][pos_x];
    t0 = *(pblock++);
    t1 = *(pblock++);
    t2 = *(pblock++);
    t3 = *(pblock  );

    p0 =  t0 + t2;
    p1 =  t0 - t2;
    p2 = (t1 >> 1) - t3;
    p3 =  t1 + (t3 >> 1);

    *(pTmp++) = p0 + p3;
    *(pTmp++) = p1 + p2;
    *(pTmp++) = p1 - p2;
    *(pTmp++) = p0 - p3;
  }

  //  Vertical 
  for (i = 0; i < BLOCK_SIZE; i++)
  {
    pTmp = tmp + i;
    t0 = *pTmp;
    t1 = *(pTmp += BLOCK_SIZE);
    t2 = *(pTmp += BLOCK_SIZE);
    t3 = *(pTmp += BLOCK_SIZE);

    p0 = t0 + t2;
    p1 = t0 - t2;
    p2 =(t1 >> 1) - t3;
    p3 = t1 + (t3 >> 1);

    ii = i + pos_x;
    block[pos_y    ][ii] = p0 + p3;
    block[pos_y + 1][ii] = p1 + p2;
    block[pos_y + 2][ii] = p1 - p2;
    block[pos_y + 3][ii] = p0 - p3;
  }
#else

  __m128i m0, m1, m2, m3, p0, p1, p2, p3;
  // load
  m0 = _mm_load_si128((__m128i*)&tblock[pos_y  ][pos_x]);
  m1 = _mm_load_si128((__m128i*)&tblock[pos_y+1][pos_x]);
  m2 = _mm_load_si128((__m128i*)&tblock[pos_y+2][pos_x]);
  m3 = _mm_load_si128((__m128i*)&tblock[pos_y+3][pos_x]);

  //_MM_TRANSPOSE4_PS2(*((__m128*)&m0), *((__m128*)&m1), *((__m128*)&m2), *((__m128*)&m3));
  _MM_TRANSPOSE4I(m0, m1, m2, m3);

  // horizontal
  p0 = _mm_add_epi32(m0, m2);
  p1 = _mm_sub_epi32(m0, m2);
  p2 = _mm_srai_epi32(m1, 1);
  p2 = _mm_sub_epi32(p2, m3);
  p3 = _mm_srai_epi32(m3, 1);
  p3 = _mm_add_epi32(p3, m1);

  m0 = _mm_add_epi32(p0, p3);
  m1 = _mm_add_epi32(p1, p2);
  m2 = _mm_sub_epi32(p1, p2);
  m3 = _mm_sub_epi32(p0, p3);

  _MM_TRANSPOSE4_PS2(*((__m128*)&m0), *((__m128*)&m1), *((__m128*)&m2), *((__m128*)&m3));

  // vertical
  p0 = _mm_add_epi32(m0, m2);
  p1 = _mm_sub_epi32(m0, m2);
  p2 = _mm_srai_epi32(m1, 1);
  p2 = _mm_sub_epi32(p2, m3);
  p3 = _mm_srai_epi32(m3, 1);
  p3 = _mm_add_epi32(p3, m1);

  m0 = _mm_add_epi32(p0, p3);
  m1 = _mm_add_epi32(p1, p2);
  m2 = _mm_sub_epi32(p1, p2);
  m3 = _mm_sub_epi32(p0, p3);

  // store
  _mm_store_si128((__m128i*)&block[pos_y    ][pos_x], m0);
  _mm_store_si128((__m128i*)&block[pos_y +1 ][pos_x], m1);
  _mm_store_si128((__m128i*)&block[pos_y +2 ][pos_x], m2);
  _mm_store_si128((__m128i*)&block[pos_y +3 ][pos_x], m3);


#endif

}


void hadamard4x4(int **block, int **tblock)
{
  int i;
  int tmp[16];
  int *pTmp = tmp, *pblock;
  int p0,p1,p2,p3;
  int t0,t1,t2,t3;

  // Horizontal
  for (i = 0; i < BLOCK_SIZE; i++)
  {
    pblock = block[i];
    p0 = *(pblock++);
    p1 = *(pblock++);
    p2 = *(pblock++);
    p3 = *(pblock  );

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    *(pTmp++) = t0 + t1;
    *(pTmp++) = t3 + t2;
    *(pTmp++) = t0 - t1;    
    *(pTmp++) = t3 - t2;
  }

  // Vertical 
  for (i = 0; i < BLOCK_SIZE; i++)
  {
    pTmp = tmp + i;
    p0 = *pTmp;
    p1 = *(pTmp += BLOCK_SIZE);
    p2 = *(pTmp += BLOCK_SIZE);
    p3 = *(pTmp += BLOCK_SIZE);

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    tblock[0][i] = (t0 + t1) >> 1;
    tblock[1][i] = (t2 + t3) >> 1;
    tblock[2][i] = (t0 - t1) >> 1;
    tblock[3][i] = (t3 - t2) >> 1;
  }
}


void ihadamard4x4(int **tblock, int **block)
{
  int i;  
  int tmp[16];
  int *pTmp = tmp, *pblock;
  int p0,p1,p2,p3;
  int t0,t1,t2,t3;

  // Horizontal
  for (i = 0; i < BLOCK_SIZE; i++)
  {
    pblock = tblock[i];
    t0 = *(pblock++);
    t1 = *(pblock++);
    t2 = *(pblock++);
    t3 = *(pblock  );

    p0 = t0 + t2;
    p1 = t0 - t2;
    p2 = t1 - t3;
    p3 = t1 + t3;

    *(pTmp++) = p0 + p3;
    *(pTmp++) = p1 + p2;
    *(pTmp++) = p1 - p2;
    *(pTmp++) = p0 - p3;
  }

  //  Vertical 
  for (i = 0; i < BLOCK_SIZE; i++)
  {
    pTmp = tmp + i;
    t0 = *pTmp;
    t1 = *(pTmp += BLOCK_SIZE);
    t2 = *(pTmp += BLOCK_SIZE);
    t3 = *(pTmp += BLOCK_SIZE);

    p0 = t0 + t2;
    p1 = t0 - t2;
    p2 = t1 - t3;
    p3 = t1 + t3;
    
    block[0][i] = p0 + p3;
    block[1][i] = p1 + p2;
    block[2][i] = p1 - p2;
    block[3][i] = p0 - p3;
  }
}

void hadamard4x2(int **block, int **tblock)
{
  int i;
  int tmp[8];
  int *pTmp = tmp;
  int p0,p1,p2,p3;
  int t0,t1,t2,t3;

  // Horizontal
  *(pTmp++) = block[0][0] + block[1][0];
  *(pTmp++) = block[0][1] + block[1][1];
  *(pTmp++) = block[0][2] + block[1][2];
  *(pTmp++) = block[0][3] + block[1][3];

  *(pTmp++) = block[0][0] - block[1][0];
  *(pTmp++) = block[0][1] - block[1][1];
  *(pTmp++) = block[0][2] - block[1][2];
  *(pTmp  ) = block[0][3] - block[1][3];

  // Vertical
  pTmp = tmp;
  for (i=0;i<2;i++)
  {      
    p0 = *(pTmp++);
    p1 = *(pTmp++);
    p2 = *(pTmp++);
    p3 = *(pTmp++);

    t0 = p0 + p3;
    t1 = p1 + p2;
    t2 = p1 - p2;
    t3 = p0 - p3;

    tblock[i][0] = (t0 + t1);
    tblock[i][1] = (t3 + t2);
    tblock[i][2] = (t0 - t1);      
    tblock[i][3] = (t3 - t2);
  }
}

void ihadamard4x2(int **tblock, int **block)
{
  int i;  
  int tmp[8];
  int *pTmp = tmp;
  int p0,p1,p2,p3;
  int t0,t1,t2,t3;

  // Horizontal
  *(pTmp++) = tblock[0][0] + tblock[1][0];
  *(pTmp++) = tblock[0][1] + tblock[1][1];
  *(pTmp++) = tblock[0][2] + tblock[1][2];
  *(pTmp++) = tblock[0][3] + tblock[1][3];

  *(pTmp++) = tblock[0][0] - tblock[1][0];
  *(pTmp++) = tblock[0][1] - tblock[1][1];
  *(pTmp++) = tblock[0][2] - tblock[1][2];
  *(pTmp  ) = tblock[0][3] - tblock[1][3];

  // Vertical
  pTmp = tmp;
  for (i = 0; i < 2; i++)
  {
    p0 = *(pTmp++);
    p1 = *(pTmp++);
    p2 = *(pTmp++);
    p3 = *(pTmp++);

    t0 = p0 + p2;
    t1 = p0 - p2;
    t2 = p1 - p3;
    t3 = p1 + p3;

    // coefficients (transposed)
    block[0][i] = t0 + t3;
    block[1][i] = t1 + t2;
    block[2][i] = t1 - t2;
    block[3][i] = t0 - t3;
  }
}

//following functions perform 8 additions, 8 assignments. Should be a bit faster
void hadamard2x2(int **block, int tblock[4])
{
  int p0,p1,p2,p3;

  p0 = block[0][0] + block[0][4];
  p1 = block[0][0] - block[0][4];
  p2 = block[4][0] + block[4][4];
  p3 = block[4][0] - block[4][4];
 
  tblock[0] = (p0 + p2);
  tblock[1] = (p1 + p3);
  tblock[2] = (p0 - p2);
  tblock[3] = (p1 - p3);
}

void ihadamard2x2(int tblock[4], int block[4])
{
  int t0,t1,t2,t3;

  t0 = tblock[0] + tblock[1];
  t1 = tblock[0] - tblock[1];
  t2 = tblock[2] + tblock[3];
  t3 = tblock[2] - tblock[3];

  block[0] = (t0 + t2);
  block[1] = (t1 + t3);
  block[2] = (t0 - t2);
  block[3] = (t1 - t3);
}

/*
void hadamard2x2(int **block, int tblock[4])
{
  //12 additions, 4 assignments
    tblock[0] = (block[0][0] + block[0][4] + block[4][0] + block[4][4]);
    tblock[1] = (block[0][0] - block[0][4] + block[4][0] - block[4][4]);
    tblock[2] = (block[0][0] + block[0][4] - block[4][0] - block[4][4]);
    tblock[3] = (block[0][0] - block[0][4] - block[4][0] + block[4][4]);
}

void ihadamard2x2(int tblock[4], int block[4])
{
    block[0] = (tblock[0] + tblock[1] + tblock[2] + tblock[3]);
    block[1] = (tblock[0] - tblock[1] + tblock[2] - tblock[3]);
    block[2] = (tblock[0] + tblock[1] - tblock[2] - tblock[3]);
    block[3] = (tblock[0] - tblock[1] - tblock[2] + tblock[3]);
}

*/


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


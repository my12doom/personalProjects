/*!
 *************************************************************************************
 * \file blk_prediction.c
 *
 * \brief
 *    Block Prediction related functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Alexis Michael Tourapis         <alexismt@ieee.org>
 *
 *************************************************************************************
 */

#include "contributors.h"

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include "block.h"
#include "global.h"

#include "macroblock.h"
#include "mc_prediction.h"
#include "image.h"
#include "mb_access.h"
#include <emmintrin.h>

void compute_residue (imgpel **curImg, imgpel **mpr, int **mb_rres, int mb_x, int opix_x, int width, int height)
{
  imgpel *imgOrg, *imgPred;
  int    *m7;
  int i, j;

  for (j = 0; j < height; j++)
  {
    imgOrg = &curImg[j][opix_x];    
    imgPred = &mpr[j][mb_x];
    m7 = &mb_rres[j][mb_x]; 
    for (i = 0; i < width; i++)
    {
      *m7++ = *imgOrg++ - *imgPred++;
    }
  }
}

inline void sample_reconstruct4_4_255(imgpel **curImg, imgpel **mpr, int **mb_rres, int mb_x, int opix_x, int dq_bits)
{
	__m128i mm_dq2 = _mm_set1_epi16((1<<(DQ_BITS-1)));
	__m128i mm0  = _mm_set1_epi16(0);

	__m128i mm70, mm71, mm72, mm73;
	__m128i pre0, pre1, pre2, pre3;

	mm70 = _mm_loadu_si128((__m128i*) &mb_rres[0][mb_x]);
	mm71 = _mm_loadu_si128((__m128i*) &mb_rres[1][mb_x]);
	mm72 = _mm_loadu_si128((__m128i*) &mb_rres[2][mb_x]);
	mm73 = _mm_loadu_si128((__m128i*) &mb_rres[3][mb_x]);

	mm70 = _mm_packs_epi32(mm70, mm71);
	mm71 = _mm_packs_epi32(mm72, mm73);

	pre0 = _mm_unpacklo_epi8(_mm_loadu_si128((__m128i*) &mpr[0][mb_x]), mm0);
	pre1 = _mm_unpacklo_epi8(_mm_loadu_si128((__m128i*) &mpr[1][mb_x]), mm0);
	pre2 = _mm_unpacklo_epi8(_mm_loadu_si128((__m128i*) &mpr[2][mb_x]), mm0);
	pre3 = _mm_unpacklo_epi8(_mm_loadu_si128((__m128i*) &mpr[3][mb_x]), mm0);		//since mpr are unsiged 8bit, so we just add 00 makes them 16bit signed

	pre0 = _mm_unpacklo_epi64(pre0, pre1);
	pre2 = _mm_unpacklo_epi64(pre2, pre3);	// epi64 seems strange? 64bit = 4 * 16bit signed interger, the upper 64bit is wasted

	mm70 = _mm_add_epi16(mm70, mm_dq2);
	mm71 = _mm_add_epi16(mm71, mm_dq2);
	mm70 = _mm_srai_epi16(mm70, DQ_BITS);
	mm71 = _mm_srai_epi16(mm71, DQ_BITS);
	pre0 = _mm_add_epi16(mm70, pre0);
	pre2 = _mm_add_epi16(mm71, pre2);		// now pre0 = line0+line1, pre2 = line2+line3

	pre1 = _mm_unpackhi_epi64(pre0, pre0);	
	pre3 = _mm_unpackhi_epi64(pre2, pre2);

	pre0 = _mm_packus_epi16(pre0, pre0);
	pre1 = _mm_packus_epi16(pre1, pre1);
	pre2 = _mm_packus_epi16(pre2, pre2);
	pre3 = _mm_packus_epi16(pre3, pre3);

	_mm_storel_epi64((__m128i*) &curImg[0][opix_x], pre0);
	_mm_storel_epi64((__m128i*) &curImg[1][opix_x], pre1);
	_mm_storel_epi64((__m128i*) &curImg[2][opix_x], pre2);
	_mm_storel_epi64((__m128i*) &curImg[3][opix_x], pre3);		// there might be some access violation
}

inline void sample_reconstruct8_8_255(imgpel **curImg, imgpel **mpr, int **mb_rres, int mb_x, int opix_x, int dq_bits)
{
	imgpel *imgOrg, *imgPred;
	int    *m7;
	int i, j;

	__m128i tmp;
	__m128i mm_dq2 = _mm_set1_epi16((1<<(DQ_BITS-1)));
	__m128i mm0  = _mm_set1_epi16(0);

	for (j = 0; j < 8; j++)
	{
		__m128i mm7, mm72, mmPred;
		imgOrg = &curImg[j][opix_x];
		imgPred = &mpr[j][mb_x];
		m7 = &mb_rres[j][mb_x];

		mm7 = _mm_loadu_si128((__m128i*) m7);
		mm72 = _mm_loadu_si128((__m128i*) (m7+4));
		mm7 = _mm_packs_epi32(mm7, mm72);

		mmPred = _mm_loadu_si128((__m128i*) imgPred);
		mmPred = _mm_unpacklo_epi8(mmPred, mm0);

		tmp = _mm_add_epi16(mm7, mm_dq2);
		tmp = _mm_srai_epi16(tmp, DQ_BITS);
		tmp = _mm_add_epi16(tmp, mmPred);

		tmp = _mm_packus_epi16(tmp, tmp);
		_mm_storeu_si128((__m128i*) imgOrg, tmp);

	}
}

inline void sample_reconstruct16_255(imgpel **curImg, imgpel **mpr, int **mb_rres, int mb_x, int opix_x, int dq_bits)
{
	imgpel *imgOrg, *imgPred;
	int    *m7;
	int i, j;

	__m128i tmp, tmp2;
	__m128i mm_dq2 = _mm_set1_epi16((1<<(DQ_BITS-1)));
	__m128i mm0  = _mm_set1_epi16(0);

	for (j = 0; j < 16; j++)
	{
		__m128i mm7, mm72, mmPred;
		imgOrg = &curImg[j][opix_x];
		imgPred = &mpr[j][mb_x];
		m7 = &mb_rres[j][mb_x];


		//for (i=0;i<8;i++)
		//imgOrg[0] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[0], DQ_BITS) + imgPred[0]);
		//imgOrg[1] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[1], DQ_BITS) + imgPred[1]);
		//imgOrg[2] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[2], DQ_BITS) + imgPred[2]);
		//imgOrg[3] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[3], DQ_BITS) + imgPred[3]);
		//imgOrg[4] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[4], DQ_BITS) + imgPred[4]);
		//imgOrg[5] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[5], DQ_BITS) + imgPred[5]);
		//imgOrg[6] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[6], DQ_BITS) + imgPred[6]);
		//imgOrg[7] = (imgpel) iClip1( 255, rshift_rnd_sf(m7[7], DQ_BITS) + imgPred[7]);

		// SSE2

		// first 8 pixel
		mm7 = _mm_loadu_si128((__m128i*) m7);
		mm72 = _mm_loadu_si128((__m128i*) (m7+4));
		mm7 = _mm_packs_epi32(mm7, mm72);

		mmPred = _mm_loadu_si128((__m128i*) imgPred);
		mmPred = _mm_unpacklo_epi8(mmPred, mm0);

		tmp = _mm_add_epi16(mm7, mm_dq2);
		tmp = _mm_srai_epi16(tmp, DQ_BITS);
		tmp = _mm_add_epi16(tmp, mmPred);

		// second 8 pixel
		mm7 = _mm_loadu_si128((__m128i*) (m7+8));
		mm72 = _mm_loadu_si128((__m128i*) (m7+12));
		mm7 = _mm_packs_epi32(mm7, mm72);

		mmPred = _mm_loadu_si128((__m128i*) (imgPred+8));
		mmPred = _mm_unpacklo_epi8(mmPred, mm0);

		tmp2 = _mm_add_epi16(mm7, mm_dq2);
		tmp2 = _mm_srai_epi16(tmp2, DQ_BITS);
		tmp2 = _mm_add_epi16(tmp2, mmPred);

		// store
		tmp = _mm_packus_epi16(tmp, tmp2);
		_mm_storeu_si128((__m128i*) imgOrg, tmp);

	}
}

void sample_reconstruct (imgpel **curImg, imgpel **mpr, int **mb_rres, int mb_x, int opix_x, int width, int height, int max_imgpel_value, int dq_bits)
{
  imgpel *imgOrg, *imgPred;
  int    *m7;
  int i, j;


  if (width == 4 && height == 4 && max_imgpel_value == 255)
  {
	  sample_reconstruct4_4_255(curImg, mpr, mb_rres, mb_x, opix_x, dq_bits);
	  return;
  }

  if (width == 8 && height == 8 && max_imgpel_value == 255)
  {
	  sample_reconstruct8_8_255(curImg, mpr, mb_rres, mb_x, opix_x, dq_bits);
	  return;
  }

  if (width == 16 && height == 16 && max_imgpel_value == 255)
  {
	  sample_reconstruct16_255(curImg, mpr, mb_rres, mb_x, opix_x, dq_bits);
	  return;
  }

  for (j = 0; j < height; j++)
  {
    imgOrg = &curImg[j][opix_x];
    imgPred = &mpr[j][mb_x];
    m7 = &mb_rres[j][mb_x]; 
    for (i=0;i<width;i++)
      *imgOrg++ = (imgpel) iClip1( max_imgpel_value, rshift_rnd_sf(*m7++, dq_bits) + *imgPred++);
  }
}

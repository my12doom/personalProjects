
/*!
 ***************************************************************************
 * \file transform8x8.c
 *
 * \brief
 *    8x8 transform functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Yuri Vatis
 *    - Jan Muenster
 *
 * \date
 *    12. October 2003
 **************************************************************************
 */

#include "global.h"

#include "image.h"
#include "mb_access.h"
#include "elements.h"
#include "transform8x8.h"
#include "transform.h"
#include "quant.h"
#include <emmintrin.h>

static void recon8x8(int **m7, imgpel **mb_rec, imgpel **mpr, int max_imgpel_value, int ioff)
{
  int j;
  int    *m_tr  = NULL;
  imgpel *m_rec = NULL;
  imgpel *m_prd = NULL;
  __m128i mm_dq2 = _mm_set1_epi16((1<<(DQ_BITS_8-1)));
  __m128i mm0  = _mm_set1_epi16(0);
  __m128i mm7, mm72, mmPred, tmp;

  for( j = 0; j < 8; j++)
  {

	m_tr = (*m7++) + ioff;
    m_rec = (*mb_rec++) + ioff;
    m_prd = (*mpr++) + ioff;

	mm7 = _mm_loadu_si128((__m128i*) m_tr);
	mm72 = _mm_loadu_si128((__m128i*) (m_tr+4));
	mm7 = _mm_packs_epi32(mm7, mm72);

	mmPred = _mm_loadu_si128((__m128i*) m_prd);
	mmPred = _mm_unpacklo_epi8(mmPred, mm0);

	tmp = _mm_add_epi16(mm7, mm_dq2);
	tmp = _mm_srai_epi16(tmp, DQ_BITS_8);
	tmp = _mm_add_epi16(tmp, mmPred);

	tmp = _mm_packus_epi16(tmp, tmp);
	_mm_storel_epi64((__m128i*) m_rec, tmp);



	/*
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec++ = (imgpel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8)); 
    *m_rec   = (imgpel) iClip1(max_imgpel_value, (*m_prd  ) + rshift_rnd_sf(*m_tr  , DQ_BITS_8)); 
	*/
  }
}

static void copy8x8(imgpel **mb_rec, imgpel **mpr, int ioff)
{
  int j;

  for( j = 0; j < 8; j++)
  {
    memcpy((*mb_rec++) + ioff, (*mpr++) + ioff, 8 * sizeof(imgpel));
  }
}

static void recon8x8_lossless(int **m7, imgpel **mb_rec, imgpel **mpr, int max_imgpel_value, int ioff)
{
  int i, j;
  for( j = 0; j < 8; j++)
  {
    for( i = ioff; i < ioff + 8; i++)
      (*mb_rec)[i] = (imgpel) iClip1(max_imgpel_value, ((*m7)[i] + (long)(*mpr)[i])); 
    mb_rec++;
    m7++;
    mpr++;
  }
}

/*!
 ***********************************************************************
 * \brief
 *    Inverse 8x8 transformation
 ***********************************************************************
 */ 
void itrans8x8(Macroblock *currMB,   //!< current macroblock
               ColorPlane pl,        //!< used color plane       
               int ioff,             //!< index to 4x4 block
               int joff)             //!< index to 4x4 block
{
  Slice *currSlice = currMB->p_Slice;

  int    **m7     = currSlice->mb_rres[pl];

  if (currMB->is_lossless == TRUE)
  {
    recon8x8_lossless(&m7[joff], &currSlice->mb_rec[pl][joff], &currSlice->mb_pred[pl][joff], currMB->p_Vid->max_pel_value_comp[pl], ioff);
  }
  else
  {
    inverse8x8(&m7[joff], &m7[joff], ioff);
    recon8x8  (&m7[joff], &currSlice->mb_rec[pl][joff], &currSlice->mb_pred[pl][joff], currMB->p_Vid->max_pel_value_comp[pl], ioff);
  }
}

/*!
 ***********************************************************************
 * \brief
 *    Inverse 8x8 transformation
 ***********************************************************************
 */ 
void icopy8x8(Macroblock *currMB,   //!< current macroblock
               ColorPlane pl,        //!< used color plane       
               int ioff,             //!< index to 4x4 block
               int joff)             //!< index to 4x4 block
{
  Slice *currSlice = currMB->p_Slice;

  copy8x8  (&currSlice->mb_rec[pl][joff], &currSlice->mb_pred[pl][joff], ioff);
}

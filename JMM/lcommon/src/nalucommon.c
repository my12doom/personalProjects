
/*!
 ************************************************************************
 * \file  nalucommon.c
 *
 * \brief
 *    Common NALU support functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Stephan Wenger   <stewe@cs.tu-berlin.de>
 ************************************************************************
 */

#include "global.h"
#include "nalucommon.h"
#include "memalloc.h"

/*!
 *************************************************************************************
 * \brief
 *    Allocates memory for a NALU
 *
 * \param buffersize
 *     size of NALU buffer
 *
 * \return
 *    pointer to a NALU
 *************************************************************************************
 */
NALU_t *AllocNALU(int buffersize)
{
  NALU_t *n;

  if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
    no_mem_exit ("AllocNALU: n");

  n->max_size=buffersize;

  if ((n->buf = (byte*)calloc (buffersize, sizeof (byte))) == NULL)
  {
    free (n);
    no_mem_exit ("AllocNALU: n->buf");
  }

  return n;
}


/*!
 *************************************************************************************
 * \brief
 *    Frees a NALU
 *
 * \param n
 *    NALU to be freed
 *
 *************************************************************************************
 */
void FreeNALU(NALU_t *n)
{
  if (n != NULL)
  {
    if (n->buf != NULL)
    {
      free(n->buf);
      n->buf=NULL;
    }
    free (n);
  }
}

int CopyNALU(NALU_t *src, NALU_t *dst)
{
	// only buffer pointer and its size preserved
	unsigned max_size_dst = dst->max_size;
	byte *buf_dst = dst->buf;

	if (dst->max_size < src->len)
		return 0;

	memcpy(dst, src, sizeof(NALU_t));
	dst->max_size = max_size_dst;
	dst->buf = buf_dst;

	memcpy(dst->buf, src->buf, src->len);

	return 1;
}
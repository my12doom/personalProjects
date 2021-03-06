
/*!
 *************************************************************************************
 * \file annexb.h
 *
 * \brief
 *    Annex B byte stream buffer handling.
 *
 *************************************************************************************
 */

#ifndef _ANNEXB_H_
#define _ANNEXB_H_

#include "nalucommon.h"
#include "ts_stream.h"

typedef struct annex_b_struct 
{
  int  BitStreamFile;                //!< the bit stream file
  byte *iobuffer;
  byte *iobufferread;
  int bytesinbuffer;
  int is_eof;
  int iIOBufferSize;

  int IsFirstByteStreamNALU;
  int nextstartcodebytes;
  byte *Buf;

  // my12doom
  void *is_ts;							// NULL for raw stream, pointer for MPEG Transport Stream.
  struct annex_b_struct * sub_annex;	// for MVC sub stream input
} ANNEXB_t;

extern int  get_annex_b_NALU (VideoParameters *p_Vid, NALU_t *nalu, ANNEXB_t *annex_b);

extern void open_annex_b     (char *fn, ANNEXB_t *annex_b);
extern void close_annex_b    (ANNEXB_t *annex_b);
extern void malloc_annex_b   (VideoParameters *p_Vid, ANNEXB_t **p_annex_b);
extern void free_annex_b     (ANNEXB_t **p_annex_b);
extern void init_annex_b     (ANNEXB_t *annex_b);
extern void reset_annex_b    (ANNEXB_t *annex_b);
#endif


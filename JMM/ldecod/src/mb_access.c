
/*!
 *************************************************************************************
 * \file mb_access.c
 *
 * \brief
 *    Functions for macroblock neighborhoods
 *
 *  \author
 *      Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Karsten Suehring
 *************************************************************************************
 */

#include "global.h"
#include "mbuffer.h"
#include "mb_access.h"

/*!
 ************************************************************************
 * \brief
 *    returns 1 if the macroblock at the given address is available
 ************************************************************************
 */
Boolean mb_is_available(int mbAddr, Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  if ((mbAddr < 0) || (mbAddr > ((int)currSlice->dec_picture->PicSizeInMbs - 1)))
    return FALSE;

  // the following line checks both: slice number and if the mb has been decoded
  if (!currMB->DeblockCall)
  {
    if (currSlice->mb_data[mbAddr].slice_nr != currMB->slice_nr)
      return FALSE;
  }

  return TRUE;
}

Boolean mb_is_available2(int mbAddr, Macroblock *currMB, int PicSizeInMbs)
{
	Slice *currSlice = currMB->p_Slice;
	if ((mbAddr < 0) || (mbAddr > (PicSizeInMbs - 1)))
		return FALSE;

	// the following line checks both: slice number and if the mb has been decoded
	if (!currMB->DeblockCall)
	{
		if (currSlice->mb_data[mbAddr].slice_nr != currMB->slice_nr)
			return FALSE;
	}

	return TRUE;
}
/*!
 ************************************************************************
 * \brief
 *    Checks the availability of neighboring macroblocks of
 *    the current macroblock for prediction and context determination;
 ************************************************************************
 */

NeighborStruct cache[max_mb_nr];
PixelPos cache_xy[3][3][max_mb_nr];
int neighbors_init_done = 0;

void init_mb_neighbors(Slice **Slices, int slice_count)
{
	int iSlice, iMB, x, y;
	VideoParameters *p_Vid = Slices[0]->p_Vid;
	BlockPos *PicPos = p_Vid->PicPos;
	int PicWidthInMbs = p_Vid->PicWidthInMbs;
	int PicSizeInMbs = p_Vid->PicSizeInMbs;

	if (neighbors_init_done)
		return;

	// neighbor init
	for(iSlice = 0; iSlice<slice_count; ++iSlice)
	{
		Slice *currSlice = Slices[iSlice];

		if( (p_Vid->separate_colour_plane_flag != 0) )
		{
			change_plane_JV( p_Vid, currSlice->colour_plane_id, currSlice );
		}
		else
		{
			currSlice->mb_data = p_Vid->mb_data;
			currSlice->dec_picture = p_Vid->dec_picture;
			currSlice->siblock = p_Vid->siblock;
			currSlice->ipredmode = p_Vid->ipredmode;
			currSlice->intra_block = p_Vid->intra_block;
		}

		for(iMB = currSlice->start_mb_nr; iMB<currSlice->end_mb_nr_plus1; ++iMB)
		{
			// copied from CheckAvailabilityOfNeighbors()
			Macroblock *currMB = &currSlice->mb_data[iMB];
			BlockPos *p_pic_pos = &PicPos[iMB];

			currMB->slice_nr = iSlice;
			currMB->p_Slice = currSlice;
			currMB->mbAddrX = iMB;
			currMB->mbAddrA = iMB - 1;
			currMB->mbAddrD = currMB->mbAddrA - PicWidthInMbs;
			currMB->mbAddrB = currMB->mbAddrD + 1;
			currMB->mbAddrC = currMB->mbAddrB + 1;


			currMB->mbAvailA = (Boolean) (mb_is_available2(currMB->mbAddrA, currMB, PicSizeInMbs) && ((p_pic_pos->x)!=0));
			currMB->mbAvailD = (Boolean) (mb_is_available2(currMB->mbAddrD, currMB, PicSizeInMbs) && ((p_pic_pos->x)!=0));
			currMB->mbAvailC = (Boolean) (mb_is_available2(currMB->mbAddrC, currMB, PicSizeInMbs) && (((p_pic_pos + 1)->x)!=0));
			currMB->mbAvailB = (Boolean) (mb_is_available2(currMB->mbAddrB, currMB, PicSizeInMbs));        

			currMB->mb_left = (currMB->mbAvailA) ? &(currSlice->mb_data[currMB->mbAddrA]) : NULL;
			currMB->mb_up   = (currMB->mbAvailB) ? &(currSlice->mb_data[currMB->mbAddrB]) : NULL;

			memcpy(&cache[iMB], &currMB->mbAddrA, sizeof(NeighborStruct));
		}
	}


	// neighbor with offset table init
	for(iSlice = 0; iSlice<slice_count; ++iSlice)
	{
		Slice *currSlice = Slices[iSlice];
		for(iMB = currSlice->start_mb_nr; iMB<currSlice->end_mb_nr_plus1; ++iMB)
		{
			for(x=0; x<3; ++x)
			{
				for(y=0; y<3; ++y)
				{
					PixelPos * pix = &cache_xy[x][y][iMB];
					NeighborStruct *currMB = &cache[iMB];

					// copied from getNonAffNeighbour
					if (x == 0)
					{
						if (y == 0)
						{
							pix->mb_addr   = currMB->mbAddrD;
							pix->available = currMB->mbAvailD;
						}
						else if (y == 1)
						{
							pix->mb_addr   = currMB->mbAddrA;
							pix->available = currMB->mbAvailA;
						}
						else // y==2
						{
							pix->available = FALSE;
						}
					}
					else if (x==1)
					{
						if (y == 0)
						{
							pix->mb_addr   = currMB->mbAddrB;
							pix->available = currMB->mbAvailB;
						}
						else if (y==1)
						{
							pix->mb_addr   = iMB;
							pix->available = TRUE;
						}
						else// y==2
						{
							pix->available = FALSE;
						}
					}
					else if ((x==2) && (y==0))
					{
						pix->mb_addr   = currMB->mbAddrC;
						pix->available = currMB->mbAvailC;
					}
					else
					{
						pix->available = FALSE;
					}

					// after
					if (pix->available)
					{
						BlockPos *CurPos = &(p_Vid->PicPos[ pix->mb_addr ]);
						pix->pos_x = CurPos->x * 8;
						pix->pos_y = CurPos->y * 8;
					}

				}
			}
		}
	}
	neighbors_init_done = 1;
}

void CheckAvailabilityOfNeighbors(Macroblock *currMB)
{
	memcpy(&currMB->mbAddrA, &cache[currMB->mbAddrX], sizeof(NeighborStruct));
}


void CheckAvailabilityOfNeighbors2(Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  StorablePicture *dec_picture = currSlice->dec_picture; //p_Vid->dec_picture;
  const int mb_nr = currMB->mbAddrX;
  BlockPos *PicPos = currMB->p_Vid->PicPos;

  //if (neighbors_init_done)
  {
	  memcpy(&currMB->mbAddrA, &cache[mb_nr], sizeof(NeighborStruct));
	  
	  //currMB->mb_left = (currMB->mbAvailA) ? &(currSlice->mb_data[currMB->mbAddrA]) : NULL;
	  //currMB->mb_up   = (currMB->mbAvailB) ? &(currSlice->mb_data[currMB->mbAddrB]) : NULL;

	  return;
  }

  if (dec_picture->mb_aff_frame_flag)
  {
    int cur_mb_pair = mb_nr >> 1;
    currMB->mbAddrA = 2 * (cur_mb_pair - 1);
    currMB->mbAddrB = 2 * (cur_mb_pair - dec_picture->PicWidthInMbs);
    currMB->mbAddrC = 2 * (cur_mb_pair - dec_picture->PicWidthInMbs + 1);
    currMB->mbAddrD = 2 * (cur_mb_pair - dec_picture->PicWidthInMbs - 1);

    currMB->mbAvailA = (Boolean) (mb_is_available(currMB->mbAddrA, currMB) && ((PicPos[cur_mb_pair    ].x)!=0));
    currMB->mbAvailB = (Boolean) (mb_is_available(currMB->mbAddrB, currMB));
    currMB->mbAvailC = (Boolean) (mb_is_available(currMB->mbAddrC, currMB) && ((PicPos[cur_mb_pair + 1].x)!=0));
    currMB->mbAvailD = (Boolean) (mb_is_available(currMB->mbAddrD, currMB) && ((PicPos[cur_mb_pair    ].x)!=0));
  }
  else
  {
    BlockPos *p_pic_pos = &PicPos[mb_nr    ];
    currMB->mbAddrA = mb_nr - 1;
    currMB->mbAddrD = currMB->mbAddrA - dec_picture->PicWidthInMbs;
    currMB->mbAddrB = currMB->mbAddrD + 1;
    currMB->mbAddrC = currMB->mbAddrB + 1;


    currMB->mbAvailA = (Boolean) (mb_is_available(currMB->mbAddrA, currMB) && ((p_pic_pos->x)!=0));
    currMB->mbAvailD = (Boolean) (mb_is_available(currMB->mbAddrD, currMB) && ((p_pic_pos->x)!=0));
    currMB->mbAvailC = (Boolean) (mb_is_available(currMB->mbAddrC, currMB) && (((p_pic_pos + 1)->x)!=0));
    currMB->mbAvailB = (Boolean) (mb_is_available(currMB->mbAddrB, currMB));        


  }

  currMB->mb_left = (currMB->mbAvailA) ? &(currSlice->mb_data[currMB->mbAddrA]) : NULL;
  currMB->mb_up   = (currMB->mbAvailB) ? &(currSlice->mb_data[currMB->mbAddrB]) : NULL;

  memcpy(&cache[mb_nr], &currMB->mbAddrA, sizeof(NeighborStruct));
}

/*!
 ************************************************************************
 * \brief
 *    Checks the availability of neighboring macroblocks of
 *    the current macroblock for prediction and context determination;
 ************************************************************************
 */
void CheckAvailabilityOfNeighborsNormal(Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  StorablePicture *dec_picture = currSlice->dec_picture; //p_Vid->dec_picture;
  const int mb_nr = currMB->mbAddrX;
  BlockPos *PicPos = currMB->p_Vid->PicPos;

  BlockPos *p_pic_pos = &PicPos[mb_nr    ];
  currMB->mbAddrA = mb_nr - 1;
  currMB->mbAddrD = currMB->mbAddrA - dec_picture->PicWidthInMbs;
  currMB->mbAddrB = currMB->mbAddrD + 1;
  currMB->mbAddrC = currMB->mbAddrB + 1;


  currMB->mbAvailA = (Boolean) (mb_is_available(currMB->mbAddrA, currMB) && ((p_pic_pos->x)!=0));
  currMB->mbAvailD = (Boolean) (mb_is_available(currMB->mbAddrD, currMB) && ((p_pic_pos->x)!=0));
  currMB->mbAvailC = (Boolean) (mb_is_available(currMB->mbAddrC, currMB) && (((p_pic_pos + 1)->x)!=0));
  currMB->mbAvailB = (Boolean) (mb_is_available(currMB->mbAddrB, currMB));        


  currMB->mb_left = (currMB->mbAvailA) ? &(currSlice->mb_data[currMB->mbAddrA]) : NULL;
  currMB->mb_up   = (currMB->mbAvailB) ? &(currSlice->mb_data[currMB->mbAddrB]) : NULL;
}

/*!
 ************************************************************************
 * \brief
 *    Checks the availability of neighboring macroblocks of
 *    the current macroblock for prediction and context determination;
 ************************************************************************
 */
void CheckAvailabilityOfNeighborsMBAFF(Macroblock *currMB)
{
  Slice *currSlice = currMB->p_Slice;
  StorablePicture *dec_picture = currSlice->dec_picture; //p_Vid->dec_picture;
  const int mb_nr = currMB->mbAddrX;
  BlockPos *PicPos = currMB->p_Vid->PicPos;

  int cur_mb_pair = mb_nr >> 1;
  currMB->mbAddrA = 2 * (cur_mb_pair - 1);
  currMB->mbAddrB = 2 * (cur_mb_pair - dec_picture->PicWidthInMbs);
  currMB->mbAddrC = 2 * (cur_mb_pair - dec_picture->PicWidthInMbs + 1);
  currMB->mbAddrD = 2 * (cur_mb_pair - dec_picture->PicWidthInMbs - 1);

  currMB->mbAvailA = (Boolean) (mb_is_available(currMB->mbAddrA, currMB) && ((PicPos[cur_mb_pair    ].x)!=0));
  currMB->mbAvailB = (Boolean) (mb_is_available(currMB->mbAddrB, currMB));
  currMB->mbAvailC = (Boolean) (mb_is_available(currMB->mbAddrC, currMB) && ((PicPos[cur_mb_pair + 1].x)!=0));
  currMB->mbAvailD = (Boolean) (mb_is_available(currMB->mbAddrD, currMB) && ((PicPos[cur_mb_pair    ].x)!=0));

  currMB->mb_left = (currMB->mbAvailA) ? &(currSlice->mb_data[currMB->mbAddrA]) : NULL;
  currMB->mb_up   = (currMB->mbAvailB) ? &(currSlice->mb_data[currMB->mbAddrB]) : NULL;
}


/*!
 ************************************************************************
 * \brief
 *    returns the x and y macroblock coordinates for a given MbAddress
 ************************************************************************
 */
void get_mb_block_pos_normal (BlockPos *PicPos, int mb_addr, short *x, short *y)
{
  BlockPos *pPos = &PicPos[ mb_addr ];
  *x = (short) pPos->x;
  *y = (short) pPos->y;
}

/*!
 ************************************************************************
 * \brief
 *    returns the x and y macroblock coordinates for a given MbAddress
 *    for mbaff type slices
 ************************************************************************
 */
void get_mb_block_pos_mbaff (BlockPos *PicPos, int mb_addr, short *x, short *y)
{
  BlockPos *pPos = &PicPos[ mb_addr >> 1 ];
  *x = (short)  pPos->x;
  *y = (short) ((pPos->y << 1) + (mb_addr & 0x01));
}

/*!
 ************************************************************************
 * \brief
 *    returns the x and y sample coordinates for a given MbAddress
 ************************************************************************
 */
void get_mb_pos (VideoParameters *p_Vid, int mb_addr, int mb_size[2], short *x, short *y)
{
  p_Vid->get_mb_block_pos(p_Vid->PicPos, mb_addr, x, y);

  (*x) = (short) ((*x) * mb_size[0]);
  (*y) = (short) ((*y) * mb_size[1]);
}


/*!
 ************************************************************************
 * \brief
 *    get neighbouring positions for non-aff coding
 * \param currMB
 *   current macroblock
 * \param xN
 *    input x position
 * \param yN
 *    input y position
 * \param mb_size
 *    Macroblock size in pixel (according to luma or chroma MB access)
 * \param pix
 *    returns position informations
 ************************************************************************
 getNonAffNeighbour (-1,-1) of 8x8.
 getNonAffNeighbour (-1,0) of 8x8.
 getNonAffNeighbour (-1,4) of 8x8.

 getNonAffNeighbour (0,-1) of 8x8.
 getNonAffNeighbour (0,3) of 8x8.
 getNonAffNeighbour (3,0) of 8x8.
 getNonAffNeighbour (3,4) of 8x8.
 getNonAffNeighbour (4,3) of 8x8.
 getNonAffNeighbour (4,-1) of 8x8.


 getNonAffNeighbour (-1,0) of 16x16.
 getNonAffNeighbour (-1,-1) of 16x16.
 getNonAffNeighbour (-1,7) of 16x16.
 getNonAffNeighbour (-1,8) of 16x16.

 getNonAffNeighbour (0,-1) of 16x16.
 getNonAffNeighbour (0,7) of 16x16.

 getNonAffNeighbour (7,-1) of 16x16.
 getNonAffNeighbour (7,0) of 16x16.
 getNonAffNeighbour (7,7) of 16x16.
 getNonAffNeighbour (7,8) of 16x16.

 getNonAffNeighbour (8,-1) of 16x16.
 getNonAffNeighbour (8,7) of 16x16.

 getNonAffNeighbour (16,-1) of 16x16.
 getNonAffNeighbour (16,7) of 16x16.

 42029/42049, 15.737957fps84068 frames are decoded.
 42048/42049, 15.743969fps
 MD5:25521D7EE2D8005550F67595FC7249B4
 */
void getNonAffNeighbour2(Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
	int maxW = mb_size[0], maxH = mb_size[1];

	if (xN < 0)
	{
		if (yN < 0)
		{
			pix->mb_addr   = currMB->mbAddrD;
			pix->available = currMB->mbAvailD;
		}
		else if (yN < maxH)
		{
			pix->mb_addr   = currMB->mbAddrA;
			pix->available = currMB->mbAvailA;
		}
		else
		{
			pix->available = FALSE;
		}
	}
	else if (xN < maxW)
	{
		if (yN < 0)
		{
			pix->mb_addr   = currMB->mbAddrB;
			pix->available = currMB->mbAvailB;
		}
		else if (yN < maxH)
		{
			pix->mb_addr   = currMB->mbAddrX;
			pix->available = TRUE;
		}
		else
		{
			pix->available = FALSE;
		}
	}
	else if ((xN >= maxW) && (yN < 0))
	{
		pix->mb_addr   = currMB->mbAddrC;
		pix->available = currMB->mbAvailC;
	}
	else
	{
		pix->available = FALSE;
	}

	if (pix->available || currMB->DeblockCall)
	{
		BlockPos *CurPos = &(currMB->p_Vid->PicPos[ pix->mb_addr ]);
		pix->x     = (byte) (xN & (maxW - 1));
		pix->y     = (byte) (yN & (maxH - 1));    
		pix->pos_x = (short) (pix->x + CurPos->x * maxW);
		pix->pos_y = (short) (pix->y + CurPos->y * maxH);
	}
}




int tbl8_base[18] = {0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2};
int tbl16_base[18] = {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2};
int *tbl8 = tbl8_base+1;
int *tbl16 = tbl16_base+1;

/*
void getNonAffNeighbour(Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
	int size = mb_size[0];
	int x,y;

	//getNonAffNeighbour2(currMB, xN, yN, mb_size, pix);

	if (size==8)
	{
		x = tbl8[xN];
		y = tbl8[yN];
		*pix = cache_xy[x][y][currMB->mbAddrX];
		pix->x = (byte) (xN & 7);
		pix->y = (byte) (yN & 7);
		pix->pos_x += pix->x;
		pix->pos_y += pix->y;
	}
	else
	{
		x = tbl16[xN];
		y = tbl16[yN];
		*pix = cache_xy[x][y][currMB->mbAddrX];
		pix->x = (byte) (xN & 15);
		pix->y = (byte) (yN & 15);
		pix->pos_x = pix->x + (pix->pos_x * 2);
		pix->pos_y = pix->y + (pix->pos_y * 2);
	}

	
}

*/
/*!
 ************************************************************************
 * \brief
 *    get neighboring positions for aff coding
 * \param currMB
 *   current macroblock
 * \param xN
 *    input x position
 * \param yN
 *    input y position
 * \param mb_size
 *    Macroblock size in pixel (according to luma or chroma MB access)
 * \param pix
 *    returns position informations
 ************************************************************************
 */
void getAffNeighbour(Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
  VideoParameters *p_Vid = currMB->p_Vid;
  int maxW, maxH;
  int yM = -1;

  maxW = mb_size[0];
  maxH = mb_size[1];

  // initialize to "not available"
  pix->available = FALSE;

  if(yN > (maxH - 1))
  {
    return;
  }
  if (xN > (maxW - 1) && yN >= 0 && yN < maxH)
  {
    return;
  }

  if (xN < 0)
  {
    if (yN < 0)
    {
      if(!currMB->mb_field)
      {
        // frame
        if ((currMB->mbAddrX & 0x01) == 0)
        {
          // top
          pix->mb_addr   = currMB->mbAddrD  + 1;
          pix->available = currMB->mbAvailD;
          yM = yN;
        }
        else
        {
          // bottom
          pix->mb_addr   = currMB->mbAddrA;
          pix->available = currMB->mbAvailA;
          if (currMB->mbAvailA)
          {
            if(!p_Vid->mb_data[currMB->mbAddrA].mb_field)
            {
               yM = yN;
            }
            else
            {
              (pix->mb_addr)++;
               yM = (yN + maxH) >> 1;
            }
          }
        }
      }
      else
      {
        // field
        if ((currMB->mbAddrX & 0x01) == 0)
        {
          // top
          pix->mb_addr   = currMB->mbAddrD;
          pix->available = currMB->mbAvailD;
          if (currMB->mbAvailD)
          {
            if(!p_Vid->mb_data[currMB->mbAddrD].mb_field)
            {
              (pix->mb_addr)++;
               yM = 2 * yN;
            }
            else
            {
               yM = yN;
            }
          }
        }
        else
        {
          // bottom
          pix->mb_addr   = currMB->mbAddrD+1;
          pix->available = currMB->mbAvailD;
          yM = yN;
        }
      }
    }
    else
    { // xN < 0 && yN >= 0
      if (yN >= 0 && yN <maxH)
      {
        if (!currMB->mb_field)
        {
          // frame
          if ((currMB->mbAddrX & 0x01) == 0)
          {
            // top
            pix->mb_addr   = currMB->mbAddrA;
            pix->available = currMB->mbAvailA;
            if (currMB->mbAvailA)
            {
              if(!p_Vid->mb_data[currMB->mbAddrA].mb_field)
              {
                 yM = yN;
              }
              else
              {
                (pix->mb_addr)+= ((yN & 0x01) != 0);
                yM = yN >> 1;
              }
            }
          }
          else
          {
            // bottom
            pix->mb_addr   = currMB->mbAddrA;
            pix->available = currMB->mbAvailA;
            if (currMB->mbAvailA)
            {
              if(!p_Vid->mb_data[currMB->mbAddrA].mb_field)
              {
                (pix->mb_addr)++;
                 yM = yN;
              }
              else
              {
                (pix->mb_addr)+= ((yN & 0x01) != 0);
                yM = (yN + maxH) >> 1;
              }
            }
          }
        }
        else
        {
          // field
          if ((currMB->mbAddrX & 0x01) == 0)
          {
            // top
            pix->mb_addr  = currMB->mbAddrA;
            pix->available = currMB->mbAvailA;
            if (currMB->mbAvailA)
            {
              if(!p_Vid->mb_data[currMB->mbAddrA].mb_field)
              {
                if (yN < (maxH >> 1))
                {
                   yM = yN << 1;
                }
                else
                {
                  (pix->mb_addr)++;
                   yM = (yN << 1 ) - maxH;
                }
              }
              else
              {
                 yM = yN;
              }
            }
          }
          else
          {
            // bottom
            pix->mb_addr  = currMB->mbAddrA;
            pix->available = currMB->mbAvailA;
            if (currMB->mbAvailA)
            {
              if(!p_Vid->mb_data[currMB->mbAddrA].mb_field)
              {
                if (yN < (maxH >> 1))
                {
                  yM = (yN << 1) + 1;
                }
                else
                {
                  (pix->mb_addr)++;
                   yM = (yN << 1 ) + 1 - maxH;
                }
              }
              else
              {
                (pix->mb_addr)++;
                 yM = yN;
              }
            }
          }
        }
      }
    }
  }
  else
  { // xN >= 0
    if (xN >= 0 && xN < maxW)
    {
      if (yN<0)
      {
        if (!currMB->mb_field)
        {
          //frame
          if ((currMB->mbAddrX & 0x01) == 0)
          {
            //top
            pix->mb_addr  = currMB->mbAddrB;
            // for the deblocker if the current MB is a frame and the one above is a field
            // then the neighbor is the top MB of the pair
            if (currMB->mbAvailB)
            {
              if (!(currMB->DeblockCall == 1 && (p_Vid->mb_data[currMB->mbAddrB]).mb_field))
                pix->mb_addr  += 1;
            }

            pix->available = currMB->mbAvailB;
            yM = yN;
          }
          else
          {
            // bottom
            pix->mb_addr   = currMB->mbAddrX - 1;
            pix->available = TRUE;
            yM = yN;
          }
        }
        else
        {
          // field
          if ((currMB->mbAddrX & 0x01) == 0)
          {
            // top
            pix->mb_addr   = currMB->mbAddrB;
            pix->available = currMB->mbAvailB;
            if (currMB->mbAvailB)
            {
              if(!p_Vid->mb_data[currMB->mbAddrB].mb_field)
              {
                (pix->mb_addr)++;
                 yM = 2* yN;
              }
              else
              {
                 yM = yN;
              }
            }
          }
          else
          {
            // bottom
            pix->mb_addr   = currMB->mbAddrB + 1;
            pix->available = currMB->mbAvailB;
            yM = yN;
          }
        }
      }
      else
      {
        // yN >=0
        // for the deblocker if this is the extra edge then do this special stuff
        if (yN == 0 && currMB->DeblockCall == 2)
        {
          pix->mb_addr  = currMB->mbAddrB + 1;
          pix->available = TRUE;
          yM = yN - 1;
        }

        else if ((yN >= 0) && (yN <maxH))
        {
          pix->mb_addr   = currMB->mbAddrX;
          pix->available = TRUE;
          yM = yN;
        }
      }
    }
    else
    { // xN >= maxW
      if(yN < 0)
      {
        if (!currMB->mb_field)
        {
          // frame
          if ((currMB->mbAddrX & 0x01) == 0)
          {
            // top
            pix->mb_addr  = currMB->mbAddrC + 1;
            pix->available = currMB->mbAvailC;
            yM = yN;
          }
          else
          {
            // bottom
            pix->available = FALSE;
          }
        }
        else
        {
          // field
          if ((currMB->mbAddrX & 0x01) == 0)
          {
            // top
            pix->mb_addr   = currMB->mbAddrC;
            pix->available = currMB->mbAvailC;
            if (currMB->mbAvailC)
            {
              if(!p_Vid->mb_data[currMB->mbAddrC].mb_field)
              {
                (pix->mb_addr)++;
                 yM = 2* yN;
              }
              else
              {
                yM = yN;
              }
            }
          }
          else
          {
            // bottom
            pix->mb_addr   = currMB->mbAddrC + 1;
            pix->available = currMB->mbAvailC;
            yM = yN;
          }
        }
      }
    }
  }
  if (pix->available || currMB->DeblockCall)
  {
    pix->x = (byte) (xN & (maxW - 1));
    pix->y = (byte) (yM & (maxH - 1));
    get_mb_pos(p_Vid, pix->mb_addr, mb_size, &(pix->pos_x), &(pix->pos_y));
    pix->pos_x = pix->pos_x + pix->x;
    pix->pos_y = pix->pos_y + pix->y;
  }
}


/*!
 ************************************************************************
 * \brief
 *    get neighboring 4x4 block
 * \param currMB
 *   current macroblock
 * \param block_x
 *    input x block position
 * \param block_y
 *    input y block position
 * \param mb_size
 *    Macroblock size in pixel (according to luma or chroma MB access)
 * \param pix
 *    returns position informations 
 ************************************************************************
 */
/*
void get4x4Neighbour (Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
  getNonAffNeighbour(currMB, xN, yN, mb_size, pix);

  if (pix->available)
  {
    pix->x >>= 2;
    pix->y >>= 2;
    pix->pos_x >>= 2;
    pix->pos_y >>= 2;
  }
}
*/
/*!
 ************************************************************************
 * \brief
 *    get neighboring 4x4 block
 * \param currMB
 *   current macroblock
 * \param block_x
 *    input x block position
 * \param block_y
 *    input y block position
 * \param mb_size
 *    Macroblock size in pixel (according to luma or chroma MB access)
 * \param pix
 *    returns position informations
 ************************************************************************
 */
/*
void get4x4NeighbourBase (Macroblock *currMB, int block_x, int block_y, int mb_size[2], PixelPos *pix)
{
  getNonAffNeighbour(currMB, block_x, block_y, mb_size, pix);

  if (pix->available)
  {
    pix->x >>= 2;
    pix->y >>= 2;
  }
}
*/

/*!
 *************************************************************************************
 * \file mb_access.h
 *
 * \brief
 *    Functions for macroblock neighborhoods
 *
 * \author
 *     Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Karsten Suehring
 *     - Alexis Michael Tourapis         <alexismt@ieee.org>  
 *************************************************************************************
 */

#ifndef _MB_ACCESS_H_
#define _MB_ACCESS_H_

extern void CheckAvailabilityOfNeighbors(Macroblock *currMB);
extern void CheckAvailabilityOfNeighborsMBAFF(Macroblock *currMB);
extern void CheckAvailabilityOfNeighborsNormal(Macroblock *currMB);

#define max_mb_nr 8192			// level 4.1
extern PixelPos cache_xy[3][3][max_mb_nr];
extern int *tbl8;
extern int *tbl16;
__forceinline void getNonAffNeighbour(Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
	int size = mb_size[0];
	int x,y;

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


extern void getAffNeighbour      (Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix);
__forceinline void get4x4Neighbour         (Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
	int size = mb_size[0];
	int x,y;

	if (size==8)
	{
		x = tbl8[xN];
		y = tbl8[yN];
		*pix = cache_xy[x][y][currMB->mbAddrX];
		pix->x = ((byte) (xN & 7)) >> 2;
		pix->y = ((byte) (yN & 7)) >> 2;
		pix->pos_x = pix->x + (pix->pos_x >> 2);
		pix->pos_y = pix->y + (pix->pos_y >> 2);
	}
	else
	{
		x = tbl16[xN];
		y = tbl16[yN];
		*pix = cache_xy[x][y][currMB->mbAddrX];
		pix->x = ((byte) (xN & 15)) >> 2;
		pix->y = ((byte) (yN & 15)) >> 2;
		pix->pos_x = pix->x + (pix->pos_x >> 1);
		pix->pos_y = pix->y + (pix->pos_y >> 1);
	}

	if (0)
	{
		pix->x >>= 2;
		pix->y >>= 2;
		pix->pos_x >>= 2;
		pix->pos_y >>= 2;
	}
}

__forceinline void get4x4NeighbourBase     (Macroblock *currMB, int xN, int yN, int mb_size[2], PixelPos *pix)
{
	int size = mb_size[0];
	int x,y;

	if (size==8)
	{
		x = tbl8[xN];
		y = tbl8[yN];
		*pix = cache_xy[x][y][currMB->mbAddrX];
		pix->x = ((byte) (xN & 7)) >> 2;
		pix->y = ((byte) (yN & 7)) >> 2;
		//pix->pos_x = pix->x + (pix->pos_x >> 2);
		//pix->pos_y = pix->y + (pix->pos_y >> 2);
	}
	else
	{
		x = tbl16[xN];
		y = tbl16[yN];
		*pix = cache_xy[x][y][currMB->mbAddrX];
		pix->x = ((byte) (xN & 15)) >> 2;
		pix->y = ((byte) (yN & 15)) >> 2;
		//pix->pos_x = pix->x + (pix->pos_x >> 1);
		//pix->pos_y = pix->y + (pix->pos_y >> 1);
	}

	if (0)
	{
		pix->x >>= 2;
		pix->y >>= 2;
	}
}
extern Boolean mb_is_available      (int mbAddr, Macroblock *currMB);
extern void get_mb_pos              (VideoParameters *p_Vid, int mb_addr, int mb_size[2], short *x, short *y);
extern void get_mb_block_pos_normal (BlockPos *PicPos, int mb_addr, short *x, short *y);
extern void get_mb_block_pos_mbaff  (BlockPos *PicPos, int mb_addr, short *x, short *y);


#endif

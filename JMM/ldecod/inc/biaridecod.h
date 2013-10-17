
/*!
 ***************************************************************************
 * \file
 *    biaridecod.h
 *
 * \brief
 *    Headerfile for binary arithmetic decoder routines
 *
 * \author
 *    Detlev Marpe,
 *    Gabi Blättermann
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. Oct 2000
 **************************************************************************
 */

#ifndef _BIARIDECOD_H_
#define _BIARIDECOD_H_


/************************************************************************
 * D e f i n i t i o n s
 ***********************************************************************
 */

/* Range table for  LPS */
static const byte rLPS_table_64x4[64][4]=
{
  { 128, 176, 208, 240},
  { 128, 167, 197, 227},
  { 128, 158, 187, 216},
  { 123, 150, 178, 205},
  { 116, 142, 169, 195},
  { 111, 135, 160, 185},
  { 105, 128, 152, 175},
  { 100, 122, 144, 166},
  {  95, 116, 137, 158},
  {  90, 110, 130, 150},
  {  85, 104, 123, 142},
  {  81,  99, 117, 135},
  {  77,  94, 111, 128},
  {  73,  89, 105, 122},
  {  69,  85, 100, 116},
  {  66,  80,  95, 110},
  {  62,  76,  90, 104},
  {  59,  72,  86,  99},
  {  56,  69,  81,  94},
  {  53,  65,  77,  89},
  {  51,  62,  73,  85},
  {  48,  59,  69,  80},
  {  46,  56,  66,  76},
  {  43,  53,  63,  72},
  {  41,  50,  59,  69},
  {  39,  48,  56,  65},
  {  37,  45,  54,  62},
  {  35,  43,  51,  59},
  {  33,  41,  48,  56},
  {  32,  39,  46,  53},
  {  30,  37,  43,  50},
  {  29,  35,  41,  48},
  {  27,  33,  39,  45},
  {  26,  31,  37,  43},
  {  24,  30,  35,  41},
  {  23,  28,  33,  39},
  {  22,  27,  32,  37},
  {  21,  26,  30,  35},
  {  20,  24,  29,  33},
  {  19,  23,  27,  31},
  {  18,  22,  26,  30},
  {  17,  21,  25,  28},
  {  16,  20,  23,  27},
  {  15,  19,  22,  25},
  {  14,  18,  21,  24},
  {  14,  17,  20,  23},
  {  13,  16,  19,  22},
  {  12,  15,  18,  21},
  {  12,  14,  17,  20},
  {  11,  14,  16,  19},
  {  11,  13,  15,  18},
  {  10,  12,  15,  17},
  {  10,  12,  14,  16},
  {   9,  11,  13,  15},
  {   9,  11,  12,  14},
  {   8,  10,  12,  14},
  {   8,   9,  11,  13},
  {   7,   9,  11,  12},
  {   7,   9,  10,  12},
  {   7,   8,  10,  11},
  {   6,   8,   9,  11},
  {   6,   7,   9,  10},
  {   6,   7,   8,   9},
  {   2,   2,   2,   2}
};

static const byte rLPS_table_256[256]  = 
{128, 176, 208, 240, 128, 167, 197, 227, 128, 158, 187, 216, 123, 150, 178, 205,
116, 142, 169, 195, 111, 135, 160, 185, 105, 128, 152, 175, 100, 122, 144, 166,
95, 116, 137, 158, 90, 110, 130, 150, 85, 104, 123, 142, 81, 99, 117, 135, 77, 94,
111, 128, 73, 89, 105, 122, 69, 85, 100, 116, 66, 80, 95, 110, 62, 76, 90, 104,
59, 72, 86, 99, 56, 69, 81, 94, 53, 65, 77, 89, 51, 62, 73, 85, 48, 59, 69, 80,
46, 56, 66, 76, 43, 53, 63, 72, 41, 50, 59, 69, 39, 48, 56, 65, 37, 45, 54, 62,
35, 43, 51, 59, 33, 41, 48, 56, 32, 39, 46, 53, 30, 37, 43, 50, 29, 35, 41, 48,
27, 33, 39, 45, 26, 31, 37, 43, 24, 30, 35, 41, 23, 28, 33, 39, 22, 27, 32, 37,
21, 26, 30, 35, 20, 24, 29, 33, 19, 23, 27, 31, 18, 22, 26, 30, 17, 21, 25, 28,
16, 20, 23, 27, 15, 19, 22, 25, 14, 18, 21, 24, 14, 17, 20, 23, 13, 16, 19, 22,
12, 15, 18, 21, 12, 14, 17, 20, 11, 14, 16, 19, 11, 13, 15, 18, 10, 12, 15, 17,
10, 12, 14, 16, 9, 11, 13, 15, 9, 11, 12, 14, 8, 10, 12, 14, 8, 9, 11, 13, 7,
 9, 11, 12, 7, 9, 10, 12, 7, 8, 10, 11, 6, 8, 9, 11, 6, 7, 9, 10, 6, 7, 8, 9, 2,
 2, 2, 2};

static byte rLPS_table_512[512];


static const byte AC_next_state_MPS_64[64] =    
{
  1,2,3,4,5,6,7,8,9,10,
  11,12,13,14,15,16,17,18,19,20,
  21,22,23,24,25,26,27,28,29,30,
  31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,
  51,52,53,54,55,56,57,58,59,60,
  61,62,62,63
};


static const byte AC_next_state_LPS_64[64] =    
{
  0, 0, 1, 2, 2, 4, 4, 5, 6, 7,
  8, 9, 9,11,11,12,13,13,15,15,
  16,16,18,18,19,19,21,21,22,22,
  23,24,24,25,26,26,27,27,28,29,
  29,30,30,30,31,32,32,33,33,33,
  34,34,35,35,35,36,36,36,37,37,
  37,38,38,63
};

static byte AC_next_state_MPS_128[128];
static byte AC_next_state_LPS_128[128];

static const byte renorm_table_32[32]={6,5,4,4,3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

#ifdef __cplusplus
extern "C" {
#endif

/*
inline int arideco_bits_read(DecodingEnvironmentPtr dep)
{
#if (2==TRACE)
	int tmp = ((*dep->Dcodestrm_len) << 3) - dep->DbitsLeft;
	fprintf(p_trace, "tmp: %d\n", tmp);
	return tmp;
#else
	return (((*dep->Dcodestrm_len) << 3) - dep->DbitsLeft);
#endif

}
*/

// ffmpeg's defines, variables and functions
#define USE_FFMPEG_CABAC 1
#define BRANCHLESS_CABAC_DECODER 1
#define CABAC_BITS 16
#define CABAC_MASK ((1<<CABAC_BITS)-1)

extern void ff_refill2(DecodingEnvironment *c);		// with renorm
extern byte ff_h264_mlps_state[4*64];
extern byte ff_h264_lps_range[4*2*64];
extern byte ff_h264_lps_state[2*64];
extern byte ff_h264_mps_state[2*64];
extern const byte ff_h264_norm_shift[512];

void ff_init_cabac_global_tables();
void ff_init_cabac_decoder(DecodingEnvironment *c, const byte *buf, int firstbyte, int *code_len);
//int ff_get_cabac(DecodingEnvironment *c, byte * const state)
__forceinline int ff_get_cabac(DecodingEnvironment *c, byte * const state)
{

	int s = *state;
	int RangeLPS= ff_h264_lps_range[2*(c->range&0xC0) + s];
	int bit, lps_mask;

	c->range -= RangeLPS;
	lps_mask= ((c->range<<(CABAC_BITS+1)) - c->low)>>31;

	c->low -= (c->range<<(CABAC_BITS+1)) & lps_mask;
	c->range += (RangeLPS - c->range) & lps_mask;

	s^=lps_mask;
	*state= (ff_h264_mlps_state+128)[s];
	bit= s&1;

	lps_mask= ff_h264_norm_shift[c->range];
	c->range<<= lps_mask;
	c->low  <<= lps_mask;
	if(!(c->low & CABAC_MASK))
	{
		//ff_refill2(c);
		int i, x;

		x= c->low ^ (c->low-1);
		i= 7 - ff_h264_norm_shift[x>>(CABAC_BITS-1)];

		x= -CABAC_MASK;

		x+= (c->Dcodestrm[*c->Dcodestrm_len]<<9) + (c->Dcodestrm[*c->Dcodestrm_len+1]<<1);

		c->low += x<<i;
		*c->Dcodestrm_len+= CABAC_BITS/8;
	}
	return bit;
}

__forceinline int ff_get_cabac_bypass(DecodingEnvironment *c)
{
	int range;
	c->low += c->low;

	if(!(c->low & CABAC_MASK))
	{
		//ff_refill(c);
		c->low+= (c->Dcodestrm[*c->Dcodestrm_len]<<9) + (c->Dcodestrm[*c->Dcodestrm_len+1]<<1);
		c->low -= CABAC_MASK;
		*c->Dcodestrm_len+= CABAC_BITS/8;
	}

	range= c->range<<(CABAC_BITS+1);
	if(c->low < range)
		return 0;
	else
	{
		c->low -= range;
		return 1;
	}
}

int ff_get_cabac_terminate(DecodingEnvironment *c);

// JM's original CABAC functions
extern void biari_init_context (int qp, BiContextTypePtr ctx, const char* ini);
//extern void arideco_done_decoding(DecodingEnvironmentPtr dep);
#if USE_FFMPEG_CABAC
#define arideco_start_decoding ff_init_cabac_decoder
#define biari_decode_symbol ff_get_cabac
#define biari_decode_symbol_eq_prob ff_get_cabac_bypass
#define biari_decode_final ff_get_cabac_terminate
#else
extern void arideco_start_decoding(DecodingEnvironmentPtr dep, unsigned char *code_buffer, int firstbyte, int *code_len);
extern unsigned int biari_decode_symbol(DecodingEnvironment *dep, BiContextType *bi_ct );
extern unsigned int biari_decode_symbol_eq_prob(DecodingEnvironmentPtr dep);
extern unsigned int biari_decode_final(DecodingEnvironmentPtr dep);
#endif

#ifdef __cplusplus
}
#endif

#endif  // BIARIDECOD_H_


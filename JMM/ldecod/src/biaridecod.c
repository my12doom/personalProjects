/*!
 *************************************************************************************
 * \file biaridecod.c
 *
 * \brief
 *   Binary arithmetic decoder routines.
 *
 *   This modified implementation of the M Coder is based on JVT-U084 
 *   with the choice of M_BITS = 16.
 *
 * \date
 *    21. Oct 2000
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Detlev Marpe
 *    - Gabi Blaettermann
 *    - Gunnar Marten
 *************************************************************************************
 */


#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"
#include "memalloc.h"
#include "biaridecod.h"


#ifdef __cplusplus
}
#endif


#define B_BITS    10      // Number of bits to represent the whole coding interval
#define HALF      0x01FE  //(1 << (B_BITS-1)) - 2
#define QUARTER   0x0100  //(1 << (B_BITS-2))

/*!
 ************************************************************************
 * \brief
 *    Allocates memory for the DecodingEnvironment struct
 * \return DecodingContextPtr
 *    allocates memory
 ************************************************************************
 */
DecodingEnvironmentPtr arideco_create_decoding_environment()
{
  DecodingEnvironmentPtr dep;

  if ((dep = (DecodingEnvironmentPtr)mem_calloc(1,sizeof(DecodingEnvironment))) == NULL)
    no_mem_exit("arideco_create_decoding_environment: dep");
  return dep;
}


/*!
 ***********************************************************************
 * \brief
 *    Frees memory of the DecodingEnvironment struct
 ***********************************************************************
 */
void arideco_delete_decoding_environment(DecodingEnvironmentPtr dep)
{
  if (dep == NULL)
  {
    snprintf(errortext, ET_SIZE, "Error freeing dep (NULL pointer)");
    error (errortext, 200);
  }
  else
    mem_free(dep);
}

/*!
 ************************************************************************
 * \brief
 *    finalize arithetic decoding():
 ************************************************************************
 */
void arideco_done_decoding(DecodingEnvironmentPtr dep)
{
  (*dep->Dcodestrm_len)++;
#if(TRACE==2)
  fprintf(p_trace, "done_decoding: %d\n", *dep->Dcodestrm_len);
#endif
}

/*!
 ************************************************************************
 * \brief
 *    read one byte from the bitstream
 ************************************************************************
 */
static inline unsigned int getbyte(DecodingEnvironmentPtr dep)
{     
#if(TRACE==2)
  fprintf(p_trace, "get_byte: %d\n", (*dep->Dcodestrm_len));
#endif
  return dep->Dcodestrm[(*dep->Dcodestrm_len)++];
}

/*!
 ************************************************************************
 * \brief
 *    read two bytes from the bitstream
 ************************************************************************
 */
static inline unsigned int getword(DecodingEnvironmentPtr dep)
{
  int *len = dep->Dcodestrm_len;
  byte *p_code_strm = &dep->Dcodestrm[*len];
#if(TRACE==2)
  fprintf(p_trace, "get_byte: %d\n", *len);
  fprintf(p_trace, "get_byte: %d\n", *len + 1);
#endif
  *len += 2;
  return ((*p_code_strm<<8) | *(p_code_strm + 1));
}

# if !USE_FFMPEG_CABAC
/*!
 ************************************************************************
 * \brief
 *    Initializes the DecodingEnvironment for the arithmetic coder
 ************************************************************************
 FFMPEG: ff_init_cabac_decoder(context, buf, size)
 */
void arideco_start_decoding(DecodingEnvironmentPtr dep, unsigned char *code_buffer,
                                                        int firstbyte, int *code_len)
{
	// my12doom table init
	static int init = 0;
	if (!init)
	{
		for(int i=0; i<64; i++)
		{
			AC_next_state_MPS_128[i*2] = AC_next_state_MPS_64[i]*2;
			AC_next_state_MPS_128[i*2+1] = AC_next_state_MPS_64[i]*2+1;
			AC_next_state_LPS_128[i*2] = AC_next_state_LPS_64[i]*2;
			AC_next_state_LPS_128[i*2+1] = AC_next_state_LPS_64[i]*2+1;
		}

		for(int i=0; i<4; i++)
			for(int j=0; j<64; j++)
			{
				rLPS_table_512[(i<<7)+j*2] = rLPS_table_64x4[j][i];
				rLPS_table_512[(i<<7)+j*2+1] = rLPS_table_64x4[j][i];
			}

		init = 1;
	}

  dep->Dcodestrm      = code_buffer;
  dep->Dcodestrm_len  = code_len;
  //dep->Dcodestrm_len = &dep->dummy;
  *dep->Dcodestrm_len = firstbyte;

  dep->Dvalue = getbyte(dep);
  dep->Dvalue = (dep->Dvalue << 16) | getword(dep); // lookahead of 2 bytes: always make sure that bitstream buffer
                                        // contains 2 more bytes than actual bitstream
  dep->DbitsLeft = 15;
  dep->Drange = HALF;

#if (2==TRACE)
  fprintf(p_trace, "value: %d firstbyte: %d code_len: %d\n", dep->Dvalue >> dep->DbitsLeft, firstbyte, *code_len);
#endif
}


/*!
 ************************************************************************
 * \brief
 *    arideco_bits_read
 ************************************************************************
 */
//int arideco_bits_read(DecodingEnvironmentPtr dep)


/*!
************************************************************************
* \brief
*    biari_decode_symbol():
* \return
*    the decoded symbol
************************************************************************
*/

unsigned int biari_decode_symbol(DecodingEnvironment *dep, BiContextType *bi_ct )
{
  int &DbitsLeft = dep->DbitsLeft;
  unsigned int &value = dep->Dvalue;
  unsigned int &range = dep->Drange;  
  byte       &state = *bi_ct;
  unsigned int bit    = (state);
  unsigned int rLPS;

  //unsigned int rLPS   = rLPS_table_64x4[*state][(*range>>6) & 0x03];


  //rLPS = rLPS_table_256[((state&0x3f)<<2) | ((range >> 6) & 0x03)];
  rLPS = rLPS_table_512[((range&0xC0)<<1)+state];
  range -= rLPS;

  if(value < (range << DbitsLeft))   //MPS
  {
    //state = AC_next_state_MPS_64[state&0x3f] | (state&0x40); // next state 
    //if ((state) <= 61)
	//	(state) ++;

	state = AC_next_state_MPS_128[state];

    if( range >= QUARTER )
    {
      return ((state) & 1);
    }
    else 
    {
      range <<= 1;
      (DbitsLeft)--;
    }
  }
  else         // LPS 
  {
    int renorm = renorm_table_32[(rLPS>>3)];
    value -= (range << DbitsLeft);

    range = (rLPS << renorm);
    (DbitsLeft) -= renorm;

    bit ^= 0x1;
    if (!(state&0x7E))          // switch meaning of MPS if necessary, 0x7E = binary 1111110
      state^= 0x1; 

    //state = AC_next_state_LPS_64[state&0x3f] | (state&0x40); // next state 
	state = AC_next_state_LPS_128[state];
  }

  if( DbitsLeft <= 0 )
  {
    value <<= 16;
    value |=  getword(dep);    // lookahead of 2 bytes: always make sure that bitstream buffer
    // contains 2 more bytes than actual bitstream
    (DbitsLeft) += 16;
  }
  return (bit&1);
}


/*!
 ************************************************************************
 * \brief
 *    biari_decode_symbol_eq_prob():
 * \return
 *    the decoded symbol
 ************************************************************************
 */
unsigned int biari_decode_symbol_eq_prob(DecodingEnvironmentPtr dep)
{
   int tmp_value;
   unsigned int *value = &dep->Dvalue;
   int *DbitsLeft = &dep->DbitsLeft;

  if(--(*DbitsLeft) == 0)  
  {
    *value = (*value << 16) | getword( dep );  // lookahead of 2 bytes: always make sure that bitstream buffer
                                             // contains 2 more bytes than actual bitstream
    *DbitsLeft = 16;
  }
  tmp_value  = *value - (dep->Drange << *DbitsLeft);

  if (tmp_value < 0)
  {
    return 0;
  }
  else
  {
    *value = tmp_value;
    return 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *    biari_decode_symbol_final():
 * \return
 *    the decoded symbol
 ************************************************************************
 */
unsigned int biari_decode_final(DecodingEnvironmentPtr dep)
{
  unsigned int range  = dep->Drange - 2;
  int value  = dep->Dvalue;
  value -= (range << dep->DbitsLeft);

  if (value < 0) 
  {
    if( range >= QUARTER )
    {
      dep->Drange = range;
      return 0;
    }
    else 
    {   
      dep->Drange = (range << 1);
      if( --(dep->DbitsLeft) > 0 )
        return 0;
      else
      {
        dep->Dvalue = (dep->Dvalue << 16) | getword( dep ); // lookahead of 2 bytes: always make sure that bitstream buffer
                                                            // contains 2 more bytes than actual bitstream
        dep->DbitsLeft = 16;
        return 0;
      }
    }
  }
  else
  {
    return 1;
  }
}


#endif //!USE_FFMPEG_CABAC
/*!
 ************************************************************************
 * \brief
 *    Initializes a given context with some pre-defined probability state
 ************************************************************************
 FFMPEG:
 part of ff_h264_init_cabac_states()
 */
void biari_init_context (int qp, BiContextTypePtr ctx, const char* ini)
{
  int pstate = ((ini[0]* qp )>>4) + ini[1];

  if ( pstate >= 64 )
  {
    pstate = imin(126, pstate);
    //ctx->state = (uint16) (pstate - 64);
    //ctx->MPS   = 1;

	*ctx = 1+((pstate - 64)<<1);
  }
  else
  {
    pstate = imax(1, pstate);
    //ctx->state = (uint16) (63 - pstate);
    //ctx->MPS   = 0;

	*ctx = 0+((63 - pstate)<<1);
  }
}

// ffmpeg's variables
byte ff_h264_mlps_state[4*64];
byte ff_h264_lps_range[4*2*64];
byte ff_h264_lps_state[2*64];
byte ff_h264_mps_state[2*64];

static const byte lps_range[64][4]= {
	{128,176,208,240}, {128,167,197,227}, {128,158,187,216}, {123,150,178,205},
	{116,142,169,195}, {111,135,160,185}, {105,128,152,175}, {100,122,144,166},
	{ 95,116,137,158}, { 90,110,130,150}, { 85,104,123,142}, { 81, 99,117,135},
	{ 77, 94,111,128}, { 73, 89,105,122}, { 69, 85,100,116}, { 66, 80, 95,110},
	{ 62, 76, 90,104}, { 59, 72, 86, 99}, { 56, 69, 81, 94}, { 53, 65, 77, 89},
	{ 51, 62, 73, 85}, { 48, 59, 69, 80}, { 46, 56, 66, 76}, { 43, 53, 63, 72},
	{ 41, 50, 59, 69}, { 39, 48, 56, 65}, { 37, 45, 54, 62}, { 35, 43, 51, 59},
	{ 33, 41, 48, 56}, { 32, 39, 46, 53}, { 30, 37, 43, 50}, { 29, 35, 41, 48},
	{ 27, 33, 39, 45}, { 26, 31, 37, 43}, { 24, 30, 35, 41}, { 23, 28, 33, 39},
	{ 22, 27, 32, 37}, { 21, 26, 30, 35}, { 20, 24, 29, 33}, { 19, 23, 27, 31},
	{ 18, 22, 26, 30}, { 17, 21, 25, 28}, { 16, 20, 23, 27}, { 15, 19, 22, 25},
	{ 14, 18, 21, 24}, { 14, 17, 20, 23}, { 13, 16, 19, 22}, { 12, 15, 18, 21},
	{ 12, 14, 17, 20}, { 11, 14, 16, 19}, { 11, 13, 15, 18}, { 10, 12, 15, 17},
	{ 10, 12, 14, 16}, {  9, 11, 13, 15}, {  9, 11, 12, 14}, {  8, 10, 12, 14},
	{  8,  9, 11, 13}, {  7,  9, 11, 12}, {  7,  9, 10, 12}, {  7,  8, 10, 11},
	{  6,  8,  9, 11}, {  6,  7,  9, 10}, {  6,  7,  8,  9}, {  2,  2,  2,  2},
};
static const byte mps_state[64]= {
	1, 2, 3, 4, 5, 6, 7, 8,
	9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,
	25,26,27,28,29,30,31,32,
	33,34,35,36,37,38,39,40,
	41,42,43,44,45,46,47,48,
	49,50,51,52,53,54,55,56,
	57,58,59,60,61,62,62,63,
};

static const byte lps_state[64]= {
	0, 0, 1, 2, 2, 4, 4, 5,
	6, 7, 8, 9, 9,11,11,12,
	13,13,15,15,16,16,18,18,
	19,19,21,21,22,22,23,24,
	24,25,26,26,27,27,28,29,
	29,30,30,30,31,32,32,33,
	33,33,34,34,35,35,35,36,
	36,36,37,37,37,38,38,63,
};

const byte ff_h264_norm_shift[512]= {
	9,8,7,7,6,6,6,6,5,5,5,5,5,5,5,5,
	4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// ffmpeg's functions
void ff_init_cabac_global_tables()
{
	int i, j;
	for(i=0; i<64; i++)
	{
		for(j=0; j<4; j++)
		{ //FIXME check if this is worth the 1 shift we save
			ff_h264_lps_range[j*2*64+2*i+0]=
			ff_h264_lps_range[j*2*64+2*i+1]= lps_range[i][j];
		}

		ff_h264_mlps_state[128+2*i+0]=
		ff_h264_mps_state[2*i+0]= 2*mps_state[i]+0;
		ff_h264_mlps_state[128+2*i+1]=
		ff_h264_mps_state[2*i+1]= 2*mps_state[i]+1;

		if( i ){
#if BRANCHLESS_CABAC_DECODER
			ff_h264_mlps_state[128-2*i-1]= 2*lps_state[i]+0;
			ff_h264_mlps_state[128-2*i-2]= 2*lps_state[i]+1;
		}else{
			ff_h264_mlps_state[128-2*i-1]= 1;
			ff_h264_mlps_state[128-2*i-2]= 0;
#else
			ff_h264_lps_state[2*i+0]= 2*lps_state[i]+0;
			ff_h264_lps_state[2*i+1]= 2*lps_state[i]+1;
		}else{
			ff_h264_lps_state[2*i+0]= 1;
			ff_h264_lps_state[2*i+1]= 0;
#endif
		}
	}
}

void ff_init_cabac_decoder(DecodingEnvironment *c, const byte *buf, int firstbyte, int *code_len)
{
	static bool init_done = false;
	if (!init_done)
	{
		ff_init_cabac_global_tables();
		init_done = true;
	}

	/*
	dep->Dcodestrm      = code_buffer;
	dep->Dcodestrm_len  = code_len;
	//dep->Dcodestrm_len = &dep->dummy;
	*dep->Dcodestrm_len = firstbyte;

	dep->Dvalue = getbyte(dep);
	dep->Dvalue = (dep->Dvalue << 16) | getword(dep); // lookahead of 2 bytes: always make sure that bitstream buffer
	// contains 2 more bytes than actual bitstream
	dep->DbitsLeft = 15;
	dep->Drange = HALF;
	*/

	c->Dcodestrm      = (byte*)buf;
	c->Dcodestrm_len  = code_len;
	*c->Dcodestrm_len = firstbyte;



	c->low =  (c->Dcodestrm[*c->Dcodestrm_len])<<18;
	c->low+=  (c->Dcodestrm[*c->Dcodestrm_len+1])<<10;
	c->low+= ((c->Dcodestrm[*c->Dcodestrm_len+2])<<2) + 2;

	*c->Dcodestrm_len += 3;

	c->range= 0x1FE;
}

void renorm_cabac_decoder_once(DecodingEnvironment *c){
	int shift= (unsigned int)(c->range - 0x100)>>31;
	c->range<<= shift;
	c->low  <<= shift;

	if(!(c->low & CABAC_MASK))
	{
		//ff_refill(c);
		c->low+= (c->Dcodestrm[*c->Dcodestrm_len]<<9) + (c->Dcodestrm[*c->Dcodestrm_len+1]<<1);
		c->low -= CABAC_MASK;
		*c->Dcodestrm_len+= CABAC_BITS/8;
	}
}

int ff_get_cabac_terminate(DecodingEnvironment *c)
{
	c->range -= 2;
	if(c->low < c->range<<(CABAC_BITS+1)){
		renorm_cabac_decoder_once(c);
		return 0;
	}else{
		//return c->bytestream - c->bytestream_start;
		return 1;
	}
}

/*
int ff_get_cabac_bypass(DecodingEnvironment *c)
{
	int range;
	c->low += c->low;

	if(!(c->low & CABAC_MASK))
	{
		//ff_refill(c);
		c->low+= (c->bytestream[0]<<9) + (c->bytestream[1]<<1);
		c->low -= CABAC_MASK;
		c->bytestream+= CABAC_BITS/8;
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
*/

/*
int ff_get_cabac(DecodingEnvironment *c, byte * const state)
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
		ff_refill2(c);
	return bit;
}
*/

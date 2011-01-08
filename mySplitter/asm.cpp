#include "asm.h"

__declspec(align(8)) static const __int64 mask1	   = 0x00ff00ff00ff00ff;
__declspec(align(8)) static const __int64 mask2	   = 0xff00ff00ff00ff00;

void isse_yuy2_to_yv12(const BYTE* src, int src_rowsize, int src_pitch, 
                    BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV,
                    int height) {

  const BYTE** dstp= new const BYTE*[4];
  dstp[0]=dstY;
  dstp[1]=dstY+dst_pitchY;
  dstp[2]=dstU;
  dstp[3]=dstV;
  int src_pitch2 = src_pitch*2;
  int dst_pitch2 = dst_pitchY*2;

  int y=0;
  int x=0;
  src_rowsize = (src_rowsize+3)/4;
  __asm {
  push ebx    // stupid compiler forgets to save ebx!!
    movq mm7,[mask2]
    movq mm4,[mask1]
    mov edx,0
    mov esi, src
    mov edi, dstp
    jmp yloop_test
    align 16
yloop:
      mov edx,0               // x counter   
      mov eax, [src_pitch]
      jmp xloop_test
      align 16
xloop:      
      movq mm0,[esi]        // YUY2 upper line  (4 pixels luma, 2 chroma)
       movq mm1,[esi+eax]   // YUY2 lower line  
      movq mm6,mm0
       movq mm2, [esi+8]    // Load second pair
      movq mm3, [esi+eax+8]
       movq mm5,mm2
      pavgb mm6,mm1         // Average (chroma)
       pavgb mm5,mm3        // Average Chroma (second pair)
      pand mm0,mm4          // Mask luma
  	    psrlq mm5, 8
      pand mm1,mm4          // Mask luma
 	     psrlq mm6, 8
      pand mm2,mm4          // Mask luma
       pand mm3,mm4         
      pand mm5,mm4           // Mask chroma
       pand mm6,mm4          // Mask chroma
   		packuswb mm0, mm2     // Pack luma (upper)
   		 packuswb mm6, mm5    // Pack chroma
   		packuswb mm1, mm3     // Pack luma (lower)     
       movq mm5, mm6        // Chroma copy
      pand mm5, mm7         // Mask V
       pand mm6, mm4        // Mask U
      psrlq mm5,8            // shift down V
   		 packuswb mm5, mm7     // Pack U 
   		packuswb mm6, mm7     // Pack V 
       mov ebx, [edi]
      mov ecx, [edi+4]
      movq [ebx+edx*2],mm0
       movq [ecx+edx*2],mm1

      mov ecx, [edi+8]
      mov ebx, [edi+12]
       movd [ecx+edx], mm6  // Store U
      movd [ebx+edx], mm5   // Store V
      add esi, 16
      add edx, 4
xloop_test:
    cmp edx,[src_rowsize]
    jl xloop
    mov esi, src
    mov edx,[edi]
    mov ebx,[edi+4]
    mov ecx,[edi+8]
    mov eax,[edi+12]
    
    add edx, [dst_pitch2]
    add ebx, [dst_pitch2]
    add ecx, [dst_pitchUV]
    add eax, [dst_pitchUV]
    add esi, [src_pitch2]

    mov [edi],edx
    mov [edi+4],ebx
    mov [edi+8],ecx
    mov [edi+12],eax
    mov edx, [y]
    mov [src],esi
    
    add edx, 2

yloop_test:
    cmp edx,[height]
    mov [y],edx
    jl yloop
    sfence
    emms
    pop ebx
  }
   delete[] dstp;
}

void isse_yuy2_to_yv12_r(const BYTE* src, int src_rowsize, int src_pitch, 
                    BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV,
                    int height) {

  const BYTE** dstp= new const BYTE*[4];
  dstp[0]=dstY;
  dstp[1]=dstY+dst_pitchY;
  dstp[2]=dstU;
  dstp[3]=dstV;
  int src_pitch2 = src_pitch*2;
  int dst_pitch2 = dst_pitchY*2;

  int y=0;
  int x=0;
  src_rowsize = (src_rowsize+3)/4;
  __asm {
  push ebx    // stupid compiler forgets to save ebx!!
    movq mm7,[mask2]
    movq mm4,[mask1]
    mov edx,0
    mov esi, src
    mov edi, dstp
    jmp yloop_test
    align 16
yloop:
      mov edx,0               // x counter   
      mov eax, [src_pitch]
      jmp xloop_test
      align 16
xloop:      
      movq mm0,[esi]        // YUY2 upper line  (4 pixels luma, 2 chroma)
       movq mm1,[esi+eax]   // YUY2 lower line  
      movq mm6,mm0
       movq mm2, [esi+8]    // Load second pair
      movq mm3, [esi+eax+8]
       movq mm5,mm2
      //pavgb mm6,mm1         // Average (chroma)
       //pavgb mm5,mm3        // Average Chroma (second pair)

      pand mm0,mm4          // Mask luma
  	    psrlq mm5, 8
      pand mm1,mm4          // Mask luma
 	     psrlq mm6, 8
      pand mm2,mm4          // Mask luma
       pand mm3,mm4         
      pand mm5,mm4           // Mask chroma
       pand mm6,mm4          // Mask chroma
   		packuswb mm0, mm2     // Pack luma (upper)
   		 packuswb mm6, mm5    // Pack chroma
   		packuswb mm1, mm3     // Pack luma (lower)     
       movq mm5, mm6        // Chroma copy
      pand mm5, mm7         // Mask V
       pand mm6, mm4        // Mask U
      psrlq mm5,8            // shift down V
   		 packuswb mm5, mm7     // Pack U 
   		packuswb mm6, mm7     // Pack V 
       mov ebx, [edi]
      mov ecx, [edi+4]
      movq [ebx+edx*2],mm0
       movq [ecx+edx*2],mm1

      mov ecx, [edi+8]
      mov ebx, [edi+12]
       movd [ecx+edx], mm6  // Store U
      movd [ebx+edx], mm5   // Store V
      add esi, 16
      add edx, 4
xloop_test:
    cmp edx,[src_rowsize]
    jl xloop
    mov esi, src
    mov edx,[edi]
    mov ebx,[edi+4]
    mov ecx,[edi+8]
    mov eax,[edi+12]
    
    add edx, [dst_pitch2]
    add ebx, [dst_pitch2]
    add ecx, [dst_pitchUV]
    add eax, [dst_pitchUV]
    add esi, [src_pitch2]

    mov [edi],edx
    mov [edi+4],ebx
    mov [edi+8],ecx
    mov [edi+12],eax
    mov edx, [y]
    mov [src],esi
    
    add edx, 2

yloop_test:
    cmp edx,[height]
    mov [y],edx
    jl yloop
    sfence
    emms
    pop ebx
  }
   delete[] dstp;
}


bool my_1088_to_YV12(const BYTE* src, int src_rowsize, int src_pitch, 
BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV,
int height) 
{
	src += src_pitch;// skip line 0

  const BYTE** dstp= new const BYTE*[4];
  dstp[0]=dstY;
  dstp[1]=dstY+dst_pitchY;
  dstp[2]=dstU;
  dstp[3]=dstV;
  int src_pitch2 = src_pitch*2;
  int dst_pitch2 = dst_pitchY*2;

  int y=0;
  int x=0;
  src_rowsize = (src_rowsize+3)/4;
  __asm {
  push ebx    // stupid compiler forgets to save ebx!!
    movq mm7,[mask2]
    movq mm4,[mask1]
    mov edx,0
    mov esi, src
    mov edi, dstp
    jmp yloop_test
    align 16
yloop:
      mov edx,0               // x counter   
      mov eax, [src_pitch]
      jmp xloop_test
      align 16
xloop:      
      movq mm0,[esi]        // YUY2 upper line  (4 pixels luma, 2 chroma)
       movq mm1,[esi+eax]   // YUY2 lower line  
      movq mm6,mm0
       movq mm2, [esi+8]    // Load second pair
      movq mm3, [esi+eax+8]
       movq mm5,mm2
      //pavgb mm6,mm1         // Average (chroma)
       //pavgb mm5,mm3        // Average Chroma (second pair)

      pand mm0,mm4          // Mask luma
  	    psrlq mm5, 8
      pand mm1,mm4          // Mask luma
 	     psrlq mm6, 8
      pand mm2,mm4          // Mask luma
       pand mm3,mm4         
      pand mm5,mm4           // Mask chroma
       pand mm6,mm4          // Mask chroma
   		packuswb mm0, mm2     // Pack luma (upper)
   		 packuswb mm6, mm5    // Pack chroma
   		packuswb mm1, mm3     // Pack luma (lower)     
       movq mm5, mm6        // Chroma copy
      pand mm5, mm7         // Mask V
       pand mm6, mm4        // Mask U
      psrlq mm5,8            // shift down V
   		 packuswb mm5, mm7     // Pack U 
   		packuswb mm6, mm7     // Pack V 
       mov ebx, [edi]
      mov ecx, [edi+4]
      movq [ebx+edx*2],mm0
       movq [ecx+edx*2],mm1

      mov ecx, [edi+8]
      mov ebx, [edi+12]
       movd [ecx+edx], mm6  // Store U
      movd [ebx+edx], mm5   // Store V
      add esi, 16
      add edx, 4
xloop_test:
    cmp edx,[src_rowsize]
    jl xloop
    mov esi, src
    mov edx,[edi]
    mov ebx,[edi+4]
    mov ecx,[edi+8]
    mov eax,[edi+12]
    
    add edx, [dst_pitch2]
    add ebx, [dst_pitch2]
    add ecx, [dst_pitchUV]
    add eax, [dst_pitchUV]
    add esi, [src_pitch2]

    mov [edi],edx
    mov [edi+4],ebx
    mov [edi+8],ecx
    mov [edi+12],eax
    mov edx, [y]
    mov [src],esi
    
    add edx, 2
	
	// my12doom's line skip
	cmp edx, 0
	jne n1
	add esi, [src_pitch]
n1:
	cmp edx, 134
	jne n2
	add esi, [src_pitch]
n2:
	cmp edx, 270//269
	jne n3
	add esi, [src_pitch]
n3:
	cmp edx, 404
	jne n4
	add esi, [src_pitch]
n4:
	cmp edx, 540//539
	jne n5
	add esi, [src_pitch]
n5:
	cmp edx, 674
	jne n6
	add esi, [src_pitch]
n6:
	cmp edx, 810//809
	jne n7
	add esi, [src_pitch]
n7:
	cmp edx, 944
	jne n8
	add esi, [src_pitch]
n8:
    mov [src],esi
	// end my12doom's line skip
	
yloop_test:
    cmp edx,[height]
    mov [y],edx
    jl yloop
    sfence
    emms
    pop ebx
  }
   delete[] dstp;

   return true;
}

/********************************
 * Progressive YUY2 to YV12
 * 
 * (c) Copyright 2003, Klaus Post
 *
 * Converts 8x2 (8 pixels, two lines) in parallel.
 * Requires mod8 pitch for output, and mod16 pitch for input.
 ********************************/

bool is_ignored_line(int line)
{
	if (line == 1 || line == 136 || line == 272 || line == 408 || line == 544 || line == 680 || line == 816 || line == 952)
		return true;
	return false;
}

bool my_1088_to_YV12_C(const BYTE* src, int src_rowsize, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height)
{
	int source_line_upper = 0;
	int source_line_lower = 2;	// 1 is skip line
	BYTE **dstp = new BYTE*[4];
	for(int y=0; y<1080; y+=2)
	{
		for (int x=0; x<src_rowsize/16; x++)
		{
			const BYTE *src_upper = src + source_line_upper * src_pitch + x*16;
			const BYTE *src_lower = src + source_line_lower * src_pitch + x*16;

			BYTE *Y = dstY + y*dst_pitchY + x*8;
			BYTE *Y2 = Y + dst_pitchY;
			BYTE *V = dstV + y/2*dst_pitchUV +x*4;
			BYTE *U = dstU + y/2*dst_pitchUV +x*4;

			dstp[0] = Y;
			dstp[1] = Y2;
			dstp[2] = V;
			dstp[3] = U;

			__asm
			{
				push ebx

				mov esi, src_upper
				mov ebx, src_lower
				//mov eax, [src_pitch]
				movq mm7,[mask2]
				movq mm4,[mask1]
				mov edi, dstp


				movq mm0,[esi]		// YUY2 upper line  (4 pixels luma, 2 chroma)
				movq mm1,[ebx]		// YUY2 lower line
				movq mm6,mm0				// mm6 = mm0

				movq mm2, [esi+8]	// Load second pair
				movq mm3, [ebx+8]
				movq mm5,mm2			// mm5 = mm2

				// now:
				// mm0/mm6, mm2/mm5
				// mm1, mm3

				pand mm0,mm4        // Mask luma
				pand mm1,mm4        // Mask luma
				pand mm2,mm4        // Mask luma
				pand mm3,mm4		// Mask luma

				psrlq mm5, 8
				psrlq mm6, 8
				pand mm5,mm4        // Mask chroma
				pand mm6,mm4        // Mask chroma

				packuswb mm0, mm2   // Pack luma (upper)
				packuswb mm1, mm3   // Pack luma (lower)
									// now mm0 = YV12 Y upper line
									//     mm1 = YV12 Y lower line

				mov ebx, [edi]
				movq [ebx], mm0
				mov ebx, [edi+4]
				movq [ebx], mm1

				packuswb mm6, mm5   // Pack chroma

				movq mm5, mm6        // Chroma copy
				pand mm5, mm7        // Mask V
				pand mm6, mm4        // Mask U
      
				psrlq mm5,8          // shift down V
				packuswb mm5, mm7    // Pack U
				packuswb mm6, mm7    // Pack V

				mov ebx, [edi+8]
				movd [ebx], mm5
				mov ebx, [edi+12]
				movd [ebx], mm6

				pop ebx
			}
		}

		_asm
		{
			sfence
			emms
		}

		// next 2 line
		source_line_upper = source_line_lower +1;
		source_line_lower = source_line_upper +1;

		if (is_ignored_line(source_line_upper))
		{
			source_line_upper ++;
			source_line_lower ++;
		}
		else if (is_ignored_line(source_line_lower))
		{
			source_line_lower ++;
		}
	}

	delete [] dstp;

	return true;
}

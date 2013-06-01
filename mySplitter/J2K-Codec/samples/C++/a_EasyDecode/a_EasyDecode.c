
//#define J2K_STATIC	// Static linking
//#define J2K_16BITS	// 16-bit static linking version

#include "..\common.h"

void main()
{                              
	J2K_Image img; int err;

	img.version = J2K_VERSION;

	J2K_Unlock("Your registration key");

	// Open file

	err = J2K_OpenFile("test.j2k", &img);
	
	if(err != J2K_SUCCESS){ printf("\n J2K-Codec error: %s.\n", J2K_GetErrStr(err)); return; }

	// Allocate buffer

	img.buffer_bpp = img.Components;
    
	if(img.Components == 1 && (img.ColorInfo & CINFO_INDEXED)) img.buffer_bpp = 3; // Only one type of palette supported - RGB

	if(img.buffer_bpp == 3) img.buffer_bpp = 4;

	if(img.Precision > 8) img.buffer_bpp *= 2; // 16-bit image

	img.buffer_pitch = img.Width * img.buffer_bpp;
                                                          	
	img.buffer = malloc(img.buffer_pitch * img.Height); //if(!img.buffer) if you want to be paranoid...

	// Decode and save

	err = J2K_Decode(&img, "");

	if(err != J2K_SUCCESS){ printf("\n J2K-Codec error: %s.\n", J2K_GetErrStr(err)); return; }

	printf("\n Image info: %ux%u, %u component(s).\n", img.Width, img.Height, img.Components);

	SaveAsBMP("decoded.bmp", &img);

	// Finalize

	free(img.buffer);

	J2K_Close(&img);
	
	printf("\n Saved as \"decoded.bmp\".\n\n"); system("pause");
}

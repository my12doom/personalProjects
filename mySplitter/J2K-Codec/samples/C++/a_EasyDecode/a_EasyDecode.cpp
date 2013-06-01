
//#define J2K_STATIC	// Static linking
//#define J2K_16BITS	// 16-bit static linking version

#include "..\common.h"

void main()
{                              
	J2K_CImage img;

	J2K_Unlock("Your registration key");

	int a = GetTickCount();

	if(img.easyDecode("test.j2k") != J2K_SUCCESS){ printf("\n J2K-Codec error: %s.\n", img.errorStr); return; }

	int b = GetTickCount();

	printf("\n Image info: %ux%u, %u component(s).\n", img.Width, img.Height, img.Components);

	SaveAsBMP("decoded.bmp", img);

	printf("\n Saved as \"decoded.bmp\". Decoding time: %u msec\n\n", b-a);  system("pause");
}

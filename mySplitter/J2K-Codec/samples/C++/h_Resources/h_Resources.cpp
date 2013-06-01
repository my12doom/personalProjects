
//#define J2K_STATIC	// Static linking
//#define J2K_16BITS	// 16-bit static linking version

#include "..\common.h"

#include "resource.h"

void main()
{
	J2K_CImage img;

	if(img.easyDecode(IDR_RCDATA1) != J2K_SUCCESS){ printf("\n J2K-Codec error: %s.\n", img.errorStr); return; }

	printf("\n Image info: %ux%u, %u component(s).\n", img.Width, img.Height, img.Components);

	SaveAsBMP("decoded.bmp", img);

	printf("\n Saved as \"decoded.bmp\".\n\n");  system("pause");
}

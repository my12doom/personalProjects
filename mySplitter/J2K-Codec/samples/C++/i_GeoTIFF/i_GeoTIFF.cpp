
//#define J2K_STATIC	// Static linking
//#define J2K_16BITS	// 16-bit static linking version

#include "..\common.h"

void main(int ArgC, char **ArgV)
{
	// Check the library version

	int ver = J2K_GetVersion();

	if(ver < 0x200000) return; // This way you can make sure the version is at least 2.0 (or any other required)

	printf("\n J2K-Codec version: %x.%x (build %u)\n", ver>>20, (ver>>16)&0x0F, ver&0xFFFF); 


	// Remove the demo version restrictions

	J2K_Unlock("Your registration key");


	// Open the file

	J2K_CImage img;

	if(img.open("utm.jp2") != J2K_SUCCESS){ printf("\n J2K-Codec error: %s.\n", img.errorStr); return; }

	printf("\n Image info: %ux%u, %u component(s).\n", img.Width, img.Height, img.Components);


	// Let's extract and save the embedded GeoTIFF image

	int md_type, md_size; unsigned char *meta_data;

	for(int no=0; ; no++) 
	{
		// Go through all metadata looking for the GeoTIFF block

		int err = img.getMetadata(no, &md_type, &meta_data, &md_size);

		if(err != J2K_SUCCESS){ printf("\n getMetadata() error: %s.\n", img.errorStr);  return;  }

		if(!meta_data) break; // End of enumeration

		if(md_type == JP2_METADATA_COMMENT_STR)
		{
			printf("\n Comment: \"%s\"\n", meta_data);
		}

		if(md_type == JP2_METADATA_GEOTIFF)
		{
			printf("\n GeoTIFF found! Saving as 'geo.tif'...");

			// Save it

			FILE *f=fopen("geo.tif", "wb");

			if(f){ fwrite(meta_data, md_size, 1, f); fclose(f); printf(" done.\n"); }
		}
	}


	// Let's decode the image now

	if(img.decode() != J2K_SUCCESS) printf("\n Decoding error: %s.\n", img.errorStr); 
	else
	{
		// Finally save the decoded image as BMP file

		SaveAsBMP("decoded.bmp", img);

		printf("\n Image decoded.\n\n");
	}

	system("pause");
}

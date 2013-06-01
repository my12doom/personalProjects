
#ifndef __J2K_LIBRARY__
#define __J2K_LIBRARY__

#define J2K_VERSION	0x22 // compatibility version

#ifndef J2K_CODEC_API
	#ifdef __cplusplus
		#define J2K_CODEC_API(ret) extern "C" ret __stdcall
	#else
		#define J2K_CODEC_API(ret) extern ret __stdcall
	#endif
#endif

#ifndef __APPLE__
	#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
	#include <windows.h>
#else
	#define TCHAR char
#endif	

#include <stdio.h>

enum J2K_API_Error
{
	J2K_SUCCESS,
	J2K_NOT_ENOUGH_MEMORY,
	J2K_CORRUPTED_DATA,
	J2K_CANT_READ_DATA,
	J2K_INVALID_ARGUMENT,
	J2K_CANCELED,
	J2K_CANT_OPEN_FILE,
	J2K_OPTION_UNRECOGNIZED,
	J2K_NO_SUCH_TILE,
	J2K_NO_SUCH_RESOLUTION,
	J2K_BPP_TOO_SMALL,
	J2K_NOT_PART1_FORMAT,
	J2K_IMAGE_IS_TOO_LARGE,
	J2K_TOO_MANY_RES_LEVEL,
	J2K_TOO_LARGE_CODEBLOCKS,
	J2K_LAZINESS_NOT_SUPPORTED,
	J2K_VCAUSAL_NOT_SUPPORTED,
	J2K_TOO_MANY_COMPONENTS,
	J2K_BAD_COMPONENT_PRECISION,
	J2K_ONLY_UNSIG_COMPONENTS,
	J2K_DOWNSAMPLED_COMPONENTS,
	J2K_ROI_NOT_SUPPORTED,
	J2K_PROGR_CHANGE_NOT_SUP,
	J2K_64BIT_BOXES_NOT_SUP,
	J2K_INTERNAL_ERROR,

	J2K_MAX_ERRORS, // Total number of errors

	J2K_MAX_ERROR_LEN = 60 // Max error string length
};

/*/////////////////////////////////////////////////////////////////////////////////////////
// Main structure, representing a J2K image
//
// [out] means that the field is returned from J2K-Codec
// [in]  means that the field is set and passed to J2K-Codec
/////////////////////////////////////////////////////////////////////////////////////////*/

#pragma pack(4)

typedef struct 
{
	int Width, Height;		// [out] Width and height of the image in pixels (by default).
							//       If "resolution" is set or tiles were selected - this will be set to the correct size

	int Components;			// [out] The number of components in the image
	int Resolutions;		// [out] Number of resolution levels for selected tiles (by default - the whole image is selected)
	int hTiles, vTiles;		// [out] The number of tiles in horizontal and vertical directions
	int Precision;			// [out] Component precision in bits. All components must have the same precision.
	int FileType;			// [out] The image file type (0 = Lossy J2K, 1 = Lossy JP2, 2 = Lossless J2K, 3 = Lossless JP2)
	int ColorInfo;			// [out] Color information (lowest 2 bits - MCT type, 3rd bit - indexed or not, highest 28 bits - color info) - see below

	int version;			// [in]  J2K-Codec version your program was developed for. Use J2K_VERSION to initialize this field
	int resolution;			// [in]  Resolution level. If it is >= Resolutions it will be set to the maximum available level

	unsigned char *buffer;	// [in]  Destination buffer
	int buffer_bpp;			// [in]  Destination buffer bytes-per-pixel value
	int buffer_pitch;		// [in]  Destination buffer pitch (distance in bytes between two vertical pixels)

	int reserved;			// Reserved

	void *j2k;				// Pointer to the internal class for this image

} J2K_Image;

#pragma pack()

/*/////////////////////////////////////////////////////////////////////////////////////////
// Color information
//  Lowest 2 bits   - Multi-Component Transform type (None, Reversible, Irreversible)
//  Highest 29 bits - Color space (sRGB, Grayscale, YCC, ICC, RICC)
/////////////////////////////////////////////////////////////////////////////////////////*/

#define CINFO_MCT_NONE  0 /* MCT not applied */
#define CINFO_MCT_REV   1 /* Reversible */
#define CINFO_MCT_IRREV 2 /* Irreversible */

#define CINFO_INDEXED   4 /* Palette present */


/* Bi-level. Each image sample is one bit: 0 = white, 1 = black. */

#define CINFO_BI		 0 


/* YCbCr(1). This is a format often used for data that originated from a video signal.
The colourspace is based on Recommendation ITU-R BT.709-4. The valid ranges of the YCbCr
components in this space is limited to less than the full range that could be represented given
an 8-bit representation. Recommendation ITU-R BT.601-5 specifies these ranges as well as defines
a 3x3 matrix transformation that can be used to convert these samples into RGB. */

#define CINFO_YCC1		 1 


/* YCbCr(2). This is the most commonly used format for image data that was originally captured
in RGB (uncalibrated format). The colourspace is based on Recommendation ITU-R BT.601-5.
The valid ranges of the YCbCr components in this space is [0,255] for Y, and [-128,127] for Cb
and Cr (stored with an offset of 128 to convert the range to [0,255]). These ranges are different
from the ones defined in Recommendation ITU-R BT.601-5. Recommendation ITU-R BT.601-5 specifies
a 3x3 matrix transform that can be used to convert these samples into RGB. */

#define CINFO_YCC2		 3


/* YCbCr(3). This is a format often used for data that originated from a video signal.
The colourspace is based on Recommendation ITU-R BT.601-5. The valid ranges of the YCbCr
components in this space is limited to less than the full range that could be represented given
an 8-bit representation. Recommendation ITU-R BT.601-5 specifies these ranges as well as defines
a 3x3 matrix transform that can be used to convert these samples into RGB */

#define CINFO_YCC3		 4


/* PhotoYCC. This is the colour encoding method used in the Photo CD" system.
The colourspace is based on Recommendation ITU-R BT.709 reference primaries. Recommendation
ITU-R BT.709 linear RGB image signals are transformed to non-linear R'G'B' values to YCC corresponding
to Recommendation ITU-R BT.601-5. Details of this encoding method can be found in Kodak Photo CD
products, A Planning Guide for Developers, Eastman Kodak Company, Part No. DC1200R and also in Kodak
Photo CD Information Bulletin PCD045. */

#define CINFO_PHOTO_YCC	 9


/* CMY. The encoded data consists of samples of Cyan, Magenta and Yellow samples, directly suitable
for printing on typical CMY devices. A value of 0 shall indicate 0% ink coverages, whereas a value
of 2BPS-1 shall indicate 100% ink coverage for a given component sample. */

#define CINFO_CMY		11


/* CMYK. As CMY above, except that there is also a black (K) ink component. Ink coverage is defined as above. */

#define CINFO_CMYK		12


/* YCCK. This is the result of transforming original CMYK type data by computing R=(2BPS-1)-C,
G=(2 BPS-1)-M, and B=(2 BPS-1)-Y, applying the RGB to YCC transform specified for YCbCr(2) above,
and then recombining the result with the unmodified K-sample. This transform is intended to be
the same as that specified in Adobe Postscript. */

#define CINFO_YCCK		13


/* CIELab. The CIE 1976 (L*a*b*) colourspace. A colourspace defined by the CIE (Commission
Internationale de l Eclairage), having approximately equal visually perceptible differences
between equally spaced points throughout the space. The three components are L*, or Lightness,
and a* and b* in chrominance. For this colourspace, additional Enumerated parameters are specified
in the EP field as specified in Annex L.9.4.4.1 */

#define CINFO_LAB		14


/* sRGB as defined by IEC 61966-2-1 */

#define CINFO_SRGB		16


/* Greyscale. A greyscale space where image luminance is related to code values using the sRGB
non-linearity given in Eqs. (2) through (4) of IEC 61966-2-1 */

#define CINFO_GREY		17


/* Bi-level(2).This value shall be used to indicate bi-level images. Each image sample is one
bit: 1 = white, 0 = black. */

#define CINFO_BI2		18


/* CIEJab. As defined by CIE Colour Appearance Model 97s, CIE Publication 131. For this colourspace,
additional Enumerated parameters are specified in the EP field as specified in Annex L.9.4.4.2 */

#define CINFO_JAB		19


/* e-sRGB.As defined by PIMA 7667 */

#define CINFO_ESRGB		20


/* ROMM-RGB. As defined by PIMA 7666 */

#define CINFO_ROMMRGB	21


/* sRGB based YCbCr */

#define CINFO_SRGB_YCC	22


/* YPbPr(1125/60). This is the well known color space and value definition for the HDTV (1125/60/2:1)
system for production and international program exchange specified by Recommendation ITU-R BT.709-3.
The Recommendation specifies the color space conversion matrix from RGB to YPbPr(1125/60) and
the range of values of each component. The matrix is different from the 1250/50 system.
In the 8 bit/component case, the range of values of each component is [1,254], the black level
of Y is 16, the achromatic level of Pb/Pr is 128, the nominal peak of Y is 235, and the nominal
extremes of Pb/Pr are 16 and 240. In the 10-bit case, these values are defined in a similar manner. */

#define CINFO_YPP1125	23


/* YPbPr(1250/50). This is the well known color space and value definition for the HDTV (1250/50/2:1)
system for production and international program exchange specified by Recommendation ITU-R BT.709-3.
The Recommendation specifies the color space conversion matrix from RGB to YPbPr(1250/50) and
the range of values of each component. The matrix is different from the 1125/60 system.
In the 8 bit/component case, the range of values of each component is [1,254], the black level
of Y is 16, the achromatic level of Pb/Pr is 128, the nominal peak of Y is 235, and the nominal
extremes of Pb/Pr are 16 and 240. In the 10-bit case, these values are defined in a similar manner. */

#define CINFO_YPP1250	24


/* ICC profile */

#define CINFO_ICC	 16384


/* Restricted ICC profile */

#define CINFO_RICC	 32768


/*/////////////////////////////////////////////////////////////////////////////////////////*/
// These two functions below are optional. They are only for a couple of customers
// for whom the usual constructors do not work for some weird reason...
/////////////////////////////////////////////////////////////////////////////////////////*/

J2K_CODEC_API(void) J2K_InitLibrary();
J2K_CODEC_API(void) J2K_CloseLibrary();


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_GetVersion()
//
// Returns codec version and build number in the 0x205678 form, where 20 is a version (2.0)
// and 0x5678 is a build number.
/////////////////////////////////////////////////////////////////////////////////////////*/

J2K_CODEC_API(int) J2K_GetVersion();


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_GetErrorStr()
//                                                                                      
// Returns the textual description of an error by the error code.
// If the code is < 0 or >= J2K_MAX_ERRORS, an empty string is returned.
/////////////////////////////////////////////////////////////////////////////////////////*/

#ifdef _UNICODE
	#define J2K_GetErrorStr J2K_GetErrorStrW
#else
	#define J2K_GetErrorStr J2K_GetErrorStrA
#endif

J2K_CODEC_API(void) J2K_GetErrorStrA(int code,  char   *str);
J2K_CODEC_API(void) J2K_GetErrorStrW(int code, wchar_t *str);


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_Unlock()
//
// This function unlocks the full functionality of J2K-Codec. After you have purchased your
// personal registration key, you need to pass it to this function.
/////////////////////////////////////////////////////////////////////////////////////////*/

#ifdef _UNICODE
	#define J2K_Unlock J2K_UnlockW
#else
	#define J2K_Unlock J2K_UnlockA
#endif

J2K_CODEC_API(void) J2K_UnlockA(const  char   *key);
J2K_CODEC_API(void) J2K_UnlockW(const wchar_t *key);


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_StartLogging() / J2K_StopLogging()
//                                                                                      
// These functions control the debug messages logging.
//
// J2K_StartLogging() returns J2K_SUCCESS if there was no error.
// NOTES: 
//  1. Log-file name is 'j2k-codec.log'
//  2. The performance will degrade significantly if the logging is on.
/////////////////////////////////////////////////////////////////////////////////////////*/

J2K_CODEC_API(int) J2K_StartLogging();
J2K_CODEC_API(void) J2K_StopLogging();


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_Open()
//                                                                                      
// Returns J2K_SUCCESS or error code if function failed.
/////////////////////////////////////////////////////////////////////////////////////////*/

typedef struct
{
	int  (__stdcall *seek)  (void *data_source, int offset);
	int  (__stdcall *read)  (void *ptr, int size, void *data_source);
	void (__stdcall *close) (void *data_source);
}
J2K_Callbacks;

J2K_CODEC_API(int) J2K_OpenFileA(const  char   *filename, J2K_Image *image);
J2K_CODEC_API(int) J2K_OpenFileW(const wchar_t *filename, J2K_Image *image);

J2K_CODEC_API(int) J2K_OpenMemory(unsigned char *src_buffer, int src_size, J2K_Image *image);

J2K_CODEC_API(int) J2K_OpenCustom(void *data_source, J2K_Callbacks *cbs, J2K_Image *image);

#ifdef _UNICODE
	#define J2K_OpenFile J2K_OpenFileW
#else
	#define J2K_OpenFile J2K_OpenFileA
#endif

#ifdef __cplusplus
	inline int J2K_Open(const TCHAR *filename, J2K_Image *image){ return J2K_OpenFile(filename, image); }
	inline int J2K_Open(unsigned char *src_buffer, int src_size, J2K_Image *image){ return J2K_OpenMemory(src_buffer, src_size, image); }
	inline int J2K_Open(void *data_source, J2K_Callbacks *cbs, J2K_Image *image){ return J2K_OpenCustom(data_source, cbs, image); }
#endif


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_GetInfo()
//                                                                                      
// Gets various information about the image into J2K_Image structure.
// Returns J2K_SUCCESS or error code if function failed.
/////////////////////////////////////////////////////////////////////////////////////////*/

J2K_CODEC_API(int) J2K_GetInfo(J2K_Image *image);


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_GetMetadata()
//                                                                                      
// Gets a metadata block, embedded into the JP2 image. If the "data" variable set to NULL
// after the call then there are no more metadata blocks. Enumeration starts with zero.
// Returns J2K_SUCCESS or error code if function failed.
/////////////////////////////////////////////////////////////////////////////////////////*/

enum JP2_META_DATA_TYPES
{
	JP2_METADATA_COMMENT_STR,
	JP2_METADATA_COMMENT_BIN,
	JP2_METADATA_GEOTIFF,
	JP2_METADATA_XML,
	JP2_METADATA_URL,
	JP2_METADATA_PAL,
	JP2_METADATA_COLR,
	JP2_METADATA_ICC,
	JP2_METADATA_UNKNOWN 
};

J2K_CODEC_API(int) J2K_GetMetadata(J2K_Image *image, int no, int *type, unsigned char **data, int *size);


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_SelectTiles()
//                                                                                      
// Selects or unselects a tile or a tile range, depending on 'action'.
// If action!=0 then the tiles are selected for decoding, otherwise - unselected.
// The range is defined by start and end tile numbers (inclusive) in the raster order.
// If end_tile==-1 then the max tile number will be used instead.
// Returns J2K_SUCCESS or error code if the function failed.
/////////////////////////////////////////////////////////////////////////////////////////*/

J2K_CODEC_API(int) J2K_SelectTiles(J2K_Image *image, int start_tile, int end_tile, int action);


/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_Decode()
//
// Decodes the image previously opened with J2K_Open().
/////////////////////////////////////////////////////////////////////////////////////////*/


#ifdef _UNICODE
	#define J2K_Decode J2K_DecodeW
#else
	#define J2K_Decode J2K_DecodeA
#endif

#ifdef __cplusplus
	J2K_CODEC_API(int) J2K_DecodeA(J2K_Image *image, const  char   *options=NULL);
	J2K_CODEC_API(int) J2K_DecodeW(J2K_Image *image, const wchar_t *options=NULL);
#else
	J2K_CODEC_API(int) J2K_DecodeA(J2K_Image *image, const  char   *options);
	J2K_CODEC_API(int) J2K_DecodeW(J2K_Image *image, const wchar_t *options);
#endif



/*/////////////////////////////////////////////////////////////////////////////////////////
// J2K_Close()
//                                                                                      
// Destroys the image previously created by J2K_Open(). All images must be closed using
// this function to avoid memory leaks.
/////////////////////////////////////////////////////////////////////////////////////////*/

J2K_CODEC_API(int) J2K_Close(J2K_Image *image);



/*/////////////////////////////////////////////////////////////////////////////////////////
//
// C++ Wrapper
//
/////////////////////////////////////////////////////////////////////////////////////////*/

#ifdef __cplusplus

#pragma pack(4)

class J2K_CImage : public J2K_Image
{
 protected:

	bool allocated_inside;

	J2K_Image* img(){ return (J2K_Image*)this; }

	int error(int code){ if(code != J2K_SUCCESS) J2K_GetErrorStr(code, errorStr); else errorStr[0] = 0;  return code; }

 public:

	TCHAR errorStr[J2K_MAX_ERROR_LEN];


	J2K_CImage(){ version = J2K_VERSION; j2k = NULL; buffer = NULL; errorStr[0] = 0; allocated_inside = false; }


	int open(TCHAR *filename){ close(); return error(J2K_Open(filename, img())); }

	int open(unsigned char *src_buffer, int src_size){ close(); return error(J2K_Open(src_buffer, src_size, img())); }

	int open(void *data_source, J2K_Callbacks *cbs){ close(); return error(J2K_Open(data_source, cbs, img())); }

 #ifndef __APPLE__
	int open(int resource_no)
	{
		HRSRC	rh =   FindResource(NULL, MAKEINTRESOURCE(resource_no), RT_RCDATA); if(!rh) return error(J2K_CANT_OPEN_FILE);
		HGLOBAL	id =   LoadResource(NULL, rh); if(!id) return error(J2K_CANT_OPEN_FILE);
		int		sz = SizeofResource(NULL, rh); if(!sz) return error(J2K_CANT_OPEN_FILE);

		return open((unsigned char*)LockResource(id), sz);
	}
 #endif

	int setResolution(int resolution){ this->resolution = resolution; return error(J2K_GetInfo(img())); }

	int getMetadata(int no, int *type, unsigned char **data, int *size){ return error(J2K_GetMetadata(img(), no, type, data, size)); }

	int selectTiles(int start_tile, int end_tile, bool action){ return error(J2K_SelectTiles(img(), start_tile, end_tile, int(action))); }


	int decode(TCHAR *options = NULL)
	{
		int err = J2K_GetInfo(img()); if(err != J2K_SUCCESS) return error(err);  // update info in case the resolution level has changed

		if(!buffer) // allocate memory inside
		{
			if(buffer_bpp < Components)
			{
				buffer_bpp = Components;
		    
		        if(Components == 1 && (ColorInfo & CINFO_INDEXED)) buffer_bpp = 3; // Only one type of palette supported - RGB

				if(buffer_bpp == 3) buffer_bpp = 4;

				if(Precision > 8) buffer_bpp *= 2; // 16-bit image
			}

			int line_width = Width * buffer_bpp;

			if(buffer_pitch < line_width) buffer_pitch = line_width;
                                                          	
			int buffer_size = buffer_pitch * Height;

			buffer = new unsigned char[buffer_size]; if(!buffer) return error(J2K_NOT_ENOUGH_MEMORY); else allocated_inside = true;
		}
		else allocated_inside = false;

		return error(J2K_Decode(img(), options));
	}
	//--------

	int easyDecode(TCHAR *filename, int resolution = 0, TCHAR *options = NULL)
	{
		int err = open(filename); if(err == J2K_SUCCESS){ this->resolution = resolution; return error(decode(options)); } else return error(err);
	}

	int easyDecode(unsigned char *data, int data_size, int resolution = 0, TCHAR *options = NULL)
	{
		int err = open(data, data_size); if(err == J2K_SUCCESS){ this->resolution = resolution; return error(decode(options)); } else return error(err);
	}
	int easyDecode(TCHAR *filename, TCHAR *options, int resolution = 0)
	{
		return easyDecode(filename, resolution, options);
	}

 #ifndef __APPLE__
	int easyDecode(int resource_no, int resolution = 0, TCHAR *options = NULL)
	{
		int err = open(resource_no); if(err == J2K_SUCCESS){ this->resolution = resolution; return error(decode(options)); } else return error(err);
	}

	int easyDecode(int resource_no, TCHAR *options, int resolution = 0)
	{
		return easyDecode(resource_no, resolution, options);
	}
 #endif
	//--------

	void close()
	{
		if(buffer){ if(allocated_inside) delete buffer; buffer = NULL; }

		if(j2k){ J2K_Close(img()); j2k = NULL; }
	}

	~J2K_CImage(){ close(); }
};

#pragma pack()

#endif // __cplusplus

/* ///////////////////////////////////////////////////////////////////////////////////// */

#endif

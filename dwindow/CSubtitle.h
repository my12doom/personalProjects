#include <windows.h>
#include "..\PGS\PGS.h"
#include "..\..\projects\srt_parser\srt_parser.h"

typedef struct _rendered_subtitle
{
	// pos and size
	// range 0.0 - 1.0
	double left;
	double top;
	double width;
	double height;
	double aspect;

	// data size in pixel
	int width_pixel;
	int height_pixel;

	// stereo info...
	double delta;

	// pixel type and data
	enum
	{
		pixel_type_RGB,		// ARGB32
		pixel_type_YUV,		// AYUV 4:4:4
	} pixel_type;

	BYTE *data;				// if this is not NULL, you should free it after use
} rendered_subtitle;

class CSubtitleRenderer
{
public:
	virtual HRESULT load_file(wchar_t *filename)=0;	//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end)=0;
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1)=0;			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset()=0;
	virtual HRESULT seek()=0;	// to provide dshow support
};

class CPGSRenderer : public CSubtitleRenderer
{
	virtual HRESULT load_file(wchar_t *filename);	//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();		// to provide dshow support
};

class CsrtRenderer : public CSubtitleRenderer
{
	virtual HRESULT load_file(wchar_t *filename);	//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();		// to provide dshow support
};
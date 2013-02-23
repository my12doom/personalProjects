#pragma once

#ifndef INT64_C
#define INT64_C __int64
#endif

#ifndef UINT64_C
#define UINT64_C unsigned __int64
#endif

extern "C"
{
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
#include <Windows.h>
#include "CCritSec.h"

class screenshoter
{
public:
	screenshoter();
	~screenshoter();

	int open_file(const wchar_t*file);
	int seek(int64_t position);
	int set_out_format(PixelFormat ptype, int width, int height);
	int get_one_frame(void *out, int pitch);
	int close(){return close_codec_and_format();}

	double m_duration;
	int m_width;
	int m_height;

protected:
	CCritSec cs;


	int m_out_width;
	int m_out_height;

	AVCodecContext *m_input_video_codec_ctx;
	AVFormatContext *m_input_format;
	SwsContext * sws_ctx;
	int m_video_stream_id ; // = -1


	// helper functions / variables
	int open_codec();
	int close_codec();
	int close_codec_and_format();
	void ThrowError(const char*){}
	void ThrowError(const wchar_t*){}
};
#include "screenshotor.h"
#include <atlbase.h>

CCritSec g_ff_cs;
// #pragma comment(lib, "avutil.lib")
// #pragma comment(lib, "swscale.lib")
// #pragma comment(lib, "avformat.lib")

#pragma comment( lib, "libgcc.a")
#pragma comment( lib, "libmingwex.a")
#pragma comment( lib, "libavcodec.a")
#pragma comment( lib, "libavformat.a")
#pragma comment( lib, "libavutil.a")
#pragma comment( lib, "libswscale.a")
#pragma comment( lib, "libz.a")
#pragma comment( lib, "wsock32.lib")

screenshoter::screenshoter()
:sws_ctx(NULL)
{
}

screenshoter::~screenshoter()
{
	close_codec_and_format();
}

int screenshoter::open_file(const wchar_t*file)
{
	m_input_format = NULL;
	m_input_video_codec_ctx = NULL;

	CAutoLock lck(&cs);
	int rtn;

	CAutoLock lck2(&g_ff_cs);
	av_register_all();
	avformat_network_init();


	// open
	USES_CONVERSION;
	char filename[1024];
	WideCharToMultiByte(CP_UTF8, NULL, file, -1, filename, 1024, NULL, NULL);

	//const char * filename = W2A(file);
	if ((rtn = avformat_open_input(&m_input_format, filename, NULL, NULL)) < 0)
	{
		ThrowError("unsupported container format");
		return -1;
	}


	// find stream
	if (avformat_find_stream_info(m_input_format, NULL) < 0)
	{
		ThrowError("no streams found.");
		return -1;
	}

	// Find the video stream and audio stream
	m_video_stream_id = -1;
	for(unsigned int i=0; i<m_input_format->nb_streams; i++)
	{
		if(m_video_stream_id == -1 && m_input_format->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
			m_video_stream_id=i;
	}

	if (m_video_stream_id < 0)
	{
		close_codec_and_format();
		ThrowError("no video streams found.");
	}

	printf("Stream using: %d - %d\r\n", m_video_stream_id, -1);

	// show it
	av_dump_format(m_input_format, 0, filename, 0);

	// open video codec
	open_codec();

	m_duration = (double)m_input_format->duration / AV_TIME_BASE;
	if (m_duration <= 0)
		ThrowError("can't detect duration of this file!");

	// video
	if (m_video_stream_id >= 0)
	{
		m_width = m_input_video_codec_ctx->width;
		m_height = m_input_video_codec_ctx->height;
// 		vi.fps_numerator = m_input_format->streams[m_video_stream_id]->r_frame_rate.num;
// 		vi.fps_denominator = m_input_format->streams[m_video_stream_id]->r_frame_rate.den;
// 		vi.num_frames = (int)(m_duration * vi.fps_numerator / vi.fps_denominator);
// 		vi.pixel_type = VideoInfo::CS_YV12;
	}

	set_out_format(PIX_FMT_YUV420P, m_width, m_height);

	return 0;
}

int screenshoter::open_codec()
{
	CAutoLock lck(&cs);
	close_codec();

	if (m_video_stream_id >=0 )
	{
		m_input_video_codec_ctx = m_input_format->streams[m_video_stream_id]->codec;

		AVCodec * codec = avcodec_find_decoder(m_input_video_codec_ctx->codec_id);
		if (!codec)
		{
			close_codec_and_format();
			ThrowError("unsupported video codec");
		}

		if(avcodec_open(m_input_video_codec_ctx, codec)<0)
		{
			close_codec_and_format();
			ThrowError("error opening video codec");
		}
	}

	return 0;
}

int screenshoter::close_codec()
{
	CAutoLock lck(&cs);
	if (m_input_video_codec_ctx)
	{
		avcodec_close(m_input_video_codec_ctx);
		m_input_video_codec_ctx = NULL;
	}

	return 0;
}

int screenshoter::close_codec_and_format()
{
	CAutoLock lck(&cs);

	close_codec();
	if (m_input_format)
	{
		avformat_free_context(m_input_format);
		m_input_format = NULL;
	}

	return 0;
}

int screenshoter::set_out_format(PixelFormat ptype, int width, int height)
{
	m_out_width = width;
	m_out_height = height;

	if (!m_input_format || !m_input_video_codec_ctx)
		return -1;

	sws_ctx = sws_getCachedContext(sws_ctx, m_input_video_codec_ctx->width, m_input_video_codec_ctx->height, m_input_video_codec_ctx->pix_fmt, 
		width, height, ptype, SWS_BILINEAR, NULL, NULL, NULL);

	return 0;
}

int screenshoter::seek(int64_t position)		// milliseconds
{
	if (!m_input_format || !m_input_video_codec_ctx)
		return -1;

	static AVRational QQQQQQQQ = {1, AV_TIME_BASE};		//AV_TIME_BASE_Q

	int64_t seek_target = int64_t(AV_TIME_BASE) * position / 1000;
	int stream_idx = m_video_stream_id;
	if (stream_idx != -1)
		seek_target = av_rescale_q(seek_target, QQQQQQQQ, m_input_format->streams[stream_idx]->time_base);

	int o = av_seek_frame(m_input_format, stream_idx, seek_target, 0);
	avcodec_flush_buffers(m_input_video_codec_ctx);

	return o;
}

int screenshoter::get_one_frame(void *out, int pitch)
{
	if (!m_input_format || !m_input_video_codec_ctx)
		return -1;

	bool got_frame = false;

	AVPacket packet;
	AVFrame *ResizedYUV8Frame = avcodec_alloc_frame();
	AVFrame *frame = avcodec_alloc_frame();
	while (!got_frame)
	{
		// demux a packet
		memset(&packet, 0, sizeof(packet));
		int av_read_frame_result = 0;
		if((av_read_frame_result = av_read_frame(m_input_format, &packet))<0)
		{
			if (av_read_frame_result == AVERROR_EOF)		// End of File
				return AVERROR_EOF;


			char tmp[500];
			av_strerror(av_read_frame_result, tmp, 500);

			// TODO: log


			Sleep(1);
			continue;
		}


		// decode a packet
		int frameFinished;

		// Decode the next chunk of data
		int bytesDecoded=avcodec_decode_video2(m_input_video_codec_ctx, frame, &frameFinished, &packet);

		// Was there an error?
		if(bytesDecoded < 0)
		{
		}

		// Did we finish the current frame? Then we can push a frame to decoded queue
		if(frameFinished)
		{
			// convert
			ResizedYUV8Frame->data[0] = (uint8_t *)out;
			ResizedYUV8Frame->data[1] = (uint8_t *)out + pitch * m_height;
			ResizedYUV8Frame->data[2] = (uint8_t *)out + pitch * m_height*5/4;
			ResizedYUV8Frame->width = m_out_width;
			ResizedYUV8Frame->height = m_out_height;
			ResizedYUV8Frame->linesize[0] = pitch;
			ResizedYUV8Frame->linesize[1] = pitch/2;
			ResizedYUV8Frame->linesize[2] = pitch/2;

			sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, ResizedYUV8Frame->data, ResizedYUV8Frame->linesize);

			got_frame = true;
		}

		av_free_packet(&packet);
	}

	av_free(frame);
	av_free(ResizedYUV8Frame);

	return 0;
}
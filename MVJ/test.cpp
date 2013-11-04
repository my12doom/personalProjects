#include <stdio.h>
#include <stdint.h>
#include <Windows.h>


#ifndef INT64_C
#define INT64_C __int64
#endif

#ifndef UINT64_C
#define UINT64_C unsigned __int64
#endif

extern "C"
{
#include <x264.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#pragma comment(lib, "libx264.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libavcodec.a")
#pragma comment(lib, "libavutil.a")
#pragma comment(lib, "libz.a")

#define INBUF_SIZE 512000

int write_picture(AVFrame *picture, FILE* out)
{
	for(int y=0; y<picture->height; y++)
		fwrite(picture->data[0] + y * picture->linesize[0], 1, picture->width, out);
	for(int y=0; y<picture->height/2; y++)
		fwrite(picture->data[1] + y * picture->linesize[1], 1, picture->width/2 , out);
	for(int y=0; y<picture->height/2; y++)
		fwrite(picture->data[2] + y * picture->linesize[2], 1, picture->width/2 , out);

	fflush(out);
	return 0;
}

int read_a_nal(FILE*fin, uint8_t *buf, int max_buf)
{
	int ppp = ftell(fin);
	unsigned data_read = fread(buf, 1, max_buf, fin);

	if (data_read == 0)
		return 0;
	for(unsigned int i=4; i<data_read-3; i++)
		if (*(unsigned int *)(buf+i) == 0x01000000)
		{
			int delta = (long)i-max_buf;
			int pos1 = ftell(fin);
			fseek(fin, delta, SEEK_CUR);
			fseek(fin, ppp+i, SEEK_SET);
			int pos = ftell(fin);
			return i;
		}

	return data_read;
}

int decoding()
{
	uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];


	// detect for MultiView SEI message
	FILE *detect = fopen("Z:\\still.h264", "rb");
	int nal_size;
	int nal_count = 100;
	bool is_multiview_h264 = false;
	while((nal_size = read_a_nal(detect, inbuf, INBUF_SIZE)) && nal_count--)
	{
		if (nal_size <= 22)
			continue;
		if ((inbuf[4] & 0x1f) == 6)	// SEI
		{
			if (inbuf[5] == 5 && inbuf[6] >= 16)	// SEI type 5
			{
				if (memcmp(inbuf+7, "MultiViewH264", strlen("MultiViewH264")) == 0)
				{
					is_multiview_h264 = true;
					break;
				}
			}
		}
	}
	fclose(detect);

	// decoding

	avcodec_register_all();
	AVCodec * codec = avcodec_find_decoder(CODEC_ID_H264);
	AVCodecContext *c = avcodec_alloc_context3(codec);

	AVPacket packet;
	av_init_packet(&packet);
    memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);
    AVFrame *picture = avcodec_alloc_frame();
	if(codec->capabilities&CODEC_CAP_TRUNCATED)
		c->flags|= CODEC_FLAG_TRUNCATED;
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		return(1);
	}


	FILE *fin = fopen("Z:\\still.h264", "rb");
	FILE *fout = fopen("Z:\\test_dec.yuv", "wb");
	while (packet.size = read_a_nal(fin, inbuf,INBUF_SIZE))
	{
		packet.data = inbuf;
		while(packet.size>0)
		{
			int got_picture;
			int len = avcodec_decode_video2(c, picture, &got_picture, &packet);
			if (got_picture)
				write_picture(picture, fout);
			packet.size -= len >0 ? len : packet.size;
			packet.data += len >0 ? len : 0;
		}
	}
    packet.data = NULL;
    packet.size = 0;
	int got_picture;
	int len = avcodec_decode_video2(c, picture, &got_picture, &packet);
	if (got_picture)
		write_picture(picture, fout);
	len = avcodec_decode_video2(c, picture, &got_picture, &packet);
	if (got_picture)
		write_picture(picture, fout);

	fclose(fin);
	fclose(fout);
    avcodec_close(c);
    av_free(c);
    av_free(picture);

	return 0;
}

int main()
{
	FILE * fin = fopen("Z:\\ref.yuv", "rb");
	FILE * fout = fopen("Z:\\still.h264", "wb");
	unsigned char SEI[] = {0, 0, 0, 1, 6, 5, 16, 'M', 'u', 'l', 't', 'i','V','i','e','w','H','2','6','4',' ',' ',' ', 0x80};
   	fwrite(SEI, 1, sizeof(SEI), fout);

	int t = GetTickCount();
	int width = 1924;
	int height = 1080;


	x264_param_t param;
	x264_param_default_preset(&param, "placebo", "stillimage");
	//x264_param_default_preset(&param, "medium", NULL);
	// 	x264_param_apply_profile(&param, "baseline");
	param.i_width = width;
	param.i_height = height;
	param.i_fps_num = 24000;
	param.i_fps_den = 1001;
	param.i_csp = X264_CSP_I420;

	param.i_keyint_max = 25;
	param.rc.i_rc_method = X264_RC_CQP;
	param.rc.i_qp_constant = 40;
	param.rc.f_ip_factor = 0.9;

	x264_t *encoder = x264_encoder_open(&param);

	// init picture
	x264_picture_t pic_in;
	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);
	uint8_t *data = new uint8_t[width*height*3/2];
	pic_in.img.plane[0] = (uint8_t*)data;
	pic_in.img.plane[1] = pic_in.img.plane[0] + width * height;
	pic_in.img.plane[2] = pic_in.img.plane[1] + width * height / 4;


	int i_pts=0;
	x264_nal_t *nals;
	int nnal;
	int frame_size = 0;
	x264_picture_t pic_out;
	pic_in.i_pts = i_pts++;
	pic_in.i_type = X264_TYPE_AUTO;
	fread(data, 1, width*height*3/2, fin);
	int result = x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);
	for (x264_nal_t *nal = nals; nal < nals + nnal; nal++)
		fwrite(nal->p_payload, 1, nal->i_payload, fout);

	fread(data, 1, width*height*3/2, fin);
	result = x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);
	for (x264_nal_t *nal = nals; nal < nals + nnal; nal++)
		fwrite(nal->p_payload, 1, nal->i_payload, fout);


	int c = 500;

	while ( x264_encoder_delayed_frames(encoder))
	{
		x264_encoder_encode(encoder, &nals, &nnal, NULL, &pic_out);
		printf("\npackets:");
		for (x264_nal_t *nal = nals; nal < nals + nnal; nal++)
		{
			printf("%d, ", nal->i_payload);
			fwrite(nal->p_payload, 1, nal->i_payload, fout);
		}
	}



	pic_in.img.plane[0] = pic_in.img.plane[1] = pic_in.img.plane[2] = NULL;
	x264_picture_clean(&pic_in);
	x264_encoder_close(encoder);


	delete data;

	fclose(fin);
	fclose(fout);

	printf("total time:%d ms\r\n", GetTickCount() - t);


	return decoding();
}



	// read source

	// store as classic jps



	// encoding view 0

	// prediction

	// transformation

	// quantization

	// scanning

	// entropy encoder

	// entropy decoder

	// reverse scanning	

	// dequantization

	// inverse transformation


	// view 1

	// inter prediction, motion estimation and compensation

	//  -- is this needed? prediction of residual blocks

	// transformation

	// quantization

	// scanning

	// entropy encoder

	// entropy decoder

	// reverse scanning	

	// dequantization

	// inverse transformation

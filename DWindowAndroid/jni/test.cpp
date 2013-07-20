#include "inttypes.h"
#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
extern "C"
{
#include <x264.h>
}

// x264 variables
uint8_t *yuv_buffer = NULL;
int width = 640;
int height = 480;
size_t yuv_size = width * height * 3 / 2;
x264_t *encoder = NULL;
x264_picture_t pic_in, pic_out;
FILE* outf = NULL;
int64_t i_pts = 0;

// x264 functions
int x264_init();
int x264_capture(int sock = -1);
int x264_close();


int x264_init()
{
	// close
	x264_close();

	// init encoder
	x264_param_t param;
	x264_param_default_preset(&param, "ultrafast", "zerolatency");
	x264_param_apply_profile(&param, "baseline");
	//param.i_threads = 1;
	param.i_frame_reference = 1;
	param.i_width = width;
	param.i_height = height;
	param.i_fps_num = 24000;
	param.i_fps_den = 1001;
	param.i_csp = X264_CSP_I420;

	param.i_keyint_max = 25;
	//  	param.b_intra_refresh = 1;
	param.b_cabac = 1;
	param.b_annexb = 1;
	param.rc.i_rc_method = X264_RC_ABR;
	param.rc.i_bitrate = 1250;


	encoder = x264_encoder_open(&param);

	// init picture
	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);

	yuv_buffer = (uint8_t*)malloc(yuv_size);

	pic_in.img.plane[0] = yuv_buffer;
	pic_in.img.plane[1] = pic_in.img.plane[0] + width * height;
	pic_in.img.plane[2] = pic_in.img.plane[1] + width * height / 4;

	// open file
	outf = fopen("/sdcard/sbs.h264","wb");
	if (outf == NULL) {
		return -1;
	}

	return 0;
}

int x264_capture(int sock/* = -1*/)
{
	x264_nal_t *nals;
	int nnal;
	int frame_size = 0;
	bool isIDR = false;

	pic_in.i_pts = i_pts++;
	x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);
	x264_nal_t *nal;
	for (nal = nals; nal < nals + nnal; nal++) {
		frame_size += nal->i_payload;
	}
	if (sock == -1)
		fwrite(&frame_size, 1, 4, outf);
	//else
	//	send(sock, (char*)&frame_size, 4, NULL);
	for (nal = nals; nal < nals + nnal; nal++) {
		if (sock == -1)
			fwrite(nal->p_payload, 1, nal->i_payload, outf);
		//else
		//	send(sock, (char*)nal->p_payload, nal->i_payload, NULL);
		frame_size += nal->i_payload;
		if (nal->i_type == 5)
			isIDR = true;
	}

	printf("frame is %s IDR\n", isIDR ? "" : "not");
	return 0;
}

int x264_close()
{
	// close
	if (encoder)
	{
		x264_encoder_close(encoder);
		encoder = NULL;
	}

	if (outf)
	{
		fclose(outf);
		outf = NULL;
	}

	if (yuv_buffer)
	{
		free(yuv_buffer);
		yuv_buffer = NULL;
	}

	return 0;
}



// JNI
#include <jni.h>

#define CLASS com_dwindow_x264
#define NAME2(CLZ, FUN) Java_##CLZ##_##FUN
#define NAME1(CLZ, FUN) NAME2(CLZ, FUN)
#define NAME(FUN) NAME1(CLASS,FUN)

#define  LOG_TAG    "X264"
#include <android/log.h>
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
//#define LOGI(...) ;
//#define LOGE(...) ;


extern "C" 
{
JNIEXPORT jint NAME(hello)( JNIEnv * env, jobject obj)
{
	return -1;
}
JNIEXPORT jint NAME(test)( JNIEnv * env, jobject obj)
{
	FILE *f = fopen("/sdcard/sbs.yuv", "rb");
	x264_init();
	for(int i=0; i<100; i++)
	{
		LOGE("memset() %d/100",i);
		fread(yuv_buffer, 1, yuv_size, f);
		LOGE("%d/100",i);
		x264_capture();
	}
	fclose(f);
	x264_close();
	return -1;
}
}
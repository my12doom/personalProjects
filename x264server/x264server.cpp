// x264test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "inttypes.h"
#include "stdint.h"
extern "C"
{
#include <x264.h>
}
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <winsock2.h>
#include <Windows.h>

#pragma  comment(lib, "ws2_32.lib")
#pragma comment(lib, "libx264.dll.a")

int camera_init();
int camera_capture();
int camera_close();

int x264_init();
int x264_capture(int sock = -1);
int x264_close();

int TCPTest();

// socks variables
#define MAX_CONNECTION 10
#define NETWORK_TIMEOUT 5000
#define HEARTBEEP_TIMEOUT 120000	// 2 minute
#define MAX_USER_SLOT 2048
#define NETWORK_DONE (WM_USER+1)
#define UM_ICONNOTIFY (WM_USER+2)
int server_socket = -1;
bool server_stopping = false;
int server_port = 8080;
void convertBGR2YUV420(IplImage *in, unsigned char* out_y, unsigned char* out_u, unsigned char* out_v);

// camera variables
CvCapture* pCapture = NULL;
IplImage *yuvFrame = NULL;

// x264 variables
uint8_t *yuv_buffer = NULL;
int width = 640;
int height = 480;
size_t yuv_size = width * height * 3 / 2;
x264_t *encoder = NULL;
x264_picture_t pic_in, pic_out;
FILE* outf = NULL;
int64_t i_pts = 0;

int _tmain(int argc, _TCHAR* argv[])
{
	camera_init();

	// encoding
	TCPTest();
	int frame_encoded = 0;
	while (frame_encoded < 30) {
		x264_capture();

		printf("\r%d frames encoded", ++frame_encoded);
	}

	x264_close();
	camera_close();

	return 0;
}

int x264_init()
{
	// close
	x264_close();

	// init encoder
	x264_param_t param;
	x264_param_default_preset(&param, "medium", "zerolatency");
	x264_param_apply_profile(&param, "high");
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
	param.rc.i_bitrate = 1500;


	encoder = x264_encoder_open(&param);

	// init picture
	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);

	yuv_buffer = (uint8_t*)malloc(yuv_size);

	pic_in.img.plane[0] = yuv_buffer;
	pic_in.img.plane[1] = pic_in.img.plane[0] + width * height;
	pic_in.img.plane[2] = pic_in.img.plane[1] + width * height / 4;

	// open file
	outf = _tfopen(_T("Z:\\sbs.h264"), _T("wb"));
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
	camera_capture();
	pic_in.i_pts = i_pts++;
	x264_encoder_encode(encoder, &nals, &nnal, &pic_in, &pic_out);
	x264_nal_t *nal;
	for (nal = nals; nal < nals + nnal; nal++) {
		frame_size += nal->i_payload;
	}
	if (sock == -1)
		fwrite(&frame_size, 1, 4, outf);
	else
		send(sock, (char*)&frame_size, 4, NULL);
	for (nal = nals; nal < nals + nnal; nal++) {
		if (sock == -1)
			fwrite(nal->p_payload, 1, nal->i_payload, outf);
		else
			send(sock, (char*)nal->p_payload, nal->i_payload, NULL);
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

int camera_init()
{
	pCapture = cvCreateCameraCapture(-1);

	yuvFrame = cvCreateImage(cvSize(640,480), 8, 3);
	return 0;
}

int camera_capture()
{
	IplImage* pFrame = cvQueryFrame( pCapture );

	convertBGR2YUV420(pFrame, yuv_buffer, yuv_buffer + 640*480, yuv_buffer + 640*480*5/4);

	return 0;
}

int camera_close()
{
	cvReleaseImage( &yuvFrame );
	cvReleaseCapture(&pCapture);
	return 0;
}


HRESULT init_winsock()
{
	static bool inited = false;
	if (inited)
		return S_FALSE;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
		return E_FAIL;

	inited = true;

	return S_OK;
}



DWORD WINAPI handler_thread(LPVOID param)
{
	int acc_socket = *(int*)param;
	DWORD ip = ((DWORD*)param)[1];

	delete param;

	x264_init();
	int numbytes;
	char buf[1024];
	while ((numbytes=recv(acc_socket, buf, sizeof(buf)-1, 0)) > 0) 
	{
		x264_capture(acc_socket);
	}

	shutdown(acc_socket, SD_SEND);
	int timeout = GetTickCount();
	while (recv(acc_socket, buf, 99, 0) > 0 && GetTickCount() - timeout < NETWORK_TIMEOUT)
	{
		printf("got %d byte:%s\n", numbytes, buf);
		memset(buf, 0, sizeof(buf));
	}
	closesocket(acc_socket);

	return 0;
}

int TCPTest()
{
	if (FAILED(init_winsock()))
		return -1;

	// init and listen
	SOCKADDR_IN server_addr;
	int tmp_socket = socket(PF_INET, SOCK_STREAM, 0);

	if (tmp_socket == -1)
		return -1;

	printf("Server Starting @ %d....", (int)server_port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((int)server_port);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	memset(&(server_addr.sin_zero), 0, 8);
	if (bind(tmp_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == -1)
	{
		printf("FAIL!\n");
		server_socket = -1;
		closesocket(tmp_socket);
		return -1;
	}
	if (listen(tmp_socket, MAX_CONNECTION) == -1 ) 
	{
		server_socket = -1;
		printf("FAIL!\n");
		closesocket(tmp_socket);
		return -1;
	}
	printf("OK\n");

	int sock_size = sizeof(SOCKADDR_IN);
	SOCKADDR_IN user_socket;

	// wait for connection
	server_socket = tmp_socket;

	while(!server_stopping)
	{
		int acc_socket = accept(tmp_socket, (SOCKADDR*)&user_socket, &sock_size);
		int buf = 0;
// 		setsockopt(acc_socket, SOL_SOCKET, SO_SNDBUF, (char*)&buf, sizeof(buf));
// 		setsockopt(acc_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buf, sizeof(buf));

		if (server_stopping)
			break;

		if (acc_socket == -1)
			continue;

		DWORD ip = user_socket.sin_addr.S_un.S_addr;
		DWORD *para = new DWORD[2];
		para[0] = acc_socket;
		para[1] = ip;
		CreateThread(NULL, NULL, handler_thread, para, NULL, NULL);
	}

	server_socket = -1;
	return 0;
}


void convertBGR2YUV420(IplImage *in, unsigned char* out_y, unsigned char* out_u, unsigned char* out_v)  
{  
	// first, convert the input image into YCbCr 
	cvCvtColor(in, yuvFrame, CV_BGR2YCrCb); 
	/*  
	* widthStep = channel number * width  
	* if width%4 == 0  
	* for example, width = 352, width%4 == 0, widthStep = 3 * 352 = 1056  
	*/ 
	int idx_in = 0;  
	int idx_out = 0;  
	int idx_out_y = 0;  
	int idx_out_u = 0;  
	int idx_out_v = 0; 

	for(int j = 0; j < in->height; j+=1) 
	{ 
		idx_in = j * in->widthStep; 

		for(int i = 0; i < in->widthStep; i+=12) 
		{ 
			// We use the chroma sample here, and put it into the out buffer 
			// take the luminance sample 
			out_y[idx_out_y++] = yuvFrame->imageData[idx_in + i + 0]; // Y 
			out_y[idx_out_y++] = yuvFrame->imageData[idx_in + i + 3]; // Y 
			out_y[idx_out_y++] = yuvFrame->imageData[idx_in + i + 6]; // Y 
			out_y[idx_out_y++] = yuvFrame->imageData[idx_in + i + 9]; // Y 
			if((j % 2) == 0) { 
				// take the blue-difference and red-difference chroma components sample  
				out_u[idx_out_u++] = yuvFrame->imageData[idx_in + i + 1]; // Cr U  
				out_u[idx_out_u++] = yuvFrame->imageData[idx_in + i + 7]; // Cr U  
				out_v[idx_out_v++] = yuvFrame->imageData[idx_in + i + 2]; // Cb V  
				out_v[idx_out_v++] = yuvFrame->imageData[idx_in + i + 8]; // Cb V 
			}  
		}    
	}
}
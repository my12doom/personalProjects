#include <jni.h>
#include <android/log.h>
#include <time.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "decoding.h"
#include <../3dvlog.h>

#define  LOG_TAG    "my12doom.x264client.core"

#define NAME(FUN) Java_my12doom_x264client_core_##FUN

class myclient : public client
{
	void show_picture(AVFrame *frame);
};

extern "C" 
{


void * getSurfaceNativeHandle(JNIEnv * env, jobject surface);
JNIEXPORT jint NAME(init)( JNIEnv * env, jobject obj,jobject surface);

typedef void (*Surface_lock)(void *, void *, int);
typedef void (*Surface4_lock)(void *, void *, void *);
typedef void (*Surface_unlockAndPost)(void *);

Surface_lock s_lock = NULL;
Surface4_lock s_lock4 = NULL;
Surface_unlockAndPost s_unlockAndPost = NULL;
jobject g_surface;
void *g_surface_handle;
void *g_library = NULL;

typedef struct _SurfaceInfo
{
	uint32_t    w;
	uint32_t    h;
	uint32_t    s;
	uint32_t    a;
	uint32_t    b;
	uint32_t    c;
	uint32_t    reserved[2];
} SurfaceInfo;

int init_surface_library()
{
	if (g_library)
		dlclose(g_library);

	g_library = dlopen("libsurfaceflinger_client.so", RTLD_NOW);
	if (g_library)
	{
		s_lock = (Surface_lock)(dlsym(g_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"));
		s_unlockAndPost = (Surface_unlockAndPost)(dlsym(g_library, "_ZN7android7Surface13unlockAndPostEv"));
		if (s_lock && s_unlockAndPost)
			return 0;
		dlclose(g_library);
	}

	g_library = dlopen("libui.so", RTLD_NOW);
	if (g_library)
	{
		s_lock = (Surface_lock)(dlsym(g_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"));
		s_unlockAndPost = (Surface_unlockAndPost)(dlsym(g_library, "_ZN7android7Surface13unlockAndPostEv"));
		if (s_lock && s_unlockAndPost)
			return 0;
		dlclose(g_library);
	}

	g_library = dlopen("libgui.so", RTLD_NOW);
	if (g_library)
	{
		s_lock4 = (Surface4_lock)(dlsym(g_library, "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE"));
		s_unlockAndPost = (Surface_unlockAndPost)(dlsym(g_library, "_ZN7android7Surface13unlockAndPostEv"));
		if (s_lock4 && s_unlockAndPost)
			return 0;
		dlclose(g_library);
	}

	return -1;
}

JNIEXPORT jint NAME(init)( JNIEnv * env, jobject obj,jobject surface)
{
	g_surface = surface;
	g_surface_handle = getSurfaceNativeHandle(env, surface);

	LOGI("Hello surface:%08x, native surface:%08x", surface, g_surface_handle);

	if (g_surface_handle == NULL || init_surface_library() < 0)
		return -1;

	return 0;
}

myclient c;
JNIEXPORT jint NAME(startTest)( JNIEnv * env, jobject obj, jstring host, jint port)
{
	LOGE("connectting to %s : %d .... ", host, port);
	if (c.connect("192.168.11.199", 9087) < 0)
	{
		LOGE("failed connectting to %s : %d", host, port);
		return -1;
	}

	LOGE("connectting to %s : %d .... OK", host, port);

	c.start_decoding();

	return 0;
}

JNIEXPORT jint NAME(stopTest)( JNIEnv * env, jobject obj)
{
	c.stop_decoding();
	c.disconnect();

	return 0;
}

JNIEXPORT jint NAME(uninit)( JNIEnv * env, jobject obj)
{

	return 0;
}


void * getSurfaceNativeHandle(JNIEnv * env, jobject surface)
{
	jclass clz;
	jfieldID fid;
	jthrowable exp;
	clz = env->GetObjectClass(surface);
	fid = env->GetFieldID(clz, "mSurface", "I");
	void *ret = NULL;
	if(fid == NULL)
	{
		exp = env->ExceptionOccurred();
		if(exp)
		{
			env->DeleteLocalRef(exp);
			env->ExceptionClear();
		}
		fid = env->GetFieldID(clz, "mNativeSurface", "I");
	}
	if(fid != NULL)
		ret = (void *)env->GetIntField(surface, fid);

	env->DeleteLocalRef(clz);

	return ret;
}








}

#include <libyuv.h>
#include <pthread.h>

typedef struct I420_to_RGB565_parameter_tag
{
	const unsigned char *Y; int strideY;
	const unsigned char *U; int strideU;
	const unsigned char *V; int strideV;
	int src_width; int src_height;
	unsigned char *dst; int dst_stride;
	int dst_width; int dst_height;
	unsigned char*tmp_space;
}I420_to_RGB565_parameter;

int I420_to_RGB565_core(const unsigned char *Y, int strideY,
						const unsigned char *U, int strideU,
						const unsigned char *V, int strideV,
						int src_width, int src_height,
						unsigned char *dst, int dst_stride,
						int dst_width, int dst_height,
						unsigned char*tmp_space);

static void* I420_to_RGB565_thread(void*parameter)
{
	I420_to_RGB565_parameter *p = (I420_to_RGB565_parameter*)parameter;
	I420_to_RGB565_core(p->Y, p->strideY, p->U, p->strideU, p->V, p->strideV, p->src_width, p->src_height,
		p->dst, p->dst_stride, p->dst_width, p->dst_height, p->tmp_space);

	delete p;
	return NULL;
}

int I420_to_RGB565_core(const unsigned char *Y, int strideY,
						const unsigned char *U, int strideU,
						const unsigned char *V, int strideV,
						int src_width, int src_height,
						unsigned char *dst, int dst_stride,
						int dst_width, int dst_height,
						unsigned char*tmp_space)
{
	// do scale if needed
	if (src_width != dst_width || src_height != dst_height)
	{
		int dst_strideY = (dst_width+63)/64*64;
		int dst_strideU = dst_strideY/2;
		int dst_strideV = dst_strideY/2;

		unsigned char *dstY = tmp_space;
		unsigned char *dstU = tmp_space + dst_strideY * dst_height;
		unsigned char *dstV = dstU + dst_strideU * dst_height/2;

		libyuv::I420Scale(Y, strideY,
						  U, strideU,
						  V, strideV,
						  src_width, src_height,
						  dstY, dst_strideY,
						  dstU, dst_strideU,
						  dstV, dst_strideV,
						  dst_width, dst_height,
						  libyuv::kFilterBilinear
						  );

		Y = dstY;
		U = dstU;
		V = dstV;
		strideY = dst_strideY;
		strideU = dst_strideU;
		strideV = dst_strideV;
	}

	libyuv::I420ToRGB565((uint8*)Y, strideY,
						 (uint8*)U, strideU,
						 (uint8*)V, strideV,
						 dst, dst_stride, dst_width, dst_height);

	return 0;
}

void myclient::show_picture(AVFrame *frame)
{
	LOGI("got picture %dx%d", frame->width, frame->height);
	if (g_surface_handle == NULL)
		return;
	
	SurfaceInfo info;
	unsigned char *p_bits = NULL;
	if(s_lock != NULL)
		s_lock(g_surface_handle, &info, 1);
	else if(s_lock4 != NULL)
		s_lock4(g_surface_handle, &info, NULL);
	p_bits = (unsigned char *)(info.s >= info.w ? info.c : info.b);

	LOGI("surface locking done, start drawing");
	
	unsigned char * tmp = (unsigned char*)malloc(640*480*4);

	I420_to_RGB565_core(frame->data[0], frame->linesize[0],
						frame->data[2], frame->linesize[2],
						frame->data[1], frame->linesize[1],
						frame->width, frame->height,
						p_bits, info.s * 2,
						info.w, info.h,
						tmp);

	free(tmp);
	
	LOGI("drawing done");

	s_unlockAndPost(g_surface_handle);

	LOGI("post done");

}
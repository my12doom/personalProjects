#ifndef AVSMAIN_H
#define AVSMAIN_H

// extern "C"
#ifdef __cplusplus
extern "C" {
#endif

// declaration
void* create_avs();
int set_avs_resolution(void *avs, int width, int height);
int insert_frame(void *avs, void **pY, void **pV, void **pU, int view_id);
int close_avs(void *avs);		// just set no_more_data = 1

// C++ classes
#ifdef __cplusplus
}
#include "CCritSec.h"
#include "avisynth.h"
#include "m2ts_scanner.h"

typedef struct buffer_unit_struct
{
	int frame_number;
	BYTE *data;
} buffer_unit;

class CFrameBuffer
{
public:
	buffer_unit* the_buffer;
	int m_unit_count;
	int m_width;
	int m_height;

	int m_frame_count;		// total frame count
	int m_max_fn_recieved;	// = -1, max fn inserted yet
	int m_item_count;		// = 0;

	CCritSec cs;

	// functions
	CFrameBuffer();
	~CFrameBuffer();
	int init(int width, int height, int buffer_unit_count, int frame_count);
	int insert(int n, const BYTE*buf);
	int insert_no_number(const BYTE **pY, const BYTE **pV, const BYTE **pU);		// ldecod use some stupid 2D memory layout
	int remove(int n, BYTE *buf, int stride, int offset);
	// return value:// 0-n : success 
	// -1  : wrong range, no such data
	// -2  : feeder busy, should wait and retry for data
};

class JMAvs: public IClip 
{
public:
	HANDLE m_decoding_thread;

	VideoInfo vi;
	int m_frame_count;
	int m_width;
	int m_height;
	int m_buffer_unit_count;

	CFrameBuffer *left_buffer; 
	CFrameBuffer *right_buffer;

	bool m_decoding_done; // a temp buffer

	char m_m2ts_left[MAX_PATH];
	char m_m2ts_right[MAX_PATH];

	JMAvs();
	int avs_init(const char*m2ts_left, IScriptEnvironment* env, const char*m2ts_right = NULL,
		const int frame_count = -1, int buffer_count = 10,
		int fps_numerator = 24000, int fps_denominator = 1001);
	int ldecod_init(int width, int height);
	virtual ~JMAvs();

	// avisynth virtual functions
	bool __stdcall GetParity(int n){return false;}
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
	const VideoInfo& __stdcall GetVideoInfo(){return vi;}
	void __stdcall SetCacheHints(int cachehints,int frame_range) {};

	// key function
	void get_frame(int n, CFrameBuffer *the_buffer, BYTE*data, int stride, int offset);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

#endif
#endif
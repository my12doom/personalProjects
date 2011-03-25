#include <stdio.h>
#include <Windows.h>
#ifdef _DEBUG
#include <wxdebug.h>
#endif
#include <wxutil.h>
// don't free() or fclose(), C3dvDemuxer's deconstructor will do that.
typedef struct stream_3dv_struct
{
	// general info
	FILE *f;		// a pointer to file
	CCritSec *file_lock; // multi thread lock for file
	int type;		// 0 = video, 1 = mp3 audio, 2 = aac audio
	int packet_count;// audio also has frames
	int length;		// length in ms
	int CBR;		// 0 = VBR, non-0 = packet size
	DWORD *packet_index;
	DWORD *packet_size;
	int keyframe_count;
	DWORD *keyframes;
	DWORD max_packet_size;

	// video info
	int time_units;
	int time_in_units;
	//int video_fps;		// yes...it is integer, unbelievable
	//int video_fps_denominator;		// this is my code, not theirs

	// audio info
	//int audio_sample_count;
	int audio_sample_rate;
	int audio_bit_depth;
	int audio_channel;

} stream_3dv;

class C3dvDemuxer
{
public:
	C3dvDemuxer(const wchar_t *file);
	~C3dvDemuxer();

	int m_stream_count;
	stream_3dv *m_streams;

protected:
	CCritSec m_cs;
	FILE *m_file;
	int demux_riff(int index, int size);
	int handle_data(DWORD fourcc, int index, int size);
};
#include <math.h>
#include "m2ts_scanner.h"

h264_scan_result scan_m2ts(const char *name)
{
	ts::demuxer demuxer;
	h264_scan_result result;
	memset(&result, 0, sizeof(result));

	// init
	demuxer.type_todemux = 0x1b;	// type h264
	demuxer.av_only = false;
	demuxer.silence = true;
	demuxer.parse_only = true;
	demuxer.demux_target = NULL;

	// start scan file
	if (demuxer.fast_scan_file(name))
		result.error = true;

	for(std::map<u_int16_t,ts::stream>::const_iterator i=demuxer.streams.begin();i!=demuxer.streams.end();++i)
	{
		const ts::stream& s=i->second;

		// find the h264 pid
		if(s.type==0x1b)
		{
			//printf("[JM3DSource info] possible left eye pid:%4x\n", i->first);
			result.frame_length = s.frame_length;
			u_int64_t end = s.last_pts + s.frame_length;
			result.length = end - s.first_pts;
			if (!result.possible_left_pid)
				result.possible_left_pid = i->first;

		}

		// MVC right eye stream
		if (s.type == 0x20)
		{
			//printf("[JM3DSource info] possible right eye pid:%4x\n", i->first);
			if (!result.possible_right_pid)
				result.possible_right_pid = i->first;
		}
	}

	// post process
	result.fps_denominator = result.frame_length;
	result.fps_numerator = 90000;
	if (result.frame_length == 3753 || result.frame_length == 3754)
	{
		// 23.976
		result.frame_length = 3754;
		result.fps_numerator = 24000;
		result.fps_denominator = 1001;
	}

	if (result.frame_length == 1501 || result.frame_length == 1502)
	{
		// 59.94
		result.frame_length = 1501;
		result.fps_numerator = 60000;
		result.fps_denominator = 1001;
	}

	if (result.frame_length)
	{
		result.frame_count = (u_int32_t)((double)result.length / result.frame_length + 1.5);
	}

	result.length_in_second = (double)result.length / 90000;

	return result;
}

// warning : no thread safe
#pragma once

#include "..\tsdemux\ts.h"

typedef struct h264_scan_result_struct
{
	u_int64_t length;
	u_int32_t frame_length;		// in a 90Khz clock tick
								// 90000 = 1s
	u_int32_t height;
	u_int32_t width;			// after crop;

	u_int32_t frame_count;
	u_int32_t fps_numerator;
	u_int32_t fps_denominator;

	int possible_left_pid;
	int possible_right_pid;

	bool error;
} h264_scan_result;

h264_scan_result scan_m2ts(const char *name);
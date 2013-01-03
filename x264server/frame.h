#pragma once

typedef struct frameTag
{
	int ref_count;
	void *data;
	int datasize;

	int frame_type;
	int DTS;		// decode time stamp
	int PTS;		// present time stamp
} frame;
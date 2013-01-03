#pragma once
#include "frame.h"

class frame_allocator
{
public:
	frame_allocator();
	~frame_allocator();

	frameTag *alloc();			// return NULL on error, pointer to a valid frame buffer on success.
	void free(frameTag **p);	// free a frame, and set pointer to NULL.

protected:
	void *free_packets;
};
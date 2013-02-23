#include "thumbnail_creator.h"

async_thumbnail_creator::async_thumbnail_creator()
:r(400,480)
{

}
async_thumbnail_creator::~async_thumbnail_creator()
{

}

int async_thumbnail_creator::open_file(const wchar_t*file)		// 
{
	close_file();
	wcscpy(m_file_name, file);

	// and send request ;
	return 0;
}
int async_thumbnail_creator::close_file()						//
{

	return 0;
}
int async_thumbnail_creator::seek()								// seek to a frame, and start worker thread
{

	return 0;
}
int async_thumbnail_creator::get_frame()						// return the frame if worker has got the picture, or error codes
{

	return 0;
}


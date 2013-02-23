#pragma once
#include "..\Thumbnail\remote_thumbnail.h"

class async_thumbnail_creator
{
public:
	async_thumbnail_creator();
	~async_thumbnail_creator();

	int open_file(const wchar_t*file);		// 
	int close_file();						//
	int seek();								// seek to a frame, and start worker thread
	int get_frame();						// return the frame if worker has got the picture, or error codes

protected:

	int reconnect();						// on any errors, kill worker process and recreate it, reopen file, reseek, retry get_frame until max retry count reached.

	DWORD worker_thread();
	static DWORD WINAPI worker_thread_entry(LPVOID p){return ((async_thumbnail_creator*)p)->worker_thread();}

	wchar_t m_pipe_name[200];
	wchar_t m_file_name[MAX_PATH];
	remote_thumbnail r;
	int retry;
	HANDLE m_worker_process;
};

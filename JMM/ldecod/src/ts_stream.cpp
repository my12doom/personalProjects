#include <Windows.h>
#include "ts_stream.h"
#include "..\tsdemux\ts.h"

DWORD WINAPI demuxer_thread(LPVOID p);

typedef struct struct_ts_stream
{
	char filename[255];
	ts::demuxer *demuxer;
	HANDLE demuxing_thread;
} ts_stream;


void *open_ts(char *file)
{
	ts_stream *s = new ts_stream;
	s->demuxer = new ts::demuxer();
	memcpy(s->filename, file, min(strlen(file)+1, 254));
	s->filename[254] = '\0';

	if (s->demuxer->is_ts(file) == false)
	{
		delete s->demuxer;
		delete s;
		return NULL;
	}

	s->demuxer->demux_target = new CFileBuffer(1024*1024);
	s->demuxer->pid_to_demux = 0x1011;
	s->demuxing_thread = INVALID_HANDLE_VALUE;

	return s;
}
void *get_ts_sub_stream(void *main_stream)
{
	ts_stream *main_stream_s = (ts_stream*)main_stream;

	// now we really need to scan the file for 0x1012
	ts::demuxer demuxer;
	printf("Scanning file %s for MVC sub stream...", main_stream_s->filename);
	if (demuxer.fast_scan_file(main_stream_s->filename) == -1)
	{
		printf("NOT TS.\n");
		return NULL;
	}


	for(std::map<u_int16_t,ts::stream>::const_iterator i=demuxer.streams.begin();i!=demuxer.streams.end();++i)
		if (i->first == 0x1012)
		{
			ts_stream *s = new ts_stream;
			s->demuxer = new ts::demuxer();
			s->demuxer->demux_target = new CFileBuffer(1024*1024);
			s->demuxer->pid_to_demux = 0x1012;
			s->demuxing_thread = INVALID_HANDLE_VALUE;
			memcpy(s->filename, main_stream_s->filename, 255);

			printf("OK.\n");
			return s;
		}

	printf("NOT FOUND.\n");
	return NULL;
}
int read_ts_stream(void *stream, void *buf, int size)
{
	ts_stream *s = (ts_stream *)stream;
	if (s->demuxing_thread == INVALID_HANDLE_VALUE)
		s->demuxing_thread = CreateThread(NULL, NULL, demuxer_thread, stream, NULL, NULL);
	ts::demuxer *demuxer = ((ts_stream*)stream)->demuxer;
	return demuxer->demux_target->block_remove(size, (BYTE*)buf);
}
void close_ts(void *stream)
{
	ts_stream *s = (ts_stream*)stream;
	TerminateThread(s->demuxing_thread, 0);			//warning: file remain opened
	delete s->demuxer->demux_target;
	delete s->demuxer;
	delete s;
}

void switch_to_sub_stream(void *stream)
{
	ts_stream *s = (ts_stream*)stream;
	s->demuxer->pid_to_demux = 0x1012;
}

DWORD WINAPI demuxer_thread(LPVOID p)
{
	ts_stream *s = (ts_stream *)p;
	ts::demuxer *demuxer = s->demuxer;

	demuxer->av_only = false;
	demuxer->silence = true;
	demuxer->parse_only = true;

	demuxer->demux_file(s->filename);

	demuxer->demux_target->no_more_data = true;

	return 0;
}

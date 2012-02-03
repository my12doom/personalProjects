#include <Windows.h>
#include "ts_stream.h"
#include "..\tsdemux\ts.h"

#define FILE_NAME_SIZE  1024

DWORD WINAPI demuxer_thread(LPVOID p);

typedef struct struct_ts_stream
{
	char filename[FILE_NAME_SIZE];
	ts::demuxer *demuxer;
	HANDLE demuxing_thread;
} ts_stream;


void *open_ts(char *file)
{
	char tmp[FILE_NAME_SIZE];
	parse_combined_filename(file, 0, tmp);

	ts_stream *s = new ts_stream;
	s->demuxer = new ts::demuxer();
	memcpy(s->filename, file, min(strlen(file)+1, FILE_NAME_SIZE));
	s->filename[FILE_NAME_SIZE-1] = '\0';

	if (s->demuxer->is_ts(tmp) == false)
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
	char tmp[FILE_NAME_SIZE];
	parse_combined_filename(main_stream_s->filename, 0, tmp);

	// now we really need to scan the file for 0x1012
	ts::demuxer demuxer;
	demuxer.parse_only = true;
	printf("Scanning file %s for MVC sub stream...", tmp);
	if (demuxer.fast_scan_file(tmp) == -1)
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
			memcpy(s->filename, main_stream_s->filename, FILE_NAME_SIZE);

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
	char tmp[FILE_NAME_SIZE];

	demuxer->av_only = false;
	demuxer->silence = true;
	demuxer->parse_only = true;

	int n = 0;
	while (parse_combined_filename(s->filename, n++, tmp) != -1)
		demuxer->demux_file(tmp);

	demuxer->demux_target->no_more_data = true;

	return 0;
}

int parse_combined_filename(const char *combined, int n, char *out)
{
	// find the path
	int path_len = 0;
	for(int i=strlen(combined); i>0; i--)
		if (combined[i] == '\\')
		{
			path_len = i+1;
			break;
		}

	// copy path
	memcpy(out, combined, path_len);
	out[path_len] = '\0';

	const char *names = combined + path_len;
	out += path_len;
	for(int i=0; i<n; i++)
	{
		names = strstr(names, ":");
		
		if (NULL == names)
			return -1;

		names++;
	}

	// copy name
	while(*names != ':' && *names != '\0')
	{
		*out = *names;
		names++;
		out++;
	}

	*out = '\0';

	return n;
}
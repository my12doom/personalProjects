#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "tsdemux\ts.h"
#include "CFileBuffer.h"


DWORD WINAPI feeder_thread(LPVOID lpParameter);
void main2();

typedef struct feeder_thread_parameter_tags
{
	bool demux;
	char filename[MAX_PATH];
	CFileBuffer *out;		// for raw
	CFileBuffer *out_left;  // for demux
	CFileBuffer *out_right; // for demux
}feeder_thread_parameter;

const unsigned int max_nal_size = 1024000;
unsigned char *delimeter_buffer = (unsigned char*)malloc(max_nal_size);
unsigned char watermark[1024];
int watermark_size = 0;
CFileBuffer left(max_nal_size*10);
CFileBuffer right(max_nal_size*10);

void create_watermark()
{
	#define str "my12doom's mvc interlacer."

	watermark[0] = 0;
	watermark[1] = 0;
	watermark[2] = 0;
	watermark[3] = 1;
	watermark[4] = 31;	// should be fine
	strcpy((char*)watermark+5, str);
	watermark_size = 5 + strlen(str);
}

int nal_type(unsigned char *data)
{
	if (data[2] == 0x0 && data[3] == 0x1)
		return data[4] & 0x1f;
	else if (data[1] == 0x0 && data[2] == 0x1)
		return data[3] & 0x1f;

	return -1;
}

int read_a_nal(CFileBuffer *f)
{
	unsigned data_read = f->wait_for_data(max_nal_size);
	unsigned char* read_buffer = f->m_the_buffer + f->m_data_start;

	if (data_read == 0)
		return 0;
	for(unsigned int i=4; i<data_read-3; i++)
		//if (read_buffer[i] == 0)
		//if (read_buffer[i] == 0x0 && read_buffer[i+1] == 0x0 && read_buffer[i+2] == 0x0 && read_buffer[i+3] == 0x1)
		if (*(unsigned int *)(read_buffer+i) == 0x01000000)
		{
			return i;
		}
	return data_read;
}

int read_a_delimeter(CFileBuffer *f)
{
	int size = 0;
	int current_start = false;

	while(true)
	{
		int nal_size = read_a_nal(f);
		unsigned char *read_buffer = f->m_the_buffer + f->m_data_start;
		int _nal_type = nal_type(read_buffer);

		/*
		if (f == &left)
			printf("(l)");
		else
			printf("(r)");
		printf("%x ", _nal_type);
		*/

		if (_nal_type == 31) //my12doom's watermark
		{
			f->remove_data(nal_size);
			continue;
		}

		if (nal_size == 0 ||	
			nal_type(read_buffer) == 0x9 ||
			nal_type(read_buffer) == 0x18)
		{
			if (current_start || nal_size == 0)
			{
				return size;
			}
			else
			{
				current_start = true;
				memcpy(delimeter_buffer + size, read_buffer, nal_size);
				size += nal_size;

				f->remove_data(nal_size);
			}
		}
		else
		{
			memcpy(delimeter_buffer + size, read_buffer, nal_size);
			size += nal_size;

			f->remove_data(nal_size);
		}
	}
}

DWORD WINAPI feeder_thread(LPVOID lpParameter)
{
	feeder_thread_parameter *parameter = (feeder_thread_parameter*)lpParameter;
	printf("%s thread started\n", parameter->filename);

	if (!parameter->demux)
	{
		FILE *f = fopen(parameter->filename, "rb");

		if(f)
		{
			unsigned char buf[102400];
			int got = 0;
			int total = 0;
			while ((got = fread(buf, 1, 102400, f)) > 0)
			{
				parameter->out->insert(got, buf);
				total += got;
			}
			parameter->out->no_more_data = true;
		}
		else
		{
			printf("can't open file %s\n", parameter->filename);
		}
	}
	else
	{
		ts::demuxer demuxer;
		demuxer.av_only = false;
		demuxer.silence = true;
		demuxer.parse_only = true;

		demuxer.demux_target_left = parameter->out_left;
		demuxer.demux_target_right = parameter->out_right;
		demuxer.demux_file(parameter->filename);

		if (demuxer.demux_target_left)
			demuxer.demux_target_left->no_more_data = true;

		if (demuxer.demux_target_right)
			demuxer.demux_target_right->no_more_data = true;
	}

	printf("\r%s thread exit\t\t\t\t\t\n", parameter->filename);
	delete parameter;
	return 0;
}

LONGLONG FileSize(const char*filename)
{
	DWORD read;
	HANDLE f = CreateFile(filename, FILE_READ_ACCESS, 7, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(INVALID_HANDLE_VALUE == f)
		return -1;

	LARGE_INTEGER filesize;
	GetFileSizeEx(f, &filesize);

	CloseHandle(f);

	return filesize.QuadPart;
}

void main(int argc, char * argv[])
{
	main2();
	return;
	printf("my12doom's mvc interlacer\n");
	printf("my12doom.googlecode.com\n");
	printf("mailto:my12doom@gmail.com\n");
	printf("thanks to tsdemux by clark15b\n\n");

	create_watermark();
	
	/*
	FILE * f = fopen("Z:\\watermark.wm", "wb");
	fwrite(watermark, watermark_size, 1, f);
	fseek(f, 127, SEEK_CUR);
	fwrite("", 1, 1, f);
	fflush(f);
	fclose(f);
	*/
	

	if (argc<3 || argc>4)
	{
		printf("usage:");
		printf("mux.exe <output> <ssif input>\n");
		printf("mux.exe <output> <left input> <right input>\n\n");

		printf("sample:\n");
		printf("mux.exe E:\\mux.mvc E:\\00001.ssif\n");
		printf("mux.exe E:\\mux.mvc E:\\left.m2ts E:\\right.h264\n");

		printf("\npress any key to exit");
		getch();
		return;
	}

	char out_mvc_name[MAX_PATH];
	char out_avs_name[MAX_PATH];
	strcpy(out_mvc_name, argv[1]);
	strcpy(out_avs_name, argv[1]);
	strcat(out_avs_name, ".avs");


	FILE *o = fopen(out_mvc_name, "wb");
	FILE *avs = fopen(out_avs_name, "wb");

	for(int i=2; i<argc; i++)
	{
		feeder_thread_parameter * para = new feeder_thread_parameter;
		strcpy(para->filename, argv[i]);
		size_t len = strlen(argv[i]);
		if (argv[i][len-1] == '4' && argv[i][len-2] == '6' && argv[i][len-3] == '2')
			para->demux = false;
		else
			para->demux = true;

		if (para->demux)
		{
			para->out_left = &left;
			para->out_right = &right;
		}
		else
		{
			if (i == 2)
				para->out = &left;
			else
				para->out = &right;
		}

		CreateThread(NULL, NULL, feeder_thread, para, NULL, NULL);
	}
	int n = 0;
	int l = GetTickCount();
	LONGLONG muxed_size = 0;

	char *out_buffer = (char*)malloc(max_nal_size*4 + sizeof(watermark)*2);
	int out_data_count = 0;

	create_watermark();
	while (true)
	{
		int delimeter_size_left = read_a_delimeter(&left);
		//fwrite(delimeter_buffer, 1, delimeter_size_left, o);
		memcpy(out_buffer+out_data_count, delimeter_buffer, delimeter_size_left);
		out_data_count += delimeter_size_left;

		int delimeter_size_right = read_a_delimeter(&right);
		//fwrite(delimeter_buffer, 1, delimeter_size_right, o);
		memcpy(out_buffer+out_data_count, delimeter_buffer, delimeter_size_right);
		out_data_count += delimeter_size_right;

		//watermark
		memcpy(out_buffer+out_data_count, watermark, watermark_size);
		out_data_count += watermark_size;


		if (out_data_count >= max_nal_size*2)
		{
			fwrite(out_buffer, 1, out_data_count, o);
			out_data_count = 0;
		}

		if (delimeter_size_left == 0 && delimeter_size_right == 0)
			break;

		muxed_size += delimeter_size_left + delimeter_size_right;
		printf("\r%d frames muxed, %.2fMb/%.2fs, %dKB/s", ++n,
			(double)muxed_size/1024/1024,
			(double)(GetTickCount()-l)/1000,
			(int)(muxed_size/(GetTickCount()-l+1)));
	}

	fwrite(out_buffer, 1, out_data_count, o);
	free(out_buffer);

	fflush(o);
	fclose(o);

	bool sbs = true;

	// remove path
	for(int i=strlen(out_mvc_name); i>0; i--)
	{
		if (out_mvc_name[i] == '\\')
		{
			strcpy(out_mvc_name, out_mvc_name+i+1);
			break;
		}
	}
	if(sbs) fprintf(avs, "LoadPlugin(\"pd10\")\r\n");
	fprintf(avs, "grf = CreateGRF(\"%s\")\r\n", out_mvc_name);
	fprintf(avs, "v = DirectShowSource(grf, audio=false, framecount=%d)\r\n", n*2);
	if(sbs)
		fprintf(avs, "return v.sq2sbs\r\n");
	else
		fprintf(avs, "return v.AssumeFPS(v.FrameRate*2)\r\n");
	fflush(avs);
	fclose(avs);

	printf("\ndone, press any key to exit");
	getch();
}

void main2()
{
	feeder_thread_parameter * para = new feeder_thread_parameter;
	strcpy(para->filename, "F:\\TDDOWNLOAD\\BDMV STREAM SSIF 00005.mvc");
	para->demux = false;
	para->out = &left;
	CreateThread(NULL, NULL, feeder_thread, para, NULL, NULL);

	FILE * f[2];
	f[0] = fopen("Z:\\left.h264", "wb");
	f[1] = fopen("Z:\\right.h264", "wb");

	int n = 0;
	int id = 0;
	while (true)
	{
		int delimeter_size = read_a_delimeter(&left);

		if (delimeter_size == 0)
			break;

		fwrite(delimeter_buffer, 1, delimeter_size, f[id]);

		id = 1-id;

		if (id)
			n++;
	}

	fclose(f[0]);
	fclose(f[1]);

}

#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <detours.h>
#include "CFileBuffer.h"
#include "tsdemux\ts.h"

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "detoured.lib")

void add_my12doom(unsigned char *pY);

const char * pipe_name = "\\\\.\\pipe\\JM3Dpipe";
static HANDLE h_pipe = INVALID_HANDLE_VALUE;

DWORD parent_id = 0;
int width;
int height;

BYTE * left_image_buffer = NULL;
BYTE * right_image_buffer = NULL;

char left_m2ts[MAX_PATH] = "";
char right_m2ts[MAX_PATH] = "";
int left_pid = 0;
int right_pid = 0;
bool override_input = false;
CFileBuffer *buffer_left = NULL;
CFileBuffer *buffer_right = NULL;

char left_input_key[MAX_PATH] = "left.h264";
char right_input_key[MAX_PATH] = "right.h264";
HANDLE left_input_handle = INVALID_HANDLE_VALUE;
HANDLE right_input_handle = INVALID_HANDLE_VALUE;

char left_output_key[MAX_PATH] = "test_dec_ViewId0000.yuv";
char right_output_key[MAX_PATH] = "test_dec_ViewId0001.yuv";
HANDLE left_output_handle = INVALID_HANDLE_VALUE;
HANDLE right_output_handle = INVALID_HANDLE_VALUE;

static HANDLE (WINAPI * TrueCreateFileA)(
LPCSTR lpFileName,
DWORD dwDesiredAccess,
DWORD dwShareMode,
LPSECURITY_ATTRIBUTES lpSecurityAttributes,
DWORD dwCreationDisposition,
DWORD dwFlagsAndAttributes,
HANDLE hTemplateFile
) = CreateFileA;

static BOOL (WINAPI *TrueWriteFile)(
HANDLE hFile,
LPCVOID lpBuffer,
DWORD nNumberOfBytesToWrite,
LPDWORD lpNumberOfBytesWritten,
LPOVERLAPPED lpOverlapped
) = WriteFile;

static BOOL (WINAPI *TrueReadFile)(
HANDLE hFile,
LPVOID lpBuffer,
DWORD nNumberOfBytesToRead,
LPDWORD lpNumberOfBytesRead,
LPOVERLAPPED lpOverlapped
) = ReadFile;

HANDLE WINAPI MineCreateFileA(
LPCSTR lpFileName,
DWORD dwDesiredAccess,
DWORD dwShareMode,
LPSECURITY_ATTRIBUTES lpSecurityAttributes,
DWORD dwCreationDisposition,
DWORD dwFlagsAndAttributes,
HANDLE hTemplateFile
)
{
	// hook file open
	// and save the handle for identification
	HANDLE rv = INVALID_HANDLE_VALUE;
	if (override_input)
	{
		if (strstr(lpFileName, left_input_key))		
			rv = left_input_handle = TrueCreateFileA("JM_hook.dll", GENERIC_READ, 7,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
		if (strstr(lpFileName, right_input_key))
			rv = right_input_handle = TrueCreateFileA("JM_hook.dll", GENERIC_READ, 7,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
		if (rv != INVALID_HANDLE_VALUE)
		{
			return rv;
		}
	}

	rv = TrueCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, 
		dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (strstr(lpFileName, right_output_key))
		right_output_handle = rv;
	if (strstr(lpFileName, left_output_key))
		left_output_handle = rv;

	return rv;
}

BOOL WINAPI MineWriteFile(
HANDLE hFile,
LPCVOID lpBuffer,
DWORD nNumberOfBytesToWrite,
LPDWORD lpNumberOfBytesWritten,
LPOVERLAPPED lpOverlapped
)
{
	if ((left_output_handle != INVALID_HANDLE_VALUE && hFile == left_output_handle) ||
		(right_output_handle != INVALID_HANDLE_VALUE && hFile == right_output_handle))
	{
		// don't write to file
		static int last_left_data_size = 0;
		static int last_right_data_size = 0;
		static int left_n = 0;
		static int right_n = 0;

		// set view id
		int view_id = 0;		// 0 = left, 1 = right
		BYTE *target_buffer = left_image_buffer;
		int *last_data_size = &last_left_data_size;
		int *n = &left_n;
		char *view = "left";

		if (hFile == right_output_handle)
		{
			view_id = 1;
			target_buffer = right_image_buffer;
			last_data_size = &last_right_data_size;
			n = &right_n;
			view = "right";
		}

		// set view id notify
		*(DWORD*)(target_buffer + width*height*3/2) = view_id;

		// form a complete image and convert from I420 to YV12
		if (nNumberOfBytesToWrite == width*height)		
			memcpy(target_buffer, lpBuffer, width*height);					//Y
		else if (nNumberOfBytesToWrite == width*height/4)
			if (*last_data_size == width*height)
				memcpy(target_buffer+width*height+width*height/4, lpBuffer, width*height/4);	// U
			else
			{
				memcpy(target_buffer+width*height, lpBuffer,width*height/4);			// V

				// add a mask to frame 0
				if(*n == 0)
				{
					add_my12doom(target_buffer);
				}

				// send it thourgh pipe
				if (INVALID_HANDLE_VALUE != h_pipe)
				{
					DWORD written = 0;
					WriteFile(h_pipe, target_buffer, width*height*3/2+4, &written, NULL);
					*n = *n + 1;
					printf("%d frames sent(%s)\n", (*n), view);
				}
			}

		*last_data_size = nNumberOfBytesToWrite;

		SetFilePointer(hFile, nNumberOfBytesToWrite, 0, FILE_CURRENT);
		*lpNumberOfBytesWritten = nNumberOfBytesToWrite;

		return TRUE;
	}
	else
	{
		return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
			lpNumberOfBytesWritten, lpOverlapped);
	}

}
int read_buffer(int size, BYTE *buf, CFileBuffer *buffer)
{
retry:
	int result;
	result = buffer->remove(size, buf);

	if (result == 0)
		return size;		//remove OK

	if (result == -1 && !buffer->no_more_data) 
	{
		// wait for data
		Sleep(10);
		goto retry;
	}

	if (result == -1 && buffer->no_more_data)
	{
		// get the only data
		memcpy(buf, buffer->m_the_buffer, buffer->m_data_size);

		int got = buffer->m_data_size;
		buffer->m_data_size = 0;

		return got;
	}

	return 0;
}

BOOL WINAPI MineReadFile(
HANDLE hFile,
LPVOID lpBuffer,
DWORD nNumberOfBytesToRead,
LPDWORD lpNumberOfBytesRead,
LPOVERLAPPED lpOverlapped
)
{
	if (hFile == left_input_handle)
	{
		int got = read_buffer(nNumberOfBytesToRead, (BYTE*)lpBuffer, buffer_left);
        
		*lpNumberOfBytesRead = got;
		if (got>0)
			return TRUE;
		else
			return FALSE;
	}
	else if (hFile == right_input_handle)
	{
		int got = read_buffer(nNumberOfBytesToRead, (BYTE*)lpBuffer, buffer_right);

		*lpNumberOfBytesRead = got;
		if (got>0)
			return TRUE;
		else
			return FALSE;
	}
	else
		return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

VOID NullExport(){}

DWORD WINAPI parent_watcher(LPVOID lpvParam)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	HANDLE h_parent = OpenProcess(SYNCHRONIZE, FALSE, parent_id);
	WaitForSingleObject(h_parent, INFINITE);

	ExitProcess(-1);
	return 0;
}


DWORD WINAPI demux_thread(LPVOID lpvParam)
{
	int view_to_demux = (int)lpvParam;

	ts::demuxer demuxer;
	demuxer.av_only = false;
	demuxer.silence = true;
	demuxer.parse_only = true;

	if (view_to_demux == 0)
	{
		printf("%s demux thread started\n", left_m2ts);
		demuxer.pid_to_demux = left_pid;
		demuxer.demux_target = buffer_left;
		demuxer.demux_file(left_m2ts);
		buffer_left->no_more_data = true;
		printf("%s demux thread exit\n", left_m2ts);
	}

	if (view_to_demux == 1)
	{
		printf("%s demux thread started\n", right_m2ts);
		demuxer.pid_to_demux = right_pid;
		demuxer.demux_target = buffer_right;
		demuxer.demux_file(right_m2ts);
		buffer_right->no_more_data = true;
		printf("%s demux thread exit\n", right_m2ts);
	}


	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    (void)hinst;
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) 
	{
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)TrueCreateFileA, MineCreateFileA);
		DetourAttach(&(PVOID&)TrueWriteFile, MineWriteFile);
		DetourAttach(&(PVOID&)TrueReadFile, MineReadFile);
        error = DetourTransactionCommit();

		// connect to pipe
		while (1)
		{
			h_pipe = CreateFile(pipe_name,   // pipe name 
				GENERIC_READ | GENERIC_WRITE, 
				0,              // no sharing 
				NULL,           // default security attributes
				OPEN_EXISTING,  // opens existing pipe 
				0,              // default attributes 
				NULL);          // no template file 

			// Break if the pipe handle is valid. 
			if (h_pipe != INVALID_HANDLE_VALUE) 
			{
				printf("pipe opened\n");
				break;
			}

			// Exit if an error other than ERROR_PIPE_BUSY occurs.
			if (GetLastError() != ERROR_PIPE_BUSY) 
			{
				printf("Could not open pipe. GLE=%d\n", GetLastError() ); 
			}

			// All pipe instances are busy, so wait for 2 seconds.
			if ( ! WaitNamedPipe(pipe_name, 2000)) 
			{ 
				printf("Could not open pipe: 2 second wait timed out."); 
			}
		}

		// get width, height, parent processid config
		DWORD config[3+2+MAX_PATH*2/4] = {0, 0, -1, 0, 0};
		DWORD byte_read = 0;
		if (h_pipe != INVALID_HANDLE_VALUE)
			ReadFile(h_pipe, config, 20+MAX_PATH*2, &byte_read, NULL);

		width = config[0];
		height = config[1];
		parent_id = config[2];

		left_pid = config[3];
		right_pid = config[4];
		char *input_left = (char*)(config+5);
		char *input_right= ((char*)(config+5))+MAX_PATH;
		if (input_left[0] && input_right[0])
		{
			override_input = true;
			printf("left m2ts : pid %x of %s\n", left_pid, input_left);
			printf("right m2ts: pid %x of %s\n", right_pid,input_right);
			strcpy(left_m2ts, input_left);
			strcpy(right_m2ts, input_right);

			buffer_left = new CFileBuffer(1024000);
			buffer_right = new CFileBuffer(1024000);

			CreateThread(NULL, NULL, demux_thread, (LPVOID)0, NULL, NULL);
			CreateThread(NULL, NULL, demux_thread, (LPVOID)1, NULL, NULL);

		}
		printf("config : %dx%d, parent process id :%d\n", width, height, parent_id);

		// alloc memory;
		left_image_buffer = (BYTE*)malloc(width*height*3/2 +4);
		right_image_buffer = (BYTE*)malloc(width*height*3/2 +4);

		// create parent watcher thread
		CreateThread(NULL, NULL, parent_watcher, NULL, NULL, NULL);
    }
    else if (dwReason == DLL_PROCESS_DETACH) 
	{
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)TrueCreateFileA, MineCreateFileA);
		DetourDetach(&(PVOID&)TrueWriteFile, MineWriteFile);
		DetourDetach(&(PVOID&)TrueReadFile, MineReadFile);
        error = DetourTransactionCommit();

		FlushFileBuffers(h_pipe);
		CloseHandle(h_pipe);

		if(right_image_buffer){free(right_image_buffer);right_image_buffer=NULL;}
    }
    return TRUE;
}

void add_my12doom(unsigned char *pY)
{
	unsigned char mask[64*11] ={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 
			0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 
			0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 
			0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 
			0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 
			0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x00, 0x25, 0x00, 0x25, 0x25, 0x00, 
			0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

	};

	for(int i=height/2; i<height/2+11; i++)
		memcpy(pY+i*width+width/2, mask+64*(i-height/2), 64);
}
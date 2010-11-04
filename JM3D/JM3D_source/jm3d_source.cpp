#include <stdio.h>
#include <windows.h>
#include "avisynth.h"

#include "..\CCritSec.h"
#include "detours.h"
#include "m2ts_scanner.h"

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "detoured.lib")

LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\JM3Dpipe");

class CFrameBuffer;
class JM3DSource;

DWORD WINAPI pipe_feeder(LPVOID lpvParam);
void on_pipe_data(const BYTE* data, JM3DSource *_this);
BOOL DoesDllExportOrdinal1(PCHAR pszDllPath);
HANDLE start_JM();

typedef struct buffer_unit_struct
{
	int n;
	BYTE *data;
} buffer_unit;

class CFrameBuffer
{
public:
	buffer_unit* the_buffer;
	int m_unit_count;
	int m_unit_size;
	int max_n;				// total frame count
	int max_insert_n;		// = -1, max inserted n yet
	int item_count;			// = 0;

	CCritSec cs;

	CFrameBuffer(int unit_size, int unit_count);
	~CFrameBuffer();
	int insert(int n, const BYTE*buf);
	int remove(int n, BYTE *buf);		// 0-n : success 
										// -1  : wrong range, no such data
										// -2  : feeder busy, should wait and retry for data
};

class JM3DSource: public IClip 
{
public:

	HANDLE feeder_thread;
	HANDLE feeder_thread_ready;
	HANDLE feeder_process;
	VideoInfo vi;
	int m_frame_count;
	int m_width;
	int m_height;
	int m_views;

	int left_n;  // pipe data counter
	int right_n;
	CFrameBuffer left_buffer; 
	CFrameBuffer right_buffer;

	BYTE * image_data; // a temp buffer

	int m_pid_left;
	char m_m2ts_left[MAX_PATH];
	int m_pid_right;
	char m_m2ts_right[MAX_PATH];

	JM3DSource(const int frame_count, int width, int height, int buffer_count,
		int views, int pid_left, const char*m2ts_left, int pid_right, const char*m2ts_right,
		int fps_numerator, int fps_denominator, IScriptEnvironment* env);
	virtual ~JM3DSource();

	// avisynth virtual functions
	bool __stdcall GetParity(int n){return false;}
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {}
	const VideoInfo& __stdcall GetVideoInfo(){return vi;}
	void __stdcall SetCacheHints(int cachehints,int frame_range) {};

	// key function
	void get_frame(int n, CFrameBuffer *the_buffer, BYTE*data);
	void on_pipe_data(const BYTE* data);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};


CFrameBuffer::CFrameBuffer(int unit_size, int unit_count/*= buffer_size*/):
m_unit_size(unit_size),
max_insert_n(-1),
item_count(0),
m_unit_count(unit_count)
{
	CAutoLock lck(&cs);

	the_buffer = new buffer_unit[m_unit_count];
	for(int i=0; i<m_unit_count; i++)
	{
		the_buffer[i].n = -1;
		the_buffer[i].data = new BYTE[unit_size];
	}
}

CFrameBuffer::~CFrameBuffer()
{
	CAutoLock lck(&cs);
	for(int i=0; i<m_unit_count; i++)
		delete [] the_buffer[i].data;
	delete [] the_buffer;
}

int CFrameBuffer::insert(int n, const BYTE*buf)
{
	int id = -1;
	while(true)
	{
		cs.Lock();
		for(int i=0; i<m_unit_count; i++)
		{
			if (the_buffer[i].n == -1)
			{
				id = i;
				goto wait_ok;		// braeak outside, cs still locked
			}
		}
		cs.Unlock();
		Sleep(10);
	}

wait_ok:
	// insert
	memcpy(the_buffer[id].data, buf, m_unit_size);
	the_buffer[id].n = n;
	if(n>max_insert_n) max_insert_n = n;
	item_count ++;
	cs.Unlock();
	return id;
}

int CFrameBuffer::remove(int n, BYTE *buf)
{
	// find direct match;
	cs.Lock();
	for(int i=0; i<m_unit_count; i++)
	{
		if (the_buffer[i].n == n)
		{
			memcpy(buf, the_buffer[i].data, m_unit_size);
			the_buffer[i].n = -1;
			item_count --;
			cs.Unlock();
			return n;
		}
	}

	// decide return -1 or -2
	int max_possible_n = min(max_insert_n + m_unit_count - item_count - 1, max_n);
	int min_possible_n = max_insert_n - item_count + 1;
	cs.Unlock();

	if(n>max_possible_n || n<min_possible_n)
	{
		// no wait
		return -1;
	}
	else
	{
		// should wait and retry
		return -2;
	}
}

JM3DSource::JM3DSource(const int frame_count, int width, int height, int buffer_count, int views,
		   int pid_left, const char*m2ts_left, int pid_right, const char*m2ts_right,
		   int fps_numerator, int fps_denominator, IScriptEnvironment* env):
left_buffer(width*height*3/2, buffer_count),
right_buffer(width*height*3/2, buffer_count),
m_width(width), m_height(height), left_n(0), right_n(0),
m_views(views),
m_frame_count(frame_count),
m_pid_left(pid_left),
m_pid_right(pid_right),
image_data(NULL)
{
	// save m2ts override
	strncpy(m_m2ts_left, m2ts_left, MAX_PATH);
	m_m2ts_left[MAX_PATH-1] = NULL;
	strncpy(m_m2ts_right, m2ts_right, MAX_PATH);
	m_m2ts_right[MAX_PATH-1] = NULL;

	// init buffer
	left_buffer.max_n = frame_count-1;
	right_buffer.max_n = frame_count-1;

	// init vi
	vi.width = m_width;
	vi.height = m_height;
	vi.fps_numerator = fps_numerator;
	vi.fps_denominator = fps_denominator;
	vi.SetFieldBased(false);
	vi.audio_samples_per_second=0;
	vi.pixel_type = VideoInfo::CS_YV12;
	vi.num_frames = frame_count;

	// dual view
	if (m_views>1) 
	{
		vi.height *= 2;		// top-bottom views output, top=left.
		image_data = new BYTE[m_width*m_height*3/2];
	}

	// print some info
	char view_str[3][6] = {"left", "right", "both"};
	printf("[JM3DSource info] %dx%d, %.3ffps, %s eye view.\n", width, height, 
		(double)fps_numerator/fps_denominator, view_str[views]);
	printf("[JM3DSource info] total %d frames, buffer size = %d frames\n", frame_count, buffer_count);

	// create pipe thread and wait for it become ready
	feeder_thread_ready = CreateEvent(NULL, TRUE, FALSE, NULL);
	feeder_thread = CreateThread(NULL, 0, pipe_feeder, this, NULL, NULL);
	WaitForSingleObject(feeder_thread_ready, INFINITE);

	// then run JM
	feeder_process = start_JM();
	HANDLE h_parent_process = GetCurrentProcess();
	SetPriorityClass(h_parent_process, IDLE_PRIORITY_CLASS);			//host idle priority
}

JM3DSource::~JM3DSource()
{
	if (image_data)
		delete [] image_data;

	TerminateProcess(feeder_process, -1);
	DWORD wait_result = WaitForSingleObject(feeder_thread, 200);
	if (wait_result == WAIT_TIMEOUT)
		TerminateThread(feeder_thread, -1);
}

void JM3DSource::get_frame(int n, CFrameBuffer *the_buffer, BYTE*data)
{
retry:
	// check if feeder down/done
	bool feeder_done = false;
	DWORD feeder_thread_code = STILL_ACTIVE;
	GetExitCodeThread(feeder_thread, &feeder_thread_code);

	DWORD feeder_process_code = STILL_ACTIVE;
	GetExitCodeProcess(feeder_process, &feeder_process_code);

	if (feeder_thread_code != STILL_ACTIVE || feeder_process_code != STILL_ACTIVE)
		feeder_done = true;

	// try!
	int got = the_buffer->remove(n, data);

	if (got == -1  || (got == -2 && feeder_done))
	{
		// wrong range or feeder down/done
		// set it black and return
		memset(data, 0, m_width*m_height);
		memset(data+m_width*m_height, 128, m_width*m_height/2);

		return;
	}

	if (got == -2)
	{
		// feeder slow, sleep 10ms and retry later
		Sleep(10);
		goto retry;
	}
}

PVideoFrame JM3DSource::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame dst = env->NewVideoFrame(vi);

	if (m_views == 0)
		get_frame(n, &left_buffer, dst->GetWritePtr(PLANAR_Y));
	else if (m_views == 1)
		get_frame(n, &right_buffer, dst->GetWritePtr(PLANAR_Y));
	else if (m_views == 2)
	{
		// top-bottom views output, top=left.

		// get left and copy to dst;
		get_frame(n, &left_buffer, image_data);
		memcpy(dst->GetWritePtr(PLANAR_Y), image_data, m_width*m_height);
		memcpy(dst->GetWritePtr(PLANAR_V), image_data+m_width*m_height, m_width*m_height/4);
		memcpy(dst->GetWritePtr(PLANAR_U), image_data+m_width*m_height*5/4, m_width*m_height/4);

		// get right and copy to dst;
		get_frame(n, &right_buffer, image_data);
		memcpy(dst->GetWritePtr(PLANAR_Y)+m_width*m_height, image_data, m_width*m_height);
		memcpy(dst->GetWritePtr(PLANAR_V)+m_width*m_height/4, image_data+m_width*m_height, m_width*m_height/4);
		memcpy(dst->GetWritePtr(PLANAR_U)+m_width*m_height/4, image_data+m_width*m_height*5/4, m_width*m_height/4);
	}

	return dst;
}

// pipe reciever thread function
// then feed to buffer
void JM3DSource::on_pipe_data(const BYTE* data)
{
	DWORD view_id = *(DWORD*)(data+m_width*m_height*3/2);
	if (view_id == 0 && m_views != 1)
		left_buffer.insert(left_n++, data);		
	if (view_id == 1 && m_views != 0)
		right_buffer.insert(right_n++, data);
}

DWORD WINAPI pipe_feeder(LPVOID lpvParam)
{
	JM3DSource *_this = (JM3DSource*) lpvParam;


	// create the named pipe
	HANDLE hPipe = CreateNamedPipe( 
		lpszPipename,             // pipe name 
		PIPE_ACCESS_DUPLEX,      // read/write access 
		PIPE_TYPE_MESSAGE |		  // message type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		_this->m_width * _this->m_height *3/2 +4, // output buffer size 
		_this->m_width * _this->m_height *3/2 +4, // input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 

	if (hPipe == INVALID_HANDLE_VALUE) 
	{
		// wtf ... create pipe fail?
		ExitProcess(-1);
	}

	// pipe ready, wait for clients to connect
	printf("[JM3DSource info] pipe %s created, waiting for connection.\n", lpszPipename);
	SetEvent(_this->feeder_thread_ready);
	BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

	if (!fConnected)
	{
		// The client could not connect, so close the pipe and exit.
		CloseHandle(hPipe);
		return -1;
	}

	printf("[JM3DSource info] pipe connected\n");

	// alloc memeory
	// should not fail
	HANDLE hHeap = GetProcessHeap();
	BYTE* data = (BYTE*)HeapAlloc(hHeap, 0, _this->m_width * _this->m_height *3/2 + 4);

	// send width, height, processid once connected
	DWORD *config = (DWORD*)data;
	config[0] = _this->m_width;
	config[1] = _this->m_height;
	config[2] = GetCurrentProcessId();
	config[3] = _this->m_pid_left;
	config[4] = _this->m_pid_right;
	strcpy((char*)(config+5), _this->m_m2ts_left);
	strcpy((char*)(config+5)+MAX_PATH, _this->m_m2ts_right);

	DWORD written = 0;
	WriteFile(hPipe, data, 20+MAX_PATH*2, &written, NULL);

	// Loop until pipe disconnect
	while (1)
	{
		// read(and wait for) data message
		DWORD cbBytesRead = 0;
		BOOL fSuccess = ReadFile( 
			hPipe,			// handle to pipe 
			data,			// buffer to receive data 
			_this->m_width * _this->m_height *3/2 + 4,// size of buffer 
			&cbBytesRead,	// number of bytes read 
			NULL);			// not overlapped I/O 

		if (!fSuccess || cbBytesRead == 0)
			break;

		// process data
		_this->on_pipe_data(data);
	}

	// close pipe

	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe);

	// free memory
	HeapFree(hHeap, 0, data);

	printf("[JM3DSource warning] pipe disconnected, %d,%d/%d frame recieved.\t\n", 
		_this->left_n, _this->right_n, _this->m_frame_count);
	return 0;
}

// avisynth init function

void copyright_info()
{
	printf("\nmy12doom's JM bluray3D decoder avisynth plugin beta8\n");
	printf("http://my12doom.googlecode.com/\n");
	printf("mailto:my12doom@126.com\n");

	printf("tsdemux by clark15b:http://code.google.com/p/tsdemuxer/\n");

	printf("\n");
}

AVSValue __cdecl Create_JM3DSource(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	copyright_info();

	int frame_count = args[0].AsInt(-1);
	if (frame_count<0)
		env->ThrowError("frame_count must >= 0");

	int width = args[1].AsInt(1920);
	if (width<0)
		env->ThrowError("width must >= 0");

	int height = args[2].AsInt(1080);
	if (height<0)
		env->ThrowError("height must >= 0");

	int buffer_size = args[3].AsInt(10);
	if (height<0)
		env->ThrowError("buffer_size must >= 0");

	int views = args[4].AsInt(2);
	if (views<0 || views > 2)
		env->ThrowError("available views : 0 = left, 1 = right, 2 = both");

	const char *mvc = args[5].AsString("");

	return new JM3DSource(frame_count, width, height, buffer_size, views,
		0, mvc, 0, "", 24000, 1001, env);
}

AVSValue __cdecl Create_JM3DSource2(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	copyright_info();

	// get paras
	const char *file1 = args[0].AsString("");
	const char *file2 = args[1].AsString("");
	int views = args[2].AsInt(2);
	int buffer_count = args[3].AsInt(10);

	int left_pid = args[4].AsInt(-1);
	int right_pid = args[5].AsInt(-1);

	// check para and input files
	if (file1[0] == NULL)
	{
		env->ThrowError("must have a input file, if you want to use left&right.h264, use JM3DSource.");
	}

	if (views<0 || views > 2)
		env->ThrowError("available views : 0 = left, 1 = right, 2 = both");

	h264_scan_result scan_result1 = scan_m2ts(file1);
	h264_scan_result scan_result2;
	memset(&scan_result2, 0, sizeof(scan_result2));

	if (file2[0])
	{
		// double file (m2ts+m2ts)
		scan_result2 = scan_m2ts(file2);

		// reverse file if ..
		if (scan_result1.possible_right_pid && scan_result2.possible_left_pid)
		{
			const char *t = file1;
			file1 = file2;
			file2 = t;

			h264_scan_result tt = scan_result1;
			scan_result1 = scan_result2;
			scan_result2 = tt;
		}

		if (left_pid == -1) left_pid = scan_result1.possible_left_pid;
		if (right_pid == -1) right_pid = scan_result2.possible_right_pid;
	}
	else
	{
		// single file (ssif)
		if (left_pid == -1) left_pid = scan_result1.possible_left_pid;
		if (right_pid == -1) right_pid = scan_result1.possible_right_pid;
		file2 = file1;
	}

	// file check
	if (scan_result1.error)
		env->ThrowError("error scanning %s", file1);
	if (scan_result2.error)
		env->ThrowError("error scanning %s", file2);

	return new JM3DSource(scan_result1.frame_count, scan_result1.width, scan_result1.height, 
							buffer_count, views, left_pid, file1, right_pid, file2,
							scan_result1.fps_numerator, scan_result1.fps_denominator, env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("JM3DSource","[frame_count]i[width]i[height]i[buffer_count]i[views]i[mvc]s",Create_JM3DSource,0);

	env->AddFunction("JM3DSource2","[file1]s[file2]s[views]i[buffer_count]i[left_pid]i[right_pid]i",Create_JM3DSource2,0);

	return 0;
}


// detours hook helper functions
// copy from "withdll.cpp"
static BOOL CALLBACK ExportCallback(PVOID pContext,
									ULONG nOrdinal,
									PCHAR pszSymbol,
									PVOID pbTarget)
{
	(void)pContext;
	(void)pbTarget;
	(void)pszSymbol;

	if (nOrdinal == 1) 
	{
		*((BOOL *)pContext) = TRUE;
	}
	return TRUE;
}

BOOL DoesDllExportOrdinal1(PCHAR pszDllPath)
{
	HMODULE hDll = LoadLibraryEx(pszDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hDll == NULL) 
	{
		return FALSE;
	}

	BOOL validFlag = FALSE;
	DetourEnumerateExports(hDll, &validFlag, ExportCallback);
	FreeLibrary(hDll);
	return validFlag;
}

// detours starter
// partly copy from "withdll.cpp"
HANDLE start_JM()
{
	CHAR szExe[MAX_PATH] = "ldecod.exe";
	CHAR szCommand[1024]= "";
	CHAR szDllPath[MAX_PATH]= "JM_hook.dll";
	CHAR szDetouredDllPath[MAX_PATH] = "detoured.dll";

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	printf("[JM3DSource info] Starting: `%s %s' with hook '%s'\n", szExe, szCommand, szDllPath);
	fflush(stdout);

	DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED | CREATE_NEW_CONSOLE;

	SetLastError(0);
	if (!DetourCreateProcessWithDll(szExe, szCommand,
		NULL, NULL, TRUE, dwFlags, NULL, NULL,
		&si, &pi, szDetouredDllPath, szDllPath, NULL)) {
			printf("failed: %d\n", GetLastError());
			return INVALID_HANDLE_VALUE;
		}

	SetPriorityClass(pi.hProcess, IDLE_PRIORITY_CLASS);			// JM idle priority

	ResumeThread(pi.hThread);

	return pi.hProcess;
}
#include <conio.h>
#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>

#define BUFSIZE 3110400

DWORD WINAPI InstanceThread(LPVOID);
void process_data(char* data);

int _tmain(VOID) 
{ 
	BOOL   fConnected = FALSE;
	DWORD  dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;

	// The main loop creates an instance of the named pipe and 
	// then waits for a client to connect to it. When the client 
	// connects, a thread is created to handle communications 
	// with that client, and this loop is free to wait for the
	// next client connect request. It is an infinite loop.

	CreateThread(NULL, 0, InstanceThread, NULL, NULL, &dwThreadId);
	for (;;) 
	{
		Sleep(1000);
	} 

	return 0; 
} 

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{
	// create the named pipe
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\JM3Dpipe");

	HANDLE hPipe = CreateNamedPipe( 
		lpszPipename,             // pipe name 
		PIPE_ACCESS_INBOUND,      // read/write access 
		PIPE_TYPE_MESSAGE |		  // message type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		BUFSIZE,                  // output buffer size 
		BUFSIZE,                  // input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 

	if (hPipe == INVALID_HANDLE_VALUE) 
	{
		_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
		return -1;
	}

	// wait for clients to connect
	printf("pipe %s created, waiting for connection...", lpszPipename);
	BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

	if (!fConnected)
	{
		// The client could not connect, so close the pipe and exit.
		CloseHandle(hPipe);
		return -1;
	}


	HANDLE hHeap      = GetProcessHeap();
	char* data = (char*)HeapAlloc(hHeap, 0, BUFSIZE);

	// Do some extra error checking since the app will keep running even if this
	// thread fails.
	if (data == NULL)
	{
		printf( "\nERROR - Pipe Server Failure:\n");
		printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
		printf( "   InstanceThread exitting.\n");
		return (DWORD)-1;
	}

	// Print verbose messages. In production code, this should be for debugging only.
	printf("connected\n");

	// The thread's parameter is a handle to a pipe object instance. 

	// Loop until done reading
	while (1) 
	{ 
		// Read client requests from the pipe. This simplistic code only allows messages
		// up to BUFSIZE characters in length.

		// read(and wait for) data message
		DWORD cbBytesRead = 0;
		BOOL fSuccess = ReadFile( 
			hPipe,			// handle to pipe 
			data,			// buffer to receive data 
			BUFSIZE,		// size of buffer 
			&cbBytesRead,	// number of bytes read 
			NULL);			// not overlapped I/O 

		if (!fSuccess || cbBytesRead == 0)
		{   
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				_tprintf(TEXT("InstanceThread: client disconnected.\n"), GetLastError()); 
			}
			else
			{
				_tprintf(TEXT("InstanceThread ReadFile failed, GLE=%d.\n"), GetLastError()); 
			}
			break;
		}

		// process data
		process_data(data);
	}

	// Flush the pipe to allow the client to read the pipe's contents 
	// before disconnecting. Then disconnect the pipe, and close the 
	// handle to this pipe instance. 

	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 

	HeapFree(hHeap, 0, data);

	printf("InstanceThread exitting.\n");
	return 1;
}

void process_data(char* data)
{
	static init = false;
	static FILE * f = NULL;
	static frame_recieved = 0;
	if (!init)
	{
		f = fopen("C:\\yuv\\data.yuv", "wb");
		init = true;
	}

	if(f)
	{
		fwrite(data, 1, BUFSIZE, f);
		fflush(f);
		frame_recieved ++;
		printf("%d frames recieved\n", frame_recieved);
		getch();
	}
}
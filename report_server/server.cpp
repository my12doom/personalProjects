#include <stdio.h>
#include <winsock2.h>
#include <Windows.h>
#pragma  comment(lib, "ws2_32.lib")

// socks variables
#define MAX_CONNECTION 1000
#define NETWORK_TIMEOUT 5000
int server_socket = -1;
bool server_stopping = false;
int server_port = 82;


HRESULT init_winsock()
{
	static bool inited = false;
	if (inited)
		return S_FALSE;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
		return E_FAIL;

	inited = true;

	return S_OK;
}



DWORD WINAPI handler_thread(LPVOID param)
{
	int acc_socket = *(int*)param;
	DWORD ip = ((DWORD*)param)[1];

	delete param;

	int filesize = 0;
	int rtn = 0;

	if (recv(acc_socket, (char*)&filesize, 4, 0) <4)
	{
		rtn = -1;
		goto clearup;
	}

	printf("%d bytes\n", filesize);

	shutdown(acc_socket, SD_SEND);

	FILE *f = fopen("out.dat", "wb");

	char buf[4096];
	int block_size;
	int got = 0;
	while (got < filesize && (block_size = recv(acc_socket, buf, 4096, 0)) > 0 )
	{
		int left = filesize - got;

		fwrite(buf, 1, min(block_size, left), f);

		got += min(block_size, left);
	}

	fclose(f);

clearup:
	closesocket(acc_socket);
	return rtn;
}

int TCPTest()
{
	if (FAILED(init_winsock()))
		return -1;

	// init and listen
	SOCKADDR_IN server_addr;
	int tmp_socket = socket(PF_INET, SOCK_STREAM, 0);

	if (tmp_socket == -1)
		return -1;

	printf("Server Starting @ %d....", (int)server_port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((int)server_port);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	memset(&(server_addr.sin_zero), 0, 8);
	if (bind(tmp_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == -1)
	{
		printf("FAIL!\n");
		server_socket = -1;
		closesocket(tmp_socket);
		return -1;
	}
	if (listen(tmp_socket, MAX_CONNECTION) == -1 ) 
	{
		server_socket = -1;
		printf("FAIL!\n");
		closesocket(tmp_socket);
		return -1;
	}
	printf("OK\n");

	int sock_size = sizeof(SOCKADDR_IN);
	SOCKADDR_IN user_socket;

	// wait for connection
	server_socket = tmp_socket;

	while(!server_stopping)
	{
		int acc_socket = accept(tmp_socket, (SOCKADDR*)&user_socket, &sock_size);
		int buf = 0;
		// 		setsockopt(acc_socket, SOL_SOCKET, SO_SNDBUF, (char*)&buf, sizeof(buf));
		// 		setsockopt(acc_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buf, sizeof(buf));

		if (server_stopping)
			break;

		if (acc_socket == -1)
			continue;

		DWORD ip = user_socket.sin_addr.S_un.S_addr;
		DWORD *para = new DWORD[2];
		para[0] = acc_socket;
		para[1] = ip;
		CreateThread(NULL, NULL, handler_thread, para, NULL, NULL);
	}

	server_socket = -1;
	return 0;
}

LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	wchar_t reset_exe[MAX_PATH];
	GetModuleFileNameW(NULL, reset_exe, MAX_PATH);
	((wchar_t*)wcsrchr(reset_exe, L'.'))[0] = NULL;
	wcscat(reset_exe, L"reset.exe");
	wchar_t this_exe[MAX_PATH+2] = L"\"";
	GetModuleFileNameW(NULL, this_exe+1, MAX_PATH);
	wcscat(this_exe, L"\"");
	ShellExecute(NULL, NULL, reset_exe, this_exe, NULL, SW_SHOW);
	TerminateProcess(GetCurrentProcess(), -1);
	DebugBreak();

	return EXCEPTION_CONTINUE_SEARCH;
}

int main()
{
	SetUnhandledExceptionFilter(my_handler);
	return TCPTest();
}